#include "settingswidget.h"
#include "ui_settingswidget.h"
#include "configmanager.h"
#include "memorymanager.h"
#include "historyturn.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QHeaderView>

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget(parent),
    ui(new Ui::SettingsWidget)
{

    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);
    setWindowTitle("茉子 - 设置中心");
    // resize(400, 300);

    ui->setupUi(this);
    setupHistoryTable();

    ui->btn_previousMemoryPage->setEnabled(false);
    ui->btn_nextMemoryPage->setEnabled(false);

}

SettingsWidget::~SettingsWidget()
{
    delete ui;
}

void SettingsWidget::setMemoryManager(MemoryManager *manager)
{
    m_memoryManager = manager;
    if (m_memoryManager) {
        refreshHistoryTurnList();
    }
}

void SettingsWidget::setMemoryLength(int length)
{
    ui->spinBox_memoryLength->setValue(length);
}

void SettingsWidget::refreshHistoryTurnList()
{
    if(!m_memoryManager){
        qDebug()<<"[SettingsWidget]:数据库获取失败";
        return;
    }
    m_totalRecords=m_memoryManager->getTotalHistoryCount();
    m_totalPages=(m_totalRecords+m_pageSize-1)/m_pageSize;

    if (m_totalPages < 1){
        m_totalPages = 1;
    }
    if(m_currentPage<m_totalPages){
        m_currentPage=m_totalPages-1;
    }
    if(m_currentPage<0){
        m_currentPage=0;
    }
    updatePageInfo();
    loadHistoryPage(m_currentPage);
}

void SettingsWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    loadSettings();
    refreshHistoryTurnList();

}

void SettingsWidget::on_btn_configSaveAll_clicked()
{
    on_btn_saveApiKey_clicked();
    // hide();

}

void SettingsWidget::on_btn_configQuit_clicked()
{
    loadSettings();
    hide();
}

void SettingsWidget::on_btn_saveApiKey_clicked()
{
    ConfigManager &configSettings=ConfigManager::instance();
    configSettings.setApiKey(ui->lineEdit_apiKey->text());
    if(configSettings.saveSetting()){
        emit settingsSaved();
        qDebug()<<"[SettingWidget]:apiKey保存成功";
    }
    else{
        qDebug()<<"[SettingsWidget]:配置文件保存失败";
    }
}


void SettingsWidget::on_btn_saveMemoryLength_clicked()
{
    ConfigManager &configSettings=ConfigManager::instance();
    configSettings.setShortMemoryLength(ui->spinBox_memoryLength->value());
    if(configSettings.saveSetting()){
        emit settingsSaved();
        qDebug()<<"[SettingWidget]:茉子短期记忆轮数成功设置为"<<ui->spinBox_memoryLength->value()<<"轮";
    }
    else{
        qDebug()<<"[SettingsWidget]:茉子短期记忆轮数设置失败";
    }

}


void SettingsWidget::on_btn_claenTempMemory_clicked()
{
    refreshHistoryTurnList();
    qDebug()<<"[SettingsWidget]:短期记忆实时读取数据库，暂时无法清空";
}


void SettingsWidget::on_btn_deleteSelectedMemory_clicked()
{
    if(!m_memoryManager){
        qDebug()<<"[SettingsWidget]:数据库获取失败";
        return;
    }
    QList<int> ids=getSelectedMemoryIDs();
    if(ids.isEmpty()){
        qDebug()<<"[SettingsWidget]:你他妈故意找茬是不是？我一个做删除的，能删除你的空对象？";
        return;
    }
    QMessageBox::StandardButton confirm = QMessageBox::question(
        this,
        "确认删除",
        QString("确定要删除选中的 %1 条记录吗？此操作不可撤销！").arg(ids.size()),
        QMessageBox::Yes | QMessageBox::No
        );
    if (confirm != QMessageBox::Yes) return;

    // API Key 验证
    bool ok;
    QString inputKey = QInputDialog::getText(
        this,
        "身份验证",
        "请输入您的 API Key 以确认删除操作：",
        QLineEdit::Password,
        "",
        &ok
        );
    if (!ok || inputKey.isEmpty()) {
        QMessageBox::warning(this, "验证取消", "已取消删除操作");
        qDebug()<<"[SettingsWidget]:取消删除操作";
        return;
    }

    if (!verifyApiKey(inputKey)) {
        QMessageBox::warning(this, "验证失败", "API Key 不正确，拒绝删除！");
        qDebug()<<"[SettingsWidget]:删除验证失败";
        return;
    }
    int successCount = 0;
    for (int id : ids) {
        if (m_memoryManager->deleteTurnByID(id)) {
            successCount++;
        }
    }
    QMessageBox::information(
        this,
        "删除完成",
        QString("成功删除 %1 条记录").arg(successCount)
        );
    qDebug()<<"[SettingsWidget]:共删除"<<successCount<<"条记录";
    refreshHistoryTurnList();

}


void SettingsWidget::on_btn_claenAllMemory_clicked()
{
    if(!m_memoryManager){
        qDebug()<<"[SettingsWidget]:数据库获取失败";
        return;
    }
    QMessageBox::StandardButton confirm = QMessageBox::question(
        this,
        "⚠️ 危险操作",
        "确定要清空所有历史记录吗？此操作不可撤销！",
        QMessageBox::Yes | QMessageBox::No
        );
    if (confirm != QMessageBox::Yes) return;

    bool ok;
    QString inputKey = QInputDialog::getText(
        this,
        "身份验证",
        "请输入您的 API Key 以确认删除操作：",
        QLineEdit::Password,
        "",
        &ok
        );
    if (!ok || inputKey.isEmpty()) {
        QMessageBox::warning(this, "验证取消", "已取消删除操作");
        qDebug()<<"[SettingsWidget]:取消删除操作";
        return;
    }

    if (!verifyApiKey(inputKey)) {
        QMessageBox::warning(this, "验证失败", "API Key 不正确，拒绝删除！");
        qDebug()<<"[SettingsWidget]:删除验证失败";
        return;
    }
    if (m_memoryManager->clearAllHistory()) {
        QMessageBox::information(this, "清空完成", "所有历史记录已清空");
        qDebug()<<"[SettingsWidget]:所有历史记录已清空";
        refreshHistoryTurnList();
    }
    else {
        QMessageBox::critical(this, "清空失败", "清空历史记录时发生错误");
        qDebug()<<"[SettingsWidget]:历史记录清空失败";
    }
}


