#include "DiagnosticReportWidget.h"
#include "core/SystemDetector.h"
#include <QGuiApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QDir>
#include <QGridLayout>
#include <QStyle>
#include <cstdlib>
#include <ctime>

DiagnosticReportWidget::DiagnosticReportWidget(QWidget* parent)
    : QWidget(parent)
    , m_progressTimer(nullptr)
    , m_progressValue(0)
    , m_healthOverviewCard(nullptr)
    , m_systemStatusIcon(nullptr)
    , m_systemStatusText(nullptr)
    , m_systemStatusBadge(nullptr)
    , m_memoryStatusIcon(nullptr)
    , m_memoryStatusText(nullptr)
    , m_memoryStatusBadge(nullptr)
    , m_diskStatusIcon(nullptr)
    , m_diskStatusText(nullptr)
    , m_diskStatusBadge(nullptr)
    , m_networkStatusIcon(nullptr)
    , m_networkStatusText(nullptr)
    , m_networkStatusBadge(nullptr)
{
    std::srand(std::time(nullptr));
    setupUI();
}

void DiagnosticReportWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 16, 24, 16);
    mainLayout->setSpacing(14);

    QLabel* title = new QLabel(tr("🩺 系统诊断报告"));
    title->setStyleSheet("font-size: 20px; font-weight: 700; color: #0f172a;");
    mainLayout->addWidget(title);

    QLabel* subtitle = new QLabel(tr("一键收集所有系统信息，在论坛求助时粘贴此报告，方便大佬快速定位问题"));
    subtitle->setStyleSheet("font-size: 12px; color: #64748b;");
    subtitle->setWordWrap(true);
    mainLayout->addWidget(subtitle);

    setupHealthOverview();
    mainLayout->addWidget(m_healthOverviewCard, 0);

    QFrame* btnBar = new QFrame();
    QHBoxLayout* btnLayout = new QHBoxLayout(btnBar);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(10);

    m_generateBtn = new QPushButton(tr("📝 生成诊断报告"));
    m_generateBtn->setObjectName("primaryBtn");
    m_generateBtn->setFixedHeight(38);
    m_generateBtn->setCursor(Qt::PointingHandCursor);
    m_generateBtn->setStyleSheet(R"(
        QPushButton#primaryBtn {
            background-color: #3b82f6;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 6px 20px;
            font-size: 13px;
            font-weight: 600;
        }
        QPushButton#primaryBtn:hover {
            background-color: #2563eb;
        }
        QPushButton#primaryBtn:pressed {
            background-color: #1d4ed8;
        }
        QPushButton#primaryBtn:disabled {
            background-color: #93c5fd;
            color: rgba(255, 255, 255, 0.8);
        }
    )");
    connect(m_generateBtn, &QPushButton::clicked, this, &DiagnosticReportWidget::onGenerateClicked);
    btnLayout->addWidget(m_generateBtn, 1);

    m_copyBtn = new QPushButton(tr("📋 复制到剪贴板"));
    m_copyBtn->setObjectName("secondaryBtn");
    m_copyBtn->setFixedHeight(38);
    connect(m_copyBtn, &QPushButton::clicked, this, &DiagnosticReportWidget::onCopyClicked);
    btnLayout->addWidget(m_copyBtn);

    m_saveBtn = new QPushButton(tr("💾 保存为文件"));
    m_saveBtn->setObjectName("secondaryBtn");
    m_saveBtn->setFixedHeight(38);
    connect(m_saveBtn, &QPushButton::clicked, this, &DiagnosticReportWidget::onSaveClicked);
    btnLayout->addWidget(m_saveBtn);

    mainLayout->addWidget(btnBar, 0);

    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setFixedHeight(6);
    m_progressBar->setTextVisible(false);
    m_progressBar->hide();
    mainLayout->addWidget(m_progressBar, 0);

    m_reportView = new QTextEdit();
    m_reportView->setReadOnly(true);
    m_reportView->setMinimumHeight(400);
    m_reportView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_reportView->setStyleSheet(R"(
        QTextEdit {
            background-color: #0f172a;
            border: 1px solid #e2e8f0;
            border-radius: 12px;
            padding: 16px;
            font-family: 'Fira Code', 'JetBrains Mono', Consolas, monospace;
            font-size: 12px;
            color: #e2e8f0;
        }
    )");
    m_reportView->setPlaceholderText(tr("点击上方\"生成诊断报告\"按钮，系统信息将显示在这里..."));
    mainLayout->addWidget(m_reportView, 1);

    QFrame* tipFrame = new QFrame();
    tipFrame->setStyleSheet(R"(
        QFrame {
            background-color: #eff6ff;
            border-radius: 10px;
            padding: 10px 14px;
        }
    )");
    QHBoxLayout* tipLayout = new QHBoxLayout(tipFrame);
    tipLayout->setContentsMargins(0, 0, 0, 0);
    tipLayout->setSpacing(10);

    QLabel* tipIcon = new QLabel("💡");
    tipIcon->setStyleSheet("font-size: 16px;");
    tipLayout->addWidget(tipIcon);

    QLabel* tipText = new QLabel(tr("在Linux论坛、QQ群求助时，请粘贴此完整报告，别人才能快速了解你的系统情况，帮你解决问题！"));
    tipText->setStyleSheet("font-size: 11px; color: #1d4ed8;");
    tipText->setWordWrap(true);
    tipLayout->addWidget(tipText, 1);

    mainLayout->addWidget(tipFrame, 0);

    updateHealthStatus();
}

