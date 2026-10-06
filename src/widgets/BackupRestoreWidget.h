#ifndef BACKUPRESTOREWIDGET_H
#define BACKUPRESTOREWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QListWidget>
#include <QProgressBar>
#include <QCheckBox>
#include <QScrollArea>
#include <QList>
#include <QComboBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QRadioButton>
#include <QButtonGroup>

struct BackupItem {
    QString id;
    QString name;
    QString dateTime;
    QString size;
    QStringList contents;
    bool isFullBackup;
};

struct TimeshiftSnapshot {
    QString name;
    QString dateTime;
    QString size;
    QString type;
    bool isMounted;
};

class BackupRestoreWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BackupRestoreWidget(QWidget* parent = nullptr);

private slots:
    void onBackupClicked();
    void onRestoreClicked(const QString& backupId);
    void onDeleteClicked(const QString& backupId);
    void onDetailClicked(const QString& backupId);
    void onCreateSnapshotClicked();
    void onViewSnapshotsClicked();
    void onScheduleEnabledChanged(bool enabled);
    void onScheduleTypeChanged(const QString& type);
    void onSnapshotRestoreClicked(const QString& name);
    void onSnapshotDeleteClicked(const QString& name);
    void onBrowseSnapshotClicked(const QString& name);
    void onDejaDupBackupClicked();
    void onDejaDupOpenClicked();

private:
    void setupUI();
    void initBackupItems();
    void initTimeshiftSnapshots();
    QFrame* createBackupCard(const BackupItem& item);
    QFrame* createSnapshotCard(const TimeshiftSnapshot& snap);
    void refreshBackupList();
    void refreshSnapshotList();
    int getSelectedContentCount();
    bool checkTimeshiftInstalled();
    bool checkDejaDupInstalled();
    QString runTimeshiftCmd(const QStringList& args, int timeoutMs = 30000);
    void refreshSnapshotsFromSystem();
    QString snapshotTypeFromTag(const QString& tag);

    QCheckBox* m_themeCheck;
    QCheckBox* m_iconsCheck;
    QCheckBox* m_extensionsCheck;
    QCheckBox* m_shortcutsCheck;
    QCheckBox* m_panelCheck;
    QCheckBox* m_dataCheck;
    QPushButton* m_backupBtn;
    QProgressBar* m_progressBar;
    QVBoxLayout* m_backupListLayout;
    QVBoxLayout* m_snapshotListLayout;
    QList<BackupItem> m_backupItems;
    QList<TimeshiftSnapshot> m_snapshots;
    QLabel* m_timeshiftStatusLabel;
    QLabel* m_snapshotCountLabel;
    QCheckBox* m_scheduleCheck;
    QComboBox* m_scheduleTypeCombo;
    QSpinBox* m_keepSpinBox;
    QLabel* m_nextBackupLabel;
    QLabel* m_dejaDupStatusLabel;
};

#endif
