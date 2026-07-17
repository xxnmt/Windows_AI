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
    void setApiKey(const QString &apiKey);\

    QString getTTSUrl() const;
    void setTTSUrl(const QString &attsUrl);

    QString getConfigDirPath()const;
    QString getConfigFilePath()const;

private:
    ConfigManager();
    ~ConfigManager() = default;

    void initAppDataPath();

    QString m_apiKey;
    QString m_ttsUrl;

    QString m_configDirPath;
    QString m_configFilePath;


};

#endif // CONFIGMANAGER_H
