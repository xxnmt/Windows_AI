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

// ============== v4: 角色档案 CRUD ==============

bool MemoryManager::upsertCharacterProfile(const QString &subject, const QString &key, const QString &value)
{
    if (!m_db.isOpen() && !m_db.open()) {
        qDebug() << "[MemoryManager]数据库打开失败:" << m_db.lastError();
        return false;
    }
    QSqlQuery query(m_db);
    // key 归一化：同 subject 内用编辑距离匹配已有 key，避免语义重复
    QString normalizedKey = normalizeProfileKey(subject, key);
    // 先尝试查询已有记录（同 subject + normalizedKey）
    query.prepare("SELECT id, value FROM character_profile WHERE subject=? AND key=?");
    query.addBindValue(subject);
    query.addBindValue(normalizedKey);
    if (query.exec() && query.next()) {
        int existingId = query.value(0).toInt();
        QString existingValue = query.value(1).toString();
        // 合并 value：去重合并
        QString mergedValue = mergeProfileValue(normalizedKey, existingValue, value);
        query.prepare("UPDATE character_profile SET value=?, updated_at=? WHERE id=?");
        query.addBindValue(mergedValue);
        query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
        query.addBindValue(existingId);
        if (!query.exec()) {
            qDebug() << "[MemoryManager]更新角色档案失败:" << query.lastError();
            return false;
        }
        qDebug() << "[MemoryManager]角色档案已更新:" << subject << normalizedKey << mergedValue;
        return true;
    } else {
        // 新增
        query.prepare("INSERT OR REPLACE INTO character_profile (subject, key, value, updated_at) VALUES (?, ?, ?, ?)");
        query.addBindValue(subject);
        query.addBindValue(normalizedKey);
        query.addBindValue(value);
        query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
        if (!query.exec()) {
            qDebug() << "[MemoryManager]新增角色档案失败:" << query.lastError();
            return false;
        }
        qDebug() << "[MemoryManager]角色档案已新增:" << subject << normalizedKey << value;
        return true;
    }
}

QList<CharacterProfile> MemoryManager::getCharacterProfiles(const QString &subject)
{
    QList<CharacterProfile> profiles;
    if (!m_db.isOpen() && !m_db.open()) {
        qDebug() << "[MemoryManager]数据库打开失败:" << m_db.lastError();
        return profiles;
    }
    QSqlQuery query(m_db);
    if (subject.isEmpty()) {
        query.prepare("SELECT id, subject, key, value, updated_at FROM character_profile ORDER BY subject, key");
    } else {
        query.prepare("SELECT id, subject, key, value, updated_at FROM character_profile WHERE subject=? ORDER BY key");
        query.addBindValue(subject);
    }
    if (!query.exec()) {
        qDebug() << "[MemoryManager]查询角色档案失败:" << query.lastError();
        return profiles;
    }
    while (query.next()) {
        CharacterProfile p;
        p.id = query.value(0).toLongLong();
        p.subject = query.value(1).toString();
        p.key = query.value(2).toString();
        p.value = query.value(3).toString();
        p.updatedAt = query.value(4).toDateTime();
        profiles.append(p);
    }
    qDebug() << "[MemoryManager]获取角色档案成功，共" << profiles.size() << "条";
    return profiles;
}

bool MemoryManager::deleteCharacterProfile(qlonglong id)
{
    if (!m_db.isOpen() && !m_db.open()) {
        qDebug() << "[MemoryManager]数据库打开失败:" << m_db.lastError();
        return false;
    }
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM character_profile WHERE id=?");
    query.addBindValue(id);
    if (!query.exec()) {
        qDebug() << "[MemoryManager]删除角色档案失败:" << query.lastError();
        return false;
    }
    qDebug() << "[MemoryManager]角色档案已删除 id=" << id;
    return true;
}

// ============== v4: 情景记忆 CRUD ==============

bool MemoryManager::addEpisodicMemory(const QString &content, const QString &type, double importance, const QDateTime &eventTime, const QString &sourceIds)
{
    if (!m_db.isOpen() && !m_db.open()) {
        qDebug() << "[MemoryManager]数据库打开失败:" << m_db.lastError();
        return false;
    }
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO episodic_memory (content, event_time, importance, type, status, last_accessed, last_decay_at, source_ids) "
                  "VALUES (?, ?, ?, ?, 'active', ?, ?, ?)");
    query.addBindValue(content);
    QDateTime et = eventTime.isValid() ? eventTime : QDateTime::currentDateTime();
    query.addBindValue(et.toString("yyyy-MM-dd HH:mm:ss"));
    query.addBindValue(importance);
    query.addBindValue(type.isEmpty() ? "event" : type);
    query.addBindValue(et.toString("yyyy-MM-dd HH:mm:ss"));
    query.addBindValue(et.toString("yyyy-MM-dd HH:mm:ss"));
    query.addBindValue(sourceIds);
    if (!query.exec()) {
        qDebug() << "[MemoryManager]新增情景记忆失败:" << query.lastError();
        return false;
    }
    qDebug() << "[MemoryManager]情景记忆已新增:" << content << "type=" << type << "importance=" << importance;
    return true;
}

