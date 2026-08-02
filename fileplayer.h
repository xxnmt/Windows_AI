#ifndef FILEPLAYER_H
#define FILEPLAYER_H

#include "ipcmplayer.h"
#include <QAudioSink>
#include <QFile>
#include <QTimer>
#include <QBuffer>

class FilePlayer : public IPcmPlayer
{
    Q_OBJECT
public:
    explicit FilePlayer(QObject *parent = nullptr);
    ~FilePlayer();
    void startPlayer(int sampleRate,int channels,int sampleBits)override;
    void stopPlayer()override;
    void writePcm(const QByteArray &pcmData)override;
    bool getisPlaying()const override;
    void playFile(const QString &filePath);

private slots:
    void onStateChanged(QAudio::State state);
    void pushData();
private:

    QAudioSink *m_audioSink = nullptr;
    QFile m_file;
    bool m_isplaying = false;
    QTimer *m_timer;
    QBuffer *m_buffer = nullptr;
    static const int CHUNK_SIZE = 4096;
};

#endif // FILEPLAYER_H
