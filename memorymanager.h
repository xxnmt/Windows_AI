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

    QList<HistoryTurn> getHistoryTurn(int N);
    QList<HistoryTurn> getHistoryTurn(int offset,int limit);
    qlonglong getTotalHistoryCount();

    bool deleteTurnByID(int id);
    bool clearAllHistory();


signals:

private:
    void initDatabase(const QString &dbFilePath);
    QSqlDatabase m_db;
};

#endif // MEMORYMANAGER_H
