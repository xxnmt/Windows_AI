#include "bubblewidget.h"
#include "ui_bubblewidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QEvent>
BubbleWidget::BubbleWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::BubbleWidget)
{
    ui->setupUi(this);

    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);

    hide();

    m_timer=new QTimer;
    connect(m_timer,&QTimer::timeout,this,&BubbleWidget::typeWriteEffect);

}

BubbleWidget::~BubbleWidget()
{
    delete ui;
}

void BubbleWidget::showMessage(const QString& text)
{
    m_text=text;
    m_idex=0;
    ui->label_text->clear();

    setFixedSize(250,120);
    updatePosition();
    this->show();
    m_timer->start(50);

}

void BubbleWidget::attachTo(QWidget *master, std::function<QPoint ()> positionProvider)
{
    if(m_master){
        m_master->removeEventFilter(this);
    }

    m_master = master;
    m_positionProvider = positionProvider;

    if(m_master){
        m_master->installEventFilter(this);
        updatePosition();
    }

}

void BubbleWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);//抗锯齿
    //paint the label_text
    QPainterPath path;
    path.addRoundedRect(rect(),15,15);
    painter.fillPath(path,QColor(0,0,0,180));
}

bool BubbleWidget::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == m_master &&(event->type()==QEvent::Resize || event->type()==QEvent::Move)){
        updatePosition();

    }
    return QWidget::eventFilter(watched,event);
}

void BubbleWidget::typeWriteEffect()
{
    if(m_idex<m_text.length()){
        m_idex++;
        ui->label_text->setText(m_text.left(m_idex));
    }
    else{
        m_timer->stop();
    }
}

void BubbleWidget::updatePosition()
{
    if (m_master && m_positionProvider) {
        //执行传入的 Lambda 表达式，瞬间拿到最新的目标挂载点坐标
        this->move(m_positionProvider());
    }
}
