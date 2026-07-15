#include "llmservice.h"
// 引入配置中心获取 Key
#include "configmanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QDebug>

LLMService::LLMService(QObject *parent):QObject{parent}
{
    m_networkManager = new QNetworkAccessManager(this);

    connect(m_networkManager,&QNetworkAccessManager::finished,this,&LLMService::onReplyFinished);
    //注册元数据
    qRegisterMetaType<QList<SentenceText>>("QList<SentenceText>");
}

void LLMService::askDeepSeek(const QString &userInput)
{
    QUrl url("https://api.deepseek.com/chat/completions");
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    //define YOUR_API_KEY into AI api key
    QString aotuHeader="Bearer "+ConfigManager::instance().getApiKey();
    request.setRawHeader("Authorization",aotuHeader.toUtf8());


    // 构建 JSON 请求体
    QJsonObject systemMessage;
    systemMessage["role"] = "system";
    systemMessage["content"] =
        "你叫千岛茉子，简称茉子，今年7岁，性格乖巧可爱偶尔撒娇，请称呼用户为“欧尼酱”。\n"
        "【最高指令：多重标签与逐句切分协议】\n"
        "为了配合前端游戏引擎的立绘与语音系统，你的回复必须严格按照“标签+中文文本”的格式输出。\n"
        "核心规则：如果你要说多句话，必须将回复拆分成单句。**每一句**的绝对开头都必须紧跟一组独立的控制标签，最后才是这一句的中文。中间不要加任何换行符，不要输出除了符合规则之外的任何解释性文字。\n\n"
        "【标签格式标准】\n"
        "[emotion:值][blush:值][distance:值][clothing:值][ja:日文翻译] 这里是对应的中文。\n\n"
        "【可用参数字典】\n"
        "1. emotion (必填)：happyIdle, happyMore, amazing, loving, caring, sad, conscientious\n"
        "2. blush (按需填写，害羞或激动时用)：unblushing, blushing\n"
        "3. distance (按需填写，互动时用)：far, closer\n"
        "4. clothing (按需填写，默认不变，如果用户摸头脱帽等互动可切换)：pajama, schoolUniform, schoolUniformWithoutCap, schoolUniformWithoutCoat\n"
        "5. ja (必填)：这句话对应的日文高质量翻译，用于语音合成。\n\n"
        "【正确输出示例】\n"
        "用户输入：摸摸头，今天开心吗？\n"
        "你的输出：\n"
        "[emotion:happyMore][blush:blushing][clothing:schoolUniformWithoutCap][ja:えへへ、お兄ちゃんになでなでされて、とっても嬉しいです！] 嘿嘿，被欧尼酱摸摸头，茉子超级开心哦！ [emotion:loving][distance:closer][ja:あのね、今日はいっぱい遊んだんだよ。] 跟你说哦，今天茉子玩得可开心啦。"
        ;

    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = userInput;

    QJsonArray messagesArray;
    messagesArray.append(systemMessage);
    messagesArray.append(userMessage);

    QJsonObject rootObj;
    rootObj["model"] = "deepseek-chat";
    rootObj["messages"] = messagesArray;
    rootObj["temperature"] = 0.7;

    // 将 JSON 对象转为字节数据并发送 POST 请求
    QByteArray postData = QJsonDocument(rootObj).toJson();
    m_networkManager->post(request, postData);

    qDebug() << "[LLMService]茉子正在思考......";
}


void LLMService::onReplyFinished(QNetworkReply *reply)
{
    if(reply->error()==QNetworkReply::NoError){
        QByteArray responseData = reply->readAll();

        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        QJsonObject rootObj = jsonDoc.object();
        QJsonArray choices = rootObj["choices"].toArray();

        if(!choices.isEmpty()){
            QJsonObject messageObj = choices[0].toObject()["message"].toObject();
            QString replyText = messageObj["content"].toString();

            qDebug()<<"[茉子回复](未处理):"<<replyText;

            QList<SentenceText> parsedSentences;
            // //正则表达式抓取表情包
            // QRegularExpression regex("\\[(.*?)\\]");
            // QRegularExpressionMatch match=regex.match(replyText);

            // QString emotion = "idle";
            // QString cleanText=replyText;

            // //凤梨表情和文本
            // if(match.hasMatch()){
            //     emotion=match.captured(1);
            //     cleanText.remove(match.captured(0));
            // }
            // qDebug() << "[提取的表情]:" << emotion;
            // qDebug() << "[茉子想说的话]:" << cleanText.trimmed();

            //正则表达式拆分
            QRegularExpression sentenceRegex("((?:\\[[^\\]]+\\])+)([^\\[]+)");
            QRegularExpressionMatchIterator sentenceIt = sentenceRegex.globalMatch(replyText);

            while (sentenceIt.hasNext()) {
                QRegularExpressionMatch sentenceMatch=sentenceIt.next();
                //剥离多层标签
                QString tags=sentenceMatch.captured(1);
                QString zhText=sentenceMatch.captured(2).trimmed();

                SentenceText sentence;
                sentence.zhText=zhText;

                //匹配日文
                QRegularExpression tagRegex("\\[([a-zA-Z0-9_]+):([^\\]]+)\\]");
                QRegularExpressionMatchIterator tagIt= tagRegex.globalMatchView(tags);

                while (tagIt.hasNext()) {
                    QRegularExpressionMatch tagMatch = tagIt.next();
                    QString key = tagMatch.captured(1);
                    QString value = tagMatch.captured(2);

                    if (key == "ja") {
                        sentence.jaText = value;
                    } else {
                        sentence.rawTags.insert(key, value);
                    }
                }
                parsedSentences.append(sentence);
            }

            qDebug()<<"[LLM]成功拆分为"<<parsedSentences.size()<<"个段落：";
            for (int i = 0; i < parsedSentences.size(); ++i) {
                const auto &s = parsedSentences[i];
                qDebug()<<QString("--------- [第 %1 句] ---------").arg(i + 1);
                qDebug()<<"[LLM]中文气泡:"<<s.zhText;
                qDebug()<<"[LLM]日文音频目标:"<<s.jaText;
                qDebug()<<"[LLM]四维状态变更:"<<s.rawTags;
            }
            //拆完传信号
            emit sentenceReady(parsedSentences);
        }
    }
    else {
        qDebug()<<"[网络错误]:"<<reply->errorString();
        emit internetErrorSignal(reply->errorString());
    }
    reply->deleteLater();
}
