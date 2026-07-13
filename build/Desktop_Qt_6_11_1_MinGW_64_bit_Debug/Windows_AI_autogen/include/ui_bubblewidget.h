/********************************************************************************
** Form generated from reading UI file 'bubblewidget.ui'
**
** Created by: Qt User Interface Compiler version 6.11.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_BUBBLEWIDGET_H
#define UI_BUBBLEWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_BubbleWidget
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *label_text;

    void setupUi(QWidget *BubbleWidget)
    {
        if (BubbleWidget->objectName().isEmpty())
            BubbleWidget->setObjectName("BubbleWidget");
        BubbleWidget->resize(400, 300);
        verticalLayout = new QVBoxLayout(BubbleWidget);
        verticalLayout->setObjectName("verticalLayout");
        label_text = new QLabel(BubbleWidget);
        label_text->setObjectName("label_text");
        label_text->setStyleSheet(QString::fromUtf8("color: white; font-family: 'Microsoft YaHei'; font-size: 14px; font-weight: bold;"));
        label_text->setWordWrap(true);

        verticalLayout->addWidget(label_text);


        retranslateUi(BubbleWidget);

        QMetaObject::connectSlotsByName(BubbleWidget);
    } // setupUi

    void retranslateUi(QWidget *BubbleWidget)
    {
        BubbleWidget->setWindowTitle(QCoreApplication::translate("BubbleWidget", "Form", nullptr));
        label_text->setText(QCoreApplication::translate("BubbleWidget", "TextLabel", nullptr));
    } // retranslateUi

};

namespace Ui {
    class BubbleWidget: public Ui_BubbleWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_BUBBLEWIDGET_H
