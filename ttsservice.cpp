#include "ttsservice.h"
#include "apittsprovider.h"
#include "configmanager.h"
#include <QDebug>
#include <QFile>

TTSService::TTSService(QObject *parent)
    : QObject(parent),
    m_isSynthesizing(false),
    m_isPlaying(false),
    m_provider(nullptr)
{
    m_mediaPlayer=new QMediaPlayer;
    m_audioOutput=new QAudioOutput;
    m_mediaPlayer->setAudioOutput(m_audioOutput);

    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged, this, &TTSService::onPlaybackStateChanged);

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

// ---------------- 模拟 TTS 生成线程 (Producer) ----------------
void TTSService::processTtsQueue()
{
    if (m_isSynthesizing || m_ttsQueue.isEmpty()) return;

    m_isSynthesizing = true;
    SentenceText currentSentence = m_ttsQueue.dequeue();

    qDebug()<<"[TTS]开始合成语音(日文):"<<currentSentence.jaText;
    m_provider->synthesize(currentSentence);

}

void TTSService::onTtsFinished(const QString &audioPath, const SentenceText &sentence)
{
    qDebug()<<"[TTS]合成成功，进入待播放队列:"<<audioPath;
    m_isSynthesizing=false;
    m_playQueue.enqueue({sentence,audioPath});
    processPlayQueue();
    processTtsQueue();
}

void TTSService::onTtsFailed(const QString &errorMsg, const SentenceText &sentence)
{
    qDebug()<<"[TTS]合成失败，跳过:"<<errorMsg;
    processTtsQueue();
}

void TTSService::processPlayQueue()
{
    if (m_isPlaying || m_playQueue.isEmpty()) return;

    m_isPlaying = true;
    PlayItem item = m_playQueue.dequeue();

    // 触发信号，通知 AppController 刷新气泡(中文)和立绘(四维标签)
    emit playAudioAction(item.sentence.zhText, item.sentence.rawTags);
    if (item.audioPath.isEmpty()) {
        // 【模拟播放分支】如果路径为空，说明使用的是 MockTTSProvider
        qDebug()<<"[TTS]开始模拟播放，同步更新UI.";
        // 模拟播放耗时：根据日文字数，正常语速每个字约 250ms
        int playDuration = qMax(1000, item.sentence.jaText.length() * 250);
        QTimer::singleShot(playDuration, this, &TTSService::onMockPlayFinished);
    }
    else{
        qDebug()<<"[TTS]UI Ready,开始播放语音:"<<item.sentence.jaText<<item.sentence.zhText;

        m_mediaPlayer->setSource(QUrl::fromLocalFile(item.audioPath));
        m_mediaPlayer->play();
    }


}

void TTSService::onPlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    if(state==QMediaPlayer::StoppedState){
        qDebug()<<"[TTS]:本句语音播放完毕";
    }
    QString playedFile = m_mediaPlayer->source().toLocalFile();
    if (QFile::exists(playedFile)) {
        QFile::remove(playedFile);
    }

    m_isPlaying = false;
    processPlayQueue();
}

void TTSService::onMockPlayFinished()
{
    qDebug()<<"[TTS]当前模拟音频完毕";
    m_isPlaying = false;
    processPlayQueue();
}
