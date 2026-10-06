#ifndef DASHBOARDWIDGET_H
#define DASHBOARDWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QProgressBar>
#include <QTimer>

class DashboardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardWidget(QWidget* parent = nullptr);

signals:
    void commandTriggered(const QString& commandId);
    void showCommandDetail(const QString& commandId);
    void executeInTerminal(const QString& commandId);

private slots:
    void updateStats();
    void onQuickActionClicked();
    void onInfoClicked();
    void onRecommendationClicked();
    void onRecommendationInfoClicked();

private:
    void setupUI();
    void setupQuickActions();
    void setupRecommendation();
    void setupSystemInfoCard();
    void updateRecommendation();
    void updateSystemInfo();
    double readCpuUsage();
    QString formatBytes(qint64 bytes) const;
    QString formatUptime(long seconds) const;
    QString getCpuModel() const;

    QLabel* m_welcomeLabel;
    QLabel* m_systemInfoLabel;
    QLabel* m_uptimeLabel;

    QLabel* m_cpuValue;
    QLabel* m_cpuLabel;
    QProgressBar* m_cpuBar;

    QLabel* m_memValue;
    QLabel* m_memLabel;
    QProgressBar* m_memBar;

    QLabel* m_diskValue;
    QLabel* m_diskLabel;
    QProgressBar* m_diskBar;

    QFrame* m_recommendationCard;
    QLabel* m_recommendationIcon;
    QLabel* m_recommendationTitle;
    QLabel* m_recommendationDesc;
    QPushButton* m_recommendationBtn;
    QPushButton* m_recommendationInfoBtn;
    QString m_recommendationCommandId;

    QFrame* m_sysInfoCard;
    QLabel* m_cpuModelLabel;
    QLabel* m_gpuModelLabel;
    QLabel* m_kernelLabel;
    QLabel* m_archLabel;
    QLabel* m_hostnameLabel;

    QGridLayout* m_actionsLayout;
    QTimer* m_statsTimer;

    double m_prevCpuTotal;
    double m_prevCpuIdle;
    bool m_firstCpuRead;
};

#endif
