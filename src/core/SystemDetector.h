#ifndef SYSTEMDETECTOR_H
#define SYSTEMDETECTOR_H

#include <QObject>
#include <QString>
#include <QThread>
#include <QFutureWatcher>
#include <QMutex>

enum class DistroType {
    Ubuntu,
    Fedora,
    Debian,
    OpenSUSE,
    Arch,
    Manjaro,
    Mint,
    Unknown
};

enum class DesktopType {
    GNOME,
    KDE,
    XFCE,
    LXQt,
    MATE,
    Cinnamon,
    i3,
    Sway,
    Hyprland,
    Unknown
};

enum class GpuVendor {
    NVIDIA,
    AMD,
    Intel,
    VirtIO,
    Unknown
};

struct CpuInfo {
    QString modelName;
    int coreCount;
    int threadCount;
    double usagePercent;
};

struct SystemInfo {
    DistroType distro = DistroType::Unknown;
    QString distroName;
    QString distroVersion;
    QString osReleaseId;
    QString osReleaseIdLike;
    DesktopType desktop = DesktopType::Unknown;
    QString desktopName;
    QString desktopVersion;
    QString kernelVersion;
    QString architecture;
    GpuVendor gpuVendor = GpuVendor::Unknown;
    QString gpuModel;
    bool isDualBoot = false;
    qint64 diskTotalBytes = 0;
    qint64 diskUsedBytes = 0;
    qint64 memoryTotalBytes = 0;
    qint64 memoryUsedBytes = 0;
    double cpuUsage = 0.0;
    CpuInfo cpuInfo;
    QString hostname;
    QString username;
    QString homeDir;
    long uptimeSeconds = 0;
    bool detected = false;
};

class SystemDetector : public QObject
{
    Q_OBJECT

public:
    static SystemDetector* instance();

    void detectAllAsync();
    void detectAll();
    SystemInfo getSystemInfo() const;
    bool isDetected() const;

    QString distroName() const;
    QString desktopName() const;
    bool isGNOME() const;
    bool isKDE() const;
    bool isXFCE() const;
    bool isUbuntu() const;
    bool isFedora() const;
    bool isOpenSUSE() const;
    bool isDebian() const;
    bool isArch() const;
    bool isArchBased() const;

    QString getPackageManagerPrefix() const;
    QString getUpdateCommandId() const;
    QString getCleanCacheCommandId() const;
    QString getUnlockCommandId() const;
    QString getAutoremoveCommandId() const;
    bool isDualBoot() const;
    QString getGpuVendorText() const;

signals:
    void detectionFinished();
    void detectionProgress(const QString& status);

private:
    SystemDetector();
    void detectDistro();
    void detectDesktop();
    void detectKernelArch();
    void detectGPU();
    void detectDualBoot();
    void detectDiskUsage();
    void detectMemoryUsage();
    void detectCpuInfo();
    void detectUptime();
    void detectHostUser();
    void detectAsyncInternal();

    mutable QMutex m_mutex;
    SystemInfo m_info;
    bool m_initialized;
    static SystemDetector* s_instance;
};

#endif
