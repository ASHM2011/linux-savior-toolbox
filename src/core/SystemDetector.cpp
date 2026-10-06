#include "SystemDetector.h"
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QDir>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QTimer>
#include <QtConcurrent/QtConcurrent>
#include <QCoreApplication>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <fstream>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>

SystemDetector* SystemDetector::s_instance = nullptr;

SystemDetector* SystemDetector::instance()
{
    if (!s_instance) {
        s_instance = new SystemDetector();
    }
    return s_instance;
}

SystemDetector::SystemDetector()
    : m_initialized(false)
{
}

void SystemDetector::detectAllAsync()
{
    if (m_initialized) {
        emit detectionFinished();
        return;
    }
    
    emit detectionProgress(tr("正在检测系统信息..."));

    // 丢弃 QFuture：fire-and-forget 的后台检测任务，避免 Qt6 [[nodiscard]] 警告
    (void)QtConcurrent::run([this]() {
        detectAsyncInternal();
    });
}

void SystemDetector::detectAsyncInternal()
{
    QMutexLocker locker(&m_mutex);
    
    emit detectionProgress(tr("正在检测发行版..."));
    detectDistro();
    
    emit detectionProgress(tr("正在检测桌面环境..."));
    detectDesktop();
    
    emit detectionProgress(tr("正在检测内核与架构..."));
    detectKernelArch();
    
    emit detectionProgress(tr("正在检测CPU信息..."));
    detectCpuInfo();
    
    emit detectionProgress(tr("正在检测GPU..."));
    detectGPU();
    
    emit detectionProgress(tr("正在检测双系统..."));
    detectDualBoot();
    
    emit detectionProgress(tr("正在检测磁盘使用..."));
    detectDiskUsage();
    
    emit detectionProgress(tr("正在检测内存使用..."));
    detectMemoryUsage();
    
    emit detectionProgress(tr("正在检测运行时间..."));
    detectUptime();
    
    emit detectionProgress(tr("正在检测主机信息..."));
    detectHostUser();
    
    m_info.detected = true;
    m_initialized = true;
    
    QMetaObject::invokeMethod(this, [this]() {
        emit detectionFinished();
    }, Qt::QueuedConnection);
}

void SystemDetector::detectAll()
{
    QMutexLocker locker(&m_mutex);
    detectDistro();
    detectDesktop();
    detectKernelArch();
    detectCpuInfo();
    detectGPU();
    detectDualBoot();
    detectDiskUsage();
    detectMemoryUsage();
    detectUptime();
    detectHostUser();
    m_info.detected = true;
    m_initialized = true;
}

SystemInfo SystemDetector::getSystemInfo() const
{
    QMutexLocker locker(&m_mutex);
    return m_info;
}

bool SystemDetector::isDetected() const
{
    QMutexLocker locker(&m_mutex);
    return m_initialized;
}

