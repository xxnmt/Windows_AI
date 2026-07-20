#include "settingswidget.h"
#include "ui_settingswidget.h"

#include "configmanager.h"

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SettingsWidget)
{

    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);
    setWindowTitle("茉子 - 设置中心");
    // resize(400, 300);

    ui->setupUi(this);
}

SettingsWidget::~SettingsWidget()
{
    delete ui;
}

void SettingsWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    loadSettings();
}

void SettingsWidget::on_btn_configSaveAll_clicked()
{
    on_btn_saveApiKey_clicked();
    hide();

}

void SettingsWidget::on_btn_configQuit_clicked()
{
    hide();
}


void SettingsWidget::loadSettings()
{
    ConfigManager &configSettings= ConfigManager::instance();
    ui->lineEdit_apiKey->setText(configSettings.getApiKey());

}


void SettingsWidget::on_btn_saveApiKey_clicked()
{
    ConfigManager &configSettings=ConfigManager::instance();
    configSettings.setApiKey(ui->lineEdit_apiKey->text());
    if(configSettings.saveSetting()){
        emit settingsSaved();
        qDebug()<<"[SettingWidget]:apiKey保存成功";
    }
    else{
        qDebug()<<"[SettingsWidget]:配置文件保存失败";
    }
}