QList<EpisodicMemory> MemoryManager::getActiveEpisodicMemories(double minImportance, int limit)
{
    QList<EpisodicMemory> memories;
    if (!m_db.isOpen() && !m_db.open()) {
        qDebug() << "[MemoryManager]数据库打开失败:" << m_db.lastError();
        return memories;
    }
    QSqlQuery query(m_db);
    query.prepare("SELECT id, content, event_time, importance, type, status, last_accessed, source_ids "
                  "FROM episodic_memory WHERE status='active' AND importance >= ? "
                  "ORDER BY importance DESC, event_time DESC LIMIT ?");
    query.addBindValue(minImportance);
    query.addBindValue(limit);
    if (!query.exec()) {
        qDebug() << "[MemoryManager]查询情景记忆失败:" << query.lastError();
        return memories;
    }
    while (query.next()) {
        EpisodicMemory m;
        m.id = query.value(0).toLongLong();
        m.content = query.value(1).toString();
        m.eventTime = query.value(2).toDateTime();
        m.importance = query.value(3).toDouble();
        m.type = query.value(4).toString();
        m.status = query.value(5).toString();
        m.lastAccessed = query.value(6).toDateTime();
        m.sourceIds = query.value(7).toString();
        memories.append(m);
    }

    // 更新 last_accessed
    if (!memories.isEmpty()) {
        QStringList ids;
        for (const auto &m : memories) ids << QString::number(m.id);
        QSqlQuery upd(m_db);
        upd.prepare(QString("UPDATE episodic_memory SET last_accessed=? WHERE id IN (%1)")
                        .arg(ids.join(",")));
        upd.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
        upd.exec();
    }

    qDebug() << "[MemoryManager]获取情景记忆成功，共" << memories.size() << "条";
    return memories;
}

bool MemoryManager::deleteEpisodicMemory(qlonglong id)
{
    if (!m_db.isOpen() && !m_db.open()) {
        qDebug() << "[MemoryManager]数据库打开失败:" << m_db.lastError();
        return false;
    }
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM episodic_memory WHERE id=?");
    query.addBindValue(id);
    if (!query.exec()) {
        qDebug() << "[MemoryManager]删除情景记忆失败:" << query.lastError();
        return false;
    }
    qDebug() << "[MemoryManager]情景记忆已删除 id=" << id;
    return true;
}

bool MemoryManager::updateEpisodicMemoryStatus(qlonglong id, const QString &status)
{
    if (!m_db.isOpen() && !m_db.open()) {
        qDebug() << "[MemoryManager]数据库打开失败:" << m_db.lastError();
        return false;
    }
    QSqlQuery query(m_db);
    query.prepare("UPDATE episodic_memory SET status=? WHERE id=?");
    query.addBindValue(status);
    query.addBindValue(id);
    if (!query.exec()) {
        qDebug() << "[MemoryManager]更新情景记忆状态失败:" << query.lastError();
        return false;
    }
    qDebug() << "[MemoryManager]情景记忆状态已更新 id=" << id << "status=" << status;
    return true;
}

int MemoryManager::decayEpisodicMemory()
{
    if (!m_db.isOpen() && !m_db.open()) {
        qDebug() << "[MemoryManager]数据库打开失败:" << m_db.lastError();
        return -1;
    }
    // 衰减规则：
    // - importance >= 0.8 的里程碑/承诺不衰减
    // - 其余按 -0.05/天 衰减
    // - importance < 0.1 删除
    m_db.transaction();
    try {
        QSqlQuery query(m_db);
        QString decaySql = R"(
            UPDATE episodic_memory
            SET importance = MAX(0, importance - 0.05 * (julianday('now', 'localtime') - julianday(last_decay_at))),
                last_decay_at = datetime('now', 'localtime')
            WHERE importance < 0.8
              AND (julianday('now', 'localtime') - julianday(last_decay_at)) >= 0.0416
        )";
        if (!query.exec(decaySql)) throw query.lastError();
        int updatedRows = query.numRowsAffected();

        QString cleanSql = "DELETE FROM episodic_memory WHERE importance < 0.1 AND status='active'";
        if (!query.exec(cleanSql)) throw query.lastError();
        int deletedRows = query.numRowsAffected();

        m_db.commit();
        if (updatedRows > 0 || deletedRows > 0) {
            qDebug()<<QString("[MemoryManager]情景记忆衰减完成:更新%1条，删除%2条。")
                            .arg(updatedRows).arg(deletedRows);
        } else {
            qDebug()<<"[MemoryManager]情景记忆保持最新，无需衰减。";
        }
        return deletedRows;
    } catch (const QSqlError &err) {
        m_db.rollback();
        qDebug()<<"[MemoryManager]情景记忆衰减失败:"<<err.text();
        return -1;
    }
}

