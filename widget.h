#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QMouseEvent>
#include <QLabel>

#include <QNetworkAccessManager>
#include <QNetworkReply>

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget() override;

    //AI transmission
    void askDeepSeek(const QString& userInput);


protected:
    //mouseevent Rewrite
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);

private slots:
    //finish AI 回复
    void onReplyFinished(QNetworkReply* reply);
private:
    QPoint dragPosition; //鼠标点击时相对坐标
    QLabel *imageLabel;  // 立绘图片的标签
    QNetworkAccessManager* networkManager;
};
#endif // WIDGET_H
