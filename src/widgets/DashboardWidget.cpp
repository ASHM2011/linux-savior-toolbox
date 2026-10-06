#include "DashboardWidget.h"
#include "core/SystemDetector.h"
#include "core/CommandExecutor.h"
#include "core/CommandMetadata.h"
#include "widgets/CommandDetailDialog.h"
#include <QToolTip>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QRegularExpression>
#include <QStyle>
#include <QScrollArea>

DashboardWidget::DashboardWidget(QWidget* parent)
    : QWidget(parent)
    , m_prevCpuTotal(0)
    , m_prevCpuIdle(0)
    , m_firstCpuRead(true)
{
    setupUI();
    setupQuickActions();
    setupRecommendation();
    setupSystemInfoCard();
    updateStats();

    m_statsTimer = new QTimer(this);
    m_statsTimer->setInterval(2000);
    connect(m_statsTimer, &QTimer::timeout, this, &DashboardWidget::updateStats);
    m_statsTimer->start();
}

void DashboardWidget::setupUI()
{
    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget* contentWidget = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setContentsMargins(32, 24, 32, 24);
    mainLayout->setSpacing(24);

    QFrame* welcomeCard = new QFrame();
    welcomeCard->setObjectName("card");
    QHBoxLayout* welcomeLayout = new QHBoxLayout(welcomeCard);
    welcomeLayout->setContentsMargins(28, 24, 28, 24);
    welcomeLayout->setSpacing(20);

    QVBoxLayout* welcomeText = new QVBoxLayout();
    welcomeText->setSpacing(6);

    m_welcomeLabel = new QLabel();
    m_welcomeLabel->setStyleSheet("font-size: 20px; font-weight: 600; color: #0f172a;");
    m_welcomeLabel->setMinimumHeight(28);
    welcomeText->addWidget(m_welcomeLabel);

    m_systemInfoLabel = new QLabel();
    m_systemInfoLabel->setStyleSheet("font-size: 13px; color: #64748b;");
    m_systemInfoLabel->setMinimumHeight(20);
    welcomeText->addWidget(m_systemInfoLabel);

    m_uptimeLabel = new QLabel();
    m_uptimeLabel->setStyleSheet("font-size: 12px; color: #94a3b8;");
    m_uptimeLabel->setMinimumHeight(18);
    welcomeText->addWidget(m_uptimeLabel);

    welcomeLayout->addLayout(welcomeText, 1);

    QPushButton* oneClickBtn = new QPushButton(tr("✨ 一键优化"));
    oneClickBtn->setMinimumHeight(44);
    oneClickBtn->setCursor(Qt::PointingHandCursor);
    oneClickBtn->setStyleSheet("padding: 0 20px;");
    connect(oneClickBtn, &QPushButton::clicked, [this]() {
        emit commandTriggered(SystemDetector::instance()->getUpdateCommandId());
    });
    welcomeLayout->addWidget(oneClickBtn);

    mainLayout->addWidget(welcomeCard);

    QHBoxLayout* statsLayout = new QHBoxLayout();
    statsLayout->setSpacing(16);

    auto createStatCard = [](const QString& icon, const QString& label) -> QPair<QFrame*, QPair<QLabel*, QProgressBar*>> {
        QFrame* card = new QFrame();
        card->setObjectName("statCard");
        card->setMinimumHeight(120);
        QVBoxLayout* layout = new QVBoxLayout(card);
        layout->setContentsMargins(20, 18, 20, 18);
        layout->setSpacing(10);

        QLabel* iconLabel = new QLabel(icon + "  " + label);
        iconLabel->setStyleSheet("font-size: 13px; color: #64748b; font-weight: 500;");
        iconLabel->setMinimumHeight(20);
        layout->addWidget(iconLabel);

        QLabel* valueLabel = new QLabel("--");
        valueLabel->setStyleSheet("font-size: 28px; font-weight: 700; color: #0f172a;");
        valueLabel->setMinimumHeight(36);
        layout->addWidget(valueLabel);

        QProgressBar* bar = new QProgressBar();
        bar->setRange(0, 100);
        bar->setValue(0);
        bar->setTextVisible(false);
        bar->setFixedHeight(8);
        layout->addWidget(bar);

        return {card, {valueLabel, bar}};
    };

    auto cpuResult = createStatCard("💻", tr("CPU使用率"));
    statsLayout->addWidget(cpuResult.first, 1);
    m_cpuValue = cpuResult.second.first;
    m_cpuBar = cpuResult.second.second;

    auto memResult = createStatCard("🧠", tr("内存使用"));
    statsLayout->addWidget(memResult.first, 1);
    m_memValue = memResult.second.first;
    m_memBar = memResult.second.second;

    auto diskResult = createStatCard("💾", tr("磁盘空间"));
    statsLayout->addWidget(diskResult.first, 1);
    m_diskValue = diskResult.second.first;
    m_diskBar = diskResult.second.second;

    mainLayout->addLayout(statsLayout);

    QLabel* recTitle = new QLabel(tr("📌 今日推荐"));
    recTitle->setStyleSheet("font-size: 16px; font-weight: 600; color: #0f172a;");
    recTitle->setMinimumHeight(24);
    mainLayout->addWidget(recTitle);

    m_recommendationCard = new QFrame();
    m_recommendationCard->setObjectName("card");
    m_recommendationCard->setMinimumHeight(80);
    mainLayout->addWidget(m_recommendationCard);

    QLabel* infoTitle = new QLabel(tr("🖥️ 系统信息"));
    infoTitle->setStyleSheet("font-size: 16px; font-weight: 600; color: #0f172a;");
    infoTitle->setMinimumHeight(24);
    mainLayout->addWidget(infoTitle);

    m_sysInfoCard = new QFrame();
    m_sysInfoCard->setObjectName("card");
    m_sysInfoCard->setMinimumHeight(120);
    mainLayout->addWidget(m_sysInfoCard);

    QLabel* actionsTitle = new QLabel(tr("⚡ 快捷操作"));
    actionsTitle->setStyleSheet("font-size: 16px; font-weight: 600; color: #0f172a;");
    actionsTitle->setMinimumHeight(24);
    mainLayout->addWidget(actionsTitle);

    QFrame* actionsCard = new QFrame();
    actionsCard->setObjectName("card");
    actionsCard->setMinimumHeight(120);
    m_actionsLayout = new QGridLayout(actionsCard);
    m_actionsLayout->setContentsMargins(24, 24, 24, 24);
    m_actionsLayout->setSpacing(16);

    mainLayout->addWidget(actionsCard);

    mainLayout->addStretch(1);

    scrollArea->setWidget(contentWidget);
    outerLayout->addWidget(scrollArea);
}