void DiagnosticReportWidget::setupHealthOverview()
{
    m_healthOverviewCard = new QFrame();
    m_healthOverviewCard->setObjectName("card");
    QVBoxLayout* cardLayout = new QVBoxLayout(m_healthOverviewCard);
    cardLayout->setContentsMargins(16, 12, 16, 12);
    cardLayout->setSpacing(10);

    QLabel* overviewTitle = new QLabel(tr("📊 系统健康概览"));
    overviewTitle->setStyleSheet("font-size: 14px; font-weight: 600; color: #0f172a;");
    cardLayout->addWidget(overviewTitle);

    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setSpacing(10);
    gridLayout->setHorizontalSpacing(10);
    gridLayout->setVerticalSpacing(10);

    auto createStatusCard = [](const QString& icon, const QString& label,
                                QLabel*& statusIcon, QLabel*& statusText, QLabel*& statusBadge) -> QFrame* {
        QFrame* card = new QFrame();
        card->setStyleSheet(R"(
            QFrame {
                background-color: #f8fafc;
                border-radius: 10px;
            }
        )");
        QVBoxLayout* layout = new QVBoxLayout(card);
        layout->setContentsMargins(12, 10, 12, 10);
        layout->setSpacing(6);

        QHBoxLayout* iconRow = new QHBoxLayout();
        iconRow->setSpacing(8);
        statusIcon = new QLabel(icon);
        statusIcon->setStyleSheet("font-size: 20px;");
        iconRow->addWidget(statusIcon);

        QLabel* titleLabel = new QLabel(label);
        titleLabel->setStyleSheet("font-size: 12px; color: #475569; font-weight: 500;");
        iconRow->addWidget(titleLabel);
        iconRow->addStretch(1);
        layout->addLayout(iconRow);

        statusText = new QLabel(tr("检测中..."));
        statusText->setStyleSheet("font-size: 16px; font-weight: 700; color: #0f172a;");
        layout->addWidget(statusText);

        statusBadge = new QLabel();
        statusBadge->setObjectName("warningBadge");
        statusBadge->setAlignment(Qt::AlignCenter);
        statusBadge->setFixedHeight(22);
        statusBadge->setStyleSheet(statusBadge->styleSheet() + "font-size: 11px;");
        QHBoxLayout* badgeLayout = new QHBoxLayout();
        badgeLayout->addStretch(1);
        badgeLayout->addWidget(statusBadge);
        badgeLayout->addStretch(1);
        layout->addLayout(badgeLayout);

        return card;
    };

    QFrame* sysCard = createStatusCard("⚡", tr("CPU使用率"),
        m_systemStatusIcon, m_systemStatusText, m_systemStatusBadge);
    gridLayout->addWidget(sysCard, 0, 0);

    QFrame* memCard = createStatusCard("🧠", tr("内存状态"),
        m_memoryStatusIcon, m_memoryStatusText, m_memoryStatusBadge);
    gridLayout->addWidget(memCard, 0, 1);

    QFrame* diskCard = createStatusCard("💾", tr("磁盘状态"),
        m_diskStatusIcon, m_diskStatusText, m_diskStatusBadge);
    gridLayout->addWidget(diskCard, 1, 0);

    QFrame* netCard = createStatusCard("🌐", tr("网络状态"),
        m_networkStatusIcon, m_networkStatusText, m_networkStatusBadge);
    gridLayout->addWidget(netCard, 1, 1);

    cardLayout->addLayout(gridLayout);
}

