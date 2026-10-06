#ifndef DUALBOOTWIDGET_H
#define DUALBOOTWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QScrollArea>
#include <QGridLayout>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QListWidget>
#include <QProgressBar>
#include <QGroupBox>

struct PartitionInfo {
    QString device;
    QString mountPoint;
    QString fsType;
    QString size;
    QString used;
    QString label;
    bool isMounted;
    bool isWindows;
};

struct BootEntry {
    QString id;
    QString name;
    QString type;
    bool isDefault;
};

class DualBootWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DualBootWidget(QWidget* parent = nullptr);

signals:
    void commandTriggered(const QString& commandId);
    void executeInTerminal(const QString& commandId);

private slots:
    void onFixTimeClicked();
    void onInstallNTFSClicked();
    void onFixGrubClicked();
    void onBootManageClicked();
    void onRedetectClicked();
    void onForceShowClicked();
    void onDefaultBootChanged(const QString& entry);
    void onBootTimeoutChanged(int timeout);
    void onMountPartition(const QString& device);
    void onUnmountPartition(const QString& device);
    void onOpenWindowsFiles();
    void onRepairGrubClicked();
    void onUpdateGrubClicked();
    void onSetDefaultBootClicked();

private:
    void setupUI();
    void initPartitions();
    void initBootEntries();
    bool detectDualBoot();
    bool detectWindowsPartitions();
    bool detectWindowsBootEntry();
    QFrame* createStatusCard();
    QFrame* createToolCard(const QString& icon, const QString& title,
                           const QString& desc, const QString& command,
                           const QString& btnText, const QString& btnObjectName,
                           const char* slot);
    QFrame* createNotFoundCard();
    void setupToolsLayout(QVBoxLayout* layout);
    QFrame* createBootManagerSection();
    QFrame* createPartitionSection();
    QFrame* createFileAccessSection();

    bool m_isDualBoot;
    bool m_forceShow;
    QList<PartitionInfo> m_partitions;
    QList<BootEntry> m_bootEntries;
    QComboBox* m_defaultBootCombo;
    QSpinBox* m_bootTimeoutSpin;
    QCheckBox* m_bootMenuCheck;
    QVBoxLayout* m_partitionListLayout;
    QLabel* m_windowsStatusLabel;
};

#endif
