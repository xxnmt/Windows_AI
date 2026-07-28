#include "memorymanager.h"

#include <QStandardPaths>
#include <QDir>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

#include "historyturn.h"
MemoryManager::MemoryManager(const QString &dbFilePath,QObject *parent)
    : QObject{parent}
{
    initDatabase(dbFilePath);
}

MemoryManager::~MemoryManager()
{
    if(m_db.isOpen()){
        m_db.close();

        qDebug()<<"[MemoryManager]:Sqlite已关闭";
    }
}

bool MemoryManager::saveQATurn(const QString &userInput, const QString &rawReply, const QList<SentenceText> &sentences)
{
    if(!m_db.isOpen()){
        qDebug()<<"[MemoryManager]:数据库打开失败";
        return false;
    }

    QJsonObject rootObj;
    QJsonArray sentencesArray;
    for (const SentenceText &sentence : sentences) {
        QJsonObject sentenceObj;
        sentenceObj["zh_text"] = sentence.zhText;
        sentenceObj["ja_text"] = sentence.jaText;

        QJsonObject tagsObj;
        for (QMap<QString, QString>::const_iterator it=sentence.rawTags.constBegin();
             it!=sentence.rawTags.constEnd(); it++) {
            tagsObj[it.key()] = it.value();
        }
        sentenceObj["tags"] = tagsObj;
        sentencesArray.append(sentenceObj);
    }
    rootObj["sentences"]=sentencesArray;
    QString JsonStr = QJsonDocument(rootObj).toJson(QJsonDocument::Compact);

    // 执行原子行入库
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO chat_history (user_input, raw_reply) "
                  "VALUES (:user_input, :raw_reply)");
    query.bindValue(":user_input", userInput);
    query.bindValue(":raw_reply", JsonStr);

    if (!query.exec()) {
        qDebug()<<"[MemoryManager]:本轮对话保存失败:"<<query.lastError().text();
        return false;
    }

    qDebug()<<"[MemoryManager]:成功保存本轮对话";
    return true;

}

QList<HistoryTurn> MemoryManager::getHistoryTurn(int N)
{
    QList<HistoryTurn> historyTurnList;
    if(!m_db.open()){
        qDebug()<<"[MemoryManager]:数据库打开失败"<<m_db.lastError();
        return historyTurnList;
    }
    if(N<=0){
        return historyTurnList;
    }

    QSqlQuery query(m_db);
    QString sql = R"(
        SELECT user_input, raw_reply,timestamp
        FROM (
            SELECT id, user_input, raw_reply,timestamp
            FROM chat_history
            ORDER BY id DESC
            LIMIT :limit
        ) ORDER BY id ASC;
    )";
    query.prepare(sql);
    query.bindValue(":limit",N);
    if(!query.exec()){
        qDebug()<<"[MemoryManager]获取历史记录失败:"<<query.lastError();
    }
    while(query.next()){
        HistoryTurn turn;
        turn.userInput = query.value("user_input").toString();
        turn.rawReply = query.value("raw_reply").toString();
        turn.timestamp = query.value("timestamp").toDateTime();
        historyTurnList.append(turn);
    }
    qDebug()<<"[MemoryManager]获取历史记录成功";
    return historyTurnList;
}

QList<HistoryTurn> MemoryManager::getHistoryTurn(int offset, int limit)
{
    QList<HistoryTurn> historyList;
    if(!m_db.open()){
        qDebug()<<"[MemoryManager]数据库打开失败:"<<m_db.lastError();
        return historyList;
    }
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT id, user_input, raw_reply, timestamp
        FROM chat_history
        ORDER BY id DESC
        LIMIT :limit OFFSET :offset
    )");
    query.bindValue(":limit", limit);
    query.bindValue(":offset", offset);
    if(!query.exec()){
        qDebug()<<"[MemoryManager]分页查询失败:"<<query.lastError();
        return historyList;
    }
    while (query.next()) {
        HistoryTurn turn;
        turn.id = query.value("id").toInt();
        turn.userInput = query.value("user_input").toString();
        turn.rawReply = query.value("raw_reply").toString();
        turn.timestamp = query.value("timestamp").toDateTime();
        historyList.append(turn);
    }
    qDebug()<<"[MemoryManager]分页查询成功";
    return historyList;

}

