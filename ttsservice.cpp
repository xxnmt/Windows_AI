#include "ttsservice.h"
#include "apittsprovider.h"
#include "configmanager.h"
#include <QDebug>
#include <QFile>
#include "ipcmplayer.h"

TTSService::TTSService(QObject *parent)
    : QObject(parent),
    m_isSynthesizing(false),
    m_isPlaying(false),
    m_provider(nullptr),
    m_player(nullptr)
{

    reloadProvider();
    m_processManager = new TTSProcessManager(
        ConfigManager::instance().getPythonPath(),
        ConfigManager::instance().getGPTSovitsRootPath(),
        {"-a", "127.0.0.1", "-p", "9880"},
        9880,
        this);
    connect(m_processManager,&TTSProcessManager::apiReady,this,&TTSService::onApiReady);
    connect(m_processManager,&TTSProcessManager::apiFailed,this,&TTSService::onApiFailed);
    m_processManager->apiStart();
    qDebug()<<"[TTS]:正在启动api";
}

void TTSService::enqueueSentences(const QList<SentenceText> &sentences)
{
    // 将解析出的句子推入合成队列
    for (const SentenceText &s : sentences) {
        m_ttsQueue.enqueue(s);
    }
    // 尝试启动合成流水线
    processTtsQueue();
}

void TTSService::reloadProvider()
{
    if(m_provider){
        m_provider->deleteLater();
        m_provider=nullptr;
    }
    QString mode=ConfigManager::instance().getTTSMode();

    if(mode=="api"){
        m_provider =new ApiTTSProvider;
        qDebug()<<"[TTS]:已加载ApiProvider";
        QTimer::singleShot(100, this, [this]() {
            if (m_provider) {
                qDebug()<<"[TTS]:预热中";
                m_provider->warmUp();
            }
        });
    }
    if(mode=="mock"){
        qDebug()<<"[TTS]:已加载MoceTTSProvider";
    }
    connect(m_provider, &ITTSProvider::synthesisFinished, this, &TTSService::onTtsFinished);
    connect(m_provider, &ITTSProvider::synthesisFailed, this, &TTSService::onTtsFailed);
}

void TTSService::switchModel(const QString &gptPath, const QString &sovitsPath)
{
    if (ApiTTSProvider *apiProvider = qobject_cast<ApiTTSProvider*>(m_provider)) {
        apiProvider->switchModel(gptPath, sovitsPath);
    }
}


void TTSService::processTtsQueue()
{
    if (m_isSynthesizing || m_ttsQueue.isEmpty()) return;

    // API 未就绪：全部转入 pending，保持顺序
    if(!m_processManager->isApiReady()){
        while(!m_ttsQueue.isEmpty()){
            m_pendingSentences.enqueue(m_ttsQueue.dequeue());
        }
        static bool firstTime=true;
        if(firstTime){
            QMap<QString, QString> tags;
            emit playAudioAction("茉子还在准备声音哦，再等一下啦...",tags);
            firstTime=false;
            qDebug()<<"[TTS]:api未完成启动，缓存本次请求";
        }
        return;
    }

    m_isSynthesizing = true;
    SentenceText currentSentence = m_ttsQueue.dequeue();
    if(m_provider->isStreamingMode()){
        disconnect(m_provider, &ITTSProvider::pcmDataReady, nullptr, nullptr);
        m_pendingStreamPlayer = IPcmPlayer::create(IPcmPlayer::Stream, this);
        // connect(m_provider,&ITTSProvider::pcmDataReady,m_pendingStreamPlayer,&IPcmPlayer::writePcm);
        qDebug()<<"[TTS]开始合成语音(日文流式):"<<currentSentence.jaText;
        if(!m_isPlaying&&m_playQueue.isEmpty()){
            bool firstChunkPending=true;
            connect(m_provider, &ITTSProvider::pcmDataReady, this,
                    [this, currentSentence, firstChunkPending](const QByteArray &pcm) mutable {
                        if(firstChunkPending){
                            firstChunkPending = false;
                            emit playAudioAction(currentSentence.zhText, currentSentence.rawTags);
                            // 首块到达时服务端WAV头已解析，用真实采样率校正播放器（与默认相同则为空操作）
                            if(m_pendingStreamPlayer){
                                m_pendingStreamPlayer->updateSampleRate(apiSampleRate());
                            }
                        }
                        if(m_pendingStreamPlayer){
                            m_pendingStreamPlayer->writePcm(pcm);
                        }
                    });

            startStreamPlayer(m_pendingStreamPlayer,currentSentence);
        }
        else{
            connect(m_provider,&ITTSProvider::pcmDataReady,m_pendingStreamPlayer,&IPcmPlayer::writePcm);
        }
    }
    else{
        qDebug()<<"[TTS]开始合成语音(日文):"<<currentSentence.jaText;

    }
    m_provider->synthesize(currentSentence);

}

