#include "ttsprocessmanager.h"
#include <QDebug>
#include <QTcpSocket>



TTSProcessManager::TTSProcessManager(const QString &apiPythonPath, const QString &scriptDir,
                                     const QStringList &args,int port, QObject *parent):
    QObject(parent),
    m_apiPythonPath(apiPythonPath),
    m_scriptDir(scriptDir),
    m_args(args),
    m_port(port)
{
    qDebug()<<"[TTSPM]:以构建,apiPath:"<<m_apiPythonPath;
}

TTSProcessManager::~TTSProcessManager()
{
    apiStop();
    qDebug()<<"[TTSPM]:api已关闭";
}

bool TTSProcessManager::isApiReady()
{
    return m_isApiReady;
}

void TTSProcessManager::apiStart()
{
    if(m_process){
        if(m_process->state()==QProcess::Running){
            qDebug()<<"[TTSPM]:服务已启动，无需重复启动";
            return;
        }
        delete m_process;
        m_process =nullptr;
    }
    m_process=new QProcess(this);
    m_process->setWorkingDirectory(m_scriptDir);
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_process,&QProcess::readyReadStandardOutput,this,&TTSProcessManager::onReadyReadStandOutput);
    connect(m_process,&QProcess::stateChanged,this,&TTSProcessManager::onProcessStateChanged);

    QStringList fullArgs=m_args;
    fullArgs.prepend(m_scriptDir+"/api_v2.py");
    m_process->start(m_apiPythonPath,fullArgs);
    if(!m_timer){
        m_timer=new QTimer(this);
        m_timer->setInterval(1000);
        connect(m_timer,&QTimer::timeout,this,&TTSProcessManager::onHealthCheck);
    }
    m_timer->start();

    QTimer::singleShot(60000,this,[this](){
        if(!m_isApiReady){
            if(m_timer) m_timer->stop();
            qDebug()<<"[TTSPM]:启动超时（60秒）";
            emit apiFailed("启动超时（60秒）");
        }
    });
    qDebug()<<"[TTSPM]:api启动中，请稍后";
}

void TTSProcessManager::apiStop()
{
    if(m_timer){
        m_timer->stop();
        delete m_timer;
        m_timer=nullptr;
    }
    if(m_process){
        m_process->terminate();
        if(!m_process->waitForFinished(3000)){
            m_process->kill();
        }
        delete m_process;
        m_process=nullptr;
    }
    m_isApiReady=false;
}

void TTSProcessManager::onReadyReadStandOutput()
{
    QString data=QString::fromUtf8(m_process->readAllStandardOutput());
    emit statusMessage(data);
    if(!m_isApiReady&&data.contains(READY_MARKER)){
        qDebug()<<"[TTSPM](stdOut):api启动成功";
        m_isApiReady=true;
        if(m_timer) m_timer->stop();

        emit apiReady();
    }
}

void TTSProcessManager::onProcessStateChanged(QProcess::ProcessState state)
{
    if(state==QProcess::NotRunning&&!m_isApiReady){
        if(m_timer) m_timer->stop();
        qDebug()<<"[TTSPM]:api进程异常终止";
        emit apiFailed("api进程异常终止");
    }
}

void TTSProcessManager::onHealthCheck()
{
    if(m_isApiReady) {
        m_timer->stop();
        return;
    }
    QTcpSocket socket;
    socket.connectToHost("127.0.0.1",m_port);
    if(socket.waitForConnected(1000)){
        m_isApiReady=true;
        socket.disconnectFromHost();
        qDebug()<<"[TTSPM]（tcp）:api启动成功";
        if(m_timer) m_timer->stop();
        emit apiReady();
    }
}
