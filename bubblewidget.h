#ifndef BUBBLEWIDGET_H
#define BUBBLEWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QString>
#include <functional>

namespace Ui {
class BubbleWidget;
}

class BubbleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BubbleWidget(QWidget *parent = nullptr);
    ~BubbleWidget();

    void showMessage(const QString& text);
    //绑定追踪目标与位置计算回调
    void attachTo(QWidget *master,std::function<QPoint()> positionProvider);

protected:
    void paintEvent(QPaintEvent* event);

    bool eventFilter(QObject *watched,QEvent *event);

private slots:
    void typeWriteEffect();

private:
    Ui::BubbleWidget *ui;

    void updatePosition();


    QTimer* m_timer;
    QString m_text;
    int m_idex;
    //存储跟踪状态
    QWidget* m_master = nullptr;
    std::function<QPoint()> m_positionProvider;
};

#endif // BUBBLEWIDGET_H
