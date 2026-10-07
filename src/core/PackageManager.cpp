#include "PackageManager.h"
#include "CommandExecutor.h"
#include "CommandMetadata.h"
#include <QDebug>
#include <QRegularExpression>
#include <QCoreApplication>

/**
 * 纵深防御：包名白名单校验（与 Taru/Rust/Electron 完全对齐）
 *   单包：[A-Za-z0-9._+-]  1..=128
 *   多包：空格分隔，总长度 <= 256
 * 返回空字符串表示合法。
 */
static QString validatePackageArg(const QString &raw)
{
    if (raw.isEmpty()) return QCoreApplication::translate("PackageManager", "[参数校验] 包名为空。");
    if (raw.length() > 256) return QCoreApplication::translate("PackageManager", "[参数校验] 包名列表过长（上限 256 字符）。");
    static const QRegularExpression kSinglePkg(QStringLiteral(R"(^[A-Za-z0-9._+\-]{1,128}$)"));
    for (const QString &part : raw.split(QRegularExpression(QStringLiteral(R"(\s+)")), Qt::SkipEmptyParts)) {
        if (!kSinglePkg.match(part).hasMatch()) {
            return QCoreApplication::translate("PackageManager", "[安全拦截] 包名包含非法字符，仅允许字母/数字/._+-：%1").arg(part);
        }
    }
    return {};
}

PackageManager* PackageManager::s_instance = nullptr;

PackageManager* PackageManager::instance()
{
    if (!s_instance) {
        s_instance = new PackageManager();
    }
    return s_instance;
}

PackageManager::PackageManager(QObject *parent)
    : QObject(parent)
    , m_systemDetector(SystemDetector::instance())
    , m_parsingInstalledPackages(false)
{
    m_systemDetector->detectAll();
    buildCommandMap();
}

PackageManager::~PackageManager() = default;

void PackageManager::updateSystem()
{
    executeAction(Action::UpdateSystem);
}

void PackageManager::cleanCache()
{
    executeAction(Action::CleanCache);
}

void PackageManager::autoremove()
{
    executeAction(Action::Autoremove);
}

void PackageManager::installPackage(const QString &packageName)
{
    const QString err = validatePackageArg(packageName);
    if (!err.isEmpty()) {
        emit errorReady(err + QStringLiteral("\n"));
        emit finished(-1);
        return;
    }
    executeAction(Action::InstallPackage, packageName);
}

void PackageManager::removePackage(const QString &packageName)
{
    const QString err = validatePackageArg(packageName);
    if (!err.isEmpty()) {
        emit errorReady(err + QStringLiteral("\n"));
        emit finished(-1);
        return;
    }
    executeAction(Action::RemovePackage, packageName);
}

void PackageManager::fixDependencies()
{
    executeAction(Action::FixDependencies);
}

void PackageManager::unlockPackageManager()
{
    executeAction(Action::UnlockPackageManager);
}

void PackageManager::checkForUpdates()
{
    executeAction(Action::CheckForUpdates);
}

void PackageManager::getInstalledPackages()
{
    m_parsingInstalledPackages = true;
    m_accumulatedOutput.clear();

    if (m_systemDetector->isFedora() ||
        m_systemDetector->isOpenSUSE()) {
        CommandExecutor::instance()->executeRawCommand("rpm -qa --qf '%{NAME}\\n' | sort", false);
    } else if (m_systemDetector->isUbuntu() ||
               m_systemDetector->isDebian()) {
        CommandExecutor::instance()->executeRawCommand("dpkg --list | grep '^ii' | awk '{print $2}' | sort", false);
    } else {
        CommandExecutor::instance()->executeRawCommand(
            "rpm -qa --qf '%{NAME}\\n' 2>/dev/null | sort || dpkg --list 2>/dev/null | grep '^ii' | awk '{print $2}' | sort",
            false);
    }
}

bool PackageManager::isRunning() const
{
    return CommandExecutor::instance()->isRunning();
}

QString PackageManager::commandForAction(Action action, const QString &packageName) const
{
    QString cmd = m_commandMap.value(action);
    if (!packageName.isEmpty() && cmd.contains("%1")) {
        cmd = cmd.arg(packageName);
    }
    return cmd;
}

void PackageManager::onOutputReceived(const QString &output)
{
    if (m_parsingInstalledPackages) {
        m_accumulatedOutput.append(output);
    }
    emit outputReady(output);
}

void PackageManager::onErrorReceived(const QString &error)
{
    emit errorReady(error);
}

void PackageManager::onExecutionFinished(const QString &commandId, bool success, const QString &message)
{
    Q_UNUSED(commandId);
    Q_UNUSED(message);
    if (m_parsingInstalledPackages) {
        parseInstalledPackages(m_accumulatedOutput.join(QString()));
        m_parsingInstalledPackages = false;
        m_accumulatedOutput.clear();
    }
    emit finished(success ? 0 : 1);
}