void DashboardWidget::setupQuickActions()
{
    auto sysDetector = SystemDetector::instance();
    auto cmdMgr = CommandMetadataManager::instance();

    struct ActionItem {
        QString icon;
        QString name;
        QString commandId;
        QString tooltip;
    };

    QList<ActionItem> actions;

    actions.append({"🔄", tr("一键更新系统"), sysDetector->getUpdateCommandId(), tr("更新系统所有软件到最新版本")});
    actions.append({"🧹", tr("清理系统缓存"), sysDetector->getCleanCacheCommandId(), tr("清理软件包缓存，释放磁盘空间")});
    actions.append({"🖥️", tr("重启桌面环境"),
        sysDetector->isGNOME() ? "gnome_shell_restart" :
        sysDetector->isKDE() ? "plasma_restart" : "xfce4_restart",
        tr("解决桌面卡顿、无响应等问题")});
    actions.append({"🗑️", tr("清空回收站"), "rm_trash", tr("永久删除回收站中的所有文件")});

    int col = 0;
    int row = 0;
    for (const auto& action : actions) {
        QPushButton* btn = new QPushButton();
        btn->setMinimumHeight(90);
        btn->setStyleSheet(R"(
            QPushButton {
                background-color: #f8fafc;
                border: 1px solid #e2e8f0;
                border-radius: 12px;
                padding: 16px 12px;
                text-align: center;
            }
            QPushButton:hover {
                background-color: #f1f5f9;
                border-color: #cbd5e1;
            }
            QPushButton:pressed {
                background-color: #e2e8f0;
            }
        )");
        btn->setCursor(Qt::PointingHandCursor);
        btn->setToolTip(action.tooltip);

        QVBoxLayout* btnLayout = new QVBoxLayout(btn);
        btnLayout->setContentsMargins(0, 0, 0, 0);
        btnLayout->setSpacing(10);
        btnLayout->setAlignment(Qt::AlignCenter);

        QHBoxLayout* iconRow = new QHBoxLayout();
        iconRow->setAlignment(Qt::AlignCenter);
        iconRow->setSpacing(4);

        auto cmd = cmdMgr->getCommand(action.commandId);
        if (cmd.needsAdmin) {
            QLabel* shieldLabel = new QLabel("🛡️");
            shieldLabel->setStyleSheet("font-size: 14px; background: transparent;");
            shieldLabel->setAlignment(Qt::AlignCenter);
            shieldLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
            shieldLabel->setToolTip(tr("需要管理员权限"));
            iconRow->addWidget(shieldLabel);
        }

        QLabel* iconLabel = new QLabel(action.icon);
        iconLabel->setStyleSheet("font-size: 32px; background: transparent;");
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        iconLabel->setMinimumHeight(36);
        iconRow->addWidget(iconLabel);

        btnLayout->addLayout(iconRow);

        QLabel* nameLabel = new QLabel(action.name);
        nameLabel->setStyleSheet("font-size: 13px; font-weight: 500; color: #334155; background: transparent;");
        nameLabel->setAlignment(Qt::AlignCenter);
        nameLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        nameLabel->setWordWrap(true);
        nameLabel->setMinimumHeight(20);
        btnLayout->addWidget(nameLabel);

        QString cmdId = action.commandId;
        connect(btn, &QPushButton::clicked, this, [this, cmdId]() {
            emit commandTriggered(cmdId);
        });

        m_actionsLayout->addWidget(btn, row, col);

        col++;
        if (col >= 4) {
            col = 0;
            row++;
        }
    }
}

