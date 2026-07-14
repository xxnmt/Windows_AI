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

    // 删除拷贝构造和赋值操作，确保单例纯粹性
    ConfigManager(const ConfigManager&) = delete;
    void operator=(const ConfigManager&) = delete;

    QString getApiKey() const;
    void setApiKey(const QString &apiKey);

private:
    ConfigManager();
    QString m_apiKey;


};

#endif // CONFIGMANAGER_H
