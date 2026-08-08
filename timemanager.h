#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <QObject>
#include <QTimer>
#include <QTime>

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
    void notifyBlushingApplicated();

signals:
    void clotheChanged(const QString &clothe);
    void blushingReset();
    void emotionReset();
private slots:
    void onMinuteTick();

private:
    QTimer *m_timer;
    QTimer *m_blushResetTimer;
    QTimer *m_emotionResetTimer;

    QTime m_nightStart;
    QTime m_nightEnd;
    int m_blushTime;
    int m_BackIdleTime;

    bool m_isNight;
};

#endif // TIMEMANAGER_H
