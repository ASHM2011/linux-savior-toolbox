#include "SystemInfoWidget.h"
#include "core/SystemDetector.h"
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QStorageInfo>
#include <QRegularExpression>
#include <QApplication>
#include <QDateTime>
#include <QSet>
#include <QFileInfo>

SystemInfoWidget::SystemInfoWidget(QWidget* parent)
    : QWidget(parent)
    , m_cardsLayout(nullptr)
    , m_statusLabel(nullptr)
    , m_refreshBtn(nullptr)
{
    setupUI();
    QTimer::singleShot(100, this, &SystemInfoWidget::refreshInfo);
}

void SystemInfoWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(32, 24, 32, 24);
    mainLayout->setSpacing(16);

    QFrame* headerCard = new QFrame();
    headerCard->setObjectName("card");
    QHBoxLayout* headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(24, 20, 24, 20);
    headerLayout->setSpacing(16);

    QVBoxLayout* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(4);

    QLabel* titleLabel = new QLabel(tr("🖥️ 系统信息"));
    titleLabel->setObjectName("cardTitle");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: 600;");
    titleLayout->addWidget(titleLabel);

    QLabel* descLabel = new QLabel(tr("查看详细的硬件与系统信息，包括 CPU、GPU、内存、磁盘和网络"));
    descLabel->setStyleSheet("font-size: 13px; color: #64748b;");
    titleLayout->addWidget(descLabel);

    headerLayout->addLayout(titleLayout, 1);

    m_refreshBtn = new QPushButton(tr("🔄 刷新"));
    m_refreshBtn->setObjectName("secondaryBtn");
    m_refreshBtn->setMinimumHeight(36);
    m_refreshBtn->setMinimumWidth(100);
    m_refreshBtn->setCursor(Qt::PointingHandCursor);
    connect(m_refreshBtn, &QPushButton::clicked, this, &SystemInfoWidget::refreshInfo);
    headerLayout->addWidget(m_refreshBtn, 0, Qt::AlignVCenter);

    mainLayout->addWidget(headerCard);

    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setObjectName("contentArea");
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget* scrollContent = new QWidget();
    m_cardsLayout = new QVBoxLayout(scrollContent);
    m_cardsLayout->setContentsMargins(0, 0, 0, 0);
    m_cardsLayout->setSpacing(16);

    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);

    QHBoxLayout* statusRow = new QHBoxLayout();
    m_statusLabel = new QLabel(tr("就绪"));
    m_statusLabel->setStyleSheet("font-size: 12px; color: #64748b;");
    statusRow->addWidget(m_statusLabel);
    statusRow->addStretch(1);
    mainLayout->addLayout(statusRow);
}

QFrame* SystemInfoWidget::createInfoCard(const QString& icon, const QString& title)
{
    QFrame* card = new QFrame();
    card->setObjectName("card");
    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(24, 20, 24, 20);
    cardLayout->setSpacing(12);

    QLabel* titleLabel = new QLabel(icon + "  " + title);
    titleLabel->setObjectName("sectionTitle");
    cardLayout->addWidget(titleLabel);

    return card;
}

void SystemInfoWidget::addInfoRow(QGridLayout* grid, int row, const QString& key, const QString& value)
{
    QLabel* keyLabel = new QLabel(key);
    keyLabel->setStyleSheet("font-size: 13px; color: #64748b; font-weight: 500;");
    keyLabel->setMinimumWidth(140);
    keyLabel->setMaximumWidth(180);

    QLabel* valueLabel = new QLabel(value.isEmpty() ? tr("未知") : value);
    valueLabel->setStyleSheet("font-size: 13px; font-weight: 500;");
    valueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    valueLabel->setWordWrap(true);

    grid->addWidget(keyLabel, row, 0, Qt::AlignTop);
    grid->addWidget(valueLabel, row, 1, Qt::AlignTop);
}

