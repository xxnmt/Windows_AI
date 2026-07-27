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
    
    // ---------- 必须参数 ----------
    json["text"] = sentence.jaText;              // 【必须】要合成的文本内容
    json["text_lang"] = "ja";                    // 【必须】文本语言：zh(中文)、en(英文)、ja(日文)、auto(自动检测)
    json["ref_audio_path"] = ref.audioFilePath;  // 【必须】参考音频文件的绝对路径，用于提取音色
    json["prompt_text"] = ref.promptText;        // 【v3必须/其他可选】参考音频对应的文本内容
    json["prompt_lang"] = ref.promptLanguage;    // 【必须】参考音频文本的语言
    
    // ---------- 文本切分参数 ----------
    json["text_split_method"] = "cut2";          // 文本切分方法：
                                                 //   cut0: 中文标点切分（！？。，；：…等）
                                                 //   cut1: 英文标点切分（!?. , ; : …等）
                                                 //   cut2: 日文标点切分（!?. , ; : …。、等）
                                                 //   cut3: 中英混合智能切分
                                                 //   cut4: 中英日混合智能切分
                                                 //   cut5: 不切分（整段合成，默认）
    
    // ---------- 采样/生成参数 ----------
    json["top_k"] = 5;                           // Top-K采样：限制每个位置可能的token数量，越小越确定
    json["top_p"] = 1.0;                         // Top-P采样：按累积概率筛选token，1.0表示不限制
    json["temperature"] = 1.0;                   // 温度参数：控制生成随机性，越高越多样化，越低越确定
    json["repetition_penalty"] = 1.35;           // 重复惩罚：防止生成重复内容，1.0无惩罚，>1.0惩罚重复
    
    // ---------- 语速/速度参数 ----------
    json["speed_factor"] = 1.0;                  // 语速控制：1.0正常，<1.0变慢，>1.0变快
    
    // ---------- 批量推理参数 ----------
    json["batch_size"] = 1;                      // 批量大小：同时处理的句子数量，v3模型固定为1
    json["batch_threshold"] = 0.75;              // 批量切分阈值：当句子长度差小于此值时可合批
    json["split_bucket"] = false;                // 分桶处理：v3模型自动关闭，无需修改
    
    // ---------- 音频输出参数 ----------
    json["media_type"] = "wav";                  // 输出格式：wav(无损)、raw(裸PCM)、ogg(有损)、aac(有损)
    json["fragment_interval"] = 0.3;             // 分段间隔(秒)：流式模式下每段音频的间隔时长
    
    // ---------- 随机种子参数 ----------
    json["seed"] = -1;                           // 随机种子：-1完全随机，固定值可复现结果
    
    // ---------- v3模型专用参数 ----------
    json["sample_steps"] = 32;                   // 【v3专用】CFM采样步数：
                                                 //   16: 最快，质量略降（适合实时对话）
                                                 //   32: 默认，平衡质量与速度（日常使用推荐）
                                                 //   64: 最慢，质量最佳（适合成品生成）
    json["super_sampling"] = false;              // 【v3专用】超分辨率：
                                                 //   true: 音质更好但速度显著变慢(+40%~80%)
                                                 //   false: 默认，正常音质
    
    // ---------- 流式返回参数 ----------
    json["streaming_mode"] = true;              // 流式模式：
                                                 //   false: 关闭（返回完整音频文件，推荐）
                                                 //   true/1: 开启（v3自动回退为分段返回）
                                                 //   2: 中等质量流式
                                                 //   3: 低质量快速流式
    json["return_fragment"] = false;             // 分段返回：通常由streaming_mode自动控制，无需手动设置
    json["overlap_length"] = 2;                  // 流式模式语义token重叠长度，通常保持默认
    json["min_chunk_length"] = 16;               // 流式模式最小chunk长度，通常保持默认
    
    // ---------- 并行推理参数 ----------
    json["parallel_infer"] = false;              // 并行推理：v3模型自动关闭，无需修改
    
    // ---------- 辅助参考音频参数（可选） ----------
    // json["aux_ref_audio_paths"] = QJsonArray(); // 辅助参考音频列表，用于多说话人音色融合

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
    // //语音合成预热
    // SentenceText dummySentence;
    // dummySentence.jaText = ".";
    // dummySentence.zhText = "";
    // dummySentence.rawTags["emotion"] = "happyIdle";


    // QString baseUrl = ConfigManager::instance().getTTSUrl();
    // if (baseUrl.endsWith("/"))
    //     baseUrl.chop(1);
    // QUrl requestUrl(baseUrl + "/tts");

    // TTSReference ref = TTSReferenceManager::instance().getReferenceForEmotion("happyIdle");

    // QJsonObject json;
    // json["text"] = dummySentence.jaText;
    // json["text_lang"] = "ja";
    // json["ref_audio_path"] = ref.audioFilePath;
    // json["prompt_text"] = ref.promptText;
    // json["prompt_lang"] = ref.promptLanguage;
    // json["text_split_method"] = "cut0";
    // json["media_type"] = "wav";
    // json["streaming_mode"] = false;
    // json["top_k"] = 5;
    // json["top_p"] = 1.0;
    // json["temperature"] = 1.0;
    // json["batch_size"] = 1;

    // QNetworkRequest request(requestUrl);
    // request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    // QNetworkReply *reply = m_networkManager->post(request, QJsonDocument(json).toJson());

    // //预热完成后直接释放
    // connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    // qDebug()<<"[ApiTTS]预热请求已发送";
    qDebug()<<"[apitts]:预热功能暂时取消";
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

    QByteArray rawData=reply->readAll();
    if(isSegmentedResponse(rawData,reply)){
        qDebug()<<"[ApiTTS]:检测到分段/流式返回，解析中";
        QByteArray pcmData=extractAndConcatPcm(rawData);
        writePcmToWavFile(tempFilePath,pcmData,24000);
    }
    else{
        QFile file(tempFilePath);
        if (!file.open(QIODevice::WriteOnly)) {
            qDebug()<<"[ApiTTS]无法写入临时音频文件:"<<tempFilePath;
            emit synthesisFailed("无法保存音频文件", sentence);
            return;
        }
        file.write(rawData);
        file.close();
    }
    emit synthesisFinished(tempFilePath, sentence);
}

