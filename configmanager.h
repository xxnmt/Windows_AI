#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QString>
#include <QSettings>
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

    int m_shortMemoryLength=15;

    QString m_configDirPath;
    QString m_configFilePath;


};

#endif // CONFIGMANAGER_H
