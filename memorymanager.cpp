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
    // 1. key 归一化（动态匹配数据库已有 key）
    QString normalizedKey = normalizeProfileKey(key);

    QSqlQuery query(m_db);

    // 2. 先按 (key, tier) 精确匹配
    query.prepare("SELECT id, value, confidence, session_count FROM user_profile WHERE key=? AND tier=?");
    query.addBindValue(normalizedKey);
    query.addBindValue(tier);
    if (!query.exec()) {
        qDebug() << "[MemoryManager]查询画像失败:" << query.lastError();
        return false;
    }

    int existingId = -1;
    QString existingValue;
    int existingConf = 0;
    int existingCount = 0;

    if (query.next()) {
        // 3a. 同 key 同 tier：直接合并
        existingId = query.value(0).toInt();
        existingValue = query.value(1).toString();
        existingConf = query.value(2).toInt();
        existingCount = query.value(3).toInt();
    } else {
        // 3b. 同 key 不同 tier：取最稳定的 tier（数字越小越稳定）
        query.prepare("SELECT id, value, confidence, session_count, tier FROM user_profile WHERE key=? ORDER BY tier ASC LIMIT 1");
        query.addBindValue(normalizedKey);
        if (!query.exec() || !query.next()) {
            // 4. 完全无匹配：INSERT 新记录
            query.prepare("INSERT INTO user_profile (key, value, tier, confidence, first_seen, last_triggered, session_count) "
                          "VALUES (?, ?, ?, ?, ?, ?, 1)");
            query.addBindValue(normalizedKey);
            query.addBindValue(value);
            query.addBindValue(tier);
            query.addBindValue(qMin(100, 50 + confidenceGain));
            query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
            query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
            if (!query.exec()) {
                qDebug() << "[MemoryManager]插入画像失败:" << query.lastError();
                return false;
            }
            qDebug() << "[MemoryManager]画像已新增:" << normalizedKey << "tier=" << tier;
            return true;
        }
        // 取到了同 key 不同 tier 的记录
        existingId = query.value(0).toInt();
        existingValue = query.value(1).toString();
        existingConf = query.value(2).toInt();
        existingCount = query.value(3).toInt();
        int existingTier = query.value(4).toInt();
        // 保留更稳定的 tier（数字越小越稳定）
        tier = qMin(tier, existingTier);
    }

    // 5. 合并 value + 累加 confidence + 累加 session_count
    QString mergedValue = mergeProfileValue(normalizedKey, existingValue, value);
    int newConf = qMin(100, existingConf + confidenceGain);
    int newCount = existingCount + 1;

    query.prepare("UPDATE user_profile SET value=?, tier=?, confidence=?, session_count=?, last_triggered=? WHERE id=?");
    query.addBindValue(mergedValue);
    query.addBindValue(tier);
    query.addBindValue(newConf);
    query.addBindValue(newCount);
    query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    query.addBindValue(existingId);
    if (!query.exec()) {
        qDebug() << "[MemoryManager]更新画像失败:" << query.lastError();
        return false;
    }
    qDebug() << "[MemoryManager]画像已更新:" << normalizedKey
             << "conf=" << newConf << "count=" << newCount;
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
        //标记本次被注入 LLM 的画像
        if (!profiles.isEmpty()) {
            QStringList placeholders;
            for (int i = 0; i < profiles.size(); i++) placeholders << "?";
            QString ph = placeholders.join(",");
            QSqlQuery upd(m_db);
            upd.prepare(QString("UPDATE user_profile SET last_triggered=?, session_count=session_count+1 WHERE id IN (%1)").arg(ph));
            upd.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
            for (const UserProfile &p : profiles) upd.addBindValue(p.id);
            if (!upd.exec()) {
                qDebug() << "[MemoryManager]更新触发时间失败:" << upd.lastError();
            }
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
            SET confidence = CAST(ROUND(confidence - (julianday('now', 'localtime') - julianday(last_decay_at)) *
                CASE tier
                    WHEN 1 THEN :t1
                    WHEN 2 THEN :t2
                    WHEN 3 THEN :t3
                    ELSE 5.0
                END
            ) AS INTEGER),
            last_decay_at = datetime('now', 'localtime')
            WHERE (julianday('now', 'localtime') - julianday(last_decay_at)) >= 0.0416
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
                            .arg(updatedRows).arg(deletedRows);
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

QString MemoryManager::normalizeProfileKey(const QString &rawKey)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT DISTINCT key FROM user_profile");
    if(!query.exec()){
        qDebug()<<"[MemoryManager]:查询失败，保留原key "<<rawKey;
    }
    QStringList existingKeys;
    while(query.next()){
        existingKeys<<query.value(0).toString();
    }
    //全匹配
    if(existingKeys.contains(rawKey)){
        qDebug()<<"[MemoeyManager]:全匹配，无需修正"<<rawKey;
        return rawKey;
    }
    //编辑距离
    int threshold=getEditDistanceThreshold(rawKey);
    QString bestMatch;
    int minDist=threshold+1;
    for (const QString &existingKey : existingKeys) {
        int dist =levenshteinDistance(rawKey, existingKey);
        if (dist <= threshold && dist < minDist) {
            minDist = dist;
            bestMatch = existingKey;
        }
    }
    if (!bestMatch.isEmpty()) {
        qDebug()<<"[MemoryManager]key归一化:"<<rawKey<<"->"<<bestMatch<<"编辑距离="<<minDist;
        return bestMatch;
    }
    //匹配失败，判定为新维度
    qDebug()<<"[MemoryManager]新key维度:"<<rawKey;
    return rawKey;
}

