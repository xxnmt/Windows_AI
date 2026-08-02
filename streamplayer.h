#ifndef STREAMPLAYER_H
#define STREAMPLAYER_H

#include "ipcmplayer.h"
#include <QAudioSink>
#include <QBuffer>
#include <QTimer>
#include <QAudioFormat>
#include <QQueue>
#include <QMutex>
class StreamPlayer : public IPcmPlayer
{
    Q_OBJECT
public:
    explicit StreamPlayer(QObject *parent=nullptr);
    ~StreamPlayer();

    void startPlayer(int sampleRate,int channels,int sampleBits)override;
    void stopPlayer()override;
    void writePcm(const QByteArray &pcmData)override;
    bool getisPlaying()const override;

signals:
    void PcmPlayerFinished();
    void PcmPlayerError(const QString &error);
private slots:
    void onStateChanged(QAudio::State state);
    void pushData();
    private:
    QAudioSink *m_audioSink=nullptr;
    QBuffer *m_buffer;
    QTimer *m_timer;
    QQueue<QByteArray> m_queue;
    bool m_isplaying=false;
    mutable QMutex m_mutex;


};

#endif // STREAMPLAYER_H
