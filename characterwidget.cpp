#include "widget.h"

#include <QDebug>
#include <QVBoxLayout>
#include <QPixmap>
#include <QCoreApplication>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    //without side  always on top
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    //without background
    setAttribute(Qt::WA_TranslucentBackground);

    imageLabel = new QLabel(this);

    //test photo path
    QString imagePath = ":/image/test1.jpg";
    QPixmap pixmap(imagePath);

    if(!pixmap.isNull()) {
        imageLabel->setPixmap(pixmap);
        // 根据图片实际分辨率调整窗口大小
        resize(pixmap.width(), pixmap.height());
    } else {
        imageLabel->setText("未能加载 test1.png，请检查路径！");
        qDebug()<<"未能加载 test1.png，请检查路径！";
        imageLabel->setStyleSheet("color: red; background: white;");
    }

    //设置布局，让图片填满整个窗口
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0); // 取消边距
    layout->addWidget(imageLabel);

    networkManager = new QNetworkAccessManager(this);
    connect(networkManager,&QNetworkAccessManager::finished,this,&Widget::onReplyFinished);
    askDeepSeek("(打招呼)茉子好呀，今天开心吗(请你简单回复10个字以内)");


}

Widget::~Widget() = default;

void Widget::askDeepSeek(const QString &userInput)
{
    QUrl url("https://api.deepseek.com/chat/completions");
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    //define YOUR_API_KEY into AI api key
    request.setRawHeader("Authorization","Bearer sk-8150d4f2c95644b39d35c9fae00baa81");


    // 构建 JSON 请求体
    QJsonObject systemMessage;
    systemMessage["role"] = "system";
    systemMessage["content"] = "你叫千岛茉子，简称茉子，今年7岁";

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
    networkManager->post(request, postData);

    qDebug() << "[系统] 茉子正在思考...";
}

void Widget::mousePressEvent(QMouseEvent *event)
{
    if(event->button()==Qt::LeftButton){
        dragPosition=event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void Widget::mouseMoveEvent(QMouseEvent *event)
{
    if(event->buttons() & Qt::LeftButton){
        move(event->globalPosition().toPoint() - dragPosition);
        event->accept();
    }
}

void Widget::onReplyFinished(QNetworkReply *reply)
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

        }

        else {
            qDebug()<<"[网络错误]:"<<reply->errorString();
        }
    }
    reply->deleteLater();
}
