#include "fileplayer.h"
#include <QAudioFormat>
#include <QDebug>


FilePlayer::FilePlayer(QObject *parent)
{
    m_isplaying=false;
    m_timer=new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &FilePlayer::pushData);

}

FilePlayer::~FilePlayer()
{
    stopPlayer();
}

void FilePlayer::startPlayer(int sampleRate, int channels, int sampleBits)
{
    QAudioFormat fmt;
    fmt.setSampleRate(sampleRate);
    fmt.setChannelCount(channels);
    fmt.setSampleFormat(sampleBits == 16 ? QAudioFormat::Int16 : QAudioFormat::Int32);

    m_audioSink = new QAudioSink(fmt, this);
    connect(m_audioSink, &QAudioSink::stateChanged, this, &FilePlayer::onStateChanged);
    m_buffer= new QBuffer(this);
    m_buffer->open(QIODevice::ReadWrite);
    m_audioSink->start(m_buffer);
    m_isplaying = true;
    qDebug()<<"[filePlayer]PCM播放器已启动:";
}

void FilePlayer::stopPlayer()
{
    m_timer->stop();
    if (m_file.isOpen())
        m_file.close();

    if (m_audioSink) {
        m_audioSink->stop();
        delete m_audioSink;
        m_audioSink = nullptr;
    }
    if (m_buffer) {
        m_buffer->close();
        delete m_buffer;
        m_buffer = nullptr;
    }
    m_isplaying = false;
    qDebug()<<"[filePlayer]PCM播放器已关闭:";

}

void FilePlayer::writePcm(const QByteArray &pcmData)
{
    qDebug()<<"[FilePlayer]:直接读取文件，无需写入pcm";
}

bool FilePlayer::getisPlaying() const
{
    return m_isplaying;
}

void FilePlayer::playFile(const QString &filePath)
{
    if (m_file.isOpen())
        m_file.close();

    m_file.setFileName(filePath);
    if (!m_file.open(QIODevice::ReadOnly)) {
        qDebug()<<"[filePlayer]无法打开文件:"<<filePath;
        emit PcmPlayerError("无法打开文件: " + filePath);
        return;
    }
    m_file.seek(44);
    m_timer->start(10);
    qDebug()<<"[filePlayer]开始播放:"<<filePath;

}

void FilePlayer::onStateChanged(QAudio::State state)
{
    qDebug() << "[FilePlayer] 音频状态:" << state;
    if (state == QAudio::IdleState) {
        if (!m_file.isOpen() && !m_timer->isActive()) {
            m_isplaying = false;
            emit PcmPlayerFinished();
        }
    } else if (state == QAudio::StoppedState) {
        m_isplaying = false;
    }
}

void FilePlayer::pushData()
{
    if (!m_isplaying || !m_audioSink ||!m_buffer|| !m_file.isOpen())
        return;

    QByteArray data = m_file.read(CHUNK_SIZE);
    if (data.isEmpty()) {
        m_timer->stop();
        m_file.close();
        return;
    }
    qint64 written=m_buffer->write(data);
    if(written<0){
        qDebug()<<"[steamPlayer]:写入m_buffer失败";
        emit PcmPlayerError("写入音频缓冲区失败");
        m_timer->stop();
    }
}

