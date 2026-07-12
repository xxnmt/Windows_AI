#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QMouseEvent>
#include <QLabel>

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget() override;

protected:
    //mouseevent Rewrite
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);

private:
private:
    QPoint dragPosition; // 记录鼠标点击时的相对坐标
    QLabel *imageLabel;  // 用来承载立绘图片的标签
};
#endif // WIDGET_H