QString SystemInfoWidget::runCommand(const QString& program, const QStringList& arguments, int timeoutMs)
{
    QProcess proc;
    proc.start(program, arguments);
    if (!proc.waitForStarted(timeoutMs)) {
        return QString();
    }
    proc.waitForFinished(timeoutMs);
    return QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
}

QString SystemInfoWidget::readSysFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    return QString::fromLocal8Bit(file.readAll()).trimmed();
}

QString SystemInfoWidget::formatBytes(qint64 bytes)
{
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }
    return QString("%1 %2").arg(size, 0, 'f', 1).arg(units[unit]);
}

void SystemInfoWidget::refreshInfo()
{
    m_statusLabel->setText(tr("正在收集系统信息..."));
    QApplication::processEvents();

    while (m_cardsLayout->count() > 0) {
        QLayoutItem* item = m_cardsLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    loadOverviewInfo();
    loadCpuInfo();
    loadGpuInfo();
    loadMemoryInfo();
    loadDiskInfo();
    loadNetworkInfo();

    m_cardsLayout->addStretch(1);
    m_statusLabel->setText(tr("上次刷新: ") + QDateTime::currentDateTime().toString("HH:mm:ss"));
}

void SystemInfoWidget::loadOverviewInfo()
{
    QFrame* card = createInfoCard("💻", tr("系统概览"));
    QGridLayout* grid = new QGridLayout();
    grid->setHorizontalSpacing(24);
    grid->setVerticalSpacing(10);
    grid->setColumnStretch(1, 1);

    auto detector = SystemDetector::instance();
    SystemInfo info = detector->getSystemInfo();

    int row = 0;
    QString distroText = info.distroName;
    if (!info.distroVersion.isEmpty()) distroText += " " + info.distroVersion;
    addInfoRow(grid, row++, tr("操作系统"), distroText.isEmpty() ? runCommand("lsb_release", {"-d", "-s"}) : distroText);
    addInfoRow(grid, row++, tr("内核版本"), info.kernelVersion.isEmpty()
        ? runCommand("uname", {"-r"}) : info.kernelVersion);
    addInfoRow(grid, row++, tr("系统架构"), info.architecture.isEmpty()
        ? runCommand("uname", {"-m"}) : info.architecture);
    addInfoRow(grid, row++, tr("桌面环境"), info.desktopName);
    addInfoRow(grid, row++, tr("主机名"), info.hostname.isEmpty()
        ? runCommand("hostname", {}) : info.hostname);
    addInfoRow(grid, row++, tr("当前用户"), info.username);

    long up = info.uptimeSeconds;
    if (up <= 0) {
        QString raw = readSysFile("/proc/uptime").section(' ', 0, 0);
        up = raw.toLong();
    }
    long days = up / 86400;
    long hours = (up % 86400) / 3600;
    long mins = (up % 3600) / 60;
    QString uptimeText;
    if (days > 0) uptimeText = QString(tr("%1 天 %2 小时")).arg(days).arg(hours);
    else if (hours > 0) uptimeText = QString(tr("%1 小时 %2 分钟")).arg(hours).arg(mins);
    else uptimeText = QString(tr("%1 分钟")).arg(mins);
    addInfoRow(grid, row++, tr("运行时间"), uptimeText);

    static_cast<QVBoxLayout*>(card->layout())->addLayout(grid);
    m_cardsLayout->addWidget(card);
}

void SystemInfoWidget::loadCpuInfo()
{
    QFrame* card = createInfoCard("⚡", tr("处理器 (CPU)"));
    QGridLayout* grid = new QGridLayout();
    grid->setHorizontalSpacing(24);
    grid->setVerticalSpacing(10);
    grid->setColumnStretch(1, 1);

    QString cpuinfo = readSysFile("/proc/cpuinfo");
    QString model;
    int processors = 0;
    double maxMhz = 0.0;
    QSet<QString> physicalIds;
    QSet<QString> coreIds;

    const QStringList blocks = cpuinfo.split("\n\n", Qt::SkipEmptyParts);
    for (const QString& block : blocks) {
        const QStringList lines = block.split('\n');
        QString physId, coreId;
        for (const QString& line : lines) {
            if (line.startsWith("model name") && model.isEmpty()) {
                model = line.section(':', 1).trimmed();
            } else if (line.startsWith("physical id")) {
                physId = line.section(':', 1).trimmed();
            } else if (line.startsWith("core id")) {
                coreId = physId + ":" + line.section(':', 1).trimmed();
            } else if (line.startsWith("cpu MHz")) {
                double mhz = line.section(':', 1).trimmed().toDouble();
                if (mhz > maxMhz) maxMhz = mhz;
            }
        }
        processors++;
        if (!physId.isEmpty()) physicalIds.insert(physId);
        if (!coreId.isEmpty()) coreIds.insert(coreId);
    }

    auto detector = SystemDetector::instance();
    CpuInfo ci = detector->getSystemInfo().cpuInfo;
    if (model.isEmpty()) model = ci.modelName;

    int physicalCores = coreIds.isEmpty() ? ci.coreCount : coreIds.size();
    if (physicalCores <= 0) physicalCores = processors;

    int row = 0;
    addInfoRow(grid, row++, tr("型号"), model);
    addInfoRow(grid, row++, tr("物理核心数"), QString::number(physicalCores));
    addInfoRow(grid, row++, tr("逻辑线程数"), QString::number(processors > 0 ? processors : ci.threadCount));
    if (maxMhz > 0) {
        addInfoRow(grid, row++, tr("当前频率"), QString("%1 MHz").arg(maxMhz, 0, 'f', 0));
    }

    QString cache;
    const QStringList lines = cpuinfo.split('\n');
    for (const QString& line : lines) {
        if (line.startsWith("cache size")) {
            cache = line.section(':', 1).trimmed();
            break;
        }
    }
    if (!cache.isEmpty()) {
        addInfoRow(grid, row++, tr("缓存"), cache);
    }

    static_cast<QVBoxLayout*>(card->layout())->addLayout(grid);
    m_cardsLayout->addWidget(card);
}

void SystemInfoWidget::loadGpuInfo()
{
    QFrame* card = createInfoCard("🎮", tr("显卡 (GPU)"));
    QGridLayout* grid = new QGridLayout();
    grid->setHorizontalSpacing(24);
    grid->setVerticalSpacing(10);
    grid->setColumnStretch(1, 1);

    auto detector = SystemDetector::instance();
    SystemInfo info = detector->getSystemInfo();

    int row = 0;
    addInfoRow(grid, row++, tr("厂商"), detector->getGpuVendorText());

    QString gpuModel = info.gpuModel;
    if (gpuModel.isEmpty()) {
        QString lspci = runCommand("sh", {"-c", "lspci 2>/dev/null | grep -iE 'vga|3d|display'"});
        if (!lspci.isEmpty()) {
            QString firstLine = lspci.section('\n', 0, 0);
            gpuModel = firstLine.section(':', 2).trimmed();
        }
    }
    addInfoRow(grid, row++, tr("型号"), gpuModel);

    QString allGpus = runCommand("sh", {"-c", "lspci 2>/dev/null | grep -iE 'vga|3d|display'"});
    if (!allGpus.isEmpty()) {
        const QStringList gpuLines = allGpus.split('\n', Qt::SkipEmptyParts);
        if (gpuLines.size() > 1) {
            for (int i = 1; i < gpuLines.size(); ++i) {
                addInfoRow(grid, row++, QString(tr("显卡 %1")).arg(i + 1), gpuLines[i].section(':', 2).trimmed());
            }
        }
    }

    QString driver;
    if (info.gpuVendor == GpuVendor::NVIDIA) {
        driver = runCommand("sh", {"-c", "nvidia-smi --query-gpu=driver_version --format=csv,noheader 2>/dev/null"});
        if (!driver.isEmpty()) driver.prepend("NVIDIA ");
    }
    if (driver.isEmpty()) {
        QString driPath = "/sys/bus/pci/devices";
        QDir pciDir(driPath);
        const QStringList devs = pciDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& dev : devs) {
            QString classFile = readSysFile(driPath + "/" + dev + "/class");
            if (classFile.startsWith("0x03")) {
                QString driverLink = driPath + "/" + dev + "/driver";
                QFileInfo fi(driverLink);
                if (fi.isSymbolicLink()) {
                    driver = QFileInfo(fi.symLinkTarget()).fileName();
                    break;
                }
            }
        }
    }
    addInfoRow(grid, row++, tr("驱动"), driver);

    QString sessionType = qEnvironmentVariable("XDG_SESSION_TYPE");
    if (sessionType.isEmpty()) sessionType = tr("未知");
    addInfoRow(grid, row++, tr("显示服务器"), sessionType == "wayland" ? "Wayland" :
               (sessionType == "x11" ? "X11 (Xorg)" : sessionType));

    static_cast<QVBoxLayout*>(card->layout())->addLayout(grid);
    m_cardsLayout->addWidget(card);
}

