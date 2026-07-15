#include "appcontroller.h"

#include "appcontroller.h"
#include "characterwidget.h"
#include "bubblewidget.h"
#include "llmservice.h"

#include <QDebug>

AppController::AppController(QObject *parent)
    : QObject{parent}
{
    m_character = new CharacterWidget;
    m_bubble = new BubbleWidget;
    m_llmService = new LLMService;
    //bubble love character
    m_bubble->attachTo(m_character,[this](){ return m_character->getBubbleAnchorPos();});

    connect(m_character, &CharacterWidget::userChat, m_llmService, &LLMService::askDeepSeek);
    connect(m_llmService, &LLMService::replyReady, this, &AppController::handleMakoReply);

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

void AppController::handleMakoReply(const QString &cleanText, const QString &emotion)
{
    // m_ttsService->synthesizeSpeech(text, emotion);
    m_bubble->showMessage(cleanText);
    QString path = QString(":/image/far/schoolUniform/unblushing/%1.png").arg(emotion);
    m_character->updatePath(path);
    return;
}

void AppController::handleSystemError(const QString &errorMsg)
{
    return;
}
