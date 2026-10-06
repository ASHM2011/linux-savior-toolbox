#ifndef SERVICEMANAGERWIDGET_H
#define SERVICEMANAGERWIDGET_H

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

struct ServiceInfo {
    QString name;
    QString loadState;
    QString activeState;
    QString subState;
    QString description;
};

class ServiceManagerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ServiceManagerWidget(QWidget* parent = nullptr);

signals:
    void commandTriggered(const QString& command, const QString& displayName, bool needsRoot);

private slots:
    void refreshServices();
    void onSearchTextChanged(const QString& text);
    void onServiceSelectionChanged();
    void startService();
    void stopService();
    void restartService();
    void enableService();
    void disableService();

private:
    void setupUI();
    void setupToolbar();
    void setupTable();
    void loadServices();
    void filterServices(const QString& filter);
    void executeServiceCommand(const QString& serviceName, const QString& action);
    void updateButtonStates();

    QLineEdit* m_searchEdit;
    QPushButton* m_refreshBtn;
    QPushButton* m_startBtn;
    QPushButton* m_stopBtn;
    QPushButton* m_restartBtn;
    QPushButton* m_enableBtn;
    QPushButton* m_disableBtn;
    QTableWidget* m_table;
    QLabel* m_statusLabel;

    QList<ServiceInfo> m_allServices;
};

#endif