void SystemInfoWidget::loadMemoryInfo()
{
    QFrame* card = createInfoCard("🧠", tr("内存 (RAM)"));
    QGridLayout* grid = new QGridLayout();
    grid->setHorizontalSpacing(24);
    grid->setVerticalSpacing(10);
    grid->setColumnStretch(1, 1);

    QString meminfo = readSysFile("/proc/meminfo");
    qint64 memTotal = 0, memAvailable = 0, swapTotal = 0, swapFree = 0;
    const QStringList lines = meminfo.split('\n');
    for (const QString& line : lines) {
        if (line.startsWith("MemTotal:")) memTotal = line.section(QRegularExpression("\\s+"), 1, 1).toLongLong() * 1024;
        else if (line.startsWith("MemAvailable:")) memAvailable = line.section(QRegularExpression("\\s+"), 1, 1).toLongLong() * 1024;
        else if (line.startsWith("SwapTotal:")) swapTotal = line.section(QRegularExpression("\\s+"), 1, 1).toLongLong() * 1024;
        else if (line.startsWith("SwapFree:")) swapFree = line.section(QRegularExpression("\\s+"), 1, 1).toLongLong() * 1024;
    }

    qint64 memUsed = memTotal - memAvailable;
    double memPercent = memTotal > 0 ? (memUsed * 100.0 / memTotal) : 0.0;

    int row = 0;
    addInfoRow(grid, row++, tr("总内存"), formatBytes(memTotal));
    addInfoRow(grid, row++, tr("已使用"), QString("%1 (%2%)").arg(formatBytes(memUsed)).arg(memPercent, 0, 'f', 1));
    addInfoRow(grid, row++, tr("可用"), formatBytes(memAvailable));
    if (swapTotal > 0) {
        qint64 swapUsed = swapTotal - swapFree;
        addInfoRow(grid, row++, tr("交换分区"), QString(tr("%1 (已用 %2)")).arg(formatBytes(swapTotal), formatBytes(swapUsed)));
    } else {
        addInfoRow(grid, row++, tr("交换分区"), tr("未启用"));
    }

    static_cast<QVBoxLayout*>(card->layout())->addLayout(grid);
    m_cardsLayout->addWidget(card);
}

