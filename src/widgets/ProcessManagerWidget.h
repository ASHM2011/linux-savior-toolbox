#ifndef PROCESSMANAGERWIDGET_H
#define PROCESSMANAGERWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QProcess>
#include <QTimer>

struct ProcessInfo {
    int pid;
    QString user;
    double cpuPercent;
    double memPercent;
    QString command;
};

class ProcessManagerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProcessManagerWidget(QWidget* parent = nullptr);

signals:
    void commandTriggered(const QString& command, const QString& displayName, bool needsRoot);

private slots:
    void refreshProcesses();
    void onSearchTextChanged(const QString& text);
    void onProcessSelectionChanged();
    void terminateProcess();
    void killProcess();
    void sortByColumn(int column);

private:
    void setupUI();
    void setupToolbar();
    void setupTable();
    void loadProcesses();
    void filterProcesses(const QString& filter);
    void executeKillCommand(int pid, int signal);
    void updateButtonStates();

    QLineEdit* m_searchEdit;
    QPushButton* m_refreshBtn;
    QPushButton* m_terminateBtn;
    QPushButton* m_killBtn;
    QTableWidget* m_table;
    QLabel* m_statusLabel;
    QTimer* m_refreshTimer;

    QList<ProcessInfo> m_allProcesses;
    int m_sortColumn;
    Qt::SortOrder m_sortOrder;
};

#endif