qlonglong MemoryManager::getTotalHistoryCount()
{
    if(!m_db.open()){
        qDebug()<<"[MemoryManager]数据库打开失败:"<<m_db.lastError();
        return 0;
    }
    QSqlQuery query(m_db);
    if (query.exec("SELECT COUNT(*) FROM chat_history")) {
        if (query.next()) {
                qDebug()<<"[MemoryManager]获取历史总记录数成功";
            return query.value(0).toInt();
        }
    }
    qDebug()<<"[MemoryManager]获取历史总记录数（0）成功";
    return 0;
}

bool MemoryManager::deleteTurnByID(int id)
{
    if(!m_db.open()){
        qDebug()<<"[MemoryManager]数据库打开失败:"<<m_db.lastError();
        return false;
    }
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM chat_history WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug()<<"[MemoryManager]删除id:"<<id<<"的记录失败:"<<query.lastError();
        return false;
    }
    qDebug()<<"[MemoryManager]删除id:"<<id<<"的记录成功";
    return true;
}

bool MemoryManager::clearAllHistory()
{
    if(!m_db.open()){
        qDebug()<<"[MemoryManager]数据库打开失败:"<<m_db.lastError();
        return false;
    }
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM chat_history");
    if (!query.exec()) {
        qDebug()<<"[MemoryManager]清空记录失败:"<<query.lastError();
        return false;
    }
    qDebug()<<"[MemoryManager]清空记录成功";
    return true;
}

QList<HistoryTurn> MemoryManager::getUnsummarizedTurns(qlonglong &outLastEndId, QList<qlonglong> &outSourceIds)
{
    QList<HistoryTurn> turns;
    outSourceIds.clear();
    outLastEndId = -1;
    if(!m_db.open()){
        qDebug()<<"[MemoryManager]数据库打开失败:"<<m_db.lastError();
        return turns;
    }
    QSqlQuery query(m_db);
    QString sql = R"(
        SELECT id, user_input, raw_reply
        FROM chat_history
        WHERE id > (SELECT COALESCE(MAX(covered_turn_end_id), 0) FROM long_term_summary)
        ORDER BY id ASC;
    )";
    if(query.exec(sql)){
        while(query.next()){
            qlonglong id=query.value("id").toLongLong();
            HistoryTurn turn;
            turn.id = id;
            turn.userInput = query.value("user_input").toString();
            turn.rawReply = query.value("raw_reply").toString();

            turns.append(turn);
            outSourceIds.append(id);
            outLastEndId = id;
        }
    }
    else{
        qDebug()<<"[MemoryManager]获取未摘要历史失败"<<query.lastError();
    }
    qDebug()<<"[MemoryManager]获取未摘要历史成功";
    return turns;
}

bool MemoryManager::upsertUserProfile(const QString &key, const QString &value, int tier, int confidenceGain)
{
    if(!m_db.open()){
        qDebug()<<"[MemoryManager]数据库打开失败:"<<m_db.lastError();
        return false;
    }

    QSqlQuery query(m_db);
    QString sql = R"(
        INSERT INTO user_profile (key, value, tier, confidence, session_count, last_triggered)
        VALUES (:key, :value, :tier, 50 + :gain, 1, datetime('now', 'localtime'))
        ON CONFLICT(key, value) DO UPDATE SET
            confidence = MIN(100, confidence + :gain),
            session_count = session_count + 1,
            last_triggered = datetime('now', 'localtime'),
            tier = excluded.tier
    )";

    query.prepare(sql);
    query.bindValue(":key", key);
    query.bindValue(":value", value);
    query.bindValue(":tier", tier);
    query.bindValue(":gain", confidenceGain);

    if (!query.exec()) {
        qDebug()<<"[MemoryManager]更新用户画像失败:"<<query.lastError();
        return false;
    }
    qDebug()<<"[MemoryManager]更新用户画像成功";
    return true;
}