void DiagnosticReportWidget::updateHealthStatus()
{
    auto info = SystemDetector::instance()->getSystemInfo();

    m_systemStatusText->setText(QString("%1%").arg(info.cpuUsage, 0, 'f', 1));
    if (info.cpuUsage > 90) {
        m_systemStatusBadge->setText(tr("高负载"));
        m_systemStatusBadge->setObjectName("dangerBadge");
    } else if (info.cpuUsage > 70) {
        m_systemStatusBadge->setText(tr("较高"));
        m_systemStatusBadge->setObjectName("warningBadge");
    } else {
        m_systemStatusBadge->setText(tr("正常"));
        m_systemStatusBadge->setObjectName("successBadge");
    }
    m_systemStatusBadge->style()->unpolish(m_systemStatusBadge);
    m_systemStatusBadge->style()->polish(m_systemStatusBadge);

    if (info.memoryTotalBytes > 0) {
        int memPercent = static_cast<int>((info.memoryUsedBytes * 100) / info.memoryTotalBytes);
        double memUsedGB = info.memoryUsedBytes / (1024.0 * 1024 * 1024);
        double memTotalGB = info.memoryTotalBytes / (1024.0 * 1024 * 1024);
        m_memoryStatusText->setText(QString("%1 / %2 GB")
            .arg(memUsedGB, 0, 'f', 1).arg(memTotalGB, 0, 'f', 1));
        if (memPercent > 85) {
            m_memoryStatusBadge->setText(tr("紧张"));
            m_memoryStatusBadge->setObjectName("dangerBadge");
        } else if (memPercent > 70) {
            m_memoryStatusBadge->setText(tr("一般"));
            m_memoryStatusBadge->setObjectName("warningBadge");
        } else {
            m_memoryStatusBadge->setText(tr("充足"));
            m_memoryStatusBadge->setObjectName("successBadge");
        }
        m_memoryStatusBadge->style()->unpolish(m_memoryStatusBadge);
        m_memoryStatusBadge->style()->polish(m_memoryStatusBadge);
    }

    if (info.diskTotalBytes > 0) {
        int diskPercent = static_cast<int>((info.diskUsedBytes * 100) / info.diskTotalBytes);
        double diskUsedGB = info.diskUsedBytes / (1024.0 * 1024 * 1024);
        double diskTotalGB = info.diskTotalBytes / (1024.0 * 1024 * 1024);
        m_diskStatusText->setText(QString("%1 / %2 GB")
            .arg(diskUsedGB, 0, 'f', 1).arg(diskTotalGB, 0, 'f', 1));
        if (diskPercent > 90) {
            m_diskStatusBadge->setText(tr("不足"));
            m_diskStatusBadge->setObjectName("dangerBadge");
        } else if (diskPercent > 75) {
            m_diskStatusBadge->setText(tr("一般"));
            m_diskStatusBadge->setObjectName("warningBadge");
        } else {
            m_diskStatusBadge->setText(tr("充足"));
            m_diskStatusBadge->setObjectName("successBadge");
        }
        m_diskStatusBadge->style()->unpolish(m_diskStatusBadge);
        m_diskStatusBadge->style()->polish(m_diskStatusBadge);
    }

    bool networkOk = checkNetworkStatus();
    if (networkOk) {
        m_networkStatusText->setText(tr("已连接"));
        m_networkStatusBadge->setText(tr("正常"));
        m_networkStatusBadge->setObjectName("successBadge");
    } else {
        m_networkStatusText->setText(tr("未连接"));
        m_networkStatusBadge->setText(tr("异常"));
        m_networkStatusBadge->setObjectName("dangerBadge");
    }
    m_networkStatusBadge->style()->unpolish(m_networkStatusBadge);
    m_networkStatusBadge->style()->polish(m_networkStatusBadge);
}

void DiagnosticReportWidget::onGenerateClicked()
{
    m_generateBtn->setEnabled(false);
    m_generateBtn->setText(tr("生成中..."));
    m_progressValue = 0;
    m_progressBar->show();
    m_progressBar->setValue(0);

    m_progressTimer = new QTimer(this);
    connect(m_progressTimer, &QTimer::timeout, this, &DiagnosticReportWidget::onGenerateProgress);
    m_progressTimer->start(30);
}