void DashboardWidget::setupRecommendation()
{
    QHBoxLayout* recLayout = new QHBoxLayout(m_recommendationCard);
    recLayout->setContentsMargins(24, 20, 24, 20);
    recLayout->setSpacing(20);

    m_recommendationIcon = new QLabel("💡");
    m_recommendationIcon->setStyleSheet("font-size: 36px;");
    m_recommendationIcon->setFixedWidth(56);
    m_recommendationIcon->setAlignment(Qt::AlignCenter);
    recLayout->addWidget(m_recommendationIcon);

    QVBoxLayout* recText = new QVBoxLayout();
    recText->setSpacing(6);

    m_recommendationTitle = new QLabel();
    m_recommendationTitle->setStyleSheet("font-size: 16px; font-weight: 600; color: #0f172a;");
    m_recommendationTitle->setMinimumHeight(24);
    recText->addWidget(m_recommendationTitle);

    m_recommendationDesc = new QLabel();
    m_recommendationDesc->setStyleSheet("font-size: 13px; color: #64748b;");
    m_recommendationDesc->setWordWrap(true);
    m_recommendationDesc->setMinimumHeight(20);
    recText->addWidget(m_recommendationDesc);

    recLayout->addLayout(recText, 1);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);

    m_recommendationInfoBtn = new QPushButton("❓");
    m_recommendationInfoBtn->setFixedSize(44, 44);
    m_recommendationInfoBtn->setObjectName("secondaryBtn");
    m_recommendationInfoBtn->setCursor(Qt::PointingHandCursor);
    m_recommendationInfoBtn->setToolTip(tr("查看详细说明"));
    connect(m_recommendationInfoBtn, &QPushButton::clicked, this, &DashboardWidget::onRecommendationInfoClicked);
    btnLayout->addWidget(m_recommendationInfoBtn);

    m_recommendationBtn = new QPushButton(tr("立即执行"));
    m_recommendationBtn->setMinimumHeight(44);
    m_recommendationBtn->setCursor(Qt::PointingHandCursor);
    m_recommendationBtn->setStyleSheet("padding: 0 20px;");
    connect(m_recommendationBtn, &QPushButton::clicked, this, &DashboardWidget::onRecommendationClicked);
    btnLayout->addWidget(m_recommendationBtn);

    recLayout->addLayout(btnLayout);

    updateRecommendation();
}

void DashboardWidget::setupSystemInfoCard()
{
    QGridLayout* infoLayout = new QGridLayout(m_sysInfoCard);
    infoLayout->setContentsMargins(24, 20, 24, 20);
    infoLayout->setHorizontalSpacing(32);
    infoLayout->setVerticalSpacing(14);

    auto createInfoItem = [](const QString& label, const QString& value) -> QPair<QLabel*, QLabel*> {
        QLabel* labelWidget = new QLabel(label);
        labelWidget->setStyleSheet("font-size: 12px; color: #94a3b8;");
        labelWidget->setMinimumHeight(18);
        QLabel* valueWidget = new QLabel(value);
        valueWidget->setStyleSheet("font-size: 13px; color: #334155; font-weight: 500;");
        valueWidget->setMinimumHeight(20);
        valueWidget->setWordWrap(true);
        return {labelWidget, valueWidget};
    };

    auto cpuItem = createInfoItem(tr("CPU 型号"), "--");
    infoLayout->addWidget(cpuItem.first, 0, 0);
    infoLayout->addWidget(cpuItem.second, 0, 1);
    m_cpuModelLabel = cpuItem.second;

    auto gpuItem = createInfoItem(tr("GPU 型号"), "--");
    infoLayout->addWidget(gpuItem.first, 0, 2);
    infoLayout->addWidget(gpuItem.second, 0, 3);
    m_gpuModelLabel = gpuItem.second;

    auto kernelItem = createInfoItem(tr("内核版本"), "--");
    infoLayout->addWidget(kernelItem.first, 1, 0);
    infoLayout->addWidget(kernelItem.second, 1, 1);
    m_kernelLabel = kernelItem.second;

    auto archItem = createInfoItem(tr("系统架构"), "--");
    infoLayout->addWidget(archItem.first, 1, 2);
    infoLayout->addWidget(archItem.second, 1, 3);
    m_archLabel = archItem.second;

    auto hostItem = createInfoItem(tr("主机名"), "--");
    infoLayout->addWidget(hostItem.first, 2, 0);
    infoLayout->addWidget(hostItem.second, 2, 1);
    m_hostnameLabel = hostItem.second;

    updateSystemInfo();
}

