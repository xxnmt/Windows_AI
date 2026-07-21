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
        // m_apiKey="sk-placeholder-key";
        // m_ttsUrl="http://127.0.0.1:9880";
        return saveSetting();
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug()<<"[ConfigManger]配置文件损坏，已初始化";
        return saveSetting();
    }

    QByteArray fileData=file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(fileData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "[ConfigManager]配置文件损坏，已初始化";
        return saveSetting();
    }

    QJsonObject rootObj = doc.object();

    // 读取 api 模块下的参数
    if (rootObj.contains("api") && rootObj["api"].isObject()) {
        QJsonObject apiObj = rootObj["api"].toObject();
        m_apiKey = apiObj.value("deepseek_api_key").toString("sk-placeholder-key");
        m_ttsUrl = apiObj.value("gpt_sovits_url").toString("http://127.0.0.1:9880");
    }
    else
    {
        qDebug()<<"[ConfigManger]:配置文件损坏";
        return saveSetting();
    }

    if (rootObj.contains("memory") && rootObj["memory"].isObject()) {
        QJsonObject memoryObj = rootObj["memory"].toObject();
        m_shortMemoryLength = memoryObj.value("short_term_length").toInt(15);
    }
    else
    {
        qDebug()<<"[ConfigManger]:配置文件损坏";
        return saveSetting();
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

    QJsonObject memoryObj;
    memoryObj.insert("short_term_length", m_shortMemoryLength);

    QJsonObject rootObj;
    rootObj.insert("api", apiObj);
    rootObj.insert("memory", memoryObj);

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

int ConfigManager::getShortMemoryLength()
{
    return m_shortMemoryLength;
}

void ConfigManager::setShortMemoryLength(const int length)
{
    m_shortMemoryLength=length;
}

QString ConfigManager::getConfigDirPath() const
{
    return m_configDirPath;
}

QString ConfigManager::getConfigFilePath() const
{
    return m_configFilePath;
}

QString ConfigManager::getAppDataPath() const
{
    QDir appPath(m_configDirPath);
    appPath.cdUp();
    return appPath.absolutePath();
}

QString ConfigManager::getMemoryPath() const
{
    return getAppDataPath()+"/memory/QianDaoMoZi_memory.db";
}

ConfigManager::ConfigManager() {
    initAppDataPath();
    loadSetting();
}
