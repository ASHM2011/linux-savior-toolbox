#include "BackupRestoreWidget.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QTimer>
#include <QDateTime>
#include <QScrollArea>
#include <QProcess>
#include <QStandardPaths>
#include <QDir>
#include <QApplication>
#include <QRegularExpression>

BackupRestoreWidget::BackupRestoreWidget(QWidget* parent)
    : QWidget(parent)
    , m_themeCheck(nullptr)
    , m_iconsCheck(nullptr)
    , m_extensionsCheck(nullptr)
    , m_shortcutsCheck(nullptr)
    , m_panelCheck(nullptr)
    , m_dataCheck(nullptr)
    , m_backupBtn(nullptr)
    , m_progressBar(nullptr)
    , m_backupListLayout(nullptr)
    , m_snapshotListLayout(nullptr)
    , m_timeshiftStatusLabel(nullptr)
    , m_snapshotCountLabel(nullptr)
    , m_scheduleCheck(nullptr)
    , m_scheduleTypeCombo(nullptr)
    , m_keepSpinBox(nullptr)
    , m_nextBackupLabel(nullptr)
    , m_dejaDupStatusLabel(nullptr)
{
    initBackupItems();
    initTimeshiftSnapshots();
    setupUI();
    QTimer::singleShot(500, this, &BackupRestoreWidget::refreshSnapshotsFromSystem);
}

void BackupRestoreWidget::initBackupItems()
{
    // 从备份目录扫描真实的备份文件
    QString backupDir = QDir::homePath() + "/文档/backup";
    QDir dir(backupDir, "desktop_backup_*.tar.gz", QDir::Time, QDir::Files);
    const QStringList files = dir.entryList();

    for (const QString& file : files) {
        QString filePath = backupDir + "/" + file;
        QFileInfo fi(filePath);

        // 从文件名提取时间戳: desktop_backup_20260714_153022.tar.gz
        QString timestamp = file;
        timestamp.remove("desktop_backup_").remove(".tar.gz");
        QDateTime dt = QDateTime::fromString(timestamp, "yyyyMMdd_hhmmss");

        BackupItem item;
        item.id = "backup_" + timestamp;
        item.name = dt.isValid()
            ? tr("备份 - ") + dt.toString("MM-dd hh:mm")
            : file;
        item.dateTime = dt.isValid()
            ? dt.toString("yyyy-MM-dd hh:mm:ss")
            : fi.lastModified().toString("yyyy-MM-dd hh:mm:ss");

        qint64 bytes = fi.size();
        if (bytes > 1024 * 1024 * 1024)
            item.size = QString::number(bytes / 1024.0 / 1024.0 / 1024.0, 'f', 2) + " GB";
        else if (bytes > 1024 * 1024)
            item.size = QString::number(bytes / 1024.0 / 1024.0, 'f', 2) + " MB";
        else if (bytes > 1024)
            item.size = QString::number(bytes / 1024.0, 'f', 2) + " KB";
        else
            item.size = QString::number(bytes) + " B";

        item.contents = {tr("桌面配置")};
        item.isFullBackup = false;
        m_backupItems.append(item);
    }
}

void BackupRestoreWidget::initTimeshiftSnapshots()
{
    // 初始为空，真实快照列表由 refreshSnapshotsFromSystem() 异步加载
}

bool BackupRestoreWidget::checkTimeshiftInstalled()
{
    QProcess process;
    process.start("which", QStringList() << "timeshift");
    process.waitForFinished(2000);
    return process.exitCode() == 0;
}

bool BackupRestoreWidget::checkDejaDupInstalled()
{
    QProcess process;
    process.start("which", QStringList() << "deja-dup");
    process.waitForFinished(2000);
    return process.exitCode() == 0;
}

QString BackupRestoreWidget::runTimeshiftCmd(const QStringList& args, int timeoutMs)
{
    QProcess process;
    process.start(QStringLiteral("pkexec"),
                  QStringList() << QStringLiteral("timeshift") << args);
    if (!process.waitForStarted(5000)) {
        return QString();
    }
    process.waitForFinished(timeoutMs);
    return QString::fromUtf8(process.readAllStandardOutput());
}

QString BackupRestoreWidget::snapshotTypeFromTag(const QString& tag)
{
    if (tag == "O") return tr("手动");
    if (tag == "B") return tr("启动");
    if (tag == "H") return tr("每小时");
    if (tag == "D") return tr("每日");
    if (tag == "W") return tr("每周");
    if (tag == "M") return tr("每月");
    return tr("未知");
}

void BackupRestoreWidget::refreshSnapshotsFromSystem()
{
    if (!checkTimeshiftInstalled()) {
        if (m_timeshiftStatusLabel) {
            m_timeshiftStatusLabel->setText(tr("Timeshift 未安装"));
            m_timeshiftStatusLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #dc2626;");
        }
        return;
    }

    QString output = runTimeshiftCmd({"--list-snapshots"}, 15000);
    if (output.isEmpty()) {
        // 可能用户取消了授权或 timeshift 未配置
        return;
    }

    m_snapshots.clear();
    const QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    bool inList = false;
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.startsWith("Name") || trimmed.startsWith("----") || trimmed.startsWith("List")) {
            inList = true;
            continue;
        }
        if (!inList || trimmed.isEmpty()) continue;

        // 典型行: 2026-07-14_10-30-00  2026-07-14  10:30:00  D  12.5 GB
        // 或:    2026-07-14_10-30-00  2026-07-14  10:30:00  D  12.5  GB
        // 按空白分割
        QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() < 4) continue;

        TimeshiftSnapshot snap;
        snap.name = parts[0];
        QString datePart = parts.size() > 1 ? parts[1] : QString();
        QString timePart = parts.size() > 2 ? parts[2] : QString();
        snap.dateTime = datePart + " " + timePart;
        snap.type = snapshotTypeFromTag(parts.size() > 3 ? parts[3] : "O");
        snap.isMounted = false;

        // 提取大小（最后几个字段可能是 "12.5 GB"）
        if (parts.size() >= 6) {
            snap.size = parts[parts.size() - 2] + " " + parts[parts.size() - 1];
        } else if (parts.size() == 5) {
            snap.size = parts[4];
        } else {
            snap.size = tr("未知");
        }
        m_snapshots.append(snap);
    }

    refreshSnapshotList();
    if (m_snapshotCountLabel) {
        m_snapshotCountLabel->setText(QString::number(m_snapshots.size()) + tr(" 个"));
    }
}

void BackupRestoreWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget* content = new QWidget();
    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(32, 24, 32, 24);
    contentLayout->setSpacing(20);

    QLabel* title = new QLabel(tr("💾 系统备份与恢复"));
    title->setStyleSheet("font-size: 22px; font-weight: 700; color: #0f172a;");
    contentLayout->addWidget(title);

    QLabel* subtitle = new QLabel(tr("桌面配置一键备份 + Timeshift 系统快照，双重保障数据安全"));
    subtitle->setStyleSheet("font-size: 13px; color: #64748b;");
    contentLayout->addWidget(subtitle);

    QHBoxLayout* topRow = new QHBoxLayout();
    topRow->setSpacing(20);

    QFrame* backupPanel = new QFrame();
    backupPanel->setObjectName("card");
    QVBoxLayout* backupLayout = new QVBoxLayout(backupPanel);
    backupLayout->setContentsMargins(20, 20, 20, 20);
    backupLayout->setSpacing(16);

    QLabel* backupTitle = new QLabel(tr("📦 桌面配置备份"));
    backupTitle->setStyleSheet("font-size: 15px; font-weight: 600; color: #0f172a;");
    backupLayout->addWidget(backupTitle);

    m_themeCheck = new QCheckBox(tr("🎨 主题与样式"));
    m_themeCheck->setChecked(true);
    m_themeCheck->setStyleSheet("font-size: 13px;");
    backupLayout->addWidget(m_themeCheck);

    m_iconsCheck = new QCheckBox(tr("🖼️ 图标主题"));
    m_iconsCheck->setChecked(true);
    m_iconsCheck->setStyleSheet("font-size: 13px;");
    backupLayout->addWidget(m_iconsCheck);

    m_extensionsCheck = new QCheckBox(tr("🧩 GNOME扩展"));
    m_extensionsCheck->setChecked(true);
    m_extensionsCheck->setStyleSheet("font-size: 13px;");
    backupLayout->addWidget(m_extensionsCheck);

    m_shortcutsCheck = new QCheckBox(tr("⌨️ 快捷键设置"));
    m_shortcutsCheck->setChecked(true);
    m_shortcutsCheck->setStyleSheet("font-size: 13px;");
    backupLayout->addWidget(m_shortcutsCheck);

    m_panelCheck = new QCheckBox(tr("📋 面板布局"));
    m_panelCheck->setChecked(true);
    m_panelCheck->setStyleSheet("font-size: 13px;");
    backupLayout->addWidget(m_panelCheck);

    m_dataCheck = new QCheckBox(tr("📁 应用数据"));
    m_dataCheck->setChecked(false);
    m_dataCheck->setStyleSheet("font-size: 13px;");
    backupLayout->addWidget(m_dataCheck);

    backupLayout->addStretch(1);

    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(8);
    backupLayout->addWidget(m_progressBar);

    m_backupBtn = new QPushButton(tr("✨ 立即备份"));
    m_backupBtn->setFixedHeight(42);
    m_backupBtn->setCursor(Qt::PointingHandCursor);
    connect(m_backupBtn, &QPushButton::clicked, this, &BackupRestoreWidget::onBackupClicked);
    backupLayout->addWidget(m_backupBtn);

    topRow->addWidget(backupPanel, 1);

    QFrame* timeshiftPanel = new QFrame();
    timeshiftPanel->setObjectName("card");
    QVBoxLayout* timeshiftLayout = new QVBoxLayout(timeshiftPanel);
    timeshiftLayout->setContentsMargins(20, 20, 20, 20);
    timeshiftLayout->setSpacing(16);

    QLabel* timeshiftTitle = new QLabel(tr("⏰ Timeshift 系统快照"));
    timeshiftTitle->setStyleSheet("font-size: 15px; font-weight: 600; color: #0f172a;");
    timeshiftLayout->addWidget(timeshiftTitle);

    QFrame* statusFrame = new QFrame();
    statusFrame->setStyleSheet(R"(
        QFrame {
            background-color: #f0fdf4;
            border-radius: 8px;
        }
    )");
    QHBoxLayout* statusLayout = new QHBoxLayout(statusFrame);
    statusLayout->setContentsMargins(12, 10, 12, 10);
    statusLayout->setSpacing(10);

    QLabel* statusIcon = new QLabel("✅");
    statusIcon->setStyleSheet("font-size: 20px;");
    statusLayout->addWidget(statusIcon);

    QVBoxLayout* statusTextLayout = new QVBoxLayout();
    statusTextLayout->setSpacing(2);

    m_timeshiftStatusLabel = new QLabel(tr("Timeshift 已安装"));
    m_timeshiftStatusLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #15803d;");
    statusTextLayout->addWidget(m_timeshiftStatusLabel);

    QLabel* statusDesc = new QLabel(tr("可创建系统级快照，出问题时可完全恢复"));
    statusDesc->setStyleSheet("font-size: 11px; color: #22c55e;");
    statusTextLayout->addWidget(statusDesc);

    statusLayout->addLayout(statusTextLayout, 1);
    timeshiftLayout->addWidget(statusFrame);

    QFrame* infoFrame = new QFrame();
    infoFrame->setStyleSheet(R"(
        QFrame {
            background-color: #f8fafc;
            border-radius: 8px;
        }
    )");
    QVBoxLayout* infoLayout = new QVBoxLayout(infoFrame);
    infoLayout->setContentsMargins(12, 10, 12, 10);
    infoLayout->setSpacing(6);

    QHBoxLayout* snapshotRow1 = new QHBoxLayout();
    QLabel* snapLabel = new QLabel(tr("📸 现有快照"));
    snapLabel->setStyleSheet("font-size: 12px; color: #64748b;");
    snapshotRow1->addWidget(snapLabel);
    snapshotRow1->addStretch(1);
    m_snapshotCountLabel = new QLabel(QString::number(m_snapshots.size()) + tr(" 个"));
    m_snapshotCountLabel->setStyleSheet("font-size: 12px; font-weight: 600; color: #334155;");
    snapshotRow1->addWidget(m_snapshotCountLabel);
    infoLayout->addLayout(snapshotRow1);

    QHBoxLayout* snapshotRow2 = new QHBoxLayout();
    QLabel* latestLabel = new QLabel(tr("🕐 最新快照"));
    latestLabel->setStyleSheet("font-size: 12px; color: #64748b;");
    snapshotRow2->addWidget(latestLabel);
    snapshotRow2->addStretch(1);
    QLabel* latestDate = new QLabel(m_snapshots.isEmpty() ? tr("无") : m_snapshots.first().dateTime.left(10));
    latestDate->setStyleSheet("font-size: 12px; font-weight: 600; color: #334155;");
    snapshotRow2->addWidget(latestDate);
    infoLayout->addLayout(snapshotRow2);

    timeshiftLayout->addWidget(infoFrame);

    QFrame* scheduleFrame = new QFrame();
    scheduleFrame->setStyleSheet(R"(
        QFrame {
            background-color: #eff6ff;
            border-radius: 8px;
        }
    )");
    QVBoxLayout* scheduleLayout = new QVBoxLayout(scheduleFrame);
    scheduleLayout->setContentsMargins(12, 10, 12, 10);
    scheduleLayout->setSpacing(8);

    m_scheduleCheck = new QCheckBox(tr("🔄 启用自动备份计划"));
    m_scheduleCheck->setChecked(true);
    m_scheduleCheck->setStyleSheet("font-size: 12px; font-weight: 600; color: #1d4ed8;");
    connect(m_scheduleCheck, &QCheckBox::toggled, this, &BackupRestoreWidget::onScheduleEnabledChanged);
    scheduleLayout->addWidget(m_scheduleCheck);

    QHBoxLayout* typeRow = new QHBoxLayout();
    typeRow->setSpacing(8);
    QLabel* typeLabel = new QLabel(tr("频率:"));
    typeLabel->setStyleSheet("font-size: 11px; color: #64748b;");
    typeRow->addWidget(typeLabel);
    m_scheduleTypeCombo = new QComboBox();
    m_scheduleTypeCombo->addItems({tr("每日"), tr("每周"), tr("每月"), tr("每小时")});
    m_scheduleTypeCombo->setCurrentIndex(0);
    m_scheduleTypeCombo->setStyleSheet("font-size: 11px;");
    connect(m_scheduleTypeCombo, &QComboBox::currentTextChanged, this, &BackupRestoreWidget::onScheduleTypeChanged);
    typeRow->addWidget(m_scheduleTypeCombo, 1);
    scheduleLayout->addLayout(typeRow);

    QHBoxLayout* keepRow = new QHBoxLayout();
    keepRow->setSpacing(8);
    QLabel* keepLabel = new QLabel(tr("保留:"));
    keepLabel->setStyleSheet("font-size: 11px; color: #64748b;");
    keepRow->addWidget(keepLabel);
    m_keepSpinBox = new QSpinBox();
    m_keepSpinBox->setRange(1, 30);
    m_keepSpinBox->setValue(5);
    m_keepSpinBox->setSuffix(tr(" 个"));
    m_keepSpinBox->setStyleSheet("font-size: 11px;");
    keepRow->addWidget(m_keepSpinBox, 1);
    scheduleLayout->addLayout(keepRow);

    m_nextBackupLabel = new QLabel(tr("📅 下次备份: 明天 02:00"));
    m_nextBackupLabel->setStyleSheet("font-size: 11px; color: #3b82f6;");
    scheduleLayout->addWidget(m_nextBackupLabel);

    timeshiftLayout->addWidget(scheduleFrame);

    timeshiftLayout->addStretch(1);

    QPushButton* createSnapshotBtn = new QPushButton(tr("📸 创建系统快照"));
    createSnapshotBtn->setObjectName("secondaryBtn");
    createSnapshotBtn->setFixedHeight(38);
    createSnapshotBtn->setCursor(Qt::PointingHandCursor);
    connect(createSnapshotBtn, &QPushButton::clicked, this, &BackupRestoreWidget::onCreateSnapshotClicked);
    timeshiftLayout->addWidget(createSnapshotBtn);

    QPushButton* viewSnapshotsBtn = new QPushButton(tr("📂 管理快照"));
    viewSnapshotsBtn->setObjectName("secondaryBtn");
    viewSnapshotsBtn->setFixedHeight(38);
    viewSnapshotsBtn->setCursor(Qt::PointingHandCursor);
    connect(viewSnapshotsBtn, &QPushButton::clicked, this, &BackupRestoreWidget::onViewSnapshotsClicked);
    timeshiftLayout->addWidget(viewSnapshotsBtn);

    topRow->addWidget(timeshiftPanel, 1);

    contentLayout->addLayout(topRow);

    QLabel* backupListTitle = new QLabel(tr("📂 桌面配置备份"));
    backupListTitle->setStyleSheet("font-size: 15px; font-weight: 600; color: #0f172a; margin-top: 8px;");
    contentLayout->addWidget(backupListTitle);

    QFrame* backupListFrame = new QFrame();
    backupListFrame->setObjectName("card");
    QVBoxLayout* backupListFrameLayout = new QVBoxLayout(backupListFrame);
    backupListFrameLayout->setContentsMargins(16, 16, 16, 16);
    backupListFrameLayout->setSpacing(12);

    m_backupListLayout = new QVBoxLayout();
    m_backupListLayout->setSpacing(10);
    backupListFrameLayout->addLayout(m_backupListLayout);

    refreshBackupList();

    contentLayout->addWidget(backupListFrame);

    QLabel* snapshotListTitle = new QLabel(tr("📸 Timeshift 系统快照"));
    snapshotListTitle->setStyleSheet("font-size: 15px; font-weight: 600; color: #0f172a; margin-top: 8px;");
    contentLayout->addWidget(snapshotListTitle);

    QFrame* snapshotListFrame = new QFrame();
    snapshotListFrame->setObjectName("card");
    QVBoxLayout* snapshotListFrameLayout = new QVBoxLayout(snapshotListFrame);
    snapshotListFrameLayout->setContentsMargins(16, 16, 16, 16);
    snapshotListFrameLayout->setSpacing(12);

    m_snapshotListLayout = new QVBoxLayout();
    m_snapshotListLayout->setSpacing(10);
    snapshotListFrameLayout->addLayout(m_snapshotListLayout);

    refreshSnapshotList();

    contentLayout->addWidget(snapshotListFrame);

    // ============ Deja Dup 个人文件备份 ============
    QLabel* dejaDupTitle = new QLabel(tr("🗂️ Deja Dup 个人文件备份"));
    dejaDupTitle->setStyleSheet("font-size: 15px; font-weight: 600; color: #0f172a; margin-top: 8px;");
    contentLayout->addWidget(dejaDupTitle);

    QFrame* dejaDupFrame = new QFrame();
    dejaDupFrame->setObjectName("card");
    QVBoxLayout* dejaDupLayout = new QVBoxLayout(dejaDupFrame);
    dejaDupLayout->setContentsMargins(20, 20, 20, 20);
    dejaDupLayout->setSpacing(14);

    QFrame* dejaStatusFrame = new QFrame();
    dejaStatusFrame->setStyleSheet(R"(
        QFrame { background-color: #f0fdf4; border-radius: 8px; }
    )");
    QHBoxLayout* dejaStatusLayout = new QHBoxLayout(dejaStatusFrame);
    dejaStatusLayout->setContentsMargins(12, 10, 12, 10);
    dejaStatusLayout->setSpacing(10);

    QLabel* dejaIcon = new QLabel("✅");
    dejaIcon->setStyleSheet("font-size: 20px;");
    dejaStatusLayout->addWidget(dejaIcon);

    QVBoxLayout* dejaStatusText = new QVBoxLayout();
    dejaStatusText->setSpacing(2);
    m_dejaDupStatusLabel = new QLabel(checkDejaDupInstalled() ? tr("Deja Dup 已安装") : tr("Deja Dup 未安装"));
    m_dejaDupStatusLabel->setStyleSheet(QStringLiteral("font-size: 13px; font-weight: 600; color: %1;")
        .arg(checkDejaDupInstalled() ? "#15803d" : "#dc2626"));
    dejaStatusText->addWidget(m_dejaDupStatusLabel);
    QLabel* dejaDesc = new QLabel(tr("加密备份个人文件到本地/云端，支持增量备份与版本历史"));
    dejaDesc->setStyleSheet("font-size: 11px; color: #64748b;");
    dejaStatusText->addWidget(dejaDesc);
    dejaStatusLayout->addLayout(dejaStatusText, 1);
    dejaDupLayout->addWidget(dejaStatusFrame);

    QLabel* dejaHint = new QLabel(tr("💡 Deja Dup 会备份「主目录」中的个人文件（文档、图片、下载等），\n"
                                     "自动跳过缓存与临时文件。首次使用请先在设置中选择备份存储位置。"));
    dejaHint->setStyleSheet("font-size: 12px; color: #64748b; background: #f8fafc; padding: 10px; border-radius: 6px;");
    dejaDupLayout->addWidget(dejaHint);

    QHBoxLayout* dejaBtnRow = new QHBoxLayout();
    dejaBtnRow->setSpacing(12);

    QPushButton* dejaBackupBtn = new QPushButton(tr("🚀 立即备份"));
    dejaBackupBtn->setObjectName("secondaryBtn");
    dejaBackupBtn->setFixedHeight(40);
    dejaBackupBtn->setCursor(Qt::PointingHandCursor);
    connect(dejaBackupBtn, &QPushButton::clicked, this, &BackupRestoreWidget::onDejaDupBackupClicked);
    dejaBtnRow->addWidget(dejaBackupBtn);

    QPushButton* dejaOpenBtn = new QPushButton(tr("⚙️ 打开设置与恢复"));
    dejaOpenBtn->setObjectName("secondaryBtn");
    dejaOpenBtn->setFixedHeight(40);
    dejaOpenBtn->setCursor(Qt::PointingHandCursor);
    connect(dejaOpenBtn, &QPushButton::clicked, this, &BackupRestoreWidget::onDejaDupOpenClicked);
    dejaBtnRow->addWidget(dejaOpenBtn);

    dejaDupLayout->addLayout(dejaBtnRow);

    contentLayout->addWidget(dejaDupFrame);

    contentLayout->addStretch(1);

    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);
}

