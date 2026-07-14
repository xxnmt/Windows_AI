#include "configmanager.h"

#include <QCoreApplication>
QString ConfigManager::getApiKey() const
{
    return m_apiKey;
}

void ConfigManager::setApiKey(const QString &apiKey)
{
    m_apiKey=apiKey;
}

ConfigManager::ConfigManager() {

    m_apiKey="sk-8150d4f2c95644b39d35c9fae00baa81";
}
