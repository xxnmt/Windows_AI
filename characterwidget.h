#ifndef CHARACTERWIDGET_H
#define CHARACTERWIDGET_H

#include <QWidget>
#include <QMouseEvent>
#include <QLabel>

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "BubbleWidget.h"

class CharacterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CharacterWidget(QWidget *parent = nullptr);
    ~CharacterWidget() override;

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
    //
    BubbleWidget *speakBubble;

};
#endif // CHARACTERWIDGET_H
