#ifndef LLMSERVICE_H
#define LLMSERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class LLMService : public QObject
{
    Q_OBJECT
public:
    explicit LLMService(QObject *parent = nullptr);

    void askDeepSeek(const QString& userInput);

signals:
    //数据处理通知函数
    void replyReady(const QString &cleanText,const QString &emotion);

    void internetErrorSignal(const QString &errorMessage);
private slots:
    //finish AI 回复
    void onReplyFinished(QNetworkReply* reply);
private:
    QNetworkAccessManager* m_networkManager;
};

#endif // LLMSERVICE_H