void SystemDetector::detectDistro()
{
    QFile file("/etc/os-release");
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream in(&file);
        QString content = in.readAll();
        file.close();

        QRegularExpression idRegex("^ID=(.+)$", QRegularExpression::MultilineOption);
        auto idMatch = idRegex.match(content);
        if (idMatch.hasMatch()) {
            QString id = idMatch.captured(1).remove('"').trimmed().toLower();
            m_info.osReleaseId = id;

            if (id == "ubuntu") {
                m_info.distro = DistroType::Ubuntu;
                m_info.distroName = "Ubuntu";
            } else if (id == "fedora") {
                m_info.distro = DistroType::Fedora;
                m_info.distroName = "Fedora";
            } else if (id == "debian") {
                m_info.distro = DistroType::Debian;
                m_info.distroName = "Debian";
            } else if (id == "opensuse-tumbleweed" || id == "opensuse-leap" || id == "opensuse" || id == "suse") {
                m_info.distro = DistroType::OpenSUSE;
                m_info.distroName = "openSUSE";
            } else if (id == "arch" || id == "archarm") {
                m_info.distro = DistroType::Arch;
                m_info.distroName = "Arch Linux";
            } else if (id == "manjaro" || id == "manjaro-arm") {
                m_info.distro = DistroType::Manjaro;
                m_info.distroName = "Manjaro";
            } else if (id == "linuxmint") {
                m_info.distro = DistroType::Mint;
                m_info.distroName = "Linux Mint";
            }
        }

        QRegularExpression idLikeRegex("^ID_LIKE=(.+)$", QRegularExpression::MultilineOption);
        auto idLikeMatch = idLikeRegex.match(content);
        if (idLikeMatch.hasMatch()) {
            m_info.osReleaseIdLike = idLikeMatch.captured(1).remove('"').trimmed().toLower();
        }

        QRegularExpression verRegex("^VERSION_ID=(.+)$", QRegularExpression::MultilineOption);
        auto verMatch = verRegex.match(content);
        if (verMatch.hasMatch()) {
            m_info.distroVersion = verMatch.captured(1).remove('"').trimmed();
        }

        QRegularExpression nameRegex("^PRETTY_NAME=(.+)$", QRegularExpression::MultilineOption);
        auto nameMatch = nameRegex.match(content);
        if (nameMatch.hasMatch()) {
            QString prettyName = nameMatch.captured(1).remove('"').trimmed();
            if (m_info.distroName.isEmpty()) {
                m_info.distroName = prettyName;
            }
        }
    }

    if (QFile::exists("/etc/arch-release") && m_info.distro == DistroType::Unknown) {
        m_info.distro = DistroType::Arch;
        m_info.distroName = "Arch Linux";
        m_info.osReleaseId = "arch";
    }

    if (m_info.distro == DistroType::Unknown) {
        m_info.distro = DistroType::Fedora;
        m_info.distroName = "Fedora";
        m_info.osReleaseId = "fedora";
    }
}

void SystemDetector::detectDesktop()
{
    QString xdgDesktop = qgetenv("XDG_CURRENT_DESKTOP");
    QString xdgSession = qgetenv("XDG_SESSION_DESKTOP");
    QString desktopSession = qgetenv("DESKTOP_SESSION");
    QString xdgCurrentDesktop = qgetenv("XDG_CURRENT_DESKTOP");

    QString desktopStr = xdgDesktop;
    if (desktopStr.isEmpty()) desktopStr = xdgSession;
    if (desktopStr.isEmpty()) desktopStr = desktopSession;
    if (desktopStr.isEmpty()) desktopStr = xdgCurrentDesktop;
    
    desktopStr = desktopStr.toLower();

    if (desktopStr.contains("gnome")) {
        m_info.desktop = DesktopType::GNOME;
        m_info.desktopName = "GNOME";
    } else if (desktopStr.contains("kde") || desktopStr.contains("plasma")) {
        m_info.desktop = DesktopType::KDE;
        m_info.desktopName = "KDE Plasma";
    } else if (desktopStr.contains("xfce")) {
        m_info.desktop = DesktopType::XFCE;
        m_info.desktopName = "XFCE";
    } else if (desktopStr.contains("lxqt")) {
        m_info.desktop = DesktopType::LXQt;
        m_info.desktopName = "LXQt";
    } else if (desktopStr.contains("mate")) {
        m_info.desktop = DesktopType::MATE;
        m_info.desktopName = "MATE";
    } else if (desktopStr.contains("cinnamon")) {
        m_info.desktop = DesktopType::Cinnamon;
        m_info.desktopName = "Cinnamon";
    } else if (desktopStr.contains("i3")) {
        m_info.desktop = DesktopType::i3;
        m_info.desktopName = "i3";
    } else if (desktopStr.contains("sway")) {
        m_info.desktop = DesktopType::Sway;
        m_info.desktopName = "Sway";
    } else if (desktopStr.contains("hyprland")) {
        m_info.desktop = DesktopType::Hyprland;
        m_info.desktopName = "Hyprland";
    }

    if (m_info.desktop == DesktopType::Unknown) {
        m_info.desktop = DesktopType::GNOME;
        m_info.desktopName = "GNOME";
    }
}

