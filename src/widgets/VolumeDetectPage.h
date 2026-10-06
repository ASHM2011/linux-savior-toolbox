#ifndef VOLUMEDETECTPAGE_H
#define VOLUMEDETECTPAGE_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QTimer>

class HelpDialog;

class VolumeDetectPage : public QWidget
{
    Q_OBJECT

public:
    explicit VolumeDetectPage(QWidget* parent = nullptr);

signals:
    void scanCompleted();

private slots:
    void onHelpClicked();
    void onScanProgress(int percent, const QString& stage);
    void onScanFinished();
    void onSimulationTick();

private:
    void setupUI();
    void startScan();

    QFrame* m_cardFrame;
    QLabel* m_titleLabel;
    QLabel* m_subtitleLabel;
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;
    QPushButton* m_helpBtn;

    HelpDialog* m_helpDialog;

    QTimer* m_simTimer;
    int m_simProgress;
    int m_simStage;
    QStringList m_stageTexts;
};

#endif
