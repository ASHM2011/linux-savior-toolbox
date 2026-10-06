#ifndef SYSTEMINFOWIDGET_H
#define SYSTEMINFOWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QScrollArea>
#include <QProcess>

class SystemInfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SystemInfoWidget(QWidget* parent = nullptr);

private slots:
    void refreshInfo();

private:
    void setupUI();
    QFrame* createInfoCard(const QString& icon, const QString& title);
    void addInfoRow(QGridLayout* grid, int row, const QString& key, const QString& value);
    void loadOverviewInfo();
    void loadCpuInfo();
    void loadGpuInfo();
    void loadMemoryInfo();
    void loadDiskInfo();
    void loadNetworkInfo();
    QString runCommand(const QString& program, const QStringList& arguments, int timeoutMs = 3000);
    QString readSysFile(const QString& path);
    static QString formatBytes(qint64 bytes);

    QVBoxLayout* m_cardsLayout;
    QLabel* m_statusLabel;
    QPushButton* m_refreshBtn;
};

#endif
