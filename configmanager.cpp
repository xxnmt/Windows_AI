#include "configmanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>

bool ConfigManager::loadSetting()
{
    QFile file(m_configFilePath);
    if(!file.exists()){
        qDebug()<<"[ConfigManger]配置文件不存在，已初始化";
        m_apiKey="sk-placeholder-key";
        m_ttsUrl="http://127.0.0.1:9880";
        return saveSetting();
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[ConfigManager] 无法读取配置文件:" << m_configFilePath;
        return false;
    }

    QByteArray fileData=file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(fileData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "[ConfigManager] JSON 解析失败:" << parseError.errorString();
        return false;
    }

    QJsonObject rootObj = doc.object();

    // 读取 api 模块下的参数
    if (rootObj.contains("api") && rootObj["api"].isObject()) {
        QJsonObject apiObj = rootObj["api"].toObject();
        m_apiKey = apiObj.value("deepseek_api_key").toString("sk-placeholder-key");
        m_ttsUrl = apiObj.value("gpt_sovits_url").toString("http://127.0.0.1:9880");
    } else {
        // 如果文件虽然存在但结构损坏，重建该结构
        m_apiKey = "sk-placeholder-key";
        m_ttsUrl = "http://127.0.0.1:9880";
        saveSetting();
    }

    qDebug() << "[ConfigManager] 配置成功自本地加载:" << m_configFilePath;
    return true;

}

bool ConfigManager::saveSetting()
{
    QFile file(m_configFilePath);
    if(!file.open(QIODevice::WriteOnly |QIODevice::Text)){
        qDebug()<<"[ConfigManager]无法写入配置文件:"<<m_configFilePath;
        return false;

    }

    QJsonObject apiObj;
    apiObj.insert("deepseek_api_key", m_apiKey);
    apiObj.insert("gpt_sovits_url", m_ttsUrl);

    QJsonObject rootObj;
    rootObj.insert("api", apiObj);

    QJsonDocument doc(rootObj);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    qDebug()<<"[ConfigManger]配置已保存:"<<m_configFilePath;
    return true;
}

void ConfigManager::initAppDataPath()
{
    QDir appDir(QCoreApplication::applicationDirPath());
    m_configDirPath=appDir.filePath("app_data/config");
    m_configFilePath=QDir(m_configDirPath).filePath("setting.json");

    if(!QDir().mkpath(m_configDirPath)){
        qDebug() << "[ConfigManager] 无法创建配置目录:" << m_configDirPath;
    }
    else{
        qDebug()<<"[ConfigManager]目录已存在，无需重复创建:"<<m_configDirPath;
    }



}

QString ConfigManager::getApiKey() const
{
    return m_apiKey;
}

void ConfigManager::setApiKey(const QString &apiKey)
{
    m_apiKey=apiKey;
}

QString ConfigManager::getTTSUrl() const
{
    return m_ttsUrl;
}

void ConfigManager::setTTSUrl(const QString &ttsUrl)
{
    m_ttsUrl=ttsUrl;
}

ConfigManager::ConfigManager() {
    initAppDataPath();
    loadSetting();
}
