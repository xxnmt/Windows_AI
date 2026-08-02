#include "streamplayer.h"
#include <qdebug>


StreamPlayer::StreamPlayer(QObject *parent)
{
    m_audioSink=nullptr;
    m_isplaying=false;
    m_timer=new QTimer(this);
    connect(m_timer,&QTimer::timeout,this,&StreamPlayer::pushData);
    qDebug()<<"[StreamPlayer]:构造成功";
}

StreamPlayer::~StreamPlayer()
{
    stopPlayer();
}

void StreamPlayer::startPlayer(int sampleRate, int channels, int sampleBits)
{
    QAudioFormat audioFormat;
    audioFormat.setSampleRate(sampleRate);
    audioFormat.setChannelCount(channels);
    audioFormat.setSampleFormat(sampleBits==16?QAudioFormat::Int16:QAudioFormat::Int32);

    m_audioSink=new QAudioSink(audioFormat,this);
    connect(m_audioSink,&QAudioSink::stateChanged,this,&StreamPlayer::onStateChanged);

    m_buffer=new QBuffer(this);
    m_buffer->open(QIODevice::ReadWrite);
    m_audioSink->start(m_buffer);
    m_timer->start(20);
    m_isplaying=true;
    qDebug()<<QString("[StreamPlayer]开始播放:采样率:1%;通道:2%;位深:3%")
                    .arg(sampleRate,channels,sampleBits);

}

void StreamPlayer::stopPlayer()
{
    if (m_audioSink) {
        m_audioSink->stop();
        delete m_audioSink;
        m_audioSink = nullptr;
    }
    if(m_buffer){
        m_buffer->close();
        delete m_buffer;
        m_buffer=nullptr;
    }
    QMutexLocker locker(&m_mutex);
    m_queue.clear();
    m_isplaying = false;
    qDebug()<<"[StreamPlayer]:停止播放";
}

void StreamPlayer::writePcm(const QByteArray &pcmData)
{
    if (!m_isplaying || !m_audioSink){
        qDebug()<<"[StreamPlayer]:writePcm异常";
        return;
    }
    QMutexLocker locker(&m_mutex);
    m_queue.enqueue(pcmData);

}

bool StreamPlayer::getisPlaying() const
{
    return m_isplaying;
}

void StreamPlayer::onStateChanged(QAudio::State state)
{
    qDebug()<<"[StreamPlayer]音频状态:"<<state;
    if (state == QAudio::IdleState) {
        QMutexLocker locker(&m_mutex);
        if (m_queue.isEmpty()) {
            m_isplaying = false;
            emit PcmPlayerFinished();
        }
    }
    else if (state == QAudio::StoppedState) {
        m_isplaying = false;
    }
}

void StreamPlayer::pushData()
{
    if (!m_isplaying || !m_audioSink){
        qDebug()<<"[StreamPlayer]:pushData异常";
        return;
    }
    QMutexLocker locker(&m_mutex);
    while (!m_queue.isEmpty()) {
        QByteArray chunk=m_queue.dequeue();
        locker.unlock();
        qint64 written =m_buffer->write(chunk);
        if(written<0){
            qDebug()<<"[steamPlayer]:写入m_buffer失败";
            emit PcmPlayerError("写入音频缓冲区失败");
        }
        locker.relock();
    }
}
