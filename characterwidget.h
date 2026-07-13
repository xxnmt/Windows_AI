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
    QLabel *imageLabel;  //立绘图片的标签
    QNetworkAccessManager* networkManager;
    //
    BubbleWidget *speakBubble;
    //存储立绘中真正有像素的有效区域
    QRect m_visibleRect;
    //扫描像素的辅助函数
    QRect calculateVisibleRect(const QPixmap &pixmap);
};
#endif // CHARACTERWIDGET_H
