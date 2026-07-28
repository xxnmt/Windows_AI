#include "llmservice.h"
#include "configmanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QPointer>
#include "memorymanager.h"


LLMService::LLMService(const QString &apiKey,QObject *parent)
{
    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &LLMService::onReplyFinished);
    m_extractManager=new QNetworkAccessManager(this);

    qRegisterMetaType<QList<SentenceText>>("QList<SentenceText>");
    m_apiKey=apiKey;

    QString configDirPath=ConfigManager::instance().getConfigDirPath();
    m_localPromptPath=QDir(configDirPath).filePath("prompt.txt");

    initializePromptFile();
    m_systemPromptCache=loadSystemPrompt();
}


void LLMService::askDeepSeek(const QString &userInput, const QList<HistoryTurn> &historyQA)
{


    QUrl url("https://api.deepseek.com/chat/completions");
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    //define YOUR_API_KEY into AI api key
    QString aotuHeader="Bearer "+m_apiKey;
    request.setRawHeader("Authorization",aotuHeader.toUtf8());
    m_systemPromptCache=loadSystemPrompt();

    //prompt
    QString finalSystemPrompt = m_systemPromptCache;

    // QJsonArray messagesArray;
    // QJsonObject systemMessage;
    // systemMessage["role"] = "system";
    // systemMessage["content"] = finalSystemPrompt;
    // messagesArray.append(systemMessage);
    //用户画像
    if (m_memoryManager) {
        QList<UserProfile> profiles = m_memoryManager->getActiveUserProfiles(30); // 阈值 30%
        qDebug()<<"[LLM]画像数量:"<<profiles.size();
        if (!profiles.isEmpty()) {
            finalSystemPrompt += "\n\n【关于欧尼酱的长期认知】\n";
            for (const UserProfile &p : profiles) {
                finalSystemPrompt += QString("- %1: %2（置信度 %3%）\n")
                                         .arg(p.key)
                                         .arg(p.value)
                                         .arg(p.confidence);
            }
        }
        //记忆摘要
        QList<LongTermSummary> summaries = m_memoryManager->getLatestSummaries(5);
        qDebug()<<"[LLM]摘要数量:"<<summaries.size();
        if (!summaries.isEmpty()) {
            finalSystemPrompt += "\n【过往重要回忆】\n";
            for (const auto &s : summaries) {
                finalSystemPrompt += QString("- %1\n").arg(s.summaryText);
            }
        }
    }

    QJsonArray messagesArray;
    QJsonObject systemMessage;
    systemMessage["role"] = "system";
    systemMessage["content"] = finalSystemPrompt;
    messagesArray.append(systemMessage);

    //短期记忆
    if (!historyQA.isEmpty()) {
        QJsonObject historyNote;
        historyNote["role"] = "system";
        historyNote["content"] = "以下内容为短期对话记录，其中时间戳供你感知时间，所有回答禁止携带时间戳。";
        messagesArray.append(historyNote);
    }
    for (const HistoryTurn &turn : historyQA) {
        // 历史用户输入
        QJsonObject histUserMsg;
        histUserMsg["role"] = "user";
        histUserMsg["name"] = QString("用户 [%1]").arg(turn.timestamp.toString("yyyy-MM-dd HH:mm:ss"));
        histUserMsg["content"] = turn.userInput;
        messagesArray.append(histUserMsg);

        // 历史茉子回复
        QJsonObject histMakoMsg;
        histMakoMsg["role"] = "assistant";
        histMakoMsg["name"] = QString("茉子 [%1]").arg(turn.timestamp.toString("yyyy-MM-dd HH:mm:ss"));
        histMakoMsg["content"] = turn.rawReply;
        messagesArray.append(histMakoMsg);
    }

    if (m_stateProvider) {
        QString currentUiState = m_stateProvider();
        QJsonObject envMsg;
        envMsg["role"] = "system";
        envMsg["content"] = QString(
                                "[当前环境信息]\n"
                                "茉子当前状态：\n%1\n"
                                "现在的时间是 %2\n"
                                "请根据时间流逝合理感知。"
                                ).arg(currentUiState,
                                     QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
        messagesArray.append(envMsg);
    }
    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = userInput;
    messagesArray.append(userMessage);
    qDebug().noquote()<<"[LLM]最终注入的Message:"<<QJsonDocument(messagesArray)
                                                           .toJson(QJsonDocument::Indented);

    QJsonObject rootObj;
    rootObj["model"] = "deepseek-v4-pro";
    rootObj["messages"] = messagesArray;
    rootObj["temperature"] = 0.7;

    QByteArray postData = QJsonDocument(rootObj).toJson();
    m_networkManager->post(request, postData);

    qDebug() << "[LLM]茉子带着"<<historyQA.size()<<"轮短期记忆正在思考......";
}

void LLMService::registerStateProvider(std::function<QString ()> provider)
{
    m_stateProvider=provider;
}

void LLMService::setMemoryManager(MemoryManager *manager)
{
    m_memoryManager=manager;
}

void LLMService::extractMemoryAsync(const QList<HistoryTurn> &turns, qlonglong lastEndId, const QList<qlonglong> &sourceIds)
{
    if (turns.isEmpty()) {
        qDebug()<<"[LLM]待摘要turns为空";
        return;
    }
    QUrl url("https://api.deepseek.com/chat/completions");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());
    QString extractionPrompt = R"(
你是一个记忆分析与总结专家。请分析以下给出的多轮用户与AI的对话，提取：
1. 用户画像 (profiles): 关于用户的已知事实（如职业、偏好、习惯、性格、今日情绪等）。
   - tier 1: 长期核心事实 (如职业、基本性格、长期爱好)
   - tier 2: 中期行为模式 (如近期工作安排、本周习惯)
   - tier 3: 短期临时状态 (如今日心情、刚刚提及的即时打算)
2. 剧情摘要 (summary): 用一句话概括这段对话的核心内容（50字以内）。

请严格输出合法 JSON，不要包含 markdown 代码块包裹标记（如 ```json），格式规范如下：
{
  "profiles": [
    {"key": "职业", "value": "程序员", "tier": 1},
    {"key": "今日状态", "value": "在写C++代码", "tier": 3}
  ],
  "summary": "用户与AI讨论了长期记忆系统的开发计划。"
}
)";
    QString conversationText;
    for (const auto &turn : turns) {
        conversationText += QString("用户: %1\nAI: %2\n---\n").arg(turn.userInput, turn.rawReply);
    }

    QJsonArray messagesArray;
    QJsonObject sysMsg; sysMsg["role"] = "system"; sysMsg["content"] = extractionPrompt;
    QJsonObject userMsg; userMsg["role"] = "user"; userMsg["content"] = conversationText;
    messagesArray.append(sysMsg);
    messagesArray.append(userMsg);

    QJsonObject rootObj;
    rootObj["model"] = "deepseek-v4-flash";
    rootObj["messages"] = messagesArray;
    rootObj["temperature"] = 0.3;

    QByteArray postData = QJsonDocument(rootObj).toJson();

    QJsonArray sourceJsonArray;
    for (qlonglong id : sourceIds) sourceJsonArray.append(id);
    QString sourceIdsJson = QJsonDocument(sourceJsonArray).toJson(QJsonDocument::Compact);

    QNetworkReply *reply = m_extractManager->post(request, postData);

    QPointer<LLMService> self(this);

    connect(reply, &QNetworkReply::finished, this, [this,self, reply, lastEndId, sourceIdsJson]() {
        if(self.isNull()) {
            reply->deleteLater();
            qDebug()<<"[LLM]摘要捕获异常";
            return;
        }
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            QString replyText = jsonDoc.object()["choices"].toArray()[0].toObject()["message"].toObject()["content"].toString();

            // 容错处理：清除大模型可能附带的 Markdown 代码块标记 (```json 和 ```)
            replyText.replace(QRegularExpression("```json|```", QRegularExpression::CaseInsensitiveOption), "");

            QJsonDocument resultDoc = QJsonDocument::fromJson(replyText.toUtf8());
            if (resultDoc.isObject()) {
                QJsonObject rootObj = resultDoc.object();
                QJsonArray profiles = rootObj["profiles"].toArray();
                QString summary = rootObj["summary"].toString();

                qDebug()<<"[LLM]记忆提取成功，画像数:"<< profiles.size()<<"摘要:"<< summary;
                emit memoryExtractionReady(profiles, summary, lastEndId, sourceIdsJson);
            } else {
                qDebug()<<"[LLM]记忆提取返回的JSON格式解析失败:"<<replyText;
            }
        } else {
            qDebug()<<"[LLM]记忆提取网络错误:"<<reply->errorString();
        }
        reply->deleteLater();
    });
}

void LLMService::setApiKey(const QString &apiKey)
{
    m_apiKey=apiKey;
}


void LLMService::onReplyFinished(QNetworkReply *reply)
{
    if(reply->error()==QNetworkReply::NoError){
        QByteArray responseData = reply->readAll();

        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        QJsonObject rootObj = jsonDoc.object();
        QJsonArray choices = rootObj["choices"].toArray();

        if(!choices.isEmpty()){
            QJsonObject messageObj = choices[0].toObject()["message"].toObject();
            QString replyText = messageObj["content"].toString();

            qDebug()<<"[LLM]茉子回复(未处理):"<<replyText;

            QPair<QList<SentenceText>, QString> parseResult=parseJsonReply(replyText);
            QList<SentenceText> parsedSentences = parseResult.first;
            QString correctedReply = parseResult.second;
//------------------------------更换为json，原格式废弃------------------------------
            // //正则表达式拆分
            // QRegularExpression sentenceRegex("((?:\\[[^\\]]+\\])+)([^\\[]+)");
            // QRegularExpressionMatchIterator sentenceIt = sentenceRegex.globalMatch(replyText);

            // while (sentenceIt.hasNext()) {
            //     QRegularExpressionMatch sentenceMatch=sentenceIt.next();
            //     //剥离多层标签
            //     QString tags=sentenceMatch.captured(1);
            //     QString zhText=sentenceMatch.captured(2).trimmed();

            //     SentenceText sentence;
            //     sentence.zhText=zhText;

            //     //匹配日文
            //     QRegularExpression tagRegex("\\[([a-zA-Z0-9_]+):([^\\]]+)\\]");
            //     QRegularExpressionMatchIterator tagIt= tagRegex.globalMatchView(tags);

            //     while (tagIt.hasNext()) {
            //         QRegularExpressionMatch tagMatch = tagIt.next();
            //         QString key = tagMatch.captured(1);
            //         QString value = tagMatch.captured(2);

            //         if (key == "ja") {
            //             sentence.jaText = value;
            //         } else {
            //             sentence.rawTags.insert(key, value);
            //         }
            //     }
            //     parsedSentences.append(sentence);
            // }

            qDebug()<<"[LLM]成功拆分为"<<parsedSentences.size()<<"个段落：";
            for (int i=0;i<parsedSentences.size();i++) {
                const auto &s = parsedSentences[i];
                qDebug()<<QString("--------- [第 %1 句] ---------").arg(i + 1);
                qDebug()<<"[LLM]中文气泡:"<<s.zhText;
                qDebug()<<"[LLM]日文音频目标:"<<s.jaText;
                qDebug()<<"[LLM]四维状态变更:"<<s.rawTags;
            }
            //拆完传信号
            emit sentencesReady(parsedSentences,correctedReply);
        }
    }
    else {
        qDebug()<<"[LLM]网络错误:"<<reply->errorString();
        emit internetErrorSignal(reply->errorString());
    }
    reply->deleteLater();
}

// void LLMService::onExtractReplyFinished(QNetworkReply *reply)
// {
//         if (reply->error() == QNetworkReply::NoError) {
//             QByteArray responseData = reply->readAll();
//             QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
//             QString replyText = jsonDoc.object()["choices"].toArray()[0].toObject()["message"].toObject()["content"].toString();

//             // 容错处理：清除大模型可能附带的 Markdown 代码块标记 (```json 和 ```)
//             replyText.replace(QRegularExpression("```json|```", QRegularExpression::CaseInsensitiveOption), "");

//             QJsonDocument resultDoc = QJsonDocument::fromJson(replyText.toUtf8());
//             if (resultDoc.isObject()) {
//                 QJsonObject rootObj = resultDoc.object();
//                 QJsonArray profiles = rootObj["profiles"].toArray();
//                 QString summary = rootObj["summary"].toString();

//                 qDebug()<<"[LLM]记忆提取成功，画像数:"<< profiles.size()<<"摘要:"<< summary;
//                 emit memoryExtractionReady(profiles, summary, lastEndId, sourceIdsJson);
//             } else {
//                 qDebug()<<"[LLM]记忆提取返回的JSON格式解析失败:"<<replyText;
//             }
//         } else {
//             qDebug()<<"[LLM]记忆提取网络错误:"<<reply->errorString();
//         }
//         reply->deleteLater();
// }

void LLMService::initializePromptFile()
{
    QFile localFile(m_localPromptPath);
    if(!localFile.exists()){
        qDebug()<<"[LLM]:prompt寻找失败，自动创建默认prompt";
        if (QFile::copy(":/image/default_config/prompt.txt", m_localPromptPath)) {
            QFile::setPermissions(m_localPromptPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
            qDebug()<<"[LLM]默认prompt成功释放至:"<<m_localPromptPath;
        }
        else{
            qDebug()<<"[LLM]默认prompt释放失败";
        }
    }
}

QString LLMService::loadSystemPrompt()
{
    QFile file(m_localPromptPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning()<<"[LLM]无法读取本地prompt，使用内置默认prompt:"<<m_localPromptPath;
        file.setFileName(":/prompts/system_prompt.txt");
    }
    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString content = in.readAll();
    return content;
}

QPair<QList<SentenceText>, QString> LLMService::parseJsonReply(const QString &replyText)
{
    QList<SentenceText> result;
    QString currentReply=replyText;

    // 容错处理：清除 Markdown 代码块标记
    QString cleanText = replyText;
    cleanText.replace(QRegularExpression("```json|```", QRegularExpression::CaseInsensitiveOption), "");
    cleanText = cleanText.trimmed();

    // 尝试直接解析
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(cleanText.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "[LLM] JSON 解析失败:" << parseError.errorString();

        // 尝试提取 JSON 对象（容忍前后有多余文本）
        QRegularExpression jsonRegex("\\{[\\s\\S]*\\}");
        QRegularExpressionMatch match = jsonRegex.match(cleanText);
        if (match.hasMatch()) {
            doc = QJsonDocument::fromJson(match.captured(0).toUtf8(), &parseError);
            if (parseError.error != QJsonParseError::NoError) {
                qDebug() << "[LLM] JSON 提取后仍无法解析";
                return qMakePair(result,currentReply);
            }
            qDebug() << "[LLM] JSON 提取成功";
        } else {
            qDebug() << "[LLM] 未找到 JSON 对象";
            return qMakePair(result,currentReply);
        }
    }

    if (!doc.isObject()) {
        qDebug() << "[LLM] JSON 根节点不是对象";
        return qMakePair(result,currentReply);
    }

    QJsonObject rootObj = doc.object();
    QJsonArray sentencesArray = rootObj.value("sentences").toArray();

    if (sentencesArray.isEmpty()) {
        qDebug() << "[LLM] sentences 数组为空";
        return qMakePair(result,currentReply);
    }

    qDebug() << "[LLM] JSON 解析成功，共" << sentencesArray.size() << "个句子";

    // SentenceText prevSentence;

    for (int i = 0; i < sentencesArray.size(); ++i) {
        QJsonObject sentenceObj = sentencesArray[i].toObject();

        SentenceText sentence;
        sentence.zhText = sentenceObj.value("zh_text").toString().trimmed();
        sentence.jaText = sentenceObj.value("ja_text").toString().trimmed();
        // sentence.originalJSON = QJsonDocument(sentenceObj).toJson(QJsonDocument::Compact);

        // 解析 tags
        QJsonObject tagsObj = sentenceObj.value("tags").toObject();
        QString prevEmotion = (i > 0) ? sentencesArray[i-1].toObject()["tags"].toObject()["emotion"].toString() : "";
        QString prevBlush = (i > 0) ? sentencesArray[i-1].toObject()["tags"].toObject()["blush"].toString() : "";
        QString prevDistance = (i > 0) ? sentencesArray[i-1].toObject()["tags"].toObject()["distance"].toString() : "";
        QString prevClothing = (i > 0) ? sentencesArray[i-1].toObject()["tags"].toObject()["clothing"].toString() : "";

        tagsObj["emotion"] = TagValidator::validateTag(
            "emotion", tagsObj["emotion"].toString(), TagValidator::VALID_EMOTIONS,
            sentence.zhText, prevEmotion
            );
        tagsObj["blush"] = TagValidator::validateTag(
            "blush", tagsObj["blush"].toString(), TagValidator::VALID_BLUSH,
            "", prevBlush
            );
        tagsObj["distance"] = TagValidator::validateTag(
            "distance", tagsObj["distance"].toString(), TagValidator::VALID_DISTANCE,
            "", prevDistance
            );
        tagsObj["clothing"] = TagValidator::validateTag(
            "clothing", tagsObj["clothing"].toString(), TagValidator::VALID_CLOTHING,
            "", prevClothing
            );
        // 更新修正后的 tags
        sentenceObj["tags"] = tagsObj;
        sentencesArray[i] = sentenceObj;

        // 构建 SentenceText
        for (auto it = tagsObj.constBegin(); it != tagsObj.constEnd(); ++it) {
            sentence.rawTags.insert(it.key(), it.value().toString());
        }
        sentence.isValidated = true;
        result.append(sentence);
    }

    // 更新根对象（修正后的 JSON）
    rootObj["sentences"] = sentencesArray;

    // 保存修正后的 JSON（用于存储和历史记忆）
    currentReply = QJsonDocument(rootObj).toJson(QJsonDocument::Compact);
    qDebug() << "[LLM] 修正后回复:" << currentReply;

    return qMakePair(result,currentReply);
}
//Tags校验
int TagValidator::levenshteinDistance(const QString &s1, const QString &s2)
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

QString TagValidator::validateTag(
    const QString &tagName,
    const QString &rawValue,
    const QStringList &validValues,
    const QString &contextText,
    const QString &prevValue)
{
    QString value=rawValue.trimmed();
    //对
    if(validValues.contains(value)){
        qDebug()<<"[LLMTVD]标签无异常:"<<tagName<<":"<<rawValue;
        return value;

    }
    //大小写
    QString valueLower=rawValue.toLower();
    for(const QString &v:validValues){
        if(v.toLower()==valueLower){
            qDebug()<<"[LLMTVD]大小写纠正:"<<tagName<<":"<<value<<"->"<<v;
            return v;
        }
    }
    //编辑距离检测
    int threshold=getEditDistanceThreshold(value);
    QString bestMatch;
    int mindistance=threshold+1;
    for(const QString &v: validValues){
        int distance=levenshteinDistance(value,v);
        if(distance<=threshold&&distance<mindistance){
            mindistance=distance;
            bestMatch=v;
        }
    }
    if(!bestMatch.isEmpty()){
        qDebug()<<"[LLMTVD]编辑距离修正:"<<tagName<<":"<<value<<"->"<<bestMatch;
        return bestMatch;
    }
    //继承
    if(!prevValue.isEmpty()&&validValues.contains(prevValue)){
        qDebug()<<"[LLMTVD]匹配失败，继承上一状态:"<<tagName<<":"<<value<<"->"<<prevValue;
        return prevValue;
    }
    //默认值
    QString defaultValue;
    if (tagName == "emotion") defaultValue = "happyIdle";
    else if (tagName == "blush") defaultValue = "unblushing";
    else if (tagName == "distance") defaultValue = "far";
    else if (tagName == "clothing") defaultValue = "schoolUniform";
    qDebug()<<"[LLMTVD]匹配+校验失败，使用默认值:"<<tagName<<":"<<value<<"->"<<defaultValue;
    return defaultValue;
}



int TagValidator::getEditDistanceThreshold(const QString &value)
{
    int len = value.length();
    if (len <= 12) return 3;
    return 4;
}

const QStringList TagValidator::VALID_CLOTHING{
    "pajama", "schoolUniform", "schoolUniformWithoutCap", "schoolUniformWithoutCoat"
};

const QStringList TagValidator::VALID_DISTANCE{
    "far", "closer"
};

const QStringList TagValidator::VALID_BLUSH{
    "unblushing", "blushing"
};

const QStringList TagValidator::VALID_EMOTIONS{
    "happyIdle", "happyMore", "amazing", "loving", "caring", "sad", "conscientious"
};