QList<UserProfile> MemoryManager::getActiveUserProfiles(int minConfidence)
{
    QList<UserProfile> profiles;
    if(!m_db.open()){
        qDebug()<<"[MemoryManager]数据库打开失败:"<<m_db.lastError();
        return profiles;
    }
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM user_profile WHERE confidence >= :minConf ORDER BY tier ASC, confidence DESC");
    query.bindValue(":minConf", minConfidence);

    if (query.exec()) {
        while (query.next()) {
            UserProfile p;
            p.id = query.value("id").toLongLong();
            p.key = query.value("key").toString();
            p.value = query.value("value").toString();
            p.tier = query.value("tier").toInt();
            p.confidence = query.value("confidence").toInt();
            p.firstSeen = query.value("first_seen").toDateTime();
            p.lastTriggered = query.value("last_triggered").toDateTime();
            p.sessionCount = query.value("session_count").toInt();
            profiles.append(p);
        }
    }
    else{
        qDebug()<<"[MemoryManager]获取用户画像失败:"<<query.lastError();
        return profiles;
    }
    qDebug()<<"[MemoryManager]获取用户画像成功";
    return profiles;
}

bool MemoryManager::deleteUserProfile(qlonglong id)
{
    if(!m_db.open()){
        qDebug()<<"[MemoryManager]数据库打开失败:"<<m_db.lastError();
        return false;
    }
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM user_profile WHERE id = :id");
    query.bindValue(":id", id);
    if (!query.exec()) {
        qDebug()<<"[MemoryManager]获取记忆摘要失败:"<<query.lastError();
        return false;
    }
    qDebug()<<"[MemoryManager]获取记忆摘要成功";
    return true;
}

int MemoryManager::scanAndApplyProfileDecay()
{
    if(!m_db.open()){
        qDebug()<<"[MemoryManager]数据库打开失败:"<<m_db.lastError();
        return -1;
    }
    //半衰期，可考虑设置在config
    double tier1Decay = 0.8;  // Tier 1 长期: -0.8/天
    double tier2Decay = 5.0;  // Tier 2 中期: -5.0/天
    double tier3Decay = 25.0; // Tier 3 短期: -25.0/天

    m_db.transaction();
    try {
        QSqlQuery query(m_db);
        QString decaySql = R"(
            UPDATE user_profile
            SET confidence = CAST(ROUND(confidence - (julianday('now', 'localtime') - julianday(last_triggered)) *
                CASE tier
                    WHEN 1 THEN :t1
                    WHEN 2 THEN :t2
                    WHEN 3 THEN :t3
                    ELSE 5.0
                END
            ) AS INTEGER),
            last_triggered = datetime('now', 'localtime')
            WHERE (julianday('now', 'localtime') - julianday(last_triggered)) >= 0.0416;
        )";

        query.prepare(decaySql);
        query.bindValue(":t1", tier1Decay);
        query.bindValue(":t2", tier2Decay);
        query.bindValue(":t3", tier3Decay);

        if (!query.exec()) throw query.lastError();
        int updatedRows = query.numRowsAffected();

        QString cleanSql = "DELETE FROM user_profile WHERE confidence <= 0;";
        if (!query.exec(cleanSql)) throw query.lastError();
        int deletedRows = query.numRowsAffected();

        m_db.commit();

        if (updatedRows > 0 || deletedRows > 0) {
            qDebug()<<QString("[MemoryManager]全量衰减扫描完成:衰减更新%1条，自动遗忘清理%2条过期画像。")
                            .arg(updatedRows,deletedRows);
        }
        else {
            qDebug()<<"[MemoryManager]画像保持最新，无需衰减。";
        }

        return deletedRows;

    } catch (const QSqlError &err) {
        m_db.rollback();
        qDebug()<<"[MemoryManager]时间衰减扫描失败:"<<err.text();
        return -1;
    }
}

