#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

#include <QObject>
#include <QObject>
#include <QSqlDatabase>
#include <QVariantMap>
#include <QList>
#include "sentencedata.h"
#include "historyturn.h"


class MemoryManager : public QObject
{
    Q_OBJECT
public:
    explicit MemoryManager(const QString &dbFilePath,QObject *parent = nullptr);
    ~MemoryManager();
    bool saveQATurn(const QString &userInput,
                          const QString &rawReply,
                          const QList<SentenceText> &sentences);

//历史对话crud
    QList<HistoryTurn> getHistoryTurn(int N);
    QList<HistoryTurn> getHistoryTurn(int offset,int limit);
    qlonglong getTotalHistoryCount();
    bool deleteTurnByID(int id);
    bool clearAllHistory();
    QList<HistoryTurn> getUnsummarizedTurns(qlonglong &outLastEndId,QList<qlonglong> &outSourceIds);

//用户画像crud
    bool upsertUserProfile(const QString &key, const QString &value, int tier, int confidenceGain);
    QList<UserProfile> getActiveUserProfiles(int minConfidence = 20);
    bool deleteUserProfile(qlonglong id);
    int scanAndApplyProfileDecay();

    QString normalizeProfileKey(const QString &rawKey);
    QString mergeProfileValue(const QString &key, const QString &oldVal, const QString &newVal);
    int levenshteinDistance(const QString &s1, const QString &s2);
    int getEditDistanceThreshold(const QString &value);

//记忆摘要crud
    bool addLongTermSummary(const QString &summaryText, qlonglong coveredEndId, const QString &sourceIdsJson);
    QList<LongTermSummary> getLatestSummaries(int limit = 5);
    bool deleteLongTermSummary(qlonglong id);

private:
    void initDatabase(const QString &dbFilePath);
    QSqlDatabase m_db;
};

#endif // MEMORYMANAGER_H