void SystemDetector::detectKernelArch()
{
    QProcess proc;
    proc.start("uname", {"-r"});
    if (proc.waitForFinished(1000)) {
        m_info.kernelVersion = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
    }

    proc.start("uname", {"-m"});
    if (proc.waitForFinished(1000)) {
        m_info.architecture = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed();
    }
}

void SystemDetector::detectCpuInfo()
{
    m_info.cpuInfo.coreCount = QThread::idealThreadCount();
    m_info.cpuInfo.threadCount = m_info.cpuInfo.coreCount;
    m_info.cpuInfo.modelName = "Unknown CPU";
    m_info.cpuUsage = 0.0;

    QFile cpuFile("/proc/cpuinfo");
    if (cpuFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream in(&cpuFile);
        QString content = in.readAll();
        cpuFile.close();

        QRegularExpression modelRegex("model name\\s*:\\s*(.+)", QRegularExpression::MultilineOption);
        auto modelMatch = modelRegex.match(content);
        if (modelMatch.hasMatch()) {
            m_info.cpuInfo.modelName = modelMatch.captured(1).trimmed();
        }

        QRegularExpression cpuRegex("^processor\\s*:", QRegularExpression::MultilineOption);
        auto cpuMatches = cpuRegex.globalMatch(content);
        int threadCount = 0;
        while (cpuMatches.hasNext()) {
            cpuMatches.next();
            threadCount++;
        }
        if (threadCount > 0) {
            m_info.cpuInfo.threadCount = threadCount;
        }

        QRegularExpression coreRegex("cpu cores\\s*:\\s*(\\d+)", QRegularExpression::MultilineOption);
        auto coreMatch = coreRegex.match(content);
        if (coreMatch.hasMatch()) {
            m_info.cpuInfo.coreCount = coreMatch.captured(1).toInt();
        } else {
            m_info.cpuInfo.coreCount = threadCount;
        }
    }

    auto readCpuStat = []() -> std::vector<unsigned long long> {
        std::ifstream file("/proc/stat");
        std::string line;
        std::getline(file, line);
        std::istringstream iss(line);
        std::string cpu;
        unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
        iss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
        return {user, nice, system, idle, iowait, irq, softirq, steal};
    };

    try {
        auto prev = readCpuStat();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto curr = readCpuStat();

        unsigned long long prevIdle = prev[3] + prev[4];
        unsigned long long currIdle = curr[3] + curr[4];
        unsigned long long prevTotal = 0, currTotal = 0;
        for (size_t i = 0; i < prev.size(); i++) {
            prevTotal += prev[i];
            currTotal += curr[i];
        }

        unsigned long long totald = currTotal - prevTotal;
        unsigned long long idled = currIdle - prevIdle;

        if (totald > 0) {
            m_info.cpuUsage = (double)(totald - idled) / totald * 100.0;
            m_info.cpuInfo.usagePercent = m_info.cpuUsage;
        }
    } catch (...) {
        m_info.cpuUsage = 0.0;
    }
}

