#ifndef LLMSERVICE_H
#define LLMSERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "sentencedata.h"
#include "historyturn.h"

class MemoryManager;

class LLMService : public QObject
{
    Q_OBJECT
public:
    explicit LLMService(const QString& apiKey,QObject *parent = nullptr);
    //config
    void setApiKey(const QString &apiKey);
    //chat
    void askDeepSeek(const QString& userInput,const QList<HistoryTurn> &historyQA=QList<HistoryTurn>());
    void registerStateProvider(std::function<QString()> provider);
    void setMemoryManager(MemoryManager *manager);
    //Extract
    void extractMemoryAsync(const QList<HistoryTurn> &turns, qlonglong lastEndId, const QList<qlonglong> &sourceIds, const QList<UserProfile> &existingProfiles = {});

signals:
    //chat
    void sentencesReady(const QList<SentenceText> &sentences,const QString &rawReply);
    void internetErrorSignal(const QString &errorMessage);
    //Extract
    void memoryExtractionReady(const QJsonArray &profiles, const QString &summary, qlonglong lastEndId, const QString &sourceIdsJson);
private slots:
    //finish AI 回复
    void onReplyFinished(QNetworkReply* reply);
    //extract
    // void onExtractReplyFinished(QNetworkReply* reply);

private:
    //处理提示词的两个函数
    void initializePromptFile();
    QString loadSystemPrompt();

    QPair<QList<SentenceText>, QString> parseJsonReply(const QString &replyText);


    QNetworkAccessManager* m_networkManager;
    QNetworkAccessManager* m_extractManager;
    QString m_apiKey;

    QString m_localPromptPath;
    QString m_systemPromptCache;
    std::function<QString()> m_stateProvider;

    MemoryManager *m_memoryManager = nullptr;
};

class TagValidator{
public:
    TagValidator(const TagValidator &) = default;
    TagValidator(TagValidator &&) = default;
    TagValidator &operator=(const TagValidator &) = default;
    TagValidator &operator=(TagValidator &&) = default;
    static int levenshteinDistance(const QString &s1, const QString &s2);
    static QString validateTag(const QString &tagName, const QString &rawValue,
                               const QStringList &validValues,
                               const QString &contextText = "",
                               const QString &prevValue = "");

    static const QStringList VALID_EMOTIONS;
    static const QStringList VALID_BLUSH;
    static const QStringList VALID_DISTANCE;
    static const QStringList VALID_CLOTHING;
    static int getEditDistanceThreshold(const QString &value);


};

#endif // LLMSERVICE_H
