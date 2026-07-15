#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QQueue>
#include <QMap>
#include "sentencedata.h"

class CharacterWidget;
class BubbleWidget;
class LLMService;
class AppearanceManager;
class TTSService;

class AppController : public QObject
{
    Q_OBJECT
public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController();

    void startApp();

private slots:
    void handleMakoReply(const QList<SentenceText> &sentences);
    void handleSystemError(const QString &errorMsg);
    void onPlayAudioAction(const QString &zhText, const QMap<QString, QString> &tags);

signals:

private:
    CharacterWidget *m_character;
    BubbleWidget *m_bubble;
    LLMService *m_llmService;
    AppearanceManager *m_appearance;
    TTSService *m_ttsService;
};

#endif // APPCONTROLLER_H