void SystemDetector::detectGPU()
{
    QString gpuInfo;

    QProcess proc;
    proc.start("lspci", {"-mm"});
    if (proc.waitForFinished(2000)) {
        QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());
        QStringList lines = output.split('\n');
        for (const QString& line : lines) {
            if (line.contains("VGA", Qt::CaseInsensitive) || 
                line.contains("3D", Qt::CaseInsensitive) || 
                line.contains("Display", Qt::CaseInsensitive) ||
                line.contains("VGA compatible controller")) {
                gpuInfo = line;
                break;
            }
        }
    }

    if (gpuInfo.isEmpty()) {
        QDir drmDir("/sys/class/drm");
        QStringList cards = drmDir.entryList(QStringList() << "card*", QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& card : cards) {
            QFile vendorFile("/sys/class/drm/" + card + "/device/vendor");
            if (vendorFile.open(QFile::ReadOnly | QFile::Text)) {
                QString vendorId = vendorFile.readAll().trimmed();
                vendorFile.close();
                gpuInfo = vendorId;
                
                QFile deviceFile("/sys/class/drm/" + card + "/device/device");
                if (deviceFile.open(QFile::ReadOnly | QFile::Text)) {
                    gpuInfo += " " + deviceFile.readAll().trimmed();
                    deviceFile.close();
                }
                break;
            }
        }
    }

    if (gpuInfo.contains("NVIDIA", Qt::CaseInsensitive) || gpuInfo.contains("0x10de", Qt::CaseInsensitive)) {
        m_info.gpuVendor = GpuVendor::NVIDIA;
        QRegularExpression regex("NVIDIA Corporation\\] (.+?) \\[", QRegularExpression::CaseInsensitiveOption);
        auto match = regex.match(gpuInfo);
        m_info.gpuModel = match.hasMatch() ? match.captured(1).trimmed() : "NVIDIA";
    } else if (gpuInfo.contains("AMD", Qt::CaseInsensitive) || 
               gpuInfo.contains("Radeon", Qt::CaseInsensitive) || 
               gpuInfo.contains("Advanced Micro Devices", Qt::CaseInsensitive) ||
               gpuInfo.contains("0x1002", Qt::CaseInsensitive) ||
               gpuInfo.contains("ATI", Qt::CaseInsensitive)) {
        m_info.gpuVendor = GpuVendor::AMD;
        QRegularExpression regex("(?:AMD|ATI|Advanced Micro Devices).*?\\] (.+?) \\[", QRegularExpression::CaseInsensitiveOption);
        auto match = regex.match(gpuInfo);
        m_info.gpuModel = match.hasMatch() ? match.captured(1).trimmed() : "AMD Radeon";
    } else if (gpuInfo.contains("Intel", Qt::CaseInsensitive) || gpuInfo.contains("0x8086", Qt::CaseInsensitive)) {
        m_info.gpuVendor = GpuVendor::Intel;
        QRegularExpression regex("Intel Corporation\\] (.+?) \\[", QRegularExpression::CaseInsensitiveOption);
        auto match = regex.match(gpuInfo);
        m_info.gpuModel = match.hasMatch() ? match.captured(1).trimmed() : "Intel";
    } else if (gpuInfo.contains("Virtio", Qt::CaseInsensitive) || 
               gpuInfo.contains("Red Hat", Qt::CaseInsensitive) || 
               gpuInfo.contains("0x1af4", Qt::CaseInsensitive)) {
        m_info.gpuVendor = GpuVendor::VirtIO;
        m_info.gpuModel = tr("VirtIO (虚拟机)");
    } else {
        m_info.gpuVendor = GpuVendor::Unknown;
        if (!gpuInfo.isEmpty()) {
            QStringList parts = gpuInfo.split('"');
            if (parts.size() >= 4) {
                m_info.gpuModel = parts[2];
            } else {
                m_info.gpuModel = gpuInfo;
            }
        } else {
            m_info.gpuModel = tr("未知显卡");
        }
    }
}

void SystemDetector::detectDualBoot()
{
    m_info.isDualBoot = false;

    QProcess proc;
    proc.start("os-prober");
    if (proc.waitForFinished(3000)) {
        QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());
        if (output.contains("Windows", Qt::CaseInsensitive) ||
            output.contains("Microsoft", Qt::CaseInsensitive)) {
            m_info.isDualBoot = true;
            return;
        }
    }

    foreach (const QStorageInfo& storage, QStorageInfo::mountedVolumes()) {
        QString fsType = storage.fileSystemType().toLower();
        if (fsType == "ntfs" || fsType == "fuseblk") {
            m_info.isDualBoot = true;
            return;
        }
    }

    QDir efiDir("/boot/efi/EFI");
    if (efiDir.exists()) {
        QStringList entries = efiDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& entry : entries) {
            if (entry.contains("Windows", Qt::CaseInsensitive) ||
                entry.contains("Microsoft", Qt::CaseInsensitive) ||
                entry.contains("Boot", Qt::CaseInsensitive) && QFile::exists("/boot/efi/EFI/Microsoft/Boot/bootmgfw.efi")) {
                m_info.isDualBoot = true;
                return;
            }
        }
    }

    QFile fstabFile("/etc/fstab");
    if (fstabFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream in(&fstabFile);
        QString content = in.readAll();
        fstabFile.close();
        if (content.contains("ntfs", Qt::CaseInsensitive) || content.contains("fuseblk", Qt::CaseInsensitive)) {
            m_info.isDualBoot = true;
        }
    }
}

