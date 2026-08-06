#ifndef TTSSERVICE_H
#define TTSSERVICE_H

#include <QObject>
#include <QQueue>
#include <QTimer>
#include <QMediaPlayer>
#include <QAudioOutput>
#include "sentencedata.h"
#include "ittsprovider.h"
#include "ipcmplayer.h"
#include "ttsprocessmanager.h"

struct PlayItem {
    SentenceText sentence;
    QString audioPath;
    bool isStream=false;
    IPcmPlayer *streamPlayer = nullptr;
};

class TTSService : public QObject
{
    Q_OBJECT
public:
    explicit TTSService(QObject *parent = nullptr);

    //接收 LLM 解析完毕的多句话
    void enqueueSentences(const QList<SentenceText> &sentences);

    void reloadProvider();
    void switchModel(const QString &gptPath, const QString &sovitsPath);

signals:
    //通知中枢更新UI
    void playAudioAction(const QString &zhText, const QMap<QString, QString> &tags);

private slots:
    //TTS生成线程
    void processTtsQueue();
    void onTtsFinished(const QString &audioPath, const SentenceText &sentence);
    void onTtsFailed(const QString &errorMsg, const SentenceText &sentence);
    //pcm
    void onPcmPlayFinished();
    void onPcmPlayError(const QString &msg);
    //音频播放线程 (Consumer)
    void processPlayQueue();
    //ttsProcess
    void onApiReady();
    void onApiFailed(const QString &error);
    void processPendingSentences();


private:
    //ttsProcessApi
    TTSProcessManager *m_processManager=nullptr;
    QQueue<SentenceText> m_pendingSentences;
    bool m_isApiReady=false;

    //PCM播放
    IPcmPlayer *m_player=nullptr;
    IPcmPlayer *m_pendingStreamPlayer = nullptr;
    //两个独立队列
    QQueue<SentenceText> m_ttsQueue;   // 等待合成的队列
    QQueue<PlayItem> m_playQueue;  // 合成完毕，等待播放的队列

    //状态锁
    bool m_isSynthesizing=false;
    bool m_isPlaying=false;

    ITTSProvider *m_provider;          // 当前 TTS 抽象策略
    QMediaPlayer *m_mediaPlayer;      // 真实音频播放器
    QAudioOutput *m_audioOutput;
};

#endif // TTSSERVICE_H