void DashboardWidget::updateStats()
{
    auto info = SystemDetector::instance()->getSystemInfo();

    m_welcomeLabel->setText(QString(tr("👋 欢迎回来！系统运行正常")));

    QString sysInfo = QString(tr("%1 %2 · %3 · 内核 %4"))
        .arg(info.distroName, info.distroVersion, info.desktopName, info.kernelVersion.left(12));
    m_systemInfoLabel->setText(sysInfo);

    m_uptimeLabel->setText(tr("⏱️ 已运行 ") + formatUptime(info.uptimeSeconds));

    double cpuPercent = readCpuUsage();
    int cpuInt = qBound(0, static_cast<int>(cpuPercent), 100);
    m_cpuValue->setText(QString("%1%").arg(cpuInt));
    m_cpuBar->setValue(cpuInt);
    if (cpuInt > 80) m_cpuBar->setObjectName("dangerBar");
    else if (cpuInt > 60) m_cpuBar->setObjectName("warningBar");
    else m_cpuBar->setObjectName("");
    m_cpuBar->style()->unpolish(m_cpuBar);
    m_cpuBar->style()->polish(m_cpuBar);

    if (info.memoryTotalBytes > 0) {
        int memPercent = static_cast<int>((info.memoryUsedBytes * 100) / info.memoryTotalBytes);
        QString memText = QString("%1 / %2 GB")
            .arg(info.memoryUsedBytes / (1024.0*1024*1024), 1, 'f', 1)
            .arg(info.memoryTotalBytes / (1024.0*1024*1024), 1, 'f', 1);
        m_memValue->setText(memText);
        m_memBar->setValue(memPercent);
        if (memPercent > 85) m_memBar->setObjectName("dangerBar");
        else if (memPercent > 70) m_memBar->setObjectName("warningBar");
        else m_memBar->setObjectName("");
        m_memBar->style()->unpolish(m_memBar);
        m_memBar->style()->polish(m_memBar);
    }

    if (info.diskTotalBytes > 0) {
        int diskPercent = static_cast<int>((info.diskUsedBytes * 100) / info.diskTotalBytes);
        QString diskText = QString("%1 / %2 GB")
            .arg(info.diskUsedBytes / (1024.0*1024*1024), 1, 'f', 1)
            .arg(info.diskTotalBytes / (1024.0*1024*1024), 1, 'f', 1);
        m_diskValue->setText(diskText);
        m_diskBar->setValue(diskPercent);
        if (diskPercent > 90) m_diskBar->setObjectName("dangerBar");
        else if (diskPercent > 75) m_diskBar->setObjectName("warningBar");
        else m_diskBar->setObjectName("");
        m_diskBar->style()->unpolish(m_diskBar);
        m_diskBar->style()->polish(m_diskBar);
    }
}

