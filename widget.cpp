#include "widget.h"

#include <QDebug>
#include <QVBoxLayout>
#include <QPixmap>
#include <QCoreApplication>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    //without side  always on top
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    //without background
    setAttribute(Qt::WA_TranslucentBackground);

    imageLabel = new QLabel(this);

    //test photo path
    // QString imagePath_test = ":/image/test1.jpg";
    QPixmap pixmap(":/image/test1.jpg");

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


}

Widget::~Widget() = default;

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
