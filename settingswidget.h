#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include <QWidget>

namespace Ui {
class SettingsWidget;
}

class SettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget *parent = nullptr);
    ~SettingsWidget();
protected:
    void showEvent(QShowEvent *event)override;
signals:
    void settingsSaved();


private slots:
    void on_btn_configSaveAll_clicked();

    void on_btn_configQuit_clicked();


    void on_btn_saveApiKey_clicked();

private:
    Ui::SettingsWidget *ui;

    void loadSettings();
};

#endif // SETTINGSWIDGET_H
