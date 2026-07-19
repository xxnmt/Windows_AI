#ifndef LLMSERVICE_H
#define LLMSERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "sentencedata.h"


class LLMService : public QObject
{
    Q_OBJECT
public:
    explicit LLMService(const QString& apiKey,QObject *parent = nullptr);

    void askDeepSeek(const QString& userInput);

    void registerStateProvider(std::function<QString()> provider);

    void setApiKey(const QString &apiKey);

signals:
    void sentenceReady(const QList<SentenceText> &sentences);

    void internetErrorSignal(const QString &errorMessage);
private slots:
    //finish AI 回复
    void onReplyFinished(QNetworkReply* reply);
private:
    //处理提示词的两个函数
    void initializePromptFile();
    QString loadSystemPrompt();


    QNetworkAccessManager* m_networkManager;
    QString m_apiKey;

    QString m_localPromptPath;
    QString m_systemPromptCache;
    std::function<QString()> m_stateProvider;
};

#endif // LLMSERVICE_H
