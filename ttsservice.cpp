#include "ttsservice.h"
#include "apittsprovider.h"
#include "configmanager.h"
#include <QDebug>
#include <QFile>
#include "streamplayer.h"
#include "fileplayer.h"

TTSService::TTSService(QObject *parent)
    : QObject(parent),
    m_isSynthesizing(false),
    m_isPlaying(false),
    m_provider(nullptr),
    m_player(nullptr)
{

    reloadProvider();
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
    // auto *streamPlayer = new StreamPlayer(this);
    // m_player = streamPlayer;
    // m_player->startPlayer(24000, 1, 16);
    // connect(m_provider, &ITTSProvider::pcmDataReady, m_player, &IPcmPlayer::writePcm);

    m_isSynthesizing = true;

    SentenceText currentSentence = m_ttsQueue.dequeue();

    qDebug()<<"[TTS]开始合成语音(日文):"<<currentSentence.jaText;
    m_provider->synthesize(currentSentence);

}

void TTSService::onTtsFinished(const QString &audioPath, const SentenceText &sentence)
{
    qDebug()<<"[TTS]合成成功，进入待播放队列:"<<audioPath;
    m_isSynthesizing=false;
    PlayItem item;
    item.sentence = sentence;
    item.audioPath = audioPath;
    // item.isStream = audioPath.isEmpty();   // 空路径即为流式合成
    item.isStream=false;
    m_playQueue.enqueue(item);
    processPlayQueue();
    processTtsQueue();
}

void TTSService::onTtsFailed(const QString &errorMsg, const SentenceText &sentence)
{
    qDebug()<<"[TTS]合成失败，使用模拟替代:"<<errorMsg;
    m_isSynthesizing=false;
    m_playQueue.enqueue({sentence,""});
    processPlayQueue();
    processTtsQueue();
}

void TTSService::onPcmPlayFinished()
{
    if (m_player) {
        // 如果是流式播放，断开数据信号
        disconnect(m_provider, &ITTSProvider::pcmDataReady, m_player, &IPcmPlayer::writePcm);
        // 删除播放器并释放
        m_player->deleteLater();
        m_player = nullptr;
    }
    m_isPlaying = false;
    processPlayQueue();
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
        qDebug()<<"[TTS]UI Ready,开始播放语音:"<<item.sentence.jaText<<item.sentence.zhText;
        // StreamPlayer *m_streamPlayer =new StreamPlayer(this);
        // m_player =m_streamPlayer;
        connect(m_player, &IPcmPlayer::PcmPlayerFinished,
                this, &TTSService::onPcmPlayFinished);
        connect(m_player, &IPcmPlayer::PcmPlayerError,
                this, &TTSService::onPcmPlayError);
        // m_player->startPlayer(24000, 1, 16);
        // 连接合成器流式数据
        // connect(m_provider, &ITTSProvider::pcmDataReady,
        //         dynamic_cast<StreamPlayer*>(m_player), &StreamPlayer::writePcm);
    }
    else{
        qDebug()<<"[TTS]UI Ready,开始播放语音:"<<item.sentence.jaText<<item.sentence.zhText;

        FilePlayer *filePlayer = new FilePlayer(this);
        m_player = filePlayer;

        connect(filePlayer, &FilePlayer::PcmPlayerFinished, this, &TTSService::onPcmPlayFinished);
        connect(filePlayer, &FilePlayer::PcmPlayerError, this, &TTSService::onPcmPlayError);
        filePlayer->startPlayer(24000, 1, 16);
        filePlayer->playFile(item.audioPath);
    }
}

