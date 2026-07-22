#ifndef HISTORYTURN_H
#define HISTORYTURN_H
#include <QString>
#include <QDateTime>

//聊天记录
struct HistoryTurn {
    qlonglong id = -1;//id
    QDateTime timestamp;//时间戳
    QString userInput;//用户输入
    QString rawReply;//未处理回复
};

//用户画像
struct UserProfile {
    qlonglong id = -1;//id
    QString key;//画像属性
    QString value;//画像属性值
    int tier = 2;//属性半衰期 长中短
    int confidence = 50;//置信度
    QDateTime firstSeen;//首次记录
    QDateTime lastTriggered;//上次触发
    int sessionCount = 1;//出发次数
};

//记忆摘要
struct LongTermSummary {
    qlonglong id = -1;//id
    QString summaryText;//摘要内容
    qlonglong coveredTurnEndId = -1;//摘要末尾
    QString sourceIds; //摘要id
    bool isDirty = false;//摘要状态 0 正常 1 待重建
    QDateTime createdAt;//创建于
    QDateTime updatedAt;//更新于
};


#endif // HISTORYTURN_H
