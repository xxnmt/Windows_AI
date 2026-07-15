#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>


class CharacterWidget;
class BubbleWidget;
class LLMService;
// class TTSService;

class AppController : public QObject
{
    Q_OBJECT
public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController();

    void startApp();

private slots:
    void handleMakoReply(const QString &cleanText, const QString &emotion);
    void handleSystemError(const QString &errorMsg);

signals:

private:
    CharacterWidget *m_character;
    BubbleWidget *m_bubble;
    LLMService *m_llmService;
    // TTSService      *m_ttsService;
};

#endif // APPCONTROLLER_H
