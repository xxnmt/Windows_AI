#include "memorymanager.h"

#include <QStandardPaths>
#include <QDir>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

#include "configmanager.h"
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

    QJsonArray jsonArray;
    for (const SentenceText &sentence : sentences) {
        QJsonObject sentenceObj;
        sentenceObj["zh_text"] = sentence.zhText;
        sentenceObj["ja_text"] = sentence.jaText;

        QJsonObject tagsObj;
        for (QMap<QString, QString>::const_iterator it = sentence.rawTags.constBegin(); it != sentence.rawTags.constEnd(); ++it) {
            tagsObj[it.key()] = it.value();
        }
        sentenceObj["tags"] = tagsObj;

        jsonArray.append(sentenceObj);
    }
    QString parsedJsonStr = QJsonDocument(jsonArray).toJson(QJsonDocument::Compact);

    // 执行原子行入库
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO chat_history (user_input, raw_reply, parsed_json) "
                  "VALUES (:user_input, :raw_reply, :parsed_json)");
    query.bindValue(":user_input", userInput);
    query.bindValue(":raw_reply", rawReply);
    query.bindValue(":parsed_json", parsedJsonStr);

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
    if(!m_db.isOpen()){
        qDebug()<<"[MemoryManager]:数据库打开失败"<<m_db.lastError();
        return historyTurnList;
    }
    if(N<=0){
        return historyTurnList;
    }

    QSqlQuery query(m_db);
    QString sql = R"(
        SELECT user_input, raw_reply FROM (
            SELECT id, user_input, raw_reply
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
        historyTurnList.append(turn);
    }
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
        qDebug()<<"[MemoryManager]查询失败:"<<query.lastError();
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
    return historyList;

}

qlonglong MemoryManager::getTotalHistoryCount()
{
    if(!m_db.isOpen()){
        qDebug()<<"[MemoryManager]数据库打开失败:"<<m_db.lastError();
        return 0;
    }
    QSqlQuery query(m_db);
    if (query.exec("SELECT COUNT(*) FROM chat_history")) {
        if (query.next()) {
            return query.value(0).toInt();
        }
    }
    return 0;
}

bool MemoryManager::deleteTurnByID(int id)
{
    if(!m_db.isOpen()){
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
        if (currentVersion == 0) {
            // V1 版本：初始建表
            QString createTableSql = R"(
                CREATE TABLE IF NOT EXISTS chat_history (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    timestamp DATETIME DEFAULT (datetime('now', 'localtime')),
                    user_input TEXT NOT NULL,
                    raw_reply TEXT NOT NULL,
                    parsed_json TEXT NOT NULL
                );
            )";
            if (!query.exec(createTableSql)) throw query.lastError();

            // 标记版本升级为 1
            if (!query.exec("PRAGMA user_version = 1")) throw query.lastError();
            currentVersion = 1;
        }
        m_db.commit();
        qDebug()<<"[MemoryManager]数据库初始化（升级）完成,结构版本为:"<<currentVersion;
    }
    catch(const QSqlError &error){
        m_db.rollback();
        qDebug()<<"[MemoryManager]数据库更新失败:"<<error;
    }


}
