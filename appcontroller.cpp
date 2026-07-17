#include "appcontroller.h"
#include "characterwidget.h"
#include "bubblewidget.h"
#include "llmservice.h"
#include "appearancemanager.h"
#include "ttsservice.h"
#include "chatwidget.h"
#include "anchormanager.h"
#include "configmanager.h"
#include <QDebug>

AppController::AppController(QObject *parent)
    : QObject{parent}
{
    m_character = new CharacterWidget;
    m_bubble = new BubbleWidget;
    m_llmService = new LLMService(ConfigManager::instance().getApiKey());
    m_appearance = new AppearanceManager;
    m_ttsService = new TTSService;
    m_chatWidget = new ChatWidget;

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
    if (m_character) delete m_character;
    if (m_bubble) delete m_bubble;
}

void AppController::startApp()
{
    m_character->show();

    qDebug()<<"[user]:你好呀、茉子，今天开心吗？";

    emit m_character->userChat("你好呀、茉子，今天开心吗？");

}
void AppController::handleMakoReply(const QList<SentenceText> &sentences)
{
    m_ttsService->enqueueSentences(sentences);
}

void AppController::handleSystemError(const QString &errorMsg)
{
    // 1. 在控制台打印真实的错误信息，方便开发者调试
    qDebug() << "[AppController 拦截到系统错误]：" << errorMsg;

    // 2. 以角色口吻在气泡中向用户反馈错误
    m_bubble->showMessage("啊哦，欧尼酱，网络好像断开了呢，茉子联系不到服务器啦...");

    // 3. (可选) 让茉子切换到一个委屈或惊讶的立绘状态
    QMap<QString, QString> errorTags;
    errorTags.insert("emotion", "sad"); // 假设你有 sad 这个表情的素材
    errorTags.insert("blush", "unblushing");
    m_appearance->applyTags(errorTags); // 触发换图
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
    connect(m_character, &CharacterWidget::userChat, m_llmService, &LLMService::askDeepSeek);
    connect(m_llmService, &LLMService::sentenceReady, this, &AppController::handleMakoReply);
    connect(m_ttsService, &TTSService::playAudioAction, this, &AppController::onPlayAudioAction);
    connect(m_appearance, &AppearanceManager::characterPathChanged, m_character, &CharacterWidget::updatePath);
    connect(m_llmService, &LLMService::internetErrorSignal, this, &AppController::handleSystemError);

    connect(m_character, &CharacterWidget::chatRequested, m_chatWidget,&ChatWidget::popup);
    connect(m_chatWidget, &ChatWidget::textSubmitted, m_llmService, &LLMService::askDeepSeek);
    connect(m_appearance, &AppearanceManager::characterPathChanged,m_anchorManager, &AnchorManager::updateAllAnchors);
    connect(m_bubble, &BubbleWidget::bubbleShown,m_anchorManager, &AnchorManager::updateAllAnchors);
}
