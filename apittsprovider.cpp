#include "apittsprovider.h"

#include "apittsprovider.h"
#include "configmanager.h"
#include "ttsreferencemanager.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QFile>
#include <QUuid>
#include <QDebug>
#include <QUrlQuery>
ApiTTSProvider::ApiTTSProvider(QObject *parent)
    : ITTSProvider{parent}
{
    m_networkManager=new QNetworkAccessManager(this);
}

void ApiTTSProvider::synthesize(const SentenceText &sentence)
{
    QString baseUrl=ConfigManager::instance().getTTSUrl();
    if(baseUrl.endsWith("/")){
        baseUrl.chop(1);
    }

    QUrl requestUrl(baseUrl+"/tts");
    qDebug()<<"[ApiTTS]RequestUrl:"<<requestUrl;

    TTSReference ref=TTSReferenceManager::instance().getReferenceForEmotion(sentence.rawTags.value("emotion"));

    QJsonObject json;
    json["text"] = sentence.jaText;
    json["text_lang"] = "ja"; // 基于句子数据的日文文本合成
    json["ref_audio_path"] = ref.audioFilePath;
    json["prompt_text"] = ref.promptText;
    json["prompt_lang"] = ref.promptLanguage;
    json["text_split_method"] = "cut0";
    json["media_type"] = "wav";
    json["streaming_mode"] = false;

    QNetworkRequest request(requestUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = m_networkManager->post(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply, sentence]() {
        onNetworkReplyFinished(reply, sentence);
        qDebug()<<"[ApiTTS]语音合成完成:"<<sentence.jaText<<sentence.zhText;
    });
    qDebug() << "[ApiTTS] 请求体:" << QJsonDocument(json).toJson(QJsonDocument::Compact);
}

void ApiTTSProvider::switchModel(const QString &gptPath, const QString &sovitsPath)
{
    QString baseUrl = ConfigManager::instance().getTTSUrl();
    if (baseUrl.endsWith("/"))
        baseUrl.chop(1);

    auto sendRequest = [this, baseUrl](const QString &endpoint, const QString &path) {
        QUrl url(baseUrl + endpoint);
        QUrlQuery query;
        query.addQueryItem("weights_path", path);
        url.setQuery(query);

        QNetworkRequest request(url);
        QNetworkReply *reply = m_networkManager->get(request);
        connect(reply, &QNetworkReply::finished, this, [reply, endpoint]() {
            if (reply->error() == QNetworkReply::NoError) {
                qDebug() << "[ApiTTS] 模型切换成功:" << endpoint;
            } else {
                qDebug() << "[ApiTTS] 模型切换失败:" << endpoint << reply->errorString();
            }
            reply->deleteLater();
        });
    };

    if (!gptPath.isEmpty())
        sendRequest("/set_gpt_weights", gptPath);
    if (!sovitsPath.isEmpty())
        sendRequest("/set_sovits_weights", sovitsPath);
}

void ApiTTSProvider::warmUp()
{
    //语音合成预热
    SentenceText dummySentence;
    dummySentence.jaText = ".";
    dummySentence.zhText = "";
    dummySentence.rawTags["emotion"] = "happyIdle";


    QString baseUrl = ConfigManager::instance().getTTSUrl();
    if (baseUrl.endsWith("/"))
        baseUrl.chop(1);
    QUrl requestUrl(baseUrl + "/tts");

    TTSReference ref = TTSReferenceManager::instance().getReferenceForEmotion("happyIdle");

    QJsonObject json;
    json["text"] = dummySentence.jaText;
    json["text_lang"] = "ja";
    json["ref_audio_path"] = ref.audioFilePath;
    json["prompt_text"] = ref.promptText;
    json["prompt_lang"] = ref.promptLanguage;
    json["text_split_method"] = "cut0";
    json["media_type"] = "wav";
    json["streaming_mode"] = false;
    json["top_k"] = 5;
    json["top_p"] = 1.0;
    json["temperature"] = 1.0;
    json["batch_size"] = 1;

    QNetworkRequest request(requestUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = m_networkManager->post(request, QJsonDocument(json).toJson());

    //预热完成后直接释放
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    qDebug()<<"[ApiTTS]预热请求已发送";
}

void ApiTTSProvider::onNetworkReplyFinished(QNetworkReply *reply, SentenceText sentence)
{
    reply->deleteLater();
    if(reply->error()){
        qDebug()<<"[ApiTTS]:网络请求失败"<<reply->errorString();
        emit synthesisFailed(reply->errorString(),sentence);
        return;
    }
    QString tempDirPath = ConfigManager::instance().getAppDataPath() + "/temp_audio";
    QDir().mkpath(tempDirPath);
    QString tempFilePath = tempDirPath + "/" + QUuid::createUuid()
                                                   .toString(QUuid::WithoutBraces) + ".wav";
    QFile file(tempFilePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "[ApiTTS] 无法写入临时音频文件:" << tempFilePath;
        emit synthesisFailed("无法保存音频文件", sentence);
        return;
    }

    file.write(reply->readAll());
    file.close();

    emit synthesisFinished(tempFilePath, sentence);
}