void PackageManager::executeAction(Action action, const QString &packageName)
{
    QString command = commandForAction(action, packageName);
    bool needsRoot = true;

    emit started();

    auto executor = CommandExecutor::instance();
    connect(executor, &CommandExecutor::outputReceived, this, &PackageManager::onOutputReceived, Qt::UniqueConnection);
    connect(executor, &CommandExecutor::errorReceived, this, &PackageManager::onErrorReceived, Qt::UniqueConnection);
    connect(executor, &CommandExecutor::executionFinished, this, &PackageManager::onExecutionFinished, Qt::UniqueConnection);

    executor->executeRawCommand(command, needsRoot);
}

void PackageManager::buildCommandMap()
{
    if (m_systemDetector->isFedora()) {

        m_commandMap[Action::UpdateSystem] = QStringLiteral("dnf upgrade --refresh -y");
        m_commandMap[Action::CleanCache] = QStringLiteral("dnf clean all && dnf autoremove -y");
        m_commandMap[Action::Autoremove] = QStringLiteral("dnf autoremove -y");
        m_commandMap[Action::InstallPackage] = QStringLiteral("dnf install -y %1");
        m_commandMap[Action::RemovePackage] = QStringLiteral("dnf remove -y %1");
        m_commandMap[Action::FixDependencies] = QStringLiteral("dnf install -f -y");
        m_commandMap[Action::UnlockPackageManager] = QStringLiteral("rm -f /var/lib/dnf/lock* /var/lib/rpm/.rpm.lock");
        m_commandMap[Action::CheckForUpdates] = QStringLiteral("dnf check-update");

    } else if (m_systemDetector->isUbuntu() || m_systemDetector->isDebian()) {

        m_commandMap[Action::UpdateSystem] = QStringLiteral("apt update && apt upgrade -y");
        m_commandMap[Action::CleanCache] = QStringLiteral("apt clean && apt autoclean && apt autoremove -y");
        m_commandMap[Action::Autoremove] = QStringLiteral("apt autoremove -y");
        m_commandMap[Action::InstallPackage] = QStringLiteral("apt install -y %1");
        m_commandMap[Action::RemovePackage] = QStringLiteral("apt remove -y %1");
        m_commandMap[Action::FixDependencies] = QStringLiteral("apt install -f -y");
        m_commandMap[Action::UnlockPackageManager] = QStringLiteral("rm -f /var/lib/dpkg/lock* /var/lib/apt/lists/lock");
        m_commandMap[Action::CheckForUpdates] = QStringLiteral("apt update && apt list --upgradable");

    } else if (m_systemDetector->isOpenSUSE()) {

        m_commandMap[Action::UpdateSystem] = QStringLiteral("zypper refresh && zypper up -y");
        m_commandMap[Action::CleanCache] = QStringLiteral("zypper clean -a && zypper rm -u -y");
        m_commandMap[Action::Autoremove] = QStringLiteral("zypper rm -u -y");
        m_commandMap[Action::InstallPackage] = QStringLiteral("zypper install -y %1");
        m_commandMap[Action::RemovePackage] = QStringLiteral("zypper remove -y %1");
        m_commandMap[Action::FixDependencies] = QStringLiteral("zypper verify --repair -y");
        m_commandMap[Action::UnlockPackageManager] = QStringLiteral("rm -f /var/lib/zypp/lock /var/lib/rpm/.rpm.lock");
        m_commandMap[Action::CheckForUpdates] = QStringLiteral("zypper list-updates");

    } else if (m_systemDetector->isArch() || m_systemDetector->isArchBased()) {

        m_commandMap[Action::UpdateSystem] = QStringLiteral("pacman -Syu --noconfirm");
        m_commandMap[Action::CleanCache] = QStringLiteral("pacman -Sc --noconfirm");
        m_commandMap[Action::Autoremove] = QStringLiteral("pacman -Qdtq | pacman -Rs --noconfirm -");
        m_commandMap[Action::InstallPackage] = QStringLiteral("pacman -S --noconfirm %1");
        m_commandMap[Action::RemovePackage] = QStringLiteral("pacman -Rns --noconfirm %1");
        m_commandMap[Action::FixDependencies] = QStringLiteral("pacman -Dk");
        m_commandMap[Action::UnlockPackageManager] = QStringLiteral("rm -f /var/lib/pacman/db.lck");
        m_commandMap[Action::CheckForUpdates] = QStringLiteral("pacman -Qu");

    } else {

        m_commandMap[Action::UpdateSystem] = QStringLiteral("apt update && apt upgrade -y");
        m_commandMap[Action::CleanCache] = QStringLiteral("apt clean && apt autoclean && apt autoremove -y");
        m_commandMap[Action::Autoremove] = QStringLiteral("apt autoremove -y");
        m_commandMap[Action::InstallPackage] = QStringLiteral("apt install -y %1");
        m_commandMap[Action::RemovePackage] = QStringLiteral("apt remove -y %1");
        m_commandMap[Action::FixDependencies] = QStringLiteral("apt install -f -y");
        m_commandMap[Action::UnlockPackageManager] = QStringLiteral("rm -f /var/lib/dpkg/lock* /var/lib/apt/lists/lock");
        m_commandMap[Action::CheckForUpdates] = QStringLiteral("apt update && apt list --upgradable");
    }
}

void PackageManager::parseInstalledPackages(const QString &output)
{
    QStringList lines = output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    QStringList packages;
    packages.reserve(lines.size());

    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            packages.append(trimmed);
        }
    }

    emit installedPackagesReady(packages);
}
