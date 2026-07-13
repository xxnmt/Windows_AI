#include "characterwidget.h"

#include <QDebug>
#include <QVBoxLayout>
#include <QPixmap>
#include <QCoreApplication>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>

#include <QRegularExpression>
#include <QRegularExpressionMatch>

CharacterWidget::CharacterWidget(QWidget *parent)
    : QWidget(parent)
{
    //without side  always on top
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    //without background
    setAttribute(Qt::WA_TranslucentBackground);

    imageLabel = new QLabel(this);
    speakBubble = new BubbleWidget();


    //test photo path
    QString imagePath = ":/image/far/schoolUniform/1.png";
    QPixmap pixmap(imagePath);

    if(!pixmap.isNull()) {
        imageLabel->setPixmap(pixmap);
        // 根据图片实际分辨率调整窗口大小
        resize(pixmap.width(), pixmap.height());

        //人物真实位置（排除空白背景
        m_visibleRect= calculateVisibleRect(pixmap);
        qDebug()<<"[功能调试]人物边界为:"<<m_visibleRect;
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
    connect(networkManager,&QNetworkAccessManager::finished,this,&CharacterWidget::onReplyFinished);
    askDeepSeek("(打招呼)茉子好呀，今天开心吗(请你简单回复10个字以内)");


}

CharacterWidget::~CharacterWidget() {
    if (speakBubble) {
        delete speakBubble;
    }
}

void CharacterWidget::askDeepSeek(const QString &userInput)
{
    QUrl url("https://api.deepseek.com/chat/completions");
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    //define YOUR_API_KEY into AI api key
    request.setRawHeader("Authorization","Bearer sk-8150d4f2c95644b39d35c9fae00baa81");


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
    networkManager->post(request, postData);

    qDebug() << "[系统] 茉子正在思考...";
}

void CharacterWidget::mousePressEvent(QMouseEvent *event)
{
    if(event->button()==Qt::LeftButton){
        dragPosition=event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void CharacterWidget::mouseMoveEvent(QMouseEvent *event)
{
    if(event->buttons() & Qt::LeftButton){
        move(event->globalPosition().toPoint() - dragPosition);


        //speakBubble moveEvent
        //this->geometry() 返回当前窗口（widget）的几何位置和大小
        if (speakBubble && speakBubble->isVisible()) {
            QPoint bubblePos = this->pos()+m_visibleRect.topRight() + QPoint(10, 20);
            speakBubble->move(bubblePos);
        }

        event->accept();
    }
}

void CharacterWidget::onReplyFinished(QNetworkReply *reply)
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
            if(speakBubble){
                QPoint bubblePos = this->pos()+m_visibleRect.topRight() + QPoint(10, 20);
                speakBubble->move(bubblePos);
                //.trimmed() 是 QString 的成员函数，用于移除字符串首尾的空白字符。
                speakBubble->showMessage(cleanText.trimmed());
                qDebug()<<"气泡出现";
            }
        }    
    }
    else {
        qDebug()<<"[网络错误]:"<<reply->errorString();
    }
    reply->deleteLater();
}

QRect CharacterWidget::calculateVisibleRect(const QPixmap &pixmap)
{
    QImage image = pixmap.toImage();
    int width = image.width();
    int height = image.height();
    //扫描像素点的透明度（Alpha 通道）
    int left = width, right = 0, top = height, bottom = 0;
    bool hasAlpha = false;
    for(int y=0;y<height;y++){
        for(int x=0;x<width;x++){
            if (qAlpha(image.pixel(x, y)) > 10) {
                if (x < left)   left = x;
                if (x > right)  right = x;
                if (y < top)    top = y;
                if (y > bottom) bottom = y;
                hasAlpha = true;
            }
        }
    }
    return QRect(left, top, right - left + 1, bottom - top + 1);
}
