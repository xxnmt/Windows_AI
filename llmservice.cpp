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


LLMService::LLMService(const QString &apiKey,QObject *parent)
{
    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &LLMService::onReplyFinished);
    qRegisterMetaType<QList<SentenceText>>("QList<SentenceText>");
    m_apiKey=apiKey;

    QString configDirPath=ConfigManager::instance().getConfigDirPath();
    m_localPromptPath=QDir(configDirPath).filePath("prompt.txt");

    initializePromptFile();
    m_systemPromptCache=loadSystemPrompt();
}

void LLMService::askDeepSeek(const QString &userInput)
{
    QUrl url("https://api.deepseek.com/chat/completions");
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    //define YOUR_API_KEY into AI api key
    QString aotuHeader="Bearer "+m_apiKey;
    request.setRawHeader("Authorization",aotuHeader.toUtf8());

    QString finalSystemPrompt = m_systemPromptCache;
    if (m_stateProvider) {
        QString currentUiState = m_stateProvider();

        // 动态附加实时状态上下文
        finalSystemPrompt += QString(
                                 "\n\n"
                                 "【注意：千岛茉子当前的最新实时状态上下文】\n"
                                 "%1\n"
                                 "当前现实世界系统时间: %2\n"
                                 "请根据上述最新状态（如服装变化、害羞与否、现实时间段），在下面的对话中自然融入你的语气与内容中。"
                                 ).arg(currentUiState, QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    }
    else{
        finalSystemPrompt += QString(
                                 "\n\n"
                                 "【注意：千岛茉子当前的最新实时状态上下文】\n"
                                 "当前现实世界系统时间: %1\n"
                                 "请根据上述最新状态（如服装变化、害羞与否、现实时间段），在下面的对话中自然融入你的语气与内容中。"
                                 ).arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    }
    qDebug() << "[LLM] 注入大模型的Prompt:\n" << finalSystemPrompt;

    QJsonObject systemMessage;
    systemMessage["role"] = "system";
    systemMessage["content"] = finalSystemPrompt;

    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = userInput;

    QJsonArray messagesArray;
    messagesArray.append(systemMessage);
    messagesArray.append(userMessage);

    QJsonObject rootObj;
    rootObj["model"] = "deepseek-v4-flash";
    rootObj["messages"] = messagesArray;
    rootObj["temperature"] = 0.7;

    // 将 JSON 对象转为字节数据并发送 POST 请求
    QByteArray postData = QJsonDocument(rootObj).toJson();
    m_networkManager->post(request, postData);

    qDebug() << "[LLM]茉子正在思考......";
}

void LLMService::registerStateProvider(std::function<QString ()> provider)
{
    m_stateProvider=provider;
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

            QList<SentenceText> parsedSentences;

            //正则表达式拆分
            QRegularExpression sentenceRegex("((?:\\[[^\\]]+\\])+)([^\\[]+)");
            QRegularExpressionMatchIterator sentenceIt = sentenceRegex.globalMatch(replyText);

            while (sentenceIt.hasNext()) {
                QRegularExpressionMatch sentenceMatch=sentenceIt.next();
                //剥离多层标签
                QString tags=sentenceMatch.captured(1);
                QString zhText=sentenceMatch.captured(2).trimmed();

                SentenceText sentence;
                sentence.zhText=zhText;

                //匹配日文
                QRegularExpression tagRegex("\\[([a-zA-Z0-9_]+):([^\\]]+)\\]");
                QRegularExpressionMatchIterator tagIt= tagRegex.globalMatchView(tags);

                while (tagIt.hasNext()) {
                    QRegularExpressionMatch tagMatch = tagIt.next();
                    QString key = tagMatch.captured(1);
                    QString value = tagMatch.captured(2);

                    if (key == "ja") {
                        sentence.jaText = value;
                    } else {
                        sentence.rawTags.insert(key, value);
                    }
                }
                parsedSentences.append(sentence);
            }

            qDebug()<<"[LLM]成功拆分为"<<parsedSentences.size()<<"个段落：";
            for (int i = 0; i < parsedSentences.size(); ++i) {
                const auto &s = parsedSentences[i];
                qDebug()<<QString("--------- [第 %1 句] ---------").arg(i + 1);
                qDebug()<<"[LLM]中文气泡:"<<s.zhText;
                qDebug()<<"[LLM]日文音频目标:"<<s.jaText;
                qDebug()<<"[LLM]四维状态变更:"<<s.rawTags;
            }
            //拆完传信号
            emit sentenceReady(parsedSentences);
        }
    }
    else {
        qDebug()<<"[LLM]网络错误:"<<reply->errorString();
        emit internetErrorSignal(reply->errorString());
    }
    reply->deleteLater();
}

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