void DashboardWidget::updateRecommendation()
{
    auto info = SystemDetector::instance()->getSystemInfo();
    auto sysDet = SystemDetector::instance();

    QString icon, title, desc, commandId;
    int diskPercent = 0;
    if (info.diskTotalBytes > 0) {
        diskPercent = static_cast<int>((info.diskUsedBytes * 100) / info.diskTotalBytes);
    }

    if (diskPercent > 80) {
        icon = "🧹";
        title = tr("磁盘空间不足，建议清理系统缓存");
        desc = QString(tr("当前磁盘已使用 %1%，清理软件包缓存和旧日志可以释放大量空间。")).arg(diskPercent);
        commandId = sysDet->getCleanCacheCommandId();
    } else if (info.uptimeSeconds > 7 * 86400) {
        icon = "🔄";
        title = tr("系统7天未更新，建议检查更新");
        desc = tr("长时间未更新系统可能存在安全隐患，建议定期更新系统软件。");
        commandId = sysDet->getUpdateCommandId();
    } else if (info.uptimeSeconds > 24 * 3600) {
        icon = "🖥️";
        title = tr("桌面已运行超过24小时，建议重启桌面");
        desc = tr("桌面长时间运行可能会积累内存泄漏，重启桌面可以提升流畅度。");
        commandId = sysDet->isGNOME() ? "gnome_shell_restart" :
                    sysDet->isKDE() ? "plasma_restart" : "xfce4_restart";
    } else {
        icon = "✅";
        title = tr("系统状态良好");
        desc = tr("你的系统运行状态良好，暂无特别需要处理的事项。");
        commandId = sysDet->getUpdateCommandId();
        m_recommendationBtn->setText(tr("检查更新"));
    }

    m_recommendationIcon->setText(icon);
    m_recommendationTitle->setText(title);
    m_recommendationDesc->setText(desc);
    m_recommendationCommandId = commandId;
}

void DashboardWidget::updateSystemInfo()
{
    auto info = SystemDetector::instance()->getSystemInfo();

    m_cpuModelLabel->setText(getCpuModel());
    m_gpuModelLabel->setText(info.gpuModel);
    m_kernelLabel->setText(info.kernelVersion);
    m_archLabel->setText(info.architecture);
    m_hostnameLabel->setText(info.hostname);
}

double DashboardWidget::readCpuUsage()
{
    QFile file("/proc/stat");
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        return 0.0;
    }

    QTextStream in(&file);
    QString line = in.readLine();
    file.close();

    QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (parts.size() < 8 || parts.first() != "cpu") {
        return 0.0;
    }

    double user = parts[1].toDouble();
    double nice = parts[2].toDouble();
    double system = parts[3].toDouble();
    double idle = parts[4].toDouble();
    double iowait = parts[5].toDouble();
    double irq = parts[6].toDouble();
    double softirq = parts[7].toDouble();

    double total = user + nice + system + idle + iowait + irq + softirq;
    double totalIdle = idle + iowait;

    if (m_firstCpuRead) {
        m_firstCpuRead = false;
        m_prevCpuTotal = total;
        m_prevCpuIdle = totalIdle;
        return 0.0;
    }

    double totalDiff = total - m_prevCpuTotal;
    double idleDiff = totalIdle - m_prevCpuIdle;

    m_prevCpuTotal = total;
    m_prevCpuIdle = totalIdle;

    if (totalDiff <= 0) {
        return 0.0;
    }

    double usage = 100.0 * (1.0 - idleDiff / totalDiff);
    return qBound(0.0, usage, 100.0);
}

QString DashboardWidget::formatBytes(qint64 bytes) const
{
    if (bytes > 1024LL * 1024 * 1024) {
        return QString("%1 GB").arg(bytes / (1024.0*1024*1024), 1, 'f', 1);
    } else if (bytes > 1024LL * 1024) {
        return QString("%1 MB").arg(bytes / (1024.0*1024), 1, 'f', 1);
    } else {
        return QString("%1 KB").arg(bytes / 1024.0, 1, 'f', 1);
    }
}

QString DashboardWidget::formatUptime(long seconds) const
{
    long days = seconds / 86400;
    long hours = (seconds % 86400) / 3600;
    long mins = (seconds % 3600) / 60;

    if (days > 0) {
        return QString(tr("%1天%2小时")).arg(days).arg(hours);
    } else if (hours > 0) {
        return QString(tr("%1小时%2分钟")).arg(hours).arg(mins);
    } else {
        return QString(tr("%1分钟")).arg(mins);
    }
}

QString DashboardWidget::getCpuModel() const
{
    QFile file("/proc/cpuinfo");
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        return tr("未知CPU");
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.startsWith("model name")) {
            int colonPos = line.indexOf(':');
            if (colonPos > 0) {
                QString model = line.mid(colonPos + 1).trimmed();
                file.close();
                return model;
            }
        }
    }

    file.close();
    return tr("未知CPU");
}

void DashboardWidget::onQuickActionClicked()
{
}

void DashboardWidget::onInfoClicked()
{
}

void DashboardWidget::onRecommendationClicked()
{
    if (!m_recommendationCommandId.isEmpty()) {
        emit commandTriggered(m_recommendationCommandId);
    }
}

void DashboardWidget::onRecommendationInfoClicked()
{
    if (!m_recommendationCommandId.isEmpty()) {
        emit showCommandDetail(m_recommendationCommandId);
    }
}
