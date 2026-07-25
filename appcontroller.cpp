#include "appcontroller.h"
#include "characterwidget.h"
#include "bubblewidget.h"
#include "llmservice.h"
#include "appearancemanager.h"
#include "ttsservice.h"
#include "chatwidget.h"
#include "anchormanager.h"
#include "configmanager.h"
#include "settingswidget.h"
#include "memorymanager.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonDocument>

AppController::AppController(QObject *parent)
    : QObject{parent}
{
    m_character = new CharacterWidget;
    m_bubble = new BubbleWidget;
    m_llmService = new LLMService(ConfigManager::instance().getApiKey());
    m_appearance = new AppearanceManager;
    m_ttsService = new TTSService;
    m_chatWidget = new ChatWidget;
    m_memoryManager = new MemoryManager(ConfigManager::instance().getMemoryPath());
    m_settingsWidget = new SettingsWidget;
    m_settingsWidget->setMemoryManager(m_memoryManager);
    m_settingsWidget->setMemoryLength(ConfigManager::instance().getShortMemoryLength());
    m_memoryManager->scanAndApplyProfileDecay();
    m_llmService->setMemoryManager(m_memoryManager);

    m_anchorManager = new AnchorManager(m_character, this);
    //ui绑定
    m_anchorManager->registerWidget(m_bubble, AnchorConfig{AnchorPosition::HeadRight});

    m_anchorManager->registerWidget(m_chatWidget, AnchorConfig{AnchorPosition::WaistCenter, QPoint(0, 20)});
    //业务绑定
    m_llmService->registerStateProvider([this](){return m_appearance->getCurrentStateDescription();});

    initConnections();

    m_character->updatePath(m_appearance->getPath());

}

AppController::~AppController()
{
    //1. 优先析构 AnchorManager（因为它持有了 m_character 及各个 UI 窗口的指针）
    if (m_anchorManager) {
        delete m_anchorManager;
        m_anchorManager = nullptr;
    }

    // 2. 析构独立 UI 窗口组件
    if (m_character) {
        delete m_character;
        m_character = nullptr;
    }
    if (m_bubble) {
        delete m_bubble;
        m_bubble = nullptr;
    }
    if (m_chatWidget) {
        delete m_chatWidget;
        m_chatWidget = nullptr;
    }
    if (m_settingsWidget) {
        delete m_settingsWidget;
        m_settingsWidget = nullptr;
    }

    // 3. 析构无 UI 的核心业务服务模块
    if (m_llmService) {
        delete m_llmService;
        m_llmService = nullptr;
    }
    if (m_appearance) {
        delete m_appearance;
        m_appearance = nullptr;
    }
    if (m_ttsService) {
        delete m_ttsService;
        m_ttsService = nullptr;
    }
    if (m_memoryManager) {
        delete m_memoryManager;
        m_memoryManager = nullptr;
    }

    qDebug()<<"[AppController]:所有中枢资源已安全清理完毕";
}

void AppController::startApp()
{
    m_character->show();

    qDebug()<<"[AppController]:我已启动！";

}

void AppController::handleMakoReply(const QList<SentenceText> &sentences, const QString &rawReply)
{
    m_memoryManager->saveQATurn(m_lastUserInput,rawReply,sentences);
    m_ttsService->enqueueSentences(sentences);

    qlonglong lastEndId = -1;
    QList<qlonglong> sourceIds;
    QList<HistoryTurn> unsummarizedTurns = m_memoryManager->getUnsummarizedTurns(lastEndId, sourceIds);


    const int SUMMARY_THRESHOLD = ConfigManager::instance().getShortMemoryLength();
    if (unsummarizedTurns.size() >= SUMMARY_THRESHOLD) {
        qDebug()<<"[AppController]未摘要对话达到阈值，开始触发后台记忆提取...";
        m_llmService->extractMemoryAsync(unsummarizedTurns, lastEndId, sourceIds);
    }
}


void AppController::handleSystemError(const QString &errorMsg)
{
    qDebug() << "[AppController 拦截到系统错误]：" << errorMsg;
    m_bubble->showMessage("啊哦，欧尼酱，网络好像断开了呢，茉子联系不到服务器啦...");
    QMap<QString, QString> errorTags;
    errorTags.insert("emotion", "sad");
    errorTags.insert("blush", "unblushing");
    m_appearance->applyTags(errorTags);
}

void AppController::onPlayAudioAction(const QString &zhText, const QMap<QString, QString> &tags)
{
    // 1. 你的 BubbleWidget 自己有 typeWriteEffect，直接调用即可
    m_bubble->showMessage(zhText);

    // 2. 更新多重状态标签
    m_appearance->applyTags(tags);


    qDebug()<<"[AppController] 当前播放文本:" << zhText;
    qDebug()<<"[AppController] 当前立绘路径:" << m_appearance->getPath();

}

void AppController::initConnections()
{
    connect(m_ttsService, &TTSService::playAudioAction, this, &AppController::onPlayAudioAction);
    connect(m_appearance, &AppearanceManager::characterPathChanged, m_character, &CharacterWidget::updatePath);
    connect(m_llmService, &LLMService::internetErrorSignal, this, &AppController::handleSystemError);

    connect(m_character, &CharacterWidget::chatRequested, m_chatWidget,&ChatWidget::popup);

    connect(m_appearance, &AppearanceManager::characterPathChanged,m_anchorManager, &AnchorManager::updateAllAnchors);
    connect(m_bubble, &BubbleWidget::bubbleShown,m_anchorManager, &AnchorManager::updateAllAnchors);

    connect(m_character, &CharacterWidget::settingsRequested, m_settingsWidget, &SettingsWidget::show);
    connect(m_settingsWidget, &SettingsWidget::settingsSaved, this, [this](){
        m_llmService->setApiKey(ConfigManager::instance().getApiKey());});

    connect(m_llmService,&LLMService::sentencesReady,this,&AppController::handleMakoReply);
    connect(m_chatWidget,&ChatWidget::textSubmitted,this,[this](const QString &text){
        m_lastUserInput=text;
        int memoryLength=ConfigManager::instance().getShortMemoryLength();;
        QList<HistoryTurn> shortTermMemory = m_memoryManager->getHistoryTurn(memoryLength);
        m_llmService->askDeepSeek(text,shortTermMemory);
    });
    connect(m_llmService, &LLMService::memoryExtractionReady, this, [this](const QJsonArray &profiles, const QString &summary, qlonglong lastEndId, const QString &sourceIdsJson) {

        //遍历写入用户画像
        for (const QJsonValue &val : profiles) {
            QJsonObject obj = val.toObject();
            QString key = obj["key"].toString();
            QString value = obj["value"].toString();
            int tier = obj["tier"].toInt();
            //每次提取到相关画像，置信度增加 10
            m_memoryManager->upsertUserProfile(key, value, tier, 10);
        }

        //写入长期记忆摘要
        if (!summary.isEmpty()) {
            m_memoryManager->addLongTermSummary(summary, lastEndId, sourceIdsJson);
        }

        qDebug()<<"[AppController]本次记忆提取流程已全部完成";
    });

    connect(m_settingsWidget, &SettingsWidget::ttsModelSwitchRequested,
            this, [this](const QString &gptPath, const QString &sovitsPath) {
                m_ttsService->switchModel(gptPath, sovitsPath);
            });
}