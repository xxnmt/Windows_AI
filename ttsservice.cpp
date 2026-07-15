#include "ttsservice.h"
#include <QDebug>

TTSService::TTSService(QObject *parent)
    : QObject(parent), m_isSynthesizing(false), m_isPlaying(false)
{
}

void TTSService::enqueueSentences(const QList<SentenceText> &sentences)
{
    // 将解析出的句子推入合成队列
    for (const auto &s : sentences) {
        m_ttsQueue.enqueue(s);
    }
    // 尝试启动合成流水线
    processTtsQueue();
}

// ---------------- 模拟 TTS 生成线程 (Producer) ----------------
void TTSService::processTtsQueue()
{
    if (m_isSynthesizing || m_ttsQueue.isEmpty()) return;

    m_isSynthesizing = true;
    m_currentSynthesisSentence = m_ttsQueue.dequeue();

    qDebug() << "[TTS模拟] 开始合成语音 (日文):" << m_currentSynthesisSentence.jaText;

    // 模拟合成耗时：根据日文字数计算，每个字假定需要 100ms 推理时间
    int synthesisTime = qMax(500, m_currentSynthesisSentence.jaText.length() * 100);
    QTimer::singleShot(synthesisTime, this, &TTSService::onMockTtsFinished);
}

void TTSService::onMockTtsFinished()
{
    qDebug() << "[TTS模拟] 合成完毕，推入播放队列.";
    m_isSynthesizing = false;

    // 1. 将“生成的音频(模拟)”推入播放队列
    m_playQueue.enqueue(m_currentSynthesisSentence);

    // 2. 唤醒播放线程（如果它在休眠）
    processPlayQueue();

    // 3. 继续合成下一句（并发）
    processTtsQueue();
}

// ---------------- 模拟 音频播放线程 (Consumer) ----------------
void TTSService::processPlayQueue()
{
    if (m_isPlaying || m_playQueue.isEmpty()) return;

    m_isPlaying = true;
    m_currentPlaySentence = m_playQueue.dequeue();

    // 触发信号，通知 AppController 刷新气泡(中文)和立绘(四维标签)
    qDebug() << "[播放模拟] 开始播放语音，同步更新UI.";
    emit playAudioAction(m_currentPlaySentence.zhText, m_currentPlaySentence.rawTags);

    // 模拟播放耗时：根据日文字数，正常语速每个字约 250ms
    int playDuration = qMax(1000, m_currentPlaySentence.jaText.length() * 250);
    QTimer::singleShot(playDuration, this, &TTSService::onMockPlayFinished);
}

void TTSService::onMockPlayFinished()
{
    qDebug() << "[播放模拟] 当前音频播放完毕.";
    m_isPlaying = false;

    // 继续播放下一句
    processPlayQueue();
}