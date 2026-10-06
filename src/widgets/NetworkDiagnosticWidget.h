#ifndef NETWORKDIAGNOSTICWIDGET_H
#define NETWORKDIAGNOSTICWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QProcess>
#include <QTimer>
#include <QTabWidget>

struct NetworkInterface {
    QString name;
    QString ipv4;
    QString ipv6;
    QString mac;
    QString state;
};

struct ListeningPort {
    QString protocol;
    int port;
    QString localAddress;
    QString pidProgram;
};

class NetworkDiagnosticWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NetworkDiagnosticWidget(QWidget* parent = nullptr);

signals:
    void commandTriggered(const QString& command, const QString& displayName, bool needsRoot);

private slots:
    void refreshNetworkInfo();
    void startPing();
    void stopPing();
    void onPingReadyRead();
    void onPingFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void refreshListeningPorts();

private:
    void setupUI();
    void setupInterfaceTab();
    void setupPingTab();
    void setupPortsTab();
    void loadInterfaces();
    void loadListeningPorts();
    void appendPingOutput(const QString& text, bool isError = false);

    QTabWidget* m_tabWidget;

    QWidget* m_interfaceTab;
    QTableWidget* m_interfaceTable;
    QPushButton* m_refreshInterfacesBtn;
    QLabel* m_interfaceStatusLabel;

    QWidget* m_pingTab;
    QLineEdit* m_pingAddressEdit;
    QPushButton* m_startPingBtn;
    QPushButton* m_stopPingBtn;
    QTextEdit* m_pingOutput;
    QLabel* m_pingStatusLabel;
    QProcess* m_pingProcess;

    QWidget* m_portsTab;
    QTableWidget* m_portsTable;
    QPushButton* m_refreshPortsBtn;
    QLabel* m_portsStatusLabel;
};

#endif