bool MemoryManager::addLongTermSummary(const QString &summaryText, qlonglong coveredEndId, const QString &sourceIdsJson)
{
    if(!m_db.open()){
        qDebug()<<"[MemoryManager]数据库打开失败:"<<m_db.lastError();
        return false;
    }
    QSqlQuery query(m_db);
    QString sql = R"(
        INSERT INTO long_term_summary (summary_text, covered_turn_end_id, source_ids, is_dirty)
        VALUES (:text, :end_id, :source_ids, 0)
    )";

    query.prepare(sql);
    query.bindValue(":text", summaryText);
    query.bindValue(":end_id", coveredEndId);
    query.bindValue(":source_ids", sourceIdsJson);

    if (!query.exec()) {
        qDebug()<<"[MemoryManager]添加记忆摘要失败:"<<query.lastError();
        return false;
    }
    qDebug()<<"[MemoryManager]添加记忆摘要成功";
    return true;
}

QList<LongTermSummary> MemoryManager::getLatestSummaries(int limit)
{
    QList<LongTermSummary> summaries;
    if(!m_db.open()){
        qDebug()<<"[MemoryManager]:数据库打开失败"<<m_db.lastError();
        return summaries;
    }
    if(limit<=0){
        return summaries;
    }

    QSqlQuery query(m_db);
    QString sql = R"(
        SELECT * FROM (
            SELECT * FROM long_term_summary
            WHERE is_dirty = 0
            ORDER BY id DESC
            LIMIT :limit
        ) ORDER BY id ASC
    )";

    query.prepare(sql);
    query.bindValue(":limit", limit);

    if (query.exec()) {
        while (query.next()) {
            LongTermSummary s;
            s.id = query.value("id").toLongLong();
            s.summaryText = query.value("summary_text").toString();
            s.coveredTurnEndId = query.value("covered_turn_end_id").toLongLong();
            s.sourceIds = query.value("source_ids").toString();
            s.isDirty = query.value("is_dirty").toBool();
            s.createdAt = query.value("created_at").toDateTime();
            s.updatedAt = query.value("updated_at").toDateTime();
            summaries.append(s);
        }
    }
    else{
        if (!query.exec()) {
            qDebug()<<"[MemoryManager]获取记忆摘要失败:"<<query.lastError();
            return summaries;
        }
    }
    qDebug()<<"[MemoryManager]获取记忆摘要成功";
    return summaries;
}

bool MemoryManager::deleteLongTermSummary(qlonglong id)
{
    if(!m_db.open()){
        qDebug()<<"[MemoryManager]数据库打开失败:"<<m_db.lastError();
        return false;
    }
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM long_term_summary WHERE id = :id");
    query.bindValue(":id", id);
    if (!query.exec()) {
        qDebug()<<"[MemoryManager]删除记忆摘要失败:"<<query.lastError();
        return false;
    }
    qDebug()<<"[MemoryManager]删除记忆摘要成功";
    return true;
}


