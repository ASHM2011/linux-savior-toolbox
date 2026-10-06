#ifndef DIAGNOSTICREPORTWIDGET_H
#define DIAGNOSTICREPORTWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QProgressBar>
#include <QTimer>

class DiagnosticReportWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DiagnosticReportWidget(QWidget* parent = nullptr);

private slots:
    void onGenerateClicked();
    void onCopyClicked();
    void onSaveClicked();
    void onGenerateProgress();

private:
    void setupUI();
    void setupHealthOverview();
    QString generateReport();
    QString getCpuModel() const;
    bool checkNetworkStatus() const;
    QString generateLogSummary() const;
    void updateHealthStatus();

    QTextEdit* m_reportView;
    QPushButton* m_generateBtn;
    QPushButton* m_copyBtn;
    QPushButton* m_saveBtn;
    QProgressBar* m_progressBar;
    QTimer* m_progressTimer;
    int m_progressValue;

    QFrame* m_healthOverviewCard;
    QLabel* m_systemStatusIcon;
    QLabel* m_systemStatusText;
    QLabel* m_systemStatusBadge;
    QLabel* m_memoryStatusIcon;
    QLabel* m_memoryStatusText;
    QLabel* m_memoryStatusBadge;
    QLabel* m_diskStatusIcon;
    QLabel* m_diskStatusText;
    QLabel* m_diskStatusBadge;
    QLabel* m_networkStatusIcon;
    QLabel* m_networkStatusText;
    QLabel* m_networkStatusBadge;
};

#endif