QString MemoryManager::normalizeProfileKey(const QString &subject, const QString &rawKey)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT DISTINCT key FROM character_profile WHERE subject=?");
    query.addBindValue(subject);
    if(!query.exec()){
        qDebug()<<"[MemoryManager]归一化查询失败，保留原key:"<<rawKey;
        return rawKey;
    }
    QStringList existingKeys;
    while(query.next()){
        existingKeys<<query.value(0).toString();
    }
    //全匹配
    if(existingKeys.contains(rawKey)){
        return rawKey;
    }
    //编辑距离模糊匹配（同 subject 内）
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
        qDebug()<<"[MemoryManager]key归一化["<<subject<<"]:"<<rawKey<<"->"<<bestMatch<<"编辑距离="<<minDist;
        return bestMatch;
    }
    qDebug()<<"[MemoryManager]新key维度["<<subject<<"]:"<<rawKey;
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


// ============== 关系状态 CRUD ==============

bool MemoryManager::upsertRelationshipState(const QString &dimension, double delta)
{
    if (!m_db.isOpen() && !m_db.open()) {
        qDebug() << "[MemoryManager]数据库打开失败:" << m_db.lastError();
        return false;
    }
    QSqlQuery query(m_db);
    // 查询当前值
    query.prepare("SELECT value FROM relationship_state WHERE dimension=?");
    query.addBindValue(dimension);
    if (!query.exec() || !query.next()) {
        // 维度不存在，新建（clamp 到 [0, 100]）
        double initVal = qBound(0.0, 30.0 + delta, 100.0);
        query.prepare("INSERT OR REPLACE INTO relationship_state (dimension, value, updated_at) VALUES (?, ?, ?)");
        query.addBindValue(dimension);
        query.addBindValue(initVal);
        query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
        if (!query.exec()) {
            qDebug() << "[MemoryManager]新增关系状态失败:" << query.lastError();
            return false;
        }
        qDebug() << "[MemoryManager]关系状态已新增:" << dimension << "=" << initVal;
        return true;
    }
    double oldVal = query.value(0).toDouble();
    double newVal = qBound(0.0, oldVal + delta, 100.0);
    query.prepare("UPDATE relationship_state SET value=?, updated_at=? WHERE dimension=?");
    query.addBindValue(newVal);
    query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    query.addBindValue(dimension);
    if (!query.exec()) {
        qDebug() << "[MemoryManager]更新关系状态失败:" << query.lastError();
        return false;
    }
    qDebug() << "[MemoryManager]关系状态已更新:" << dimension << oldVal << "->" << newVal << "(delta=" << delta << ")";
    return true;
}

QList<RelationshipState> MemoryManager::getRelationshipStates()
{
    QList<RelationshipState> states;
    if (!m_db.isOpen() && !m_db.open()) {
        qDebug() << "[MemoryManager]数据库打开失败:" << m_db.lastError();
        return states;
    }
    QSqlQuery query(m_db);
    if (!query.exec("SELECT dimension, value, updated_at FROM relationship_state ORDER BY dimension")) {
        qDebug() << "[MemoryManager]查询关系状态失败:" << query.lastError();
        return states;
    }
    while (query.next()) {
        RelationshipState s;
        s.dimension = query.value(0).toString();
        s.value = query.value(1).toDouble();
        s.updatedAt = query.value(2).toDateTime();
        states.append(s);
    }
    qDebug() << "[MemoryManager]获取关系状态成功，共" << states.size() << "条";
    return states;
}

