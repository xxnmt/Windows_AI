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
        qDebug()<<"[ConfigManger]:api配置缺失，使用默认值";
        m_apiKey = "sk-placeholder-key";
        m_ttsUrl = "http://127.0.0.1:9880";
    }

    if (rootObj.contains("memory") && rootObj["memory"].isObject()) {
        QJsonObject memoryObj = rootObj["memory"].toObject();
        m_shortMemoryLength = memoryObj.value("short_term_length").toInt(15);
    }
    else
    {
        qDebug()<<"[ConfigManger]:memory配置缺失，使用默认值";
        m_shortMemoryLength = 15;
    }
    //tts
    if (rootObj.contains("tts") && rootObj["tts"].isObject()) {
        QJsonObject ttsObj = rootObj["tts"].toObject();
        m_ttsMode = ttsObj.value("mode").toString("api");
        m_ttsRef.audioFilePath = ttsObj.value("ref_audio_path").toString("");
        m_ttsRef.promptText = ttsObj.value("prompt_text").toString("");
        m_ttsRef.promptLanguage = ttsObj.value("prompt_lang").toString("ja");
        m_gptWeightsPath = ttsObj.value("gpt_weights_path").toString("");
        m_sovitsWeightsPath = ttsObj.value("sovits_weights_path").toString("");
        m_gptSovitsRootPath = ttsObj.value("gpt_sovits_root_path").toString("");
        m_gptModelDir = ttsObj.value("gpt_model_dir").toString("");
        m_sovitsModelDir = ttsObj.value("sovits_model_dir").toString("");
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

    QJsonObject ttsObj;
    ttsObj.insert("mode", m_ttsMode);
    ttsObj.insert("ref_audio_path", m_ttsRef.audioFilePath);
    ttsObj.insert("prompt_text", m_ttsRef.promptText);
    ttsObj.insert("prompt_lang", m_ttsRef.promptLanguage);
    ttsObj.insert("gpt_weights_path", m_gptWeightsPath);
    ttsObj.insert("sovits_weights_path", m_sovitsWeightsPath);
    ttsObj.insert("gpt_sovits_root_path", m_gptSovitsRootPath);
    ttsObj.insert("gpt_model_dir", m_gptModelDir);
    ttsObj.insert("sovits_model_dir", m_sovitsModelDir);
    rootObj.insert("tts", ttsObj);

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

QString ConfigManager::getGPTModelDir() const
{
    return m_gptModelDir;
}

void ConfigManager::setGPTModelDir(const QString &path)
{
    m_gptModelDir=path;
}

QString ConfigManager::getSoVITSModelDir() const
{
    return m_sovitsModelDir;
}

void ConfigManager::setSoVITSModelDir(const QString &path)
{
    m_sovitsModelDir=path;
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

QString ConfigManager::getTTSMode() const
{
    return m_ttsMode;
}

void ConfigManager::setTTSMode(const QString &mode)
{
    m_ttsMode=mode;
}

TTSReference ConfigManager::getTTSReference() const
{
    return m_ttsRef;
}

void ConfigManager::setTTSReference(const TTSReference &ref)
{
    m_ttsRef=ref;
}

QString ConfigManager::getGPTWeightsPath() const
{
    return m_gptWeightsPath;
}

void ConfigManager::setGPTWeightsPath(const QString &path)
{
    m_gptWeightsPath=path;
}

QString ConfigManager::getSoVITSWeightsPath() const
{
    return m_sovitsWeightsPath;
}

void ConfigManager::setSoVITSWeightsPath(const QString &path)
{
    m_sovitsWeightsPath=path;
}

QString ConfigManager::getGPTSovitsRootPath() const
{
    return m_gptSovitsRootPath;
}

void ConfigManager::setGPTSovitsRootPath(const QString &path)
{
    m_gptSovitsRootPath=path;
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
