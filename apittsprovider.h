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
    //流式，分段监测与处理
    bool isSegmentedResponse(const QByteArray &rawData,QNetworkReply *reply);
    QByteArray extractAndConcatPcm(const QByteArray &rawData);
    void writePcmToWavFile(const QString &filePath,const QByteArray &pcmData,int sampleRate);
    int findPCMStart(const QByteArray &chunk);
    QByteArray extractPcmFromWavChunk(const QByteArray &wavChunk);

    QNetworkAccessManager *m_networkManager;
};
struct SynthesisContext {
    QNetworkReply *reply;
    SentenceText sentence;
    QByteArray buffer;   // 未解析的累积数据
};

#endif // APITTSPROVIDER_H
