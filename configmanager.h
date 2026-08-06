#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QString>
#include <QSettings>
#include "ttsreference.h"

class ConfigManager
{
public:
    static ConfigManager& instance(){
        static ConfigManager instance;
        return instance;
    }

    bool loadSetting();
    bool saveSetting();


    // 删除拷贝构造和赋值操作，确保单例纯粹性
    ConfigManager(const ConfigManager&) = delete;
    void operator=(const ConfigManager&) = delete;

    QString getApiKey() const;
    void setApiKey(const QString &apiKey);

    QString getTTSUrl() const;
    void setTTSUrl(const QString &ttsUrl);
    QString getTTSMode() const;
    void setTTSMode(const QString &mode);
    TTSReference getTTSReference() const;
    void setTTSReference(const TTSReference &ref);

    QString getGPTWeightsPath() const;
    void setGPTWeightsPath(const QString &path);
    QString getSoVITSWeightsPath() const;
    void setSoVITSWeightsPath(const QString &path);

    QString getGPTSovitsRootPath() const;
    QString getPythonPath() const;
    void setPythonPath(const QString &path);
    void setGPTSovitsRootPath(const QString &path);
    QString getGPTModelDir() const;
    void setGPTModelDir(const QString &path);
    QString getSoVITSModelDir() const;
    void setSoVITSModelDir(const QString &path);

    int getShortMemoryLength();
    void setShortMemoryLength(const int length);

    QString getConfigDirPath()const;
    QString getConfigFilePath()const;
    QString getAppDataPath()const;
    QString getMemoryPath()const;

private:
    ConfigManager();
    ~ConfigManager() = default;

    void initAppDataPath();

    QString m_apiKey;

    QString m_ttsUrl;
    QString m_ttsMode = "api";
    TTSReference m_ttsRef;
    QString m_gptWeightsPath;
    QString m_sovitsWeightsPath;
    QString m_gptSovitsRootPath;
    QString m_pythonPath;

    int m_shortMemoryLength=15;

    QString m_configDirPath;
    QString m_configFilePath;
    QString m_gptModelDir;
    QString m_sovitsModelDir;


};

#endif // CONFIGMANAGER_H
