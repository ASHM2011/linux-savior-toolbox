#ifndef SYSTEMCLEANUPWIDGET_H
#define SYSTEMCLEANUPWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QTabWidget>
#include <QProgressBar>
#include <QCheckBox>
#include <QScrollArea>
#include <QMap>
#include <QString>

class SystemCleanupWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SystemCleanupWidget(QWidget* parent = nullptr);

signals:
    void commandTriggered(const QString& commandId);
    void showCommandDetail(const QString& commandId);

private slots:
    void onScanClicked();
    void onCleanClicked();
    void onTabChanged(int index);
    void onPackageCleanClicked();
    void onLogsCleanClicked();
    void onThumbnailCleanClicked();
    void onTrashCleanClicked();
    void onBrowserCleanClicked();

private:
    void setupUI();
    QWidget* createPackageCacheTab();
    QWidget* createTrashTab();
    QWidget* createLogsTab();
    QWidget* createThumbnailTab();
    QWidget* createBrowserCacheTab();
    QFrame* createInfoCard(const QString& title, const QString& size, const QString& count,
                           const QString& descTitle, const QString& descText,
                           const QString& buttonText, const QString& buttonObjectName,
                           const QString& progressBarObjectName,
                           QPushButton*& cleanBtn, QProgressBar*& progressBar, QLabel*& resultLabel,
                           QLabel*& sizeValueOut);
    void simulateCleanup(QProgressBar* progressBar, QPushButton* cleanBtn, QLabel* resultLabel,
                         const QString& commandId);
    QString formatSize(qint64 bytes);
    qint64 getDirSize(const QString& path);
    void scanRealSizes();

    QTabWidget* m_tabWidget;
    QPushButton* m_scanBtn;
    QPushButton* m_cleanBtn;
    QLabel* m_totalSizeLabel;
    QProgressBar* m_progressBar;

    QLabel* m_packageSizeLabel;
    QLabel* m_logsSizeLabel;
    QLabel* m_thumbnailSizeLabel;
    QLabel* m_trashSizeLabel;
    QLabel* m_browserSizeLabel;

    QPushButton* m_packageCleanBtn;
    QProgressBar* m_packageProgressBar;
    QLabel* m_packageResultLabel;

    QPushButton* m_logsCleanBtn;
    QProgressBar* m_logsProgressBar;
    QLabel* m_logsResultLabel;

    QPushButton* m_thumbnailCleanBtn;
    QProgressBar* m_thumbnailProgressBar;
    QLabel* m_thumbnailResultLabel;

    QPushButton* m_trashCleanBtn;
    QProgressBar* m_trashProgressBar;
    QLabel* m_trashResultLabel;

    QPushButton* m_browserCleanBtn;
    QProgressBar* m_browserProgressBar;
    QLabel* m_browserResultLabel;
    QCheckBox* m_chromeCheckBox;
    QCheckBox* m_firefoxCheckBox;

    QMap<QString, bool> m_cleanedTabs;
};

#endif
