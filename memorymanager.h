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
    explicit MemoryManager(QObject *parent = nullptr);
    ~MemoryManager();
    bool saveQATurn(const QString &userInput,
                          const QString &rawReply,
                          const QList<SentenceText> &sentences);
    QList<HistoryTurn> getHistoryTurn(int N);

signals:
public:
    void initDatabase();
    QSqlDatabase m_db;
};

#endif // MEMORYMANAGER_H