void SystemDetector::detectDiskUsage()
{
    QStorageInfo root = QStorageInfo::root();
    m_info.diskTotalBytes = root.bytesTotal();
    m_info.diskUsedBytes = root.bytesTotal() - root.bytesFree();
    
    if (m_info.diskTotalBytes <= 0) {
        QProcess process;
        process.start("df", {"-B1", "/"});
        if (process.waitForFinished(2000)) {
            QString output = QString::fromLocal8Bit(process.readAllStandardOutput());
            QStringList lines = output.split('\n');
            if (lines.size() >= 2) {
                QStringList parts = lines[1].split(QRegularExpression("\\s+"));
                if (parts.size() >= 3) {
                    m_info.diskTotalBytes = parts[1].toLongLong();
                    m_info.diskUsedBytes = parts[2].toLongLong();
                }
            }
        }
    }
}

void SystemDetector::detectMemoryUsage()
{
    QFile file("/proc/meminfo");
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream in(&file);
        QString content = in.readAll();
        file.close();

        QRegularExpression totalRegex("MemTotal:\\s+(\\d+)\\s+kB");
        auto totalMatch = totalRegex.match(content);
        if (totalMatch.hasMatch()) {
            m_info.memoryTotalBytes = totalMatch.captured(1).toLongLong() * 1024;
        }

        qint64 availBytes = 0;
        qint64 freeBytes = 0;
        qint64 buffers = 0;
        qint64 cached = 0;
        qint64 sreclaimable = 0;

        QRegularExpression availRegex("MemAvailable:\\s+(\\d+)\\s+kB");
        auto availMatch = availRegex.match(content);
        if (availMatch.hasMatch()) {
            availBytes = availMatch.captured(1).toLongLong() * 1024;
        }

        QRegularExpression freeRegex("MemFree:\\s+(\\d+)\\s+kB");
        auto freeMatch = freeRegex.match(content);
        if (freeMatch.hasMatch()) {
            freeBytes = freeMatch.captured(1).toLongLong() * 1024;
        }

        QRegularExpression buffersRegex("Buffers:\\s+(\\d+)\\s+kB");
        auto buffersMatch = buffersRegex.match(content);
        if (buffersMatch.hasMatch()) {
            buffers = buffersMatch.captured(1).toLongLong() * 1024;
        }

        QRegularExpression cachedRegex("Cached:\\s+(\\d+)\\s+kB");
        auto cachedMatch = cachedRegex.match(content);
        if (cachedMatch.hasMatch()) {
            cached = cachedMatch.captured(1).toLongLong() * 1024;
        }

        QRegularExpression sreclaimRegex("SReclaimable:\\s+(\\d+)\\s+kB");
        auto sreclaimMatch = sreclaimRegex.match(content);
        if (sreclaimMatch.hasMatch()) {
            sreclaimable = sreclaimMatch.captured(1).toLongLong() * 1024;
        }

        if (availBytes > 0) {
            m_info.memoryUsedBytes = m_info.memoryTotalBytes - availBytes;
        } else if (freeBytes > 0) {
            m_info.memoryUsedBytes = m_info.memoryTotalBytes - freeBytes - buffers - cached - sreclaimable;
        }
    }
}