QFrame* BackupRestoreWidget::createBackupCard(const BackupItem& item)
{
    QFrame* card = new QFrame();
    card->setStyleSheet(R"(
        QFrame {
            background-color: #f8fafc;
            border: 1px solid #e2e8f0;
            border-radius: 10px;
        }
        QFrame:hover {
            background-color: #f1f5f9;
            border-color: #cbd5e1;
        }
    )");

    QHBoxLayout* layout = new QHBoxLayout(card);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(14);

    QLabel* iconLabel = new QLabel(item.isFullBackup ? "🗄️" : "📦");
    iconLabel->setStyleSheet("font-size: 28px;");
    iconLabel->setFixedWidth(40);
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);

    QVBoxLayout* infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(4);

    QHBoxLayout* nameRow = new QHBoxLayout();
    nameRow->setSpacing(8);

    QLabel* nameLabel = new QLabel(item.name);
    nameLabel->setStyleSheet("font-size: 14px; font-weight: 600; color: #0f172a;");
    nameRow->addWidget(nameLabel);

    if (item.isFullBackup) {
        QLabel* fullBadge = new QLabel(tr("完整备份"));
        fullBadge->setObjectName("successBadge");
        fullBadge->setAlignment(Qt::AlignCenter);
        fullBadge->setFixedHeight(20);
        nameRow->addWidget(fullBadge);
    }

    nameRow->addStretch(1);
    infoLayout->addLayout(nameRow);

    QHBoxLayout* metaRow = new QHBoxLayout();
    metaRow->setSpacing(12);

    QLabel* dateLabel = new QLabel("🕐 " + item.dateTime);
    dateLabel->setStyleSheet("font-size: 12px; color: #64748b;");
    metaRow->addWidget(dateLabel);

    QLabel* sizeLabel = new QLabel("📊 " + item.size);
    sizeLabel->setStyleSheet("font-size: 12px; color: #64748b;");
    metaRow->addWidget(sizeLabel);

    metaRow->addStretch(1);
    infoLayout->addLayout(metaRow);

    QString contentsText = tr("包含: ") + item.contents.join(" · ");
    QLabel* contentsLabel = new QLabel(contentsText);
    contentsLabel->setStyleSheet("font-size: 11px; color: #94a3b8;");
    infoLayout->addWidget(contentsLabel);

    layout->addLayout(infoLayout, 1);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(6);

    QPushButton* detailBtn = new QPushButton(tr("详情"));
    detailBtn->setObjectName("secondaryBtn");
    detailBtn->setFixedHeight(32);
    detailBtn->setCursor(Qt::PointingHandCursor);
    detailBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #ffffff;
            color: #64748b;
            border: 1px solid #e2e8f0;
            border-radius: 6px;
            font-size: 12px;
            padding: 0 12px;
        }
        QPushButton:hover {
            background-color: #f1f5f9;
            color: #334155;
        }
    )");
    connect(detailBtn, &QPushButton::clicked, [this, item]() {
        onDetailClicked(item.id);
    });
    btnLayout->addWidget(detailBtn);

    QPushButton* restoreBtn = new QPushButton(tr("恢复"));
    restoreBtn->setFixedHeight(32);
    restoreBtn->setCursor(Qt::PointingHandCursor);
    restoreBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #3b82f6;
            color: white;
            border: none;
            border-radius: 6px;
            font-size: 12px;
            font-weight: 500;
            padding: 0 14px;
        }
        QPushButton:hover {
            background-color: #2563eb;
        }
    )");
    connect(restoreBtn, &QPushButton::clicked, [this, item]() {
        onRestoreClicked(item.id);
    });
    btnLayout->addWidget(restoreBtn);

    QPushButton* deleteBtn = new QPushButton(tr("删除"));
    deleteBtn->setFixedHeight(32);
    deleteBtn->setCursor(Qt::PointingHandCursor);
    deleteBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #fef2f2;
            color: #dc2626;
            border: none;
            border-radius: 6px;
            font-size: 12px;
            padding: 0 12px;
        }
        QPushButton:hover {
            background-color: #fee2e2;
        }
    )");
    connect(deleteBtn, &QPushButton::clicked, [this, item]() {
        onDeleteClicked(item.id);
    });
    btnLayout->addWidget(deleteBtn);

    layout->addLayout(btnLayout);

    return card;
}