bool ApiTTSProvider::isSegmentedResponse(const QByteArray &rawData,QNetworkReply *reply)
{
    QString contentType =reply->header(QNetworkRequest::ContentTypeHeader).toString();
    // QString transferEncoding=reply->header("Transfer-Encoding").toString();
    // QString transferEncoding = QString::fromUtf8(reply->rawHeader("Transfer-Encoding"));
    if(!contentType.contains("x-wav")){
        return false;
    }

    // QByteArray body =reply->readAll();
    int riffCount =rawData.count("RIFF");
    return riffCount>1;

}

QByteArray ApiTTSProvider::extractAndConcatPcm(const QByteArray &rawData)
{
    QByteArray resultPCM;
    int pos=0;
    int dataLength=rawData.size();
    while(pos<dataLength){
        int riffPos=rawData.indexOf("RIFF",pos);
        if(riffPos==-1)break;

        int pcmStart=findPCMStart(rawData.mid(riffPos));
        if(pcmStart==-1)break;

        int pcmDataEnd = rawData.size();
        int nextRiff = rawData.indexOf("RIFF", riffPos + 1);
        if (nextRiff != -1 && nextRiff > riffPos + pcmStart) {
            pcmDataEnd = nextRiff;
        }

        QByteArray chunkPcm = rawData.mid(riffPos + pcmStart,pcmDataEnd - (riffPos + pcmStart));
        resultPCM.append(chunkPcm);

        pos = riffPos + 1;
    }
    return resultPCM;
}

void ApiTTSProvider::writePcmToWavFile(const QString &filePath, const QByteArray &pcmData, int sampleRate)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "[ApiTTS] 无法写入音频文件:" << filePath;
        return;
    }

    int dataSize = pcmData.size();
    int fileSize = 44 + dataSize;  // WAV Header (44) + PCM 数据

    // 构建 WAV Header
    QByteArray header;
    header.append("RIFF");
    header.append(QByteArray::fromHex(
        QByteArray::number(fileSize - 8, 16).rightJustified(8, '0')));
    header.append("WAVE");
    header.append("fmt ");
    header.append(QByteArray::fromHex(
        QByteArray::number(16, 16).rightJustified(4, '0')));  // PCM chunk size
    header.append(QByteArray::fromHex("0100"));  // PCM format
    header.append(QByteArray::fromHex("0100"));  // Mono
    header.append(QByteArray::fromHex(
        QByteArray::number(sampleRate, 16).rightJustified(8, '0')));
    header.append(QByteArray::fromHex(
        QByteArray::number(sampleRate * 2, 16).rightJustified(8, '0')));
    header.append(QByteArray::fromHex("0200"));  // 16-bit
    header.append(QByteArray::fromHex("1000"));  // Block align
    header.append("data");
    header.append(QByteArray::fromHex(
        QByteArray::number(dataSize, 16).rightJustified(8, '0')));

    file.write(header);
    file.write(pcmData);
    file.close();

    qDebug() << "[ApiTTS] WAV文件写入完成:" << filePath
             << "大小:" << fileSize << "采样率:" << sampleRate;
}

int ApiTTSProvider::findPCMStart(const QByteArray &chunk)
{
    // 标准 WAV 结构：RIFF + 文件大小 + WAVE + fmt chunk + ... + data chunk
    // 简化处理：查找 "data" 标识
    int dataIdx = chunk.indexOf("data");
    if (dataIdx >= 0) {
        // "data" (4字节) + 数据大小 (4字节) = PCM 起始位置
        return dataIdx + 8;
    }
    return 44;  // fallback：标准 WAV Header 通常为 44 字节
}
