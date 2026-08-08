#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <QObject>
#include <QTimer>
#include <QTime>
#include <QElapsedTimer>

class TimeManager : public QObject
{
    Q_OBJECT
public:
    explicit TimeManager(QObject *parent = nullptr);
    void timeStart();

    void start();
    void stop();
    void setNightStart(const QTime &time);
    void setNightEnd(const QTime &time);
    void setBlushTime(const int time);
    void setBackIdleTime(const int time);

    void notifyLLMtagsApplicated();
    void notifyUserInputStarted();
    void notifyLLMEnded();

signals:
    void clotheChanged(const QString &clothe);
    void blushingReset();
    void emotionReset();
private slots:
    void onMinuteTick();

private:
    QTimer *m_timer;
    QElapsedTimer *m_blushTimer;
    QElapsedTimer *m_idleTimer;

    QTime m_nightStart;
    QTime m_nightEnd;
    int m_blushTime;
    int m_BackIdleTime;

    bool m_isNight;
    bool m_isIdleTimerActive;
    bool m_isBlushTimerActive;
    bool m_isLLMActive;
};

#endif // TIMEMANAGER_H