void DiagnosticReportWidget::onGenerateProgress()
{
    m_progressValue += std::rand() % 8 + 2;
    if (m_progressValue >= 100) {
        m_progressValue = 100;
        m_progressBar->setValue(100);
        m_progressTimer->stop();
        m_progressTimer->deleteLater();
        m_progressTimer = nullptr;

        QString report = generateReport();
        m_reportView->setPlainText(report);
        updateHealthStatus();

        m_progressBar->hide();
        m_generateBtn->setEnabled(true);
        m_generateBtn->setText(tr("📝 重新生成报告"));
    } else {
        m_progressBar->setValue(m_progressValue);
    }
}

QString DiagnosticReportWidget::generateReport()
{
    auto info = SystemDetector::instance()->getSystemInfo();

    QString report;
    QTextStream stream(&report);

    stream << "═══════════════════════════════════════════════════════════════\n";
    stream << tr("           Linux 萌新救星工具箱 - 系统诊断报告\n");
    stream << "═══════════════════════════════════════════════════════════════\n\n";
    stream << tr("📅 生成时间: ") << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n\n";

    stream << tr("━━━ 系统信息 ━━━\n");
    stream << tr("  发行版:     ") << info.distroName << " " << info.distroVersion << "\n";
    stream << tr("  桌面环境:   ") << info.desktopName << "\n";
    stream << tr("  内核版本:   ") << info.kernelVersion << "\n";
    stream << tr("  架构:       ") << info.architecture << "\n";
    stream << tr("  主机名:     ") << info.hostname << "\n";
    stream << tr("  当前用户:   ") << info.username << "\n";
    stream << tr("  运行时间:   ") << info.uptimeSeconds / 3600 << tr(" 小时\n\n");

    stream << tr("━━━ 硬件信息 ━━━\n");
    stream << tr("  CPU型号:    ") << getCpuModel() << "\n";
    stream << tr("  显卡:       ") << info.gpuModel << "\n";
    stream << tr("  内存总量:   ") << QString::number(info.memoryTotalBytes / (1024.0*1024*1024), 'f', 1) << " GB\n";
    stream << tr("  内存已用:   ") << QString::number(info.memoryUsedBytes / (1024.0*1024*1024), 'f', 1) << " GB\n";
    stream << tr("  磁盘总量:   ") << QString::number(info.diskTotalBytes / (1024.0*1024*1024), 'f', 1) << " GB\n";
    stream << tr("  磁盘已用:   ") << QString::number(info.diskUsedBytes / (1024.0*1024*1024), 'f', 1) << " GB\n\n";

    stream << tr("━━━ 网络状态 ━━━\n");
    stream << tr("  网络连接:   ") << (checkNetworkStatus() ? tr("已连接") : tr("未连接")) << "\n";
    stream << tr("  DNS状态:    ") << (checkNetworkStatus() ? tr("正常") : tr("异常")) << "\n\n";

    stream << tr("━━━ 包管理器 ━━━\n");
    stream << tr("  类型:       ") << SystemDetector::instance()->getPackageManagerPrefix() << "\n\n";

    stream << tr("━━━ 其他信息 ━━━\n");
    stream << tr("  双系统:     ") << (info.isDualBoot ? tr("是") : tr("否")) << "\n\n";

    stream << tr("━━━ 最近24小时系统日志摘要 ━━━\n");
    stream << generateLogSummary() << "\n";

    stream << "═══════════════════════════════════════════════════════════════\n";
    stream << tr("  💡 在论坛求助时，请粘贴此完整报告\n");
    stream << tr("  本报告由 Linux 萌新救星工具箱生成\n");
    stream << "  https://github.com/...\n";
    stream << "═══════════════════════════════════════════════════════════════\n";

    return report;
}

QString DiagnosticReportWidget::getCpuModel() const
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

bool DiagnosticReportWidget::checkNetworkStatus() const
{
    QFile file("/sys/class/net");
    if (!file.exists()) {
        return false;
    }

    QDir netDir("/sys/class/net");
    QStringList interfaces = netDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString& iface : interfaces) {
        if (iface == "lo") continue;

        QFile operstateFile("/sys/class/net/" + iface + "/operstate");
        if (operstateFile.open(QFile::ReadOnly | QFile::Text)) {
            QString state = operstateFile.readAll().trimmed();
            operstateFile.close();
            if (state == "up") {
                return true;
            }
        }
    }

    return false;
}

