#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include <QWidget>
#include <QStandardItemModel>

class MemoryManager;
namespace Ui {
class SettingsWidget;
}

class SettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget *parent = nullptr);
    ~SettingsWidget();

    void setMemoryManager(MemoryManager *manager);
    void setMemoryLength(int length);

    void refreshHistoryTurnList();

protected:
    void showEvent(QShowEvent *event)override;
signals:
    void settingsSaved();


private slots:
    //tab config
    void on_btn_configSaveAll_clicked();
    void on_btn_configQuit_clicked();
    void on_btn_saveApiKey_clicked();

    void on_btn_saveMemoryLength_clicked();
    void on_btn_claenTempMemory_clicked();

    void on_btn_deleteSelectedMemory_clicked();
    void on_btn_claenAllMemory_clicked();
    void on_btn_previousMemoryPage_clicked();
    void on_btn_nextMemoryPage_clicked();
    void on_btn_momorySaveAll_clicked();
    void on_btn_memoryQuit_clicked();

private:
    Ui::SettingsWidget *ui;



    MemoryManager *m_memoryManager=nullptr;
    QStandardItemModel *m_historyModel=nullptr;

    int m_currentPage = 0;
    const int m_pageSize = 50;
    int m_totalRecords = 0;
    int m_totalPages = 1;

    void loadSettings();
    void setupHistoryTable();
    void loadHistoryPage(int page);
    void updatePageInfo();
    bool verifyApiKey(const QString &inputKey);
    QList<int> getSelectedMemoryIDs();
};

#endif // SETTINGSWIDGET_H