QString MemoryManager::mergeProfileValue(const QString &key, const QString &oldVal, const QString &newVal)
{
    //同key的值value的合并逻辑

    if (newVal.isEmpty()) return oldVal;
    if (newVal.length() >= oldVal.length()) return newVal;
    return oldVal;
}

int MemoryManager::levenshteinDistance(const QString &s1, const QString &s2)
{
    int n=s1.length();
    int m=s2.length();
    if(m==0)return m;
    if(n==0)return n;

    QVector<QVector<int>>dp(n+1,QVector<int>(m+1));

    for (int i = 0; i <= n; i++) dp[i][0] = i;
    for (int j = 0; j <= m; j++) dp[0][j] = j;

    for(int i=1;i<=n;i++){
        for(int j =1;j<=m;j++) {
            int cost=(s1[i-1]==s2[j-1]?0:1);
            dp[i][j]=qMin(qMin(dp[i-1][j]+1,dp[i][j-1]+1),dp[i-1][j-1]+cost);
        }
    }
    return dp[n][m];
}
int MemoryManager::getEditDistanceThreshold(const QString &value)
{
    int len = value.length();
    if (len <= 4) return 1;
    if (len <= 8) return 2;
    if (len <= 12) return 3;
    return 4;
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
                last_decay_at DATETIME DEFAULT (datetime('now', 'localtime')),
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

        if(currentVersion<3){
            qDebug()<<"[MemoryManager]升级v3：为user_profile新增last_decay_at字段，分离衰减与触发时间职责";
            if (!query.exec("ALTER TABLE user_profile ADD COLUMN last_decay_at DATETIME")) {
                qDebug()<<"[MomeryManager]v3新增字段异常:"<<query.lastError();
            }
            // 将现有记录的 last_decay_at 初始化为 last_triggered，避免升级后首次衰减重复扣分
            if (!query.exec("UPDATE user_profile SET last_decay_at = last_triggered WHERE last_decay_at IS NULL")) {
                qDebug()<<"[MomeryManager]v3数据迁移异常:"<<query.lastError();
            }
            if (!query.exec("PRAGMA user_version = 3")){
                qDebug()<<"[MomeryManager]v3数据库异常:"<<query.lastError();
            }
            currentVersion = 3;
        }
        m_db.commit();
        qDebug()<<"[MemoryManager]数据库升级完成,结构版本为:"<<currentVersion;
    }
    catch(const QSqlError &error){
        m_db.rollback();
        qDebug()<<"[MemoryManager]数据库更新失败:"<<error;
    }
}