void SystemInfoWidget::loadDiskInfo()
{
    QFrame* card = createInfoCard("💽", tr("磁盘"));
    QGridLayout* grid = new QGridLayout();
    grid->setHorizontalSpacing(24);
    grid->setVerticalSpacing(10);
    grid->setColumnStretch(1, 1);

    int row = 0;

    QStorageInfo root = QStorageInfo::root();
    if (root.isValid() && root.isReady()) {
        qint64 total = root.bytesTotal();
        qint64 avail = root.bytesAvailable();
        qint64 used = total - avail;
        double percent = total > 0 ? (used * 100.0 / total) : 0.0;
        addInfoRow(grid, row++, tr("根分区 (/)"),
            QString("%1 / %2 (%3%)").arg(formatBytes(used), formatBytes(total)).arg(percent, 0, 'f', 1));
        addInfoRow(grid, row++, tr("文件系统"), QString::fromLocal8Bit(root.fileSystemType()));
    }

    QString lsblk = runCommand("lsblk", {"-d", "-o", "NAME,MODEL,SIZE,TYPE", "-n"});
    if (!lsblk.isEmpty()) {
        const QStringList devLines = lsblk.split('\n', Qt::SkipEmptyParts);
        int diskIdx = 0;
        for (const QString& line : devLines) {
            QString simplified = line.simplified();
            if (!simplified.endsWith("disk")) continue;
            QStringList parts = simplified.split(' ');
            if (parts.size() >= 3) {
                QString name = parts.first();
                QString size = parts.size() >= 2 ? parts.at(parts.size() - 2) : "";
                QString model = parts.mid(1, parts.size() - 3).join(" ");
                if (model.isEmpty()) model = tr("未知型号");
                addInfoRow(grid, row++, QString(tr("磁盘 /dev/%1")).arg(name),
                    QString("%1 (%2)").arg(model, size));
                diskIdx++;
            }
        }
    }

    QString mounts = runCommand("sh", {"-c", "df -h -x tmpfs -x devtmpfs -x squashfs 2>/dev/null | tail -n +2"});
    if (!mounts.isEmpty()) {
        const QStringList mountLines = mounts.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : mountLines) {
            QStringList parts = line.simplified().split(' ');
            if (parts.size() >= 6) {
                QString mountPoint = parts.at(5);
                if (mountPoint == "/") continue;
                addInfoRow(grid, row++, QString(tr("挂载 %1")).arg(mountPoint),
                    QString("%1 / %2 (%3)").arg(parts.at(2), parts.at(1), parts.at(4)));
            }
        }
    }

    static_cast<QVBoxLayout*>(card->layout())->addLayout(grid);
    m_cardsLayout->addWidget(card);
}