bool MemoryManager::initRelationshipState()
{
    if (!m_db.isOpen() && !m_db.open()) {
        qDebug() << "[MemoryManager]数据库打开失败:" << m_db.lastError();
        return false;
    }
    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM relationship_state");
    query.exec();
    int count = 0;
    if (query.next()) count = query.value(0).toInt();
    if (count > 0) {
        qDebug() << "[MemoryManager]关系状态已存在，无需初始化";
        return true;
    }
    query.prepare("INSERT OR IGNORE INTO relationship_state (dimension, value) VALUES ('intimacy', 30.0)");
    bool ok1 = query.exec();
    query.prepare("INSERT OR IGNORE INTO relationship_state (dimension, value) VALUES ('trust', 30.0)");
    bool ok2 = query.exec();
    qDebug() << "[MemoryManager]关系状态初始化:" << (ok1 && ok2 ? "成功" : "失败");
    return ok1 && ok2;
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
            qDebug()<<"[MemoryManager]正在初始化数据库表结构:";

            // 1. 对话历史表
            QString createChatHistorySql = R"(
                CREATE TABLE IF NOT EXISTS chat_history (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    timestamp DATETIME DEFAULT (datetime('now', 'localtime')),
                    user_input TEXT NOT NULL,
                    raw_reply TEXT NOT NULL
                );
            )";
            if (!query.exec(createChatHistorySql)) {
                qDebug()<<"[MemoryManager]创建chat_history异常:"<<query.lastError();
            }

            // 2. 长期摘要表
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
            if (!query.exec(createSummarySql)) {
                qDebug()<<"[MemoryManager]创建long_term_summary异常:"<<query.lastError();
            }
            query.exec("CREATE INDEX IF NOT EXISTS idx_summary_covered_end ON long_term_summary(covered_turn_end_id);");
            query.exec("CREATE INDEX IF NOT EXISTS idx_summary_is_dirty ON long_term_summary(is_dirty);");

            // 3. 角色档案表（取代已废弃的 user_profile）
            QString createCharProfileSql = R"(
                CREATE TABLE IF NOT EXISTS character_profile (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    subject TEXT NOT NULL,
                    key TEXT NOT NULL,
                    value TEXT NOT NULL,
                    updated_at DATETIME DEFAULT (datetime('now', 'localtime')),
                    UNIQUE(subject, key)
                );
            )";
            if (!query.exec(createCharProfileSql)) {
                qDebug()<<"[MemoryManager]创建character_profile异常:"<<query.lastError();
            }

            // 4. 情景记忆表
            QString createEpisodicSql = R"(
                CREATE TABLE IF NOT EXISTS episodic_memory (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    content TEXT NOT NULL,
                    event_time DATETIME DEFAULT (datetime('now', 'localtime')),
                    importance REAL DEFAULT 0.5,
                    type TEXT DEFAULT 'event',
                    status TEXT DEFAULT 'active',
                    last_accessed DATETIME DEFAULT (datetime('now', 'localtime')),
                    last_decay_at DATETIME DEFAULT (datetime('now', 'localtime')),
                    source_ids TEXT
                );
            )";
            if (!query.exec(createEpisodicSql)) {
                qDebug()<<"[MemoryManager]创建episodic_memory异常:"<<query.lastError();
            }
            query.exec("CREATE INDEX IF NOT EXISTS idx_episodic_importance ON episodic_memory(importance);");
            query.exec("CREATE INDEX IF NOT EXISTS idx_episodic_type ON episodic_memory(type);");
            query.exec("CREATE INDEX IF NOT EXISTS idx_episodic_status ON episodic_memory(status);");

            if (!query.exec("PRAGMA user_version = 1")) {
                qDebug()<<"[MemoryManager]设置数据库版本异常:"<<query.lastError();
            }
            currentVersion = 1;
        }

        // 关系状态表：无条件创建（兼容已有数据库）
        query.exec(R"(
            CREATE TABLE IF NOT EXISTS relationship_state (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                dimension TEXT NOT NULL UNIQUE,
                value REAL NOT NULL,
                updated_at DATETIME DEFAULT (datetime('now', 'localtime'))
            );
        )");
        query.prepare("SELECT COUNT(*) FROM relationship_state");
        query.exec();
        int relCount = 0;
        if (query.next()) relCount = query.value(0).toInt();
        if (relCount == 0) {
            query.prepare("INSERT OR IGNORE INTO relationship_state (dimension, value) VALUES ('intimacy', 30.0)");
            query.exec();
            query.prepare("INSERT OR IGNORE INTO relationship_state (dimension, value) VALUES ('trust', 30.0)");
            query.exec();
            qDebug()<<"[MemoryManager]关系状态已初始化: intimacy=30, trust=30";
        }

        m_db.commit();
        qDebug()<<"[MemoryManager]数据库初始化完成,结构版本为:"<<currentVersion;
    }
    catch(const QSqlError &error){
        m_db.rollback();
        qDebug()<<"[MemoryManager]数据库更新失败:"<<error;
    }
}
