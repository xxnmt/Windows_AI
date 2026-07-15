#ifndef TTSSERVICE_H
#define TTSSERVICE_H

#include <QObject>
#include <QQueue>
#include <QTimer>
#include "sentencedata.h"

class TTSService : public QObject
{
    Q_OBJECT
public:
    explicit TTSService(QObject *parent = nullptr);

    // 接口：接收 LLM 解析完毕的多句话
    void enqueueSentences(const QList<SentenceText> &sentences);

signals:
    // 通知中枢更新 UI (播放语音的同时，更新气泡中文和立绘状态)
    void playAudioAction(const QString &zhText, const QMap<QString, QString> &tags);

private slots:
    // 模拟 TTS 生成线程 (Producer)
    void processTtsQueue();
    void onMockTtsFinished();

    // 模拟 音频播放线程 (Consumer)
    void processPlayQueue();
    void onMockPlayFinished();

private:
    // 两个独立队列
    QQueue<SentenceText> m_ttsQueue;   // 等待合成的队列
    QQueue<SentenceText> m_playQueue;  // 合成完毕，等待播放的队列

    // 状态锁
    bool m_isSynthesizing;
    bool m_isPlaying;

    // 模拟正在处理的当前句
    SentenceText m_currentSynthesisSentence;
    SentenceText m_currentPlaySentence;
};

#endif // TTSSERVICE_H