void SystemDetector::detectUptime()
{
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        m_info.uptimeSeconds = info.uptime;
    } else {
        QFile uptimeFile("/proc/uptime");
        if (uptimeFile.open(QFile::ReadOnly | QFile::Text)) {
            QString content = uptimeFile.readAll();
            uptimeFile.close();
            QStringList parts = content.split(' ');
            if (!parts.isEmpty()) {
                m_info.uptimeSeconds = parts[0].toLong();
            }
        }
    }
}

void SystemDetector::detectHostUser()
{
    char hostname[256] = {0};
    if (gethostname(hostname, sizeof(hostname) - 1) == 0) {
        m_info.hostname = QString::fromLocal8Bit(hostname);
    }

    char* login = getlogin();
    if (login) {
        m_info.username = QString::fromLocal8Bit(login);
    } else {
        m_info.username = qgetenv("USER");
        if (m_info.username.isEmpty()) {
            m_info.username = qgetenv("USERNAME");
        }
        if (m_info.username.isEmpty()) {
            m_info.username = "user";
        }
    }
    m_info.homeDir = QDir::homePath();
}

QString SystemDetector::distroName() const
{
    QMutexLocker locker(&m_mutex);
    return m_info.distroName;
}

QString SystemDetector::desktopName() const
{
    QMutexLocker locker(&m_mutex);
    return m_info.desktopName;
}

bool SystemDetector::isGNOME() const
{
    QMutexLocker locker(&m_mutex);
    return m_info.desktop == DesktopType::GNOME;
}

bool SystemDetector::isKDE() const
{
    QMutexLocker locker(&m_mutex);
    return m_info.desktop == DesktopType::KDE;
}

bool SystemDetector::isXFCE() const
{
    QMutexLocker locker(&m_mutex);
    return m_info.desktop == DesktopType::XFCE;
}

bool SystemDetector::isUbuntu() const
{
    QMutexLocker locker(&m_mutex);
    return m_info.distro == DistroType::Ubuntu || m_info.distro == DistroType::Mint;
}

bool SystemDetector::isFedora() const
{
    QMutexLocker locker(&m_mutex);
    return m_info.distro == DistroType::Fedora;
}

bool SystemDetector::isOpenSUSE() const
{
    QMutexLocker locker(&m_mutex);
    return m_info.distro == DistroType::OpenSUSE;
}

bool SystemDetector::isDebian() const
{
    QMutexLocker locker(&m_mutex);
    return m_info.distro == DistroType::Debian;
}

bool SystemDetector::isArch() const
{
    QMutexLocker locker(&m_mutex);
    return m_info.distro == DistroType::Arch;
}

bool SystemDetector::isArchBased() const
{
    QMutexLocker locker(&m_mutex);
    return m_info.distro == DistroType::Arch || 
           m_info.distro == DistroType::Manjaro ||
           m_info.osReleaseIdLike.contains("arch");
}

QString SystemDetector::getPackageManagerPrefix() const
{
    QMutexLocker locker(&m_mutex);
    switch (m_info.distro) {
    case DistroType::Ubuntu:
    case DistroType::Debian:
    case DistroType::Mint:
        return "apt";
    case DistroType::Fedora:
        return "dnf";
    case DistroType::OpenSUSE:
        return "zypper";
    case DistroType::Arch:
    case DistroType::Manjaro:
        return "pacman";
    default:
        if (m_info.osReleaseIdLike.contains("debian") || m_info.osReleaseIdLike.contains("ubuntu")) {
            return "apt";
        } else if (m_info.osReleaseIdLike.contains("suse")) {
            return "zypper";
        } else if (m_info.osReleaseIdLike.contains("arch")) {
            return "pacman";
        }
        return "dnf";
    }
}

QString SystemDetector::getUpdateCommandId() const
{
    QMutexLocker locker(&m_mutex);
    switch (m_info.distro) {
    case DistroType::Ubuntu:
    case DistroType::Debian:
    case DistroType::Mint:
        return "apt_update";
    case DistroType::Fedora:
        return "dnf_update";
    case DistroType::OpenSUSE:
        return "zypper_update";
    case DistroType::Arch:
    case DistroType::Manjaro:
        return "pacman_update";
    default:
        if (m_info.osReleaseIdLike.contains("debian") || m_info.osReleaseIdLike.contains("ubuntu")) {
            return "apt_update";
        } else if (m_info.osReleaseIdLike.contains("suse")) {
            return "zypper_update";
        } else if (m_info.osReleaseIdLike.contains("arch")) {
            return "pacman_update";
        }
        return "dnf_update";
    }
}

QString SystemDetector::getCleanCacheCommandId() const
{
    QMutexLocker locker(&m_mutex);
    switch (m_info.distro) {
    case DistroType::Ubuntu:
    case DistroType::Debian:
    case DistroType::Mint:
        return "apt_clean";
    case DistroType::Fedora:
        return "dnf_clean";
    case DistroType::OpenSUSE:
        return "zypper_clean";
    case DistroType::Arch:
    case DistroType::Manjaro:
        return "pacman_clean";
    default:
        if (m_info.osReleaseIdLike.contains("debian") || m_info.osReleaseIdLike.contains("ubuntu")) {
            return "apt_clean";
        } else if (m_info.osReleaseIdLike.contains("suse")) {
            return "zypper_clean";
        } else if (m_info.osReleaseIdLike.contains("arch")) {
            return "pacman_clean";
        }
        return "dnf_clean";
    }
}

QString SystemDetector::getUnlockCommandId() const
{
    QMutexLocker locker(&m_mutex);
    switch (m_info.distro) {
    case DistroType::Ubuntu:
    case DistroType::Debian:
    case DistroType::Mint:
        return "unlock_apt";
    case DistroType::Fedora:
        return "unlock_dnf";
    case DistroType::OpenSUSE:
        return "unlock_zypper";
    case DistroType::Arch:
    case DistroType::Manjaro:
        return "unlock_pacman";
    default:
        if (m_info.osReleaseIdLike.contains("debian") || m_info.osReleaseIdLike.contains("ubuntu")) {
            return "unlock_apt";
        } else if (m_info.osReleaseIdLike.contains("suse")) {
            return "unlock_zypper";
        } else if (m_info.osReleaseIdLike.contains("arch")) {
            return "unlock_pacman";
        }
        return "unlock_dnf";
    }
}

QString SystemDetector::getAutoremoveCommandId() const
{
    QMutexLocker locker(&m_mutex);
    switch (m_info.distro) {
    case DistroType::Ubuntu:
    case DistroType::Debian:
    case DistroType::Mint:
        return "apt_autoremove";
    case DistroType::Fedora:
        return "dnf_autoremove";
    case DistroType::OpenSUSE:
        return "zypper_autoremove";
    case DistroType::Arch:
    case DistroType::Manjaro:
        return "pacman_autoremove";
    default:
        if (m_info.osReleaseIdLike.contains("debian") || m_info.osReleaseIdLike.contains("ubuntu")) {
            return "apt_autoremove";
        } else if (m_info.osReleaseIdLike.contains("suse")) {
            return "zypper_autoremove";
        } else if (m_info.osReleaseIdLike.contains("arch")) {
            return "pacman_autoremove";
        }
        return "dnf_autoremove";
    }
}

bool SystemDetector::isDualBoot() const
{
    QMutexLocker locker(&m_mutex);
    return m_info.isDualBoot;
}

QString SystemDetector::getGpuVendorText() const
{
    QMutexLocker locker(&m_mutex);
    switch (m_info.gpuVendor) {
    case GpuVendor::NVIDIA:
        return "NVIDIA";
    case GpuVendor::AMD:
        return "AMD";
    case GpuVendor::Intel:
        return "Intel";
    case GpuVendor::VirtIO:
        return "VirtIO";
    case GpuVendor::Unknown:
    default:
        return tr("未知");
    }
}