void TTSService::onTtsFinished(const QString &audioPath, const SentenceText &sentence)
{
    qDebug()<<"[TTS]合成成功，进入待播放队列:"<<audioPath;
    m_isSynthesizing=false;
    if(m_pendingStreamPlayer){
        m_pendingStreamPlayer->setSynthesisDone();
        if(m_player!=m_pendingStreamPlayer){
            PlayItem item;
            item.sentence=sentence;
            item.audioPath=audioPath;
            item.isStream=true;
            item.streamPlayer=m_pendingStreamPlayer;
            m_playQueue.enqueue(item);
            processPlayQueue();
        }
    }
    else{
        PlayItem item;
        item.sentence=sentence;
        item.audioPath=audioPath;
        item.isStream=false;
        m_playQueue.enqueue(item);
        processPlayQueue();
    }
    processTtsQueue();
    checkPlaybackQueueEmpty();
}

void TTSService::onTtsFailed(const QString &errorMsg, const SentenceText &sentence)
{
    qDebug()<<"[TTS]合成失败，使用模拟替代:"<<errorMsg;
    m_isSynthesizing=false;
    if (m_pendingStreamPlayer) {
        disconnect(m_provider, &ITTSProvider::pcmDataReady,nullptr, nullptr);
        if(m_player==m_pendingStreamPlayer){
            m_player->stopPlayer();
            m_player->deleteLater();
            m_player=nullptr;
            m_isPlaying=false;
        }
        m_pendingStreamPlayer->deleteLater();
        m_pendingStreamPlayer = nullptr;
    }
    m_playQueue.enqueue({sentence,"",false});
    processPlayQueue();
    processTtsQueue();
    checkPlaybackQueueEmpty();
}

void TTSService::onPcmPlayFinished()
{
    qDebug()<<"[TTS]onPcmPlayFinished, 队列大小="<<m_playQueue.size();
    if (m_player) {
        // 如果是流式播放，断开数据信号
        disconnect(m_provider, &ITTSProvider::pcmDataReady, this, nullptr);
        disconnect(m_provider, &ITTSProvider::pcmDataReady, m_player, &IPcmPlayer::writePcm);
        // 删除播放器并释放
        m_player->deleteLater();
        m_player = nullptr;
    }
    m_isPlaying = false;
    processPlayQueue();
    checkPlaybackQueueEmpty();
}

void TTSService::onPcmPlayError(const QString &msg)
{
    qDebug()<<"[TTS]PCM播放错误:"<<msg;
    onPcmPlayFinished();   // 直接清理，继续下一句
}