QFrame* BackupRestoreWidget::createSnapshotCard(const TimeshiftSnapshot& snap)
{
    QFrame* card = new QFrame();
    card->setStyleSheet(R"(
        QFrame {
            background-color: #f0fdf4;
            border: 1px solid #bbf7d0;
            border-radius: 10px;
        }
        QFrame:hover {
            background-color: #dcfce7;
            border-color: #86efac;
        }
    )");

    QHBoxLayout* layout = new QHBoxLayout(card);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(14);

    QLabel* iconLabel = new QLabel("📸");
    iconLabel->setStyleSheet("font-size: 28px;");
    iconLabel->setFixedWidth(40);
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);

    QVBoxLayout* infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(4);

    QHBoxLayout* nameRow = new QHBoxLayout();
    nameRow->setSpacing(8);

    QLabel* nameLabel = new QLabel(snap.name);
    nameLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #166534;");
    nameRow->addWidget(nameLabel);

    QLabel* typeBadge = new QLabel(snap.type);
    typeBadge->setStyleSheet(R"(
        QLabel {
            background-color: #22c55e;
            color: white;
            border-radius: 4px;
            padding: 2px 8px;
            font-size: 10px;
            font-weight: 600;
        }
    )");
    typeBadge->setAlignment(Qt::AlignCenter);
    typeBadge->setFixedHeight(18);
    nameRow->addWidget(typeBadge);

    nameRow->addStretch(1);
    infoLayout->addLayout(nameRow);

    QHBoxLayout* metaRow = new QHBoxLayout();
    metaRow->setSpacing(12);

    QLabel* dateLabel = new QLabel("🕐 " + snap.dateTime);
    dateLabel->setStyleSheet("font-size: 12px; color: #15803d;");
    metaRow->addWidget(dateLabel);

    QLabel* sizeLabel = new QLabel("💾 " + snap.size);
    sizeLabel->setStyleSheet("font-size: 12px; color: #15803d;");
    metaRow->addWidget(sizeLabel);

    metaRow->addStretch(1);
    infoLayout->addLayout(metaRow);

    layout->addLayout(infoLayout, 1);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(6);

    QPushButton* browseBtn = new QPushButton(tr("浏览"));
    browseBtn->setFixedHeight(32);
    browseBtn->setCursor(Qt::PointingHandCursor);
    browseBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #ffffff;
            color: #166534;
            border: 1px solid #86efac;
            border-radius: 6px;
            font-size: 12px;
            padding: 0 12px;
        }
        QPushButton:hover {
            background-color: #f0fdf4;
            color: #14532d;
        }
    )");
    connect(browseBtn, &QPushButton::clicked, [this, snap]() {
        onBrowseSnapshotClicked(snap.name);
    });
    btnLayout->addWidget(browseBtn);

    QPushButton* restoreBtn = new QPushButton(tr("恢复"));
    restoreBtn->setFixedHeight(32);
    restoreBtn->setCursor(Qt::PointingHandCursor);
    restoreBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #16a34a;
            color: white;
            border: none;
            border-radius: 6px;
            font-size: 12px;
            font-weight: 500;
            padding: 0 14px;
        }
        QPushButton:hover {
            background-color: #15803d;
        }
    )");
    connect(restoreBtn, &QPushButton::clicked, [this, snap]() {
        onSnapshotRestoreClicked(snap.name);
    });
    btnLayout->addWidget(restoreBtn);

    QPushButton* deleteBtn = new QPushButton(tr("删除"));
    deleteBtn->setFixedHeight(32);
    deleteBtn->setCursor(Qt::PointingHandCursor);
    deleteBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #fef2f2;
            color: #dc2626;
            border: none;
            border-radius: 6px;
            font-size: 12px;
            padding: 0 12px;
        }
        QPushButton:hover {
            background-color: #fee2e2;
        }
    )");
    connect(deleteBtn, &QPushButton::clicked, [this, snap]() {
        onSnapshotDeleteClicked(snap.name);
    });
    btnLayout->addWidget(deleteBtn);

    layout->addLayout(btnLayout);

    return card;
}

