#include "characterwidget.h"

#include <QDebug>
#include <QVBoxLayout>
#include <QPixmap>
#include <QCoreApplication>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

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
    // speakBubble = new BubbleWidget();


    //test photo path
    // QString imagePath = ":/image/far/schoolUniform/unblushing/happyIdle.png";
    // QPixmap pixmap(imagePath);

    // if(!pixmap.isNull()) {
    //     imageLabel->setPixmap(pixmap);
    //     // 根据图片实际分辨率调整窗口大小
    //     resize(pixmap.width(), pixmap.height());

    //     //人物真实位置（排除空白背景
    //     m_visibleRect= calculateVisibleRect(pixmap);
    //     qDebug()<<"[功能调试]人物边界为:"<<m_visibleRect;
    // } else {
    //     imageLabel->setText("未能加载 test1.png，请检查路径！");
    //     qDebug()<<"未能加载 test1.png，请检查路径！";
    //     imageLabel->setStyleSheet("color: red; background: white;");
    // }

    //设置布局，让图片填满整个窗口
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0); // 取消边距
    layout->addWidget(imageLabel);

    // m_llmService = new LLMService(this);
    // connect(m_llmService, &LLMService::replyReady, this, &CharacterWidget::onMakoReplyReady);
    // connect(m_llmService, &LLMService::internetErrorSignal, this, &CharacterWidget::onMakoError);

    // m_llmService->askDeepSeek("(打招呼)茉子好呀，今天开心吗(请你简单回复10个字以内)");
    // qDebug()<<"[CharacterWidget]茉子思考中......";


}

CharacterWidget::~CharacterWidget() {
    // if (speakBubble) {
    //     delete speakBubble;
    // }
}

void CharacterWidget::updatePath(const QString &imagePath)
{
    QPixmap pixmap(imagePath);
    if(!pixmap.isNull()){
        imageLabel->setPixmap(pixmap);
        resize(pixmap.size());

        m_visibleRect = calculateVisibleRect(pixmap);

    }
}

QPoint CharacterWidget::getBubbleAnchorPos() const
{
    return this->pos() + m_visibleRect.topRight() + QPoint(5, 10);
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
        // if (speakBubble && speakBubble->isVisible()) {
        //     QPoint bubblePos = this->pos()+m_visibleRect.topRight() + QPoint(10, 20);
        //     speakBubble->move(bubblePos);
        // }
        emit characterMoved();

        event->accept();
    }
}

// void CharacterWidget::onMakoReplyReady(const QString &cleanText, const QString &emotion)
// {
//     qDebug()<<"[CharacterWidget]收到表情:"<<emotion;
//     qDebug()<<"[CharacterWidget]收到文本:"<<cleanText;

//     if(speakBubble){
//         QPoint bubblePos = this->pos() + m_visibleRect.topRight() + QPoint(5, 10);
//         speakBubble->move(bubblePos);
//         speakBubble->showMessage(cleanText);
//     }
// }

// void CharacterWidget::onMakoError(const QString &errorMsg)
// {
//     qDebug() << "[CharacterWidget] Internet Error:" << errorMsg;
// }

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