void MemoryManager::initDatabase(const QString &dbFilePath)
{
    QDir dbDir = QFileInfo(dbFilePath).dir();
    if (!dbDir.exists()) {
        if (!dbDir.mkpath(".")) {
            qCritical() << "[MemoryManager] 无法创建数据库目录:" << dbDir.absolutePath();
            return;
        }
    }



    const QString connName = "memory_db_connection";
    if (QSqlDatabase::contains(connName)) {
        m_db = QSqlDatabase::database(connName);
    }
    else
    {
        m_db = QSqlDatabase::addDatabase("QSQLITE", connName);
    }
    m_db.setDatabaseName(dbFilePath);


    if(!m_db.open()){
        qDebug()<<"[MemoryManager]数据库打开失败:"<<m_db.lastError();
        return;
    }

    qDebug()<<"[MemoryManager]数据库连接成功:"<<m_db;

    QSqlQuery query(m_db);
    int currentVersion = 0;
    if (query.exec("PRAGMA user_version")) {
        if (query.next()) {
            currentVersion = query.value(0).toInt();
        }
    }
    qDebug()<<"[MemoryManager]当前数据库版本:"<<currentVersion;
    m_db.transaction();

    try {
        if (currentVersion<1) {
            qDebug()<<"[MemoryManager]没有数据库，正在创建对话记录表:";
            QString createTableSql = R"(
                CREATE TABLE IF NOT EXISTS chat_history (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    timestamp DATETIME DEFAULT (datetime('now', 'localtime')),
                    user_input TEXT NOT NULL,
                    raw_reply TEXT NOT NULL
                );
            )";
            if (!query.exec(createTableSql)){
                qDebug()<<"[MomeryManager]创建v1数据库异常:"<<query.lastError();
            }
            if (!query.exec("PRAGMA user_version = 1")){
                qDebug()<<"[MomeryManager]v1数据库异常:"<<query.lastError();
            }
            currentVersion = 1;
        }
        m_db.commit();
        qDebug()<<"[MemoryManager]数据库初始化完成,结构版本为:"<<currentVersion;

        if(currentVersion<2){
            qDebug()<<"[MemoryManager]没有更新v2数据库，正在创建用户画像记录表和摘要表:";
            QString createUserProfileSql = R"(
            CREATE TABLE IF NOT EXISTS user_profile (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                key TEXT NOT NULL,
                value TEXT NOT NULL,
                tier INTEGER DEFAULT 2,
                confidence INTEGER DEFAULT 50,
                first_seen DATETIME DEFAULT (datetime('now', 'localtime')),
                last_triggered DATETIME DEFAULT (datetime('now', 'localtime')),
                session_count INTEGER DEFAULT 1,
                UNIQUE(key, value)
            );
        )";
            if (!query.exec(createUserProfileSql)){
                qDebug()<<"[MomeryManager]创建v2用户画像数据库异常:"<<query.lastError();
            }

            query.exec("CREATE INDEX IF NOT EXISTS idx_user_profile_tier ON user_profile(tier);");
            query.exec("CREATE INDEX IF NOT EXISTS idx_user_profile_confidence ON user_profile(confidence);");
            query.exec("CREATE INDEX IF NOT EXISTS idx_user_profile_last_triggered ON user_profile(last_triggered);");

            QString createSummarySql = R"(
            CREATE TABLE IF NOT EXISTS long_term_summary (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                summary_text TEXT NOT NULL,
                covered_turn_end_id INTEGER NOT NULL,
                source_ids TEXT NOT NULL,
                is_dirty INTEGER DEFAULT 0,
                created_at DATETIME DEFAULT (datetime('now', 'localtime')),
                updated_at DATETIME DEFAULT (datetime('now', 'localtime'))
            );
        )";
            if (!query.exec(createSummarySql)){
                qDebug()<<"[MomeryManager]创建v2摘要数据库异常:"<<query.lastError();
            }
            query.exec("CREATE INDEX IF NOT EXISTS idx_summary_covered_end ON long_term_summary(covered_turn_end_id);");
            query.exec("CREATE INDEX IF NOT EXISTS idx_summary_is_dirty ON long_term_summary(is_dirty);");
            if (!query.exec("PRAGMA user_version = 2")){
                qDebug()<<"[MomeryManager]v2数据库异常:"<<query.lastError();
            }
            currentVersion = 2;
        }
        m_db.commit();
        qDebug()<<"[MemoryManager]数据库升级完成,结构版本为:"<<currentVersion;
    }
    catch(const QSqlError &error){
        m_db.rollback();
        qDebug()<<"[MemoryManager]数据库更新失败:"<<error;
    }
}