void TTSService::processPlayQueue()
{
    if (m_isPlaying || m_playQueue.isEmpty()) return;

    m_isPlaying = true;
    PlayItem item = m_playQueue.dequeue();

    // 触发信号，通知 AppController 刷新气泡(中文)和立绘(四维标签)
    emit playAudioAction(item.sentence.zhText, item.sentence.rawTags);
    if (item.audioPath.isEmpty()&&!item.isStream) {
        // 【模拟播放分支】如果路径为空，说明使用的是 MockTTSProvider
        qDebug()<<"[TTS]开始模拟播放，同步更新UI.";
        // 模拟播放耗时：根据日文字数，正常语速每个字约 250ms
        int playDuration = qMax(1000, item.sentence.jaText.length() * 250);
        QTimer::singleShot(playDuration, this, &TTSService::onPcmPlayFinished);
    }

    else if(item.isStream){
        qDebug()<<"[TTS]UI Ready,开始流式播放:"<<item.sentence.zhText;
        m_player=item.streamPlayer;
        connect(m_player, &IPcmPlayer::PcmPlayerFinished,this, &TTSService::onPcmPlayFinished);
        connect(m_player, &IPcmPlayer::PcmPlayerError,this, &TTSService::onPcmPlayError);
        m_player->startPlayer(apiSampleRate(), 1, 16);
    }
    else{
        qDebug()<<"[TTS]UI Ready,开始播放语音:"<<item.sentence.jaText<<item.sentence.zhText;

        m_player = IPcmPlayer::create(IPcmPlayer::File, this);

        connect(m_player, &IPcmPlayer::PcmPlayerFinished, this, &TTSService::onPcmPlayFinished);
        connect(m_player, &IPcmPlayer::PcmPlayerError,this, &TTSService::onPcmPlayError);
        m_player->startPlayer(apiSampleRate(), 1, 16);  // FilePlayer会从文件头自行校正，此处仅兜底
        m_player->setSource(item.audioPath);
    }
}

void TTSService::onApiReady()
{
    m_isApiReady=true;
    qDebug()<<"[TTS]:apiReady,正在推入缓存的未合成语音";
    QString gptPath = ConfigManager::instance().getGPTWeightsPath();
    QString sovitsPath = ConfigManager::instance().getSoVITSWeightsPath();
    if (!gptPath.isEmpty() || !sovitsPath.isEmpty()) {
        switchModel(gptPath, sovitsPath);
        qDebug()<<"[TTS]:自动切换模型 GPT:"<<gptPath<<"SoVITS:"<<sovitsPath;
    }
    processPendingSentences();
}

void TTSService::onApiFailed(const QString &error)
{
    qDebug()<<"[TTS]:api启动失败:"<<error;
    m_pendingSentences.clear();
    QMap<QString, QString> tags;
    tags.insert("emotion", "sad");
    emit playAudioAction("茉子的声音出问题了，待会儿再试吧...", tags);
}

void TTSService::processPendingSentences()
{
    while(!m_pendingSentences.isEmpty()){
        m_ttsQueue.enqueue(m_pendingSentences.dequeue());
    }
    processTtsQueue();
}

void TTSService::checkPlaybackQueueEmpty()
{
    if (m_playQueue.isEmpty() && m_ttsQueue.isEmpty()
        && !m_isPlaying && !m_isSynthesizing) {
        qDebug()<<"[TTS]:本轮对话已全部播放";
        emit playbackQueueEmpty();
    }
}

void TTSService::startStreamPlayer(IPcmPlayer *player, const SentenceText &sentence)
{
    m_player=player;
    connect(m_player,&IPcmPlayer::PcmPlayerFinished,this,&TTSService::onPcmPlayFinished);
    connect(m_player,&IPcmPlayer::PcmPlayerError,this,&TTSService::onPcmPlayError);
    // emit playAudioAction(sentence.zhText,sentence.rawTags);
    m_player->startPlayer(apiSampleRate(),1,16);
    m_isPlaying=true;
    qDebug()<<"[TTS]流式起播(边合成边播):"<<sentence.zhText;
}

int TTSService::apiSampleRate() const
{
    if (auto *api = qobject_cast<ApiTTSProvider*>(m_provider)) {
        return api->getSampleRate();
    }
    return 24000;  // 兜底默认（非ApiTTSProvider路径；实际流式/文件路径均为ApiTTSProvider）
}

