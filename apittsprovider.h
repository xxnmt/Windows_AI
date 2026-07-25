#ifndef APITTSPROVIDER_H
#define APITTSPROVIDER_H

#include "ittsprovider.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>

class ApiTTSProvider : public ITTSProvider
{
    Q_OBJECT
public:
    explicit ApiTTSProvider(QObject *parent = nullptr);
    void synthesize(const SentenceText &sentence)override;
    void switchModel(const QString &gptPath, const QString &sovitsPath);
    void warmUp()override;

private slots:
    void onNetworkReplyFinished(QNetworkReply *reply,SentenceText sentence);
private:
    QNetworkAccessManager *m_networkManager;
};

#endif // APITTSPROVIDER_H