void BackupRestoreWidget::refreshBackupList()
{
    QLayoutItem* child;
    while ((child = m_backupListLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    for (const auto& item : m_backupItems) {
        m_backupListLayout->addWidget(createBackupCard(item));
    }

    if (m_backupItems.isEmpty()) {
        QLabel* emptyLabel = new QLabel(tr("📭 暂无备份记录"));
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("font-size: 13px; color: #94a3b8; padding: 30px;");
        m_backupListLayout->addWidget(emptyLabel);
    }
}

void BackupRestoreWidget::refreshSnapshotList()
{
    QLayoutItem* child;
    while ((child = m_snapshotListLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    for (const auto& snap : m_snapshots) {
        m_snapshotListLayout->addWidget(createSnapshotCard(snap));
    }

    if (m_snapshots.isEmpty()) {
        QLabel* emptyLabel = new QLabel(tr("📸 暂无系统快照，点击上方按钮创建第一个快照"));
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("font-size: 13px; color: #94a3b8; padding: 30px;");
        m_snapshotListLayout->addWidget(emptyLabel);
    }

    if (m_snapshotCountLabel) {
        m_snapshotCountLabel->setText(QString::number(m_snapshots.size()) + tr(" 个"));
    }
}

int BackupRestoreWidget::getSelectedContentCount()
{
    int count = 0;
    if (m_themeCheck && m_themeCheck->isChecked()) count++;
    if (m_iconsCheck && m_iconsCheck->isChecked()) count++;
    if (m_extensionsCheck && m_extensionsCheck->isChecked()) count++;
    if (m_shortcutsCheck && m_shortcutsCheck->isChecked()) count++;
    if (m_panelCheck && m_panelCheck->isChecked()) count++;
    if (m_dataCheck && m_dataCheck->isChecked()) count++;
    return count;
}

void BackupRestoreWidget::onBackupClicked()
{
    if (getSelectedContentCount() == 0) {
        QMessageBox::information(this, tr("提示"), tr("请至少选择一项要备份的内容。"));
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("确认备份"),
        QString(tr("确定要开始备份吗？\n\n已选择 %1 项备份内容。")).arg(getSelectedContentCount()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes
    );

    if (reply != QMessageBox::Yes) return;

    m_backupBtn->setEnabled(false);
    m_backupBtn->setText(tr("备份中..."));
    m_progressBar->setVisible(true);
    m_progressBar->setValue(10);

    // 构建要备份的路径列表
    QStringList paths;
    QString home = QDir::homePath();
    QStringList contents;

    auto addPath = [&](const QString& rel, const QString& label) {
        QString full = home + "/" + rel;
        if (QDir(full).exists() || QFile::exists(full)) {
            paths << rel;
        }
        contents << label;
    };

    if (m_themeCheck->isChecked()) {
        addPath(".themes", tr("主题"));
        addPath(".config/gtk-3.0", tr("主题"));
        addPath(".config/gtk-4.0", tr("主题"));
        addPath(".icons", tr("图标"));
    }
    if (m_iconsCheck->isChecked()) {
        addPath(".local/share/icons", tr("图标"));
    }
    if (m_extensionsCheck->isChecked()) {
        addPath(".local/share/gnome-shell/extensions", tr("扩展"));
    }
    if (m_panelCheck->isChecked()) {
        addPath(".config/dconf", tr("面板布局"));
    }
    if (m_dataCheck->isChecked()) {
        addPath(".config", tr("数据"));
    }

    m_progressBar->setValue(30);

    // 确保备份目录存在
    QString backupDir = home + "/文档/backup";
    QDir().mkpath(backupDir);

    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString archiveName = QString("desktop_backup_%1.tar.gz").arg(timestamp);
    QString archivePath = backupDir + "/" + archiveName;

    // 使用 tar 打包（排除缓存与大文件，忽略读取失败的文件）
    QStringList tarArgs;
    tarArgs << "-czf" << archivePath
            << "--exclude=*.cache"
            << "--exclude=*/cache/*"
            << "--exclude=*.tmp"
            << "--ignore-failed-read"
            << "-C" << home;
    tarArgs << paths;

    QProcess tarProc;
    tarProc.start("tar", tarArgs);
    m_progressBar->setValue(60);

    if (!tarProc.waitForFinished(120000)) {
        tarProc.kill();
        m_backupBtn->setEnabled(true);
        m_backupBtn->setText(tr("✨ 立即备份"));
        m_progressBar->setVisible(false);
        QMessageBox::critical(this, tr("备份失败"), tr("备份超时（超过 120 秒）。\n可能是数据量过大，请减少备份项。"));
        return;
    }

    m_progressBar->setValue(100);

    // tar: 0=成功, 1=部分文件警告（归档仍有效）, 2=致命错误
    int code = tarProc.exitCode();
    bool success = (code == 0 || code == 1);
    if (!success) {
        QString err = QString::fromUtf8(tarProc.readAllStandardError());
        m_backupBtn->setEnabled(true);
        m_backupBtn->setText(tr("✨ 立即备份"));
        m_progressBar->setVisible(false);
        QMessageBox::critical(this, tr("备份失败"),
            tr("备份过程中出错：\n%1").arg(err.left(500)));
        return;
    }

    // 确保归档文件确实生成
    if (!QFile::exists(archivePath)) {
        m_backupBtn->setEnabled(true);
        m_backupBtn->setText(tr("✨ 立即备份"));
        m_progressBar->setVisible(false);
        QMessageBox::critical(this, tr("备份失败"),
            tr("归档文件未生成，请检查磁盘空间和权限。"));
        return;
    }

    // 获取实际文件大小
    QFileInfo fi(archivePath);
    qint64 sizeBytes = fi.size();
    QString sizeStr;
    if (sizeBytes > 1024 * 1024 * 1024)
        sizeStr = QString::number(sizeBytes / 1024.0 / 1024.0 / 1024.0, 'f', 2) + " GB";
    else if (sizeBytes > 1024 * 1024)
        sizeStr = QString::number(sizeBytes / 1024.0 / 1024.0, 'f', 2) + " MB";
    else if (sizeBytes > 1024)
        sizeStr = QString::number(sizeBytes / 1024.0, 'f', 2) + " KB";
    else
        sizeStr = QString::number(sizeBytes) + " B";

    // 快捷键单独用 dconf 导出（追加到 tar 会比较复杂，这里单独保存）
    if (m_shortcutsCheck->isChecked()) {
        QProcess dconf;
        dconf.start("dconf", QStringList() << "dump" << "/org/gnome/settings-daemon/plugins/media-keys/");
        dconf.waitForFinished(5000);
        if (dconf.exitCode() == 0 && !dconf.readAllStandardOutput().isEmpty()) {
            QFile f(backupDir + "/shortcuts_" + timestamp + ".dconf");
            if (f.open(QIODevice::WriteOnly)) {
                f.write(dconf.readAllStandardOutput());
                f.close();
            }
        }
        contents << tr("快捷键");
    }

    BackupItem newItem;
    newItem.id = "backup_new_" + QString::number(QDateTime::currentMSecsSinceEpoch());
    newItem.name = tr("手动备份 - ") + QDateTime::currentDateTime().toString("MM-dd hh:mm");
    newItem.dateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    newItem.size = sizeStr;
    newItem.contents = contents;
    newItem.isFullBackup = (contents.size() >= 6);

    m_backupItems.prepend(newItem);
    refreshBackupList();

    m_backupBtn->setEnabled(true);
    m_backupBtn->setText(tr("✨ 立即备份"));
    m_progressBar->setVisible(false);

    QMessageBox::information(this, tr("备份成功"),
        tr("桌面配置已成功备份！\n\n备份文件：\n%1\n大小：%2").arg(archivePath, sizeStr));
}

void BackupRestoreWidget::onRestoreClicked(const QString& backupId)
{
    BackupItem* item = nullptr;
    for (auto& i : m_backupItems) {
        if (i.id == backupId) {
            item = &i;
            break;
        }
    }
    if (!item) return;

    auto ret = QMessageBox::question(this, tr("确认恢复"),
        QString(tr("恢复备份 \"%1\" 会覆盖当前的桌面配置，确定要继续吗？\n\n建议先备份当前配置。")).arg(item->name),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (ret != QMessageBox::Yes) return;

    // 根据备份的日期时间查找对应的归档文件
    // dateTime 格式: yyyy-MM-dd hh:mm:ss  →  归档时间戳: yyyyMMdd_hhmmss
    QDateTime dt = QDateTime::fromString(item->dateTime, "yyyy-MM-dd hh:mm:ss");
    QString archiveName;
    if (dt.isValid()) {
        archiveName = QString("desktop_backup_%1.tar.gz").arg(dt.toString("yyyyMMdd_hhmmss"));
    }

    QString backupDir = QDir::homePath() + "/文档/backup";
    QString archivePath = backupDir + "/" + archiveName;

    if (!QFile::exists(archivePath)) {
        // 查找该目录下最新的归档作为后备
        QDir dir(backupDir, "desktop_backup_*.tar.gz", QDir::Time, QDir::Files);
        if (dir.entryList().isEmpty()) {
            QMessageBox::critical(this, tr("恢复失败"),
                tr("找不到备份文件：\n%1").arg(archivePath));
            return;
        }
        archivePath = backupDir + "/" + dir.entryList().first();
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QProcess tarProc;
    tarProc.start("tar", QStringList() << "-xzf" << archivePath << "--ignore-failed-read" << "-C" << QDir::homePath());
    tarProc.waitForFinished(120000);
    QApplication::restoreOverrideCursor();

    // tar: 0=成功, 1=部分文件警告, 2=致命错误
    if (tarProc.exitCode() >= 2) {
        QMessageBox::critical(this, tr("恢复失败"),
            tr("解包备份文件时出错：\n%1").arg(QString::fromUtf8(tarProc.readAllStandardError()).left(500)));
        return;
    }

    QMessageBox::information(this, tr("恢复成功"),
        tr("配置已从以下备份恢复：\n%1\n\n建议注销后重新登录以使所有更改生效。").arg(archivePath));
}

void BackupRestoreWidget::onDeleteClicked(const QString& backupId)
{
    BackupItem* item = nullptr;
    for (auto& i : m_backupItems) {
        if (i.id == backupId) {
            item = &i;
            break;
        }
    }
    if (!item) return;

    auto ret = QMessageBox::warning(this, tr("确认删除"),
        QString(tr("确定要删除备份 \"%1\" 吗？\n\n这将同时删除备份文件，删除后无法恢复！")).arg(item->name),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (ret != QMessageBox::Yes) return;

    // 删除对应的归档文件
    QDateTime dt = QDateTime::fromString(item->dateTime, "yyyy-MM-dd hh:mm:ss");
    if (dt.isValid()) {
        QString archiveName = QString("desktop_backup_%1.tar.gz").arg(dt.toString("yyyyMMdd_hhmmss"));
        QString archivePath = QDir::homePath() + "/文档/backup/" + archiveName;
        if (QFile::exists(archivePath)) {
            QFile::remove(archivePath);
        }
        // 同时尝试删除对应的 dconf 快捷键文件
        QString dconfPath = QDir::homePath() + "/文档/backup/shortcuts_" + dt.toString("yyyyMMdd_hhmmss") + ".dconf";
        if (QFile::exists(dconfPath)) {
            QFile::remove(dconfPath);
        }
    }

    for (int i = 0; i < m_backupItems.size(); i++) {
        if (m_backupItems[i].id == backupId) {
            m_backupItems.removeAt(i);
            break;
        }
    }
    refreshBackupList();
}

void BackupRestoreWidget::onDetailClicked(const QString& backupId)
{
    BackupItem* item = nullptr;
    for (auto& i : m_backupItems) {
        if (i.id == backupId) {
            item = &i;
            break;
        }
    }
    if (!item) return;

    QString detailText = QString(
        tr("📦 备份名称：%1\n" "🕐 创建时间：%2\n" "📊 文件大小：%3\n" "📋 备份类型：%4\n\n" "📁 包含内容：\n  • %5\n")
    ).arg(item->name)
     .arg(item->dateTime)
     .arg(item->size)
     .arg(item->isFullBackup ? tr("完整备份") : tr("部分备份"))
     .arg(item->contents.join("\n  • "));

    QMessageBox::information(this, tr("备份详情"), detailText);
}

void BackupRestoreWidget::onCreateSnapshotClicked()
{
    if (!checkTimeshiftInstalled()) {
        QMessageBox::warning(this, tr("未安装"),
            tr("Timeshift 未安装。请先安装：\nsudo apt install timeshift  (Debian/Ubuntu)\nsudo dnf install timeshift  (Fedora)"));
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("创建系统快照"),
        tr("确定要创建 Timeshift 系统快照吗？\n\n系统快照会保存整个系统状态，出问题时可以完全恢复。\n注意：快照需要占用较多磁盘空间。"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply != QMessageBox::Yes) return;

    QString comment = QString(tr("Linux Savior Toolbox - %1"))
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm"));

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QString output = runTimeshiftCmd({"--create", "--comments", comment}, 120000);
    QApplication::restoreOverrideCursor();

    if (output.isEmpty()) {
        QMessageBox::critical(this, tr("创建失败"),
            tr("快照创建失败。可能原因：\n1. 用户取消了授权\n2. Timeshift 未配置存储设备\n\n请运行 timeshift-gtk 进行初始配置。"));
        return;
    }

    refreshSnapshotsFromSystem();
    QMessageBox::information(this, tr("创建成功"),
        tr("系统快照创建成功！\n\n以后系统出问题时，可以通过 Timeshift 恢复到这个状态。"));
}

void BackupRestoreWidget::onViewSnapshotsClicked()
{
    refreshSnapshotsFromSystem();

    if (m_snapshots.isEmpty()) {
        QMessageBox::information(this, tr("快照列表"),
            tr("当前没有系统快照。\n\n点击「创建系统快照」按钮创建第一个快照。"));
        return;
    }

    QString snapText = tr("📸 Timeshift 系统快照列表（共 %1 个）：\n\n").arg(m_snapshots.size());
    for (int i = 0; i < m_snapshots.size(); i++) {
        const auto& s = m_snapshots[i];
        snapText += QString(tr("%1. %2\n   类型: %3\n   大小: %4\n   时间: %5\n\n"))
            .arg(i + 1)
            .arg(s.name)
            .arg(s.type)
            .arg(s.size)
            .arg(s.dateTime);
    }
    QMessageBox::information(this, tr("快照列表"), snapText);
}

void BackupRestoreWidget::onScheduleEnabledChanged(bool enabled)
{
    if (m_scheduleTypeCombo) m_scheduleTypeCombo->setEnabled(enabled);
    if (m_keepSpinBox) m_keepSpinBox->setEnabled(enabled);
    if (m_nextBackupLabel) {
        if (enabled) {
            m_nextBackupLabel->setText(tr("📅 自动备份已启用"));
            m_nextBackupLabel->setStyleSheet("font-size: 11px; color: #3b82f6;");
        } else {
            m_nextBackupLabel->setText(tr("📅 自动备份已禁用"));
            m_nextBackupLabel->setStyleSheet("font-size: 11px; color: #94a3b8;");
        }
    }

    // 通过 cron 控制 timeshift 自动快照
    QString cronPath = "/etc/cron.daily/timeshift-toolbox-check";
    QProcess proc;
    if (enabled) {
        QString script = "#!/bin/sh\ntimeshift --check\n";
        proc.start("pkexec", QStringList() << "tee" << cronPath);
        proc.write(script.toUtf8());
        proc.closeWriteChannel();
        proc.waitForFinished(5000);
        // 设置可执行权限
        QProcess chmod;
        chmod.start("pkexec", QStringList() << "chmod" << "+x" << cronPath);
        chmod.waitForFinished(3000);
    } else {
        proc.start("pkexec", QStringList() << "rm" << "-f" << cronPath);
        proc.waitForFinished(5000);
    }
}

void BackupRestoreWidget::onScheduleTypeChanged(const QString& type)
{
    if (!m_nextBackupLabel) return;
    // Timeshift 的计划频率由 /etc/timeshift/timeshift.json 控制，
    // 此处仅更新提示文案，详细配置建议使用 timeshift-gtk
    QString hint = tr("📅 频率: %1（详细配置请运行 timeshift-gtk）").arg(type);
    m_nextBackupLabel->setText(hint);
}

void BackupRestoreWidget::onSnapshotRestoreClicked(const QString& name)
{
    auto ret = QMessageBox::question(this, tr("恢复系统快照"),
        QString(tr("确定要恢复快照 \"%1\" 吗？\n\n"
                   "恢复系统快照会将系统还原到该快照的状态。\n"
                   "注意：快照之后创建/修改的系统文件将会丢失！\n\n"
                   "建议：先备份重要数据再进行恢复操作。\n"
                   "恢复完成后系统将自动重启。")).arg(name),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (ret != QMessageBox::Yes) return;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QString output = runTimeshiftCmd({"--restore", "--snapshot", name, "--yes"}, 300000);
    QApplication::restoreOverrideCursor();

    if (output.isEmpty()) {
        QMessageBox::critical(this, tr("恢复失败"),
            tr("快照恢复失败或被取消。"));
        return;
    }

    QMessageBox::information(this, tr("恢复完成"),
        tr("系统快照已恢复。\n\n请重启系统以使恢复生效。"));
    refreshSnapshotsFromSystem();
}

void BackupRestoreWidget::onSnapshotDeleteClicked(const QString& name)
{
    auto ret = QMessageBox::warning(this, tr("删除快照"),
        QString(tr("确定要删除快照 \"%1\" 吗？\n\n删除后无法恢复！")).arg(name),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (ret != QMessageBox::Yes) return;

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QString output = runTimeshiftCmd({"--delete", "--snapshot", name, "--yes"}, 60000);
    QApplication::restoreOverrideCursor();

    refreshSnapshotsFromSystem();

    if (output.isEmpty()) {
        QMessageBox::critical(this, tr("删除失败"),
            tr("快照删除失败或被取消。"));
        return;
    }

    QMessageBox::information(this, tr("删除成功"),
        tr("快照「%1」已删除。").arg(name));
}

void BackupRestoreWidget::onBrowseSnapshotClicked(const QString& name)
{
    // Timeshift 的 rsync 快照存储在 /timeshift/snapshots/<name>/
    // 直接在文件管理器中打开该目录即可浏览
    QString snapPath = QStringLiteral("/timeshift/snapshots/") + name;

    // 先尝试用 xdg-open 打开
    QProcess::startDetached(QStringLiteral("xdg-open"), QStringList() << snapPath);

    QMessageBox::information(this, tr("浏览快照"),
        tr("正在打开快照目录：\n%1\n\n"
           "如果目录为空或不存在，请确认快照类型为 rsync（BTRFS 快照需通过 subvolume 访问）。").arg(snapPath));
}

void BackupRestoreWidget::onDejaDupBackupClicked()
{
    if (!checkDejaDupInstalled()) {
        QMessageBox::warning(this, tr("未安装"),
            tr("Deja Dup 未安装。请先安装：\n"
               "sudo apt install deja-dup  (Debian/Ubuntu)\n"
               "sudo dnf install deja-dup  (Fedora)"));
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Deja Dup 备份"),
        tr("确定要立即启动 Deja Dup 备份吗？\n\n"
           "Deja Dup 会备份您的个人文件到预设的存储位置。\n"
           "首次使用请先在 Deja Dup 设置中配置备份位置。"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply != QMessageBox::Yes) return;

    // deja-dup --backup 启动备份（GUI 会显示进度）
    QProcess::startDetached(QStringLiteral("deja-dup"), QStringList() << QStringLiteral("--backup"));
}

void BackupRestoreWidget::onDejaDupOpenClicked()
{
    if (!checkDejaDupInstalled()) {
        QMessageBox::warning(this, tr("未安装"),
            tr("Deja Dup 未安装。请先安装：\n"
               "sudo apt install deja-dup  (Debian/Ubuntu)\n"
               "sudo dnf install deja-dup  (Fedora)"));
        return;
    }
    // 打开 Deja Dup GUI，用户可在此配置、恢复、管理备份
    QProcess::startDetached(QStringLiteral("deja-dup"), QStringList());
}
