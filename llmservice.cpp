#include "llmservice.h"
// 引入配置中心获取 Key
#include "configmanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

LLMService::LLMService(QObject *parent):QObject{parent}
{
    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager,&QNetworkAccessManager::finished,this,&LLMService::onReplyFinished);
}

void LLMService::askDeepSeek(const QString &userInput)
{
    QUrl url("https://api.deepseek.com/chat/completions");
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    //define YOUR_API_KEY into AI api key
    QString aotuHeader="Bearer "+ConfigManager::instance().getApiKey();
    request.setRawHeader("Authorization",aotuHeader.toUtf8());


    // 构建 JSON 请求体
    QJsonObject systemMessage;
    systemMessage["role"] = "system";
    systemMessage["content"] = "你叫千岛茉子，简称茉子，今年7岁,请暂时称呼我为欧尼酱";

    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = userInput;

    QJsonArray messagesArray;
    messagesArray.append(systemMessage);
    messagesArray.append(userMessage);

    QJsonObject rootObj;
    rootObj["model"] = "deepseek-chat";
    rootObj["messages"] = messagesArray;
    rootObj["temperature"] = 0.7;

    // 将 JSON 对象转为字节数据并发送 POST 请求
    QByteArray postData = QJsonDocument(rootObj).toJson();
    m_networkManager->post(request, postData);

    qDebug() << "[LLMService]茉子正在思考......";
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

            qDebug()<<"[茉子回复]:"<<replyText;

            //正则表达式抓取表情包
            QRegularExpression regex("\\[(.*?)\\]");
            QRegularExpressionMatch match=regex.match(replyText);

            QString emotion = "idle";
            QString cleanText=replyText;

            //凤梨表情和文本
            if(match.hasMatch()){
                emotion=match.captured(1);
                cleanText.remove(match.captured(0));
            }
            qDebug() << "[提取的表情]:" << emotion;
            qDebug() << "[茉子想说的话]:" << cleanText.trimmed();

            // //切换立绘  没有做好文件路径管理，暂时屏蔽
            // QString imagePath =QCoreApplication::applicationDirPath()+"/"+emotion+".png";
            // QPixmap pixmap(imagePath);
            // if(!imagePath.isNull()){
            //     imageLabel->setPixmap(pixmap);
            // }
            // else{
            //     imageLabel->setPixmap(QPixmap(QCoreApplication::applicationDirPath() + "/idle.png"));
            // }

            // 显示speakButton
            // if(speakBubble){
            //     QPoint bubblePos = this->pos()+m_visibleRect.topRight() + QPoint(10, 20);
            //     speakBubble->move(bubblePos);
            //     //.trimmed() 是 QString 的成员函数，用于移除字符串首尾的空白字符。
            //     speakBubble->showMessage(cleanText.trimmed());
            //     qDebug()<<"气泡出现";
            // }

            replyReady(cleanText.trimmed(),emotion);
        }
    }
    else {
        qDebug()<<"[网络错误]:"<<reply->errorString();
    }
    reply->deleteLater();
}