void SystemInfoWidget::loadNetworkInfo()
{
    QFrame* card = createInfoCard("🌐", tr("网络"));
    QGridLayout* grid = new QGridLayout();
    grid->setHorizontalSpacing(24);
    grid->setVerticalSpacing(10);
    grid->setColumnStretch(1, 1);

    int row = 0;

    QDir netDir("/sys/class/net");
    const QStringList ifaces = netDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& iface : ifaces) {
        if (iface == "lo") continue;
        QString state = readSysFile("/sys/class/net/" + iface + "/operstate");
        QString mac = readSysFile("/sys/class/net/" + iface + "/address");
        QString ipv4 = runCommand("sh", {"-c",
            QString("ip -4 -o addr show dev %1 2>/dev/null | awk '{print $4}' | head -1").arg(iface)});
        QString value = QString("%1 | %2").arg(state == "up" ? tr("已连接") : tr("未连接"), mac);
        if (!ipv4.isEmpty()) {
            value += QString(" | %1").arg(ipv4);
        }
        QString label = iface;
        if (iface.startsWith("w")) label += tr(" (无线)");
        else if (iface.startsWith("e") || iface.startsWith("en")) label += tr(" (有线)");
        else if (iface.startsWith("docker") || iface.startsWith("veth") || iface.startsWith("br-")) label += tr(" (虚拟)");
        addInfoRow(grid, row++, label, value);
    }

    QString dns = runCommand("sh", {"-c", "grep '^nameserver' /etc/resolv.conf 2>/dev/null | awk '{print $2}' | head -2 | tr '\\n' ' '"});
    if (!dns.isEmpty()) {
        addInfoRow(grid, row++, tr("DNS 服务器"), dns.trimmed());
    }

    QString gateway = runCommand("sh", {"-c", "ip route show default 2>/dev/null | awk '{print $3}' | head -1"});
    if (!gateway.isEmpty()) {
        addInfoRow(grid, row++, tr("默认网关"), gateway);
    }

    static_cast<QVBoxLayout*>(card->layout())->addLayout(grid);
    m_cardsLayout->addWidget(card);
}
