#ifndef HISTORYTURN_H
#define HISTORYTURN_H
#include <QString>
#include <QDateTime>

struct HistoryTurn {
    qlonglong id = -1;
    QDateTime timestamp;
    QString userInput;
    QString rawReply;
};

#endif // HISTORYTURN_H
