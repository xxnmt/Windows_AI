#include "timemanager.h"

TimeManager::TimeManager(QObject *parent)
    : QObject{parent}
{
    m_nightStart= QTime(22,0);
    m_nightEnd=QTime(7,0);
    m_blushTime=30;
    m_BackIdleTime=15;

    m_isNight=false;
    m_isIdleTimerActive=false;
    m_isBlushTimerActive=false;
    m_isLLMActive=false;

    m_timer=new QTimer(this);
    m_timer->setInterval(60000);
    connect(m_timer,&QTimer::timeout,this,&TimeManager::onMinuteTick);
    start();

}

void TimeManager::start()
{
    m_timer->start();
    onMinuteTick();
    qDebug()<<"[TimerManager]:已启动，夜间时间为"<<m_nightStart<<"-"<<m_nightEnd;
    qDebug()<<"[TimerManager]:脸红持续"<<m_blushTime<<" 表情持续"<<m_BackIdleTime;
}

void TimeManager::stop()
{
    if(m_timer){
        m_timer->stop();
        // delete m_timer;
        // m_timer=nullptr;
    }
}

void TimeManager::setNightStart(const QTime &time)
{
    m_nightStart=time;
}

void TimeManager::setNightEnd(const QTime &time)
{
    m_nightEnd=time;
}

void TimeManager::setBlushTime(const int time)
{
    m_blushTime=time;
}

void TimeManager::setBackIdleTime(const int time)
{
    m_BackIdleTime=time;
}

void TimeManager::notifyLLMtagsApplicated()
{
    m_isBlushTimerActive=false;
    m_isIdleTimerActive=false;
    m_isLLMActive=true;
}

void TimeManager::notifyUserInputStarted()
{
    m_isLLMActive=true;
    m_isIdleTimerActive=false;
}

void TimeManager::notifyLLMEnded()
{
    m_isLLMActive=false;
    m_isIdleTimerActive=true;
    m_idleTimer->start();
}

void TimeManager::onMinuteTick()
{
    //白天黑夜
    QTime now=QTime::currentTime();
    bool isNight;
    if(m_nightStart<=m_nightEnd){
        isNight=(now>=m_nightStart&&now<m_nightEnd);
    }
    else{
        isNight=(now<=m_nightStart||now>m_nightEnd);
    }
    if(isNight!=m_isNight){
        m_isNight=isNight;
        QString clothes=isNight?"pajama":"schoolUniform";
        qDebug()<<"[TimeManager]:时段切换，服装变为"<<clothes;
        emit clotheChanged(clothes);
    }

    //脸红退火
    if(!m_isLLMActive&&m_isBlushTimerActive){
        if(m_blushTimer->hasExpired(m_blushTime)){
            m_isBlushTimerActive=false;
            qDebug()<<"[TimeManager]:脸红退火";
            emit blushingReset();
        }
    }

    //表情退火
    if(!m_isLLMActive&&m_isIdleTimerActive){
        if(m_idleTimer->hasExpired(m_BackIdleTime)){
            m_isIdleTimerActive=false;
            qDebug()<<"[TimeManager]:表情退火";
            emit emotionReset();
        }
    }
}