void SettingsWidget::on_btn_previousMemoryPage_clicked()
{
    if(m_currentPage>0){
        m_currentPage--;
        loadHistoryPage(m_currentPage);
        updatePageInfo();
        qDebug()<<"[SettingsManager]切换上一页:"<<m_currentPage;
    }
}


void SettingsWidget::on_btn_nextMemoryPage_clicked()
{
    if(m_currentPage<m_totalPages-1){
        m_currentPage++;
        loadHistoryPage(m_currentPage);
        updatePageInfo();
        qDebug()<<"[SettingsManager]切换下一页:"<<m_currentPage;
    }
}


void SettingsWidget::on_btn_momorySaveAll_clicked()
{
    on_btn_saveMemoryLength_clicked();
    // hide();
}


void SettingsWidget::on_btn_memoryQuit_clicked()
{
    loadSettings();
    hide();
}

void SettingsWidget::loadSettings()
{
    ConfigManager &configSettings= ConfigManager::instance();
    ui->lineEdit_apiKey->setText(configSettings.getApiKey());
    ui->spinBox_memoryLength->setValue(configSettings.getShortMemoryLength());


}

void SettingsWidget::setupHistoryTable()
{
    m_historyModel = new QStandardItemModel(this);
    m_historyModel->setColumnCount(4);

    QStringList headers;
    headers << "ID" << "时间" << "用户输入" << "AI回复";
    m_historyModel->setHorizontalHeaderLabels(headers);

    ui->tableView->setModel(m_historyModel);

    //列宽设置
    ui->tableView->setColumnWidth(0, 50);   // ID
    ui->tableView->setColumnWidth(1, 150);  // 时间
    ui->tableView->setColumnWidth(2, 200);  // 用户输入


    //整行选择，可多选
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    ui->tableView->horizontalHeader()->setStretchLastSection(true);
    ui->tableView->setAlternatingRowColors(true);
}

void SettingsWidget::loadHistoryPage(int page)
{
    if(!m_memoryManager){
        qDebug()<<"[SettingsWidget]:数据库获取失败";
        return;
    }
    int offset=page*m_pageSize;
    QList<HistoryTurn> turns =m_memoryManager->getHistoryTurn(offset,m_pageSize);
    m_historyModel->removeRows(0,m_historyModel->rowCount());

    for(const HistoryTurn &turn :turns){
        QList<QStandardItem*>row;
        //ID
        QStandardItem *idItem = new QStandardItem(QString::number(turn.id));
        idItem->setData(turn.id, Qt::UserRole);  // 存储 ID
        idItem->setTextAlignment(Qt::AlignCenter);
        //Time
        QStandardItem *timeItem = new QStandardItem(
        turn.timestamp.toString("yyyy-MM-dd hh:mm:ss")
        );
        timeItem->setTextAlignment(Qt::AlignCenter);
        //user
        QString userText = turn.userInput;
        if (userText.length() > 40) {
            userText = userText.left(40) + "...";
        }
        QStandardItem *userItem = new QStandardItem(userText);
        userItem->setToolTip(turn.userInput);  // 悬停显示完整内容
        //茉子
        QString replyText = turn.rawReply;
        if (replyText.length() > 600) {
            replyText = replyText.left(600) + "...";
        }
        QStandardItem *replyItem = new QStandardItem(replyText);
        replyItem->setToolTip(turn.rawReply);

        row << idItem << timeItem << userItem << replyItem;
        m_historyModel->appendRow(row);
    }

    ui->btn_previousMemoryPage->setEnabled(m_currentPage > 0);
    ui->btn_nextMemoryPage->setEnabled(m_currentPage < m_totalPages - 1);
}

void SettingsWidget::updatePageInfo()
{
    ui->label_MemoryPage->setText(
        QString("%1 / %2 页").arg(m_currentPage + 1).arg(m_totalPages)
        );
}


bool SettingsWidget::verifyApiKey(const QString &inputKey)
{
    QString currentKey=ConfigManager::instance().getApiKey();
    qDebug()<<"api"<<ConfigManager::instance().getApiKey();
    qDebug()<<"userinput"<<inputKey;
    if(currentKey==inputKey){
        qDebug()<<"[SettingsWidget]:api删除验证成功";
        return true;
    }
    else{
        qDebug()<<"[SettingsWidget]:api删除验证失败";
        return false;
    }
}

QList<int> SettingsWidget::getSelectedMemoryIDs()
{
    QList<int>ids;
    QModelIndexList selectedRows =ui->tableView->selectionModel()->selectedRows();
    for(const QModelIndex &index:selectedRows){
        int row =index.row();
        QModelIndex idIndex=m_historyModel->index(row,0);
        int id=m_historyModel->data(idIndex,Qt::UserRole).toInt();
        if(id>0){
            ids.append(id);
        }
    }
    qDebug()<<"[SettingsWidget]已选择将要删除的记录id:"<<ids;
    return ids;
}