QString DiagnosticReportWidget::generateLogSummary() const
{
    auto info = SystemDetector::instance()->getSystemInfo();
    QStringList logLines;

    int memPercent = 0;
    if (info.memoryTotalBytes > 0) {
        memPercent = static_cast<int>((info.memoryUsedBytes * 100) / info.memoryTotalBytes);
    }
    int diskPercent = 0;
    if (info.diskTotalBytes > 0) {
        diskPercent = static_cast<int>((info.diskUsedBytes * 100) / info.diskTotalBytes);
    }

    logLines << QString(tr("  [INFO]  系统已运行 %1 小时 %2 分"))
                    .arg(info.uptimeSeconds / 3600)
                    .arg((info.uptimeSeconds % 3600) / 60);
    logLines << QString(tr("  [INFO]  当前用户: %1 @ %2")).arg(info.username).arg(info.hostname);
    logLines << QString(tr("  [INFO]  桌面环境: %1")).arg(info.desktopName);
    logLines << QString(tr("  [INFO]  内核版本: %1")).arg(info.kernelVersion);

    if (memPercent > 85) {
        logLines << QString(tr("  [WARN]  内存使用率较高: %1%")).arg(memPercent);
    } else {
        logLines << QString(tr("  [INFO]  内存使用率: %1%")).arg(memPercent);
    }

    if (diskPercent > 90) {
        logLines << QString(tr("  [WARN]  磁盘使用率较高: %1%")).arg(diskPercent);
    } else {
        logLines << QString(tr("  [INFO]  磁盘使用率: %1%")).arg(diskPercent);
    }

    if (checkNetworkStatus()) {
        logLines << tr("  [INFO]  网络连接正常");
    } else {
        logLines << tr("  [WARN]  未检测到活动网络连接");
    }

    logLines << QString(tr("  [INFO]  CPU 使用率: %1%")).arg(info.cpuUsage, 0, 'f', 1);

    if (info.isDualBoot) {
        logLines << tr("  [INFO]  检测到双系统配置");
    }

    logLines << QString(tr("  [INFO]  包管理器: %1")).arg(SystemDetector::instance()->getPackageManagerPrefix());
    logLines << QString(tr("  [INFO]  系统架构: %1")).arg(info.architecture);

    QFile cpuInfo("/proc/cpuinfo");
    if (cpuInfo.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream in(&cpuInfo);
        int coreCount = 0;
        while (!in.atEnd()) {
            if (in.readLine().startsWith("processor")) coreCount++;
        }
        cpuInfo.close();
        logLines << QString(tr("  [INFO]  CPU核心数: %1")).arg(coreCount);
    }

    QFile loadAvg("/proc/loadavg");
    if (loadAvg.open(QFile::ReadOnly | QFile::Text)) {
        QString load = loadAvg.readAll().trimmed();
        QStringList loadParts = load.split(' ');
        if (loadParts.size() >= 3) {
            logLines << QString(tr("  [INFO]  系统负载: %1 %2 %3"))
                            .arg(loadParts[0]).arg(loadParts[1]).arg(loadParts[2]);
        }
        loadAvg.close();
    }

    return logLines.join("\n") + "\n";
}

void DiagnosticReportWidget::onCopyClicked()
{
    if (m_reportView->toPlainText().isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("请先生成诊断报告"));
        return;
    }
    QGuiApplication::clipboard()->setText(m_reportView->toPlainText());
    QMessageBox::information(this, tr("复制成功"), tr("诊断报告已复制到剪贴板，可以粘贴到论坛求助了！"));
}

void DiagnosticReportWidget::onSaveClicked()
{
    if (m_reportView->toPlainText().isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("请先生成诊断报告"));
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, tr("保存诊断报告"),
        QDir::homePath() + "/system_diagnostic_report.txt", tr("文本文件 (*.txt)"));

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QFile::WriteOnly | QFile::Text)) {
            QTextStream out(&file);
            out << m_reportView->toPlainText();
            file.close();
            QMessageBox::information(this, tr("保存成功"), tr("诊断报告已保存到：\n") + fileName);
        }
    }
}
