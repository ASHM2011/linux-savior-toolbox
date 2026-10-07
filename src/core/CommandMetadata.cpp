#include "CommandMetadata.h"

CommandMetadataManager* CommandMetadataManager::s_instance = nullptr;

CommandMetadataManager* CommandMetadataManager::instance()
{
    if (!s_instance) {
        s_instance = new CommandMetadataManager();
    }
    return s_instance;
}

CommandMetadataManager::CommandMetadataManager()
{
    initCommands();
}

void CommandMetadataManager::initCommands()
{
    // ========== 系统更新类 ==========

    CommandMetadata aptUpdate;
    aptUpdate.id = "apt_update";
    aptUpdate.command = "apt update && apt upgrade -y";
    aptUpdate.needsAdmin = true;
    aptUpdate.friendlyName = tr("一键更新系统");
    aptUpdate.corePurpose = tr("刷新软件源列表并升级所有已安装软件到最新版本。");
    aptUpdate.paramBreakdown = {
        {"apt", tr("高级包工具，Debian/Ubuntu系的软件管理器")},
        {"update", tr("刷新软件源列表，获取最新的软件版本信息")},
        {"&&", tr("前面命令成功后才执行后面的命令")},
        {"apt upgrade", tr("升级所有已安装的软件包")},
        {"-y", tr("自动回答yes，不需要手动确认")}
    };
    aptUpdate.safetyLevel = SafetyLevel::Safe;
    aptUpdate.executionEffect = tr("系统中所有已安装的软件都会升级到最新版本。");
    aptUpdate.commonPitfall = tr("更新过程中不要断电或强制终止，否则可能导致系统损坏。建议在电量充足时执行。");
    aptUpdate.category = CommandCategory::SystemUpdate;
    aptUpdate.supportedDistros = {"ubuntu", "debian"};
    aptUpdate.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[aptUpdate.id] = aptUpdate;

    CommandMetadata aptUpdateOnly;
    aptUpdateOnly.id = "apt_update_only";
    aptUpdateOnly.command = "apt update";
    aptUpdateOnly.needsAdmin = true;
    aptUpdateOnly.friendlyName = tr("刷新软件源列表");
    aptUpdateOnly.corePurpose = tr("仅刷新软件源列表，获取最新的软件版本信息，不升级软件。");
    aptUpdateOnly.paramBreakdown = {
        {"apt", tr("高级包工具")},
        {"update", tr("刷新软件源列表")}
    };
    aptUpdateOnly.safetyLevel = SafetyLevel::Safe;
    aptUpdateOnly.executionEffect = tr("软件源列表被更新，系统会知道哪些软件有新版本可用。");
    aptUpdateOnly.commonPitfall = tr("此操作只刷新列表，不会真正升级软件。需要配合 upgrade 才能实际升级。");
    aptUpdateOnly.category = CommandCategory::SystemUpdate;
    aptUpdateOnly.supportedDistros = {"ubuntu", "debian"};
    aptUpdateOnly.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[aptUpdateOnly.id] = aptUpdateOnly;

    CommandMetadata aptFixDeps;
    aptFixDeps.id = "apt_fix_deps";
    aptFixDeps.command = "apt install -f -y";
    aptFixDeps.needsAdmin = true;
    aptFixDeps.friendlyName = tr("修复依赖关系");
    aptFixDeps.corePurpose = tr("修复损坏的依赖关系，补全缺失的依赖包。常用于安装软件后依赖不完整的情况。");
    aptFixDeps.paramBreakdown = {
        {"apt", tr("高级包工具")},
        {"install", tr("安装模式")},
        {"-f", tr("fix-broken，修复损坏的依赖")},
        {"-y", tr("自动确认")}
    };
    aptFixDeps.safetyLevel = SafetyLevel::Safe;
    aptFixDeps.executionEffect = tr("系统会自动安装缺失的依赖，修复损坏的包状态。");
    aptFixDeps.commonPitfall = tr("通常在安装 deb 包后或软件安装失败时使用。");
    aptFixDeps.category = CommandCategory::SystemUpdate;
    aptFixDeps.supportedDistros = {"ubuntu", "debian"};
    aptFixDeps.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[aptFixDeps.id] = aptFixDeps;

    CommandMetadata dnfUpdate;
    dnfUpdate.id = "dnf_update";
    dnfUpdate.command = "dnf upgrade --refresh -y";
    dnfUpdate.needsAdmin = true;
    dnfUpdate.friendlyName = tr("一键更新系统");
    dnfUpdate.corePurpose = tr("刷新软件源并升级所有已安装软件到最新版本。");
    dnfUpdate.paramBreakdown = {
        {"dnf", tr("Fedora/RHEL系的新一代包管理器")},
        {"upgrade", tr("升级所有已安装的软件包")},
        {"--refresh", tr("升级前先刷新软件源缓存")},
        {"-y", tr("自动回答yes，不需要手动确认")}
    };
    dnfUpdate.safetyLevel = SafetyLevel::Safe;
    dnfUpdate.executionEffect = tr("系统中所有已安装的软件都会升级到最新版本。");
    dnfUpdate.commonPitfall = tr("更新过程中不要断电或强制终止。内核更新后需要重启才能生效。");
    dnfUpdate.category = CommandCategory::SystemUpdate;
    dnfUpdate.supportedDistros = {"fedora", "rhel"};
    dnfUpdate.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[dnfUpdate.id] = dnfUpdate;

    CommandMetadata dnfUpdateOnly;
    dnfUpdateOnly.id = "dnf_update_only";
    dnfUpdateOnly.command = "dnf check-update";
    dnfUpdateOnly.needsAdmin = false;
    dnfUpdateOnly.friendlyName = tr("检查软件更新");
    dnfUpdateOnly.corePurpose = tr("检查可用的软件更新，列出可升级的软件包，但不实际升级。");
    dnfUpdateOnly.paramBreakdown = {
        {"dnf", tr("Fedora/RHEL系包管理器")},
        {"check-update", tr("检查可用更新")}
    };
    dnfUpdateOnly.safetyLevel = SafetyLevel::Safe;
    dnfUpdateOnly.executionEffect = tr("列出所有可以升级的软件包及其版本。");
    dnfUpdateOnly.commonPitfall = tr("此操作只检查不升级，不需要管理员权限。");
    dnfUpdateOnly.category = CommandCategory::SystemUpdate;
    dnfUpdateOnly.supportedDistros = {"fedora", "rhel"};
    dnfUpdateOnly.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[dnfUpdateOnly.id] = dnfUpdateOnly;

    CommandMetadata zypperUpdate;
    zypperUpdate.id = "zypper_update";
    zypperUpdate.command = "zypper refresh && zypper up -y";
    zypperUpdate.needsAdmin = true;
    zypperUpdate.friendlyName = tr("一键更新系统");
    zypperUpdate.corePurpose = tr("刷新软件源并升级所有已安装软件到最新版本。");
    zypperUpdate.paramBreakdown = {
        {"zypper", tr("openSUSE的包管理器")},
        {"refresh", tr("刷新软件源")},
        {"&&", tr("前面成功后执行后面")},
        {"up", tr("升级所有软件包")},
        {"-y", tr("自动确认")}
    };
    zypperUpdate.safetyLevel = SafetyLevel::Safe;
    zypperUpdate.executionEffect = tr("所有已安装软件升级到最新版本。");
    zypperUpdate.commonPitfall = tr("更新过程中不要断电。");
    zypperUpdate.category = CommandCategory::SystemUpdate;
    zypperUpdate.supportedDistros = {"opensuse"};
    zypperUpdate.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[zypperUpdate.id] = zypperUpdate;

    CommandMetadata zypperRefresh;
    zypperRefresh.id = "zypper_refresh";
    zypperRefresh.command = "zypper refresh";
    zypperRefresh.needsAdmin = true;
    zypperRefresh.friendlyName = tr("刷新软件源");
    zypperRefresh.corePurpose = tr("刷新所有软件源的元数据，获取最新的软件包信息。");
    zypperRefresh.paramBreakdown = {
        {"zypper", tr("openSUSE包管理器")},
        {"refresh", tr("刷新软件源元数据")}
    };
    zypperRefresh.safetyLevel = SafetyLevel::Safe;
    zypperRefresh.executionEffect = tr("软件源元数据被更新，可以看到最新的可用软件。");
    zypperRefresh.commonPitfall = tr("只刷新不升级，需要配合 zypper up 才能升级软件。");
    zypperRefresh.category = CommandCategory::SystemUpdate;
    zypperRefresh.supportedDistros = {"opensuse"};
    zypperRefresh.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[zypperRefresh.id] = zypperRefresh;

    // ========== 桌面修复类 ==========

    CommandMetadata gnomeShellRestart;
    gnomeShellRestart.id = "gnome_shell_restart";
    gnomeShellRestart.command = "gnome-shell --replace &";
    gnomeShellRestart.needsAdmin = false;
    gnomeShellRestart.friendlyName = tr("重启GNOME桌面");
    gnomeShellRestart.corePurpose = tr("重启GNOME Shell桌面环境，解决桌面卡顿、扩展失效等问题。不会关闭已打开的窗口。");
    gnomeShellRestart.paramBreakdown = {
        {"gnome-shell", tr("GNOME桌面环境的主进程")},
        {"--replace", tr("替换当前正在运行的gnome-shell进程")},
        {"&", tr("在后台运行，不占用当前终端")}
    };
    gnomeShellRestart.safetyLevel = SafetyLevel::Safe;
    gnomeShellRestart.executionEffect = tr("桌面会短暂黑屏闪烁一下，然后恢复正常。所有打开的窗口和程序都会保留。");
    gnomeShellRestart.commonPitfall = tr("这个操作不需要管理员权限！如果加了sudo反而会出问题。注意：GNOME在Wayland模式下无法重启桌面，需要注销重新登录。");
    gnomeShellRestart.category = CommandCategory::DesktopRepair;
    gnomeShellRestart.supportedDistros = {"ubuntu", "fedora", "debian", "opensuse", "arch", "manjaro"};
    gnomeShellRestart.supportedDesktops = {"gnome"};
    m_commands[gnomeShellRestart.id] = gnomeShellRestart;

    CommandMetadata plasmaRestart;
    plasmaRestart.id = "plasma_restart";
    plasmaRestart.command = "plasmashell --replace &";
    plasmaRestart.needsAdmin = false;
    plasmaRestart.friendlyName = tr("重启Plasma桌面");
    plasmaRestart.corePurpose = tr("重启KDE Plasma桌面环境，解决桌面卡顿、面板异常、插件失效等问题。不会关闭已打开的窗口。");
    plasmaRestart.paramBreakdown = {
        {"plasmashell", tr("KDE Plasma桌面的主进程")},
        {"--replace", tr("替换当前正在运行的plasmashell进程")},
        {"&", tr("后台运行")}
    };
    plasmaRestart.safetyLevel = SafetyLevel::Safe;
    plasmaRestart.executionEffect = tr("桌面会短暂刷新一下，面板和桌面组件会重新加载。所有打开的窗口保留。");
    plasmaRestart.commonPitfall = tr("不需要管理员权限！加了sudo反而会导致权限问题。");
    plasmaRestart.category = CommandCategory::DesktopRepair;
    plasmaRestart.supportedDistros = {"ubuntu", "fedora", "debian", "opensuse"};
    plasmaRestart.supportedDesktops = {"kde"};
    m_commands[plasmaRestart.id] = plasmaRestart;

    CommandMetadata xfce4Restart;
    xfce4Restart.id = "xfce4_restart";
    xfce4Restart.command = "xfce4-panel -r && xfdesktop -Q && xfdesktop &";
    xfce4Restart.needsAdmin = false;
    xfce4Restart.friendlyName = tr("重启XFCE桌面组件");
    xfce4Restart.corePurpose = tr("重启XFCE的面板和桌面组件，解决面板卡死、图标不显示等问题。");
    xfce4Restart.paramBreakdown = {
        {"xfce4-panel -r", tr("重启XFCE面板")},
        {"&&", tr("前面成功后执行后面")},
        {"xfdesktop -Q", tr("退出桌面管理器")},
        {"xfdesktop &", tr("重新启动桌面管理器")}
    };
    xfce4Restart.safetyLevel = SafetyLevel::Safe;
    xfce4Restart.executionEffect = tr("XFCE面板和桌面会重新加载，图标和菜单恢复正常。");
    xfce4Restart.commonPitfall = tr("不需要管理员权限！");
    xfce4Restart.category = CommandCategory::DesktopRepair;
    xfce4Restart.supportedDistros = {"ubuntu", "fedora", "debian", "opensuse"};
    xfce4Restart.supportedDesktops = {"xfce"};
    m_commands[xfce4Restart.id] = xfce4Restart;

    CommandMetadata nautilusRestart;
    nautilusRestart.id = "nautilus_restart";
    nautilusRestart.command = "nautilus -q";
    nautilusRestart.needsAdmin = false;
    nautilusRestart.friendlyName = tr("重启Nautilus文件管理器");
    nautilusRestart.corePurpose = tr("退出所有Nautilus文件管理器窗口，解决右键菜单卡顿、图标显示异常等问题。");
    nautilusRestart.paramBreakdown = {
        {"nautilus", tr("GNOME默认文件管理器的名字")},
        {"-q", tr("退出（quit）所有nautilus窗口")}
    };
    nautilusRestart.safetyLevel = SafetyLevel::Safe;
    nautilusRestart.executionEffect = tr("所有打开的文件管理器窗口会关闭，重新打开即可恢复正常。");
    nautilusRestart.commonPitfall = tr("这个操作不需要管理员权限！加了sudo反而会导致权限问题。");
    nautilusRestart.category = CommandCategory::DesktopRepair;
    nautilusRestart.supportedDistros = {"ubuntu", "fedora", "debian", "opensuse"};
    nautilusRestart.supportedDesktops = {"gnome"};
    m_commands[nautilusRestart.id] = nautilusRestart;

    CommandMetadata dolphinRestart;
    dolphinRestart.id = "dolphin_restart";
    dolphinRestart.command = "killall dolphin && dolphin &";
    dolphinRestart.needsAdmin = false;
    dolphinRestart.friendlyName = tr("重启Dolphin文件管理器");
    dolphinRestart.corePurpose = tr("退出并重启Dolphin文件管理器，解决文件列表不刷新、右键菜单异常等问题。");
    dolphinRestart.paramBreakdown = {
        {"killall", tr("终止指定名称的所有进程")},
        {"dolphin", tr("KDE默认文件管理器")},
        {"&&", tr("前面成功后执行后面")},
        {"dolphin &", tr("后台重新启动Dolphin")}
    };
    dolphinRestart.safetyLevel = SafetyLevel::Safe;
    dolphinRestart.executionEffect = tr("所有Dolphin窗口会关闭并重新启动一个新窗口。");
    dolphinRestart.commonPitfall = tr("不需要管理员权限！");
    dolphinRestart.category = CommandCategory::DesktopRepair;
    dolphinRestart.supportedDistros = {"ubuntu", "fedora", "debian", "opensuse"};
    dolphinRestart.supportedDesktops = {"kde"};
    m_commands[dolphinRestart.id] = dolphinRestart;

    CommandMetadata ibusRestart;
    ibusRestart.id = "ibus_restart";
    ibusRestart.command = "ibus restart";
    ibusRestart.needsAdmin = false;
    ibusRestart.friendlyName = tr("重启IBus输入法");
    ibusRestart.corePurpose = tr("重启IBus输入法框架，解决输入法无法切换、候选框不显示等问题。");
    ibusRestart.paramBreakdown = {
        {"ibus", tr("Intelligent Input Bus，Linux下常用的输入法框架")},
        {"restart", tr("重启输入法服务")}
    };
    ibusRestart.safetyLevel = SafetyLevel::Safe;
    ibusRestart.executionEffect = tr("输入法会重启，几秒钟后可以正常切换输入法。");
    ibusRestart.commonPitfall = tr("这个操作不需要管理员权限！");
    ibusRestart.category = CommandCategory::DesktopRepair;
    ibusRestart.supportedDistros = {"ubuntu", "fedora", "debian", "opensuse"};
    ibusRestart.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[ibusRestart.id] = ibusRestart;

    CommandMetadata chownHome;
    chownHome.id = "chown_home";
    chownHome.command = "chown -R $USER:$USER $HOME";
    chownHome.needsAdmin = true;
    chownHome.friendlyName = tr("修复用户目录权限");
    chownHome.corePurpose = tr("把用户主目录下所有文件的所有权改回当前用户。常用于修复用sudo打开图形程序导致的权限问题。");
    chownHome.paramBreakdown = {
        {"chown", tr("修改文件所有者（change owner）")},
        {"-R", tr("递归处理，包括所有子目录和文件")},
        {"$USER:$USER", tr("当前用户:当前用户组")},
        {"$HOME", tr("当前用户的主目录")}
    };
    chownHome.safetyLevel = SafetyLevel::Caution;
    chownHome.executionEffect = tr("主目录下所有文件的所有者都会变成当前用户。执行后建议注销重新登录。");
    chownHome.commonPitfall = tr("⚠️ 仅修改你自己的用户目录！绝对不要对 / 或 /root 等系统目录使用此命令，否则系统会崩溃。");
    chownHome.category = CommandCategory::DesktopRepair;
    chownHome.supportedDistros = {"ubuntu", "fedora", "debian", "opensuse"};
    chownHome.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[chownHome.id] = chownHome;

    CommandMetadata fixTrashPermission;
    fixTrashPermission.id = "fix_trash_permission";
    fixTrashPermission.command = "chown -R $USER:$USER ~/.local/share/Trash && chmod -R u+w ~/.local/share/Trash";
    fixTrashPermission.needsAdmin = false;
    fixTrashPermission.friendlyName = tr("修复回收站权限");
    fixTrashPermission.corePurpose = tr("修复回收站目录的权限，解决无法删除文件到回收站、回收站图标不更新等问题。");
    fixTrashPermission.paramBreakdown = {
        {"chown", tr("修改所有者为当前用户")},
        {"-R", tr("递归处理")},
        {"~/.local/share/Trash", tr("回收站目录")},
        {"chmod u+w", tr("给用户添加写权限")}
    };
    fixTrashPermission.safetyLevel = SafetyLevel::Safe;
    fixTrashPermission.executionEffect = tr("回收站目录权限恢复正常，可以正常删除和恢复文件。");
    fixTrashPermission.commonPitfall = tr("不需要管理员权限，这是用户自己目录下的操作。");
    fixTrashPermission.category = CommandCategory::DesktopRepair;
    fixTrashPermission.supportedDistros = {"ubuntu", "fedora", "debian", "opensuse"};
    fixTrashPermission.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[fixTrashPermission.id] = fixTrashPermission;

    // ========== 系统清理类 ==========

    CommandMetadata aptClean;
    aptClean.id = "apt_clean";
    aptClean.command = "apt clean";
    aptClean.needsAdmin = true;
    aptClean.friendlyName = tr("清理APT缓存");
    aptClean.corePurpose = tr("清理APT下载的安装包缓存，释放磁盘空间。不会删除已安装的软件。");
    aptClean.paramBreakdown = {
        {"apt", tr("高级包工具")},
        {"clean", tr("清理所有已下载的包文件缓存")}
    };
    aptClean.safetyLevel = SafetyLevel::Safe;
    aptClean.executionEffect = tr("删除 /var/cache/apt/archives/ 目录下所有 .deb 包文件。");
    aptClean.commonPitfall = tr("完全安全的操作，只是清理下载缓存。下次安装软件时会重新下载。");
    aptClean.category = CommandCategory::SystemCleanup;
    aptClean.supportedDistros = {"ubuntu", "debian"};
    aptClean.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[aptClean.id] = aptClean;

    CommandMetadata aptAutoremove;
    aptAutoremove.id = "apt_autoremove";
    aptAutoremove.command = "apt autoremove -y";
    aptAutoremove.needsAdmin = true;
    aptAutoremove.friendlyName = tr("清理无用依赖");
    aptAutoremove.corePurpose = tr("删除系统中不再需要的依赖包，这些包是之前安装软件时自动安装的。");
    aptAutoremove.paramBreakdown = {
        {"apt", tr("高级包工具")},
        {"autoremove", tr("自动删除不再需要的依赖包")},
        {"-y", tr("自动确认")}
    };
    aptAutoremove.safetyLevel = SafetyLevel::Safe;
    aptAutoremove.executionEffect = tr("释放一些磁盘空间，删除不再被任何软件使用的依赖包。");
    aptAutoremove.commonPitfall = tr("安全的操作，只会删除确实不再需要的包。");
    aptAutoremove.category = CommandCategory::SystemCleanup;
    aptAutoremove.supportedDistros = {"ubuntu", "debian"};
    aptAutoremove.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[aptAutoremove.id] = aptAutoremove;

    CommandMetadata dnfClean;
    dnfClean.id = "dnf_clean";
    dnfClean.command = "dnf clean all";
    dnfClean.needsAdmin = true;
    dnfClean.friendlyName = tr("清理DNF缓存");
    dnfClean.corePurpose = tr("清理DNF包管理器的所有缓存，释放磁盘空间。");
    dnfClean.paramBreakdown = {
        {"dnf", tr("Fedora/RHEL系的包管理器")},
        {"clean", tr("清理缓存")},
        {"all", tr("清理所有类型的缓存")}
    };
    dnfClean.safetyLevel = SafetyLevel::Safe;
    dnfClean.executionEffect = tr("删除所有下载的包缓存和元数据缓存。");
    dnfClean.commonPitfall = tr("完全安全的操作。下次使用dnf时会重新下载缓存。");
    dnfClean.category = CommandCategory::SystemCleanup;
    dnfClean.supportedDistros = {"fedora", "rhel"};
    dnfClean.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[dnfClean.id] = dnfClean;

    CommandMetadata dnfAutoremove;
    dnfAutoremove.id = "dnf_autoremove";
    dnfAutoremove.command = "dnf autoremove -y";
    dnfAutoremove.needsAdmin = true;
    dnfAutoremove.friendlyName = tr("清理无用依赖");
    dnfAutoremove.corePurpose = tr("删除系统中不再需要的依赖包，释放磁盘空间。");
    dnfAutoremove.paramBreakdown = {
        {"dnf", tr("Fedora/RHEL系包管理器")},
        {"autoremove", tr("自动删除不需要的依赖")},
        {"-y", tr("自动确认")}
    };
    dnfAutoremove.safetyLevel = SafetyLevel::Safe;
    dnfAutoremove.executionEffect = tr("删除不再被任何软件使用的依赖包，释放磁盘空间。");
    dnfAutoremove.commonPitfall = tr("安全的操作。只会删除确实不再需要的包。");
    dnfAutoremove.category = CommandCategory::SystemCleanup;
    dnfAutoremove.supportedDistros = {"fedora", "rhel"};
    dnfAutoremove.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[dnfAutoremove.id] = dnfAutoremove;

    CommandMetadata zypperClean;
    zypperClean.id = "zypper_clean";
    zypperClean.command = "zypper clean -a";
    zypperClean.needsAdmin = true;
    zypperClean.friendlyName = tr("清理Zypper缓存");
    zypperClean.corePurpose = tr("清理Zypper包管理器的所有缓存，释放磁盘空间。");
    zypperClean.paramBreakdown = {
        {"zypper", tr("openSUSE包管理器")},
        {"clean", tr("清理缓存")},
        {"-a", tr("all，清理所有缓存")}
    };
    zypperClean.safetyLevel = SafetyLevel::Safe;
    zypperClean.executionEffect = tr("删除所有下载的包缓存和元数据缓存。");
    zypperClean.commonPitfall = tr("完全安全的操作。下次使用zypper时会重新下载。");
    zypperClean.category = CommandCategory::SystemCleanup;
    zypperClean.supportedDistros = {"opensuse"};
    zypperClean.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[zypperClean.id] = zypperClean;

    CommandMetadata zypperAutoremove;
    zypperAutoremove.id = "zypper_autoremove";
    zypperAutoremove.command = "zypper rm -u --clean-deps -y";
    zypperAutoremove.needsAdmin = true;
    zypperAutoremove.friendlyName = tr("清理无用依赖");
    zypperAutoremove.corePurpose = tr("清理不再需要的依赖包，释放磁盘空间。");
    zypperAutoremove.paramBreakdown = {
        {"zypper", tr("openSUSE包管理器")},
        {"rm -u", tr("remove --clean-deps，清理无用依赖")},
        {"-y", tr("自动确认")}
    };
    zypperAutoremove.safetyLevel = SafetyLevel::Safe;
    zypperAutoremove.executionEffect = tr("删除不再被需要的依赖包。");
    zypperAutoremove.commonPitfall = tr("安全的操作。只会删除确实不再需要的包。");
    zypperAutoremove.category = CommandCategory::SystemCleanup;
    zypperAutoremove.supportedDistros = {"opensuse"};
    zypperAutoremove.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[zypperAutoremove.id] = zypperAutoremove;

    CommandMetadata pacmanUpdate;
    pacmanUpdate.id = "pacman_update";
    pacmanUpdate.command = "pacman -Syu --noconfirm";
    pacmanUpdate.needsAdmin = true;
    pacmanUpdate.friendlyName = tr("一键更新系统");
    pacmanUpdate.corePurpose = tr("同步软件源并升级所有已安装软件到最新版本（Arch/Manjaro系）。");
    pacmanUpdate.paramBreakdown = {
        {"pacman", tr("Arch Linux包管理器")},
        {"-Syu", tr("-S同步 -y刷新源 -u升级系统")},
        {"--noconfirm", tr("自动确认，不需要手动回答")}
    };
    pacmanUpdate.safetyLevel = SafetyLevel::Safe;
    pacmanUpdate.executionEffect = tr("系统中所有已安装的软件都会升级到最新版本。");
    pacmanUpdate.commonPitfall = tr("更新过程中不要断电或强制终止。Arch用户建议定期更新系统。");
    pacmanUpdate.category = CommandCategory::SystemUpdate;
    pacmanUpdate.supportedDistros = {"arch", "manjaro"};
    pacmanUpdate.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[pacmanUpdate.id] = pacmanUpdate;

    CommandMetadata pacmanClean;
    pacmanClean.id = "pacman_clean";
    pacmanClean.command = "pacman -Sc --noconfirm";
    pacmanClean.needsAdmin = true;
    pacmanClean.friendlyName = tr("清理Pacman缓存");
    pacmanClean.corePurpose = tr("清理pacman下载的安装包缓存，释放磁盘空间。保留当前已安装包的缓存。");
    pacmanClean.paramBreakdown = {
        {"pacman", tr("Arch Linux包管理器")},
        {"-Sc", tr("-S同步操作 -c清理缓存")},
        {"--noconfirm", tr("自动确认")}
    };
    pacmanClean.safetyLevel = SafetyLevel::Safe;
    pacmanClean.executionEffect = tr("删除旧版本的包缓存，只保留当前安装版本的包。");
    pacmanClean.commonPitfall = tr("完全安全的操作。下次安装软件时会重新下载。");
    pacmanClean.category = CommandCategory::SystemCleanup;
    pacmanClean.supportedDistros = {"arch", "manjaro"};
    pacmanClean.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[pacmanClean.id] = pacmanClean;

    CommandMetadata pacmanAutoremove;
    pacmanAutoremove.id = "pacman_autoremove";
    pacmanAutoremove.command = "pacman -Qdtq | pacman -Rs --noconfirm -";
    pacmanAutoremove.needsAdmin = true;
    pacmanAutoremove.friendlyName = tr("清理无用依赖");
    pacmanAutoremove.corePurpose = tr("删除系统中不再被任何软件包依赖的孤立包。");
    pacmanAutoremove.paramBreakdown = {
        {"pacman -Qdtq", tr("查询所有孤立包（作为依赖安装且不再被需要的包）")},
        {"pacman -Rs -", tr("递归删除这些包")},
        {"--noconfirm", tr("自动确认")}
    };
    pacmanAutoremove.safetyLevel = SafetyLevel::Safe;
    pacmanAutoremove.executionEffect = tr("删除不再被任何软件需要的依赖包，释放磁盘空间。");
    pacmanAutoremove.commonPitfall = tr("安全的操作。但建议先查看要删除的包列表再确认。");
    pacmanAutoremove.category = CommandCategory::SystemCleanup;
    pacmanAutoremove.supportedDistros = {"arch", "manjaro"};
    pacmanAutoremove.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[pacmanAutoremove.id] = pacmanAutoremove;

    CommandMetadata rmTrash;
    rmTrash.id = "rm_trash";
    rmTrash.command = "rm -rf ~/.local/share/Trash/*";
    rmTrash.needsAdmin = false;
    rmTrash.friendlyName = tr("清空回收站");
    rmTrash.corePurpose = tr("永久删除回收站中的所有文件。删除后无法恢复！");
    rmTrash.paramBreakdown = {
        {"rm", tr("删除命令")},
        {"-r", tr("递归删除")},
        {"-f", tr("强制删除")},
        {"~/.local/share/Trash/*", tr("回收站目录下的所有文件")}
    };
    rmTrash.safetyLevel = SafetyLevel::Caution;
    rmTrash.executionEffect = tr("回收站里的所有文件和文件夹都会被永久删除，无法恢复。");
    rmTrash.commonPitfall = tr("⚠️ 删除后无法恢复！确认回收站里没有需要的文件再执行。");
    rmTrash.category = CommandCategory::SystemCleanup;
    rmTrash.supportedDistros = {"ubuntu", "fedora", "debian", "opensuse"};
    rmTrash.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[rmTrash.id] = rmTrash;

    CommandMetadata cleanJournalLogs;
    cleanJournalLogs.id = "clean_journal_logs";
    cleanJournalLogs.command = "journalctl --vacuum-size=100M";
    cleanJournalLogs.needsAdmin = true;
    cleanJournalLogs.friendlyName = tr("清理系统日志");
    cleanJournalLogs.corePurpose = tr("清理systemd日志文件，只保留最近100MB的日志，释放磁盘空间。");
    cleanJournalLogs.paramBreakdown = {
        {"journalctl", tr("systemd日志管理工具")},
        {"--vacuum-size=100M", tr("清理日志直到占用空间小于100MB")}
    };
    cleanJournalLogs.safetyLevel = SafetyLevel::Safe;
    cleanJournalLogs.executionEffect = tr("旧的系统日志被删除，只保留最近的日志记录。");
    cleanJournalLogs.commonPitfall = tr("安全操作。只删除旧日志，不影响系统运行。如果需要排查问题建议先保留日志。");
    cleanJournalLogs.category = CommandCategory::SystemCleanup;
    cleanJournalLogs.supportedDistros = {"ubuntu", "fedora", "debian", "opensuse", "arch", "manjaro"};
    cleanJournalLogs.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[cleanJournalLogs.id] = cleanJournalLogs;

    CommandMetadata cleanThumbnails;
    cleanThumbnails.id = "clean_thumbnails";
    cleanThumbnails.command = "rm -rf ~/.cache/thumbnails/*";
    cleanThumbnails.needsAdmin = false;
    cleanThumbnails.friendlyName = tr("清理缩略图缓存");
    cleanThumbnails.corePurpose = tr("删除文件管理器生成的缩略图缓存，解决缩略图显示异常、占用空间过大的问题。");
    cleanThumbnails.paramBreakdown = {
        {"rm", tr("删除命令")},
        {"-rf", tr("递归强制删除")},
        {"~/.cache/thumbnails/*", tr("缩略图缓存目录")}
    };
    cleanThumbnails.safetyLevel = SafetyLevel::Safe;
    cleanThumbnails.executionEffect = tr("缩略图缓存被清空，下次打开文件夹时会重新生成缩略图。");
    cleanThumbnails.commonPitfall = tr("完全安全。不需要管理员权限。清理后缩略图会重新生成。");
    cleanThumbnails.category = CommandCategory::SystemCleanup;
    cleanThumbnails.supportedDistros = {"ubuntu", "fedora", "debian", "opensuse"};
    cleanThumbnails.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[cleanThumbnails.id] = cleanThumbnails;

    CommandMetadata cleanBrowserCache;
    cleanBrowserCache.id = "clean_browser_cache";
    cleanBrowserCache.command = "rm -rf ~/.cache/mozilla/firefox/*/cache2/* ~/.cache/google-chrome/*/Cache/* ~/.cache/chromium/*/Cache/*";
    cleanBrowserCache.needsAdmin = false;
    cleanBrowserCache.friendlyName = tr("清理浏览器缓存");
    cleanBrowserCache.corePurpose = tr("清理Firefox、Chrome、Chromium浏览器的缓存文件，释放磁盘空间。");
    cleanBrowserCache.paramBreakdown = {
        {"rm -rf", tr("递归强制删除")},
        {"~/.cache/mozilla/firefox", tr("Firefox缓存目录")},
        {"~/.cache/google-chrome", tr("Chrome缓存目录")},
        {"~/.cache/chromium", tr("Chromium缓存目录")}
    };
    cleanBrowserCache.safetyLevel = SafetyLevel::Safe;
    cleanBrowserCache.executionEffect = tr("浏览器缓存被删除，下次访问网页时会重新加载。书签和历史记录不受影响。");
    cleanBrowserCache.commonPitfall = tr("只清理缓存，不会删除书签、密码、历史记录等个人数据。");
    cleanBrowserCache.category = CommandCategory::SystemCleanup;
    cleanBrowserCache.supportedDistros = {"ubuntu", "fedora", "debian", "opensuse"};
    cleanBrowserCache.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[cleanBrowserCache.id] = cleanBrowserCache;

    CommandMetadata cleanOldKernels;
    cleanOldKernels.id = "clean_old_kernels";
    cleanOldKernels.command = "apt autoremove --purge -y";
    cleanOldKernels.needsAdmin = true;
    cleanOldKernels.friendlyName = tr("清理旧内核");
    cleanOldKernels.corePurpose = tr("清理系统中不再使用的旧内核，释放大量磁盘空间。");
    cleanOldKernels.paramBreakdown = {
        {"apt", tr("高级包工具")},
        {"autoremove --purge", tr("自动删除不需要的包及其配置文件")},
        {"-y", tr("自动确认")}
    };
    cleanOldKernels.safetyLevel = SafetyLevel::Caution;
    cleanOldKernels.executionEffect = tr("旧内核被删除，当前内核保留。释放数GB的磁盘空间。");
    cleanOldKernels.commonPitfall = tr("⚠️ 确保当前运行的内核工作正常再清理！清理后无法回退到旧内核。");
    cleanOldKernels.category = CommandCategory::SystemCleanup;
    cleanOldKernels.supportedDistros = {"ubuntu", "debian"};
    cleanOldKernels.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[cleanOldKernels.id] = cleanOldKernels;

    // ========== 包管理类 ==========

    CommandMetadata unlockApt;
    unlockApt.id = "unlock_apt";
    unlockApt.command = "rm -f /var/lib/dpkg/lock* /var/lib/apt/lists/lock";
    unlockApt.needsAdmin = true;
    unlockApt.friendlyName = tr("解锁APT包管理器");
    unlockApt.corePurpose = tr("强制删除包管理器的锁文件，解决'无法获得锁'的错误。");
    unlockApt.paramBreakdown = {
        {"rm", tr("删除命令")},
        {"-f", tr("强制删除")},
        {"/var/lib/dpkg/lock*", tr("dpkg的锁文件")},
        {"/var/lib/apt/lists/lock", tr("apt列表的锁文件")}
    };
    unlockApt.safetyLevel = SafetyLevel::Caution;
    unlockApt.executionEffect = tr("包管理器的锁被解除，可以重新使用软件中心或apt命令。");
    unlockApt.commonPitfall = tr("先确认没有其他更新程序在运行。强制解锁可能损坏包数据库。");
    unlockApt.category = CommandCategory::PackageManager;
    unlockApt.supportedDistros = {"ubuntu", "debian"};
    unlockApt.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[unlockApt.id] = unlockApt;

    CommandMetadata dpkgConfigure;
    dpkgConfigure.id = "dpkg_configure";
    dpkgConfigure.command = "dpkg --configure -a";
    dpkgConfigure.needsAdmin = true;
    dpkgConfigure.friendlyName = tr("修复dpkg中断状态");
    dpkgConfigure.corePurpose = tr("重新配置所有已解包但未配置的软件包，常用于更新被强制中断后的修复。");
    dpkgConfigure.paramBreakdown = {
        {"dpkg", tr("Debian包管理底层工具")},
        {"--configure", tr("配置软件包")},
        {"-a", tr("all，所有未配置的包")}
    };
    dpkgConfigure.safetyLevel = SafetyLevel::Caution;
    dpkgConfigure.executionEffect = tr("所有未完成配置的软件包会被重新配置，包管理器恢复正常状态。");
    dpkgConfigure.commonPitfall = tr("通常在更新被中断、出现'dpkg was interrupted'错误时使用。");
    dpkgConfigure.category = CommandCategory::PackageManager;
    dpkgConfigure.supportedDistros = {"ubuntu", "debian"};
    dpkgConfigure.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[dpkgConfigure.id] = dpkgConfigure;

    CommandMetadata unlockDnf;
    unlockDnf.id = "unlock_dnf";
    unlockDnf.command = "rm -f /var/cache/dnf/*.rpm /var/lib/rpm/__db* && rpm --rebuilddb";
    unlockDnf.needsAdmin = true;
    unlockDnf.friendlyName = tr("解锁DNF包管理器");
    unlockDnf.corePurpose = tr("修复DNF/RPM数据库锁和损坏问题，解决包管理器卡住的问题。");
    unlockDnf.paramBreakdown = {
        {"rm -f", tr("强制删除")},
        {"/var/cache/dnf/*.rpm", tr("DNF缓存的rpm包")},
        {"/var/lib/rpm/__db*", tr("RPM数据库锁文件")},
        {"rpm --rebuilddb", tr("重建RPM数据库索引")}
    };
    unlockDnf.safetyLevel = SafetyLevel::Caution;
    unlockDnf.executionEffect = tr("RPM数据库被重建，包管理器恢复可用。");
    unlockDnf.commonPitfall = tr("先确认没有其他包管理程序在运行。重建数据库需要一点时间。");
    unlockDnf.category = CommandCategory::PackageManager;
    unlockDnf.supportedDistros = {"fedora", "rhel"};
    unlockDnf.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[unlockDnf.id] = unlockDnf;

    CommandMetadata unlockZypper;
    unlockZypper.id = "unlock_zypper";
    unlockZypper.command = "rm -f /var/lib/rpm/__db* && rpm --rebuilddb && zypper clean -a";
    unlockZypper.needsAdmin = true;
    unlockZypper.friendlyName = tr("解锁Zypper包管理器");
    unlockZypper.corePurpose = tr("修复Zypper/RPM数据库锁，重建数据库索引，解决包管理器卡住的问题。");
    unlockZypper.paramBreakdown = {
        {"rm -f", tr("强制删除")},
        {"/var/lib/rpm/__db*", tr("RPM数据库锁文件")},
        {"rpm --rebuilddb", tr("重建RPM数据库")},
        {"zypper clean -a", tr("清理所有zypper缓存")}
    };
    unlockZypper.safetyLevel = SafetyLevel::Caution;
    unlockZypper.executionEffect = tr("RPM数据库重建完成，zypper恢复正常使用。");
    unlockZypper.commonPitfall = tr("先确认没有其他包管理程序在运行。");
    unlockZypper.category = CommandCategory::PackageManager;
    unlockZypper.supportedDistros = {"opensuse"};
    unlockZypper.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[unlockZypper.id] = unlockZypper;

    CommandMetadata unlockPacman;
    unlockPacman.id = "unlock_pacman";
    unlockPacman.command = "rm -f /var/lib/pacman/db.lck";
    unlockPacman.needsAdmin = true;
    unlockPacman.friendlyName = tr("解锁Pacman包管理器");
    unlockPacman.corePurpose = tr("删除pacman的锁文件，解决'无法获得锁'的错误。");
    unlockPacman.paramBreakdown = {
        {"rm -f", tr("强制删除")},
        {"/var/lib/pacman/db.lck", tr("Pacman数据库锁文件")}
    };
    unlockPacman.safetyLevel = SafetyLevel::Caution;
    unlockPacman.executionEffect = tr("pacman锁文件被删除，可以重新使用包管理器。");
    unlockPacman.commonPitfall = tr("先确认没有其他pacman进程在运行！强制解锁可能损坏数据库。");
    unlockPacman.category = CommandCategory::PackageManager;
    unlockPacman.supportedDistros = {"arch", "manjaro"};
    unlockPacman.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[unlockPacman.id] = unlockPacman;

    // ========== 网络类 ==========

    CommandMetadata networkRestart;
    networkRestart.id = "network_restart";
    networkRestart.command = "systemctl restart NetworkManager";
    networkRestart.needsAdmin = true;
    networkRestart.friendlyName = tr("重启网络服务");
    networkRestart.corePurpose = tr("重启NetworkManager网络管理服务，解决网络连接异常、WiFi找不到等问题。");
    networkRestart.paramBreakdown = {
        {"systemctl", tr("systemd服务管理工具")},
        {"restart", tr("重启服务")},
        {"NetworkManager", tr("网络管理服务")}
    };
    networkRestart.safetyLevel = SafetyLevel::Safe;
    networkRestart.executionEffect = tr("网络服务会重启，所有网络连接会断开后自动重连。");
    networkRestart.commonPitfall = tr("重启过程中网络会短暂中断，几秒钟后自动恢复。");
    networkRestart.category = CommandCategory::Network;
    networkRestart.supportedDistros = {"ubuntu", "fedora", "debian", "opensuse", "arch", "manjaro"};
    networkRestart.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[networkRestart.id] = networkRestart;

    CommandMetadata dnsFlush;
    dnsFlush.id = "dns_flush";
    dnsFlush.command = "resolvectl flush-caches 2>/dev/null || systemd-resolve --flush-caches";
    dnsFlush.needsAdmin = true;
    dnsFlush.friendlyName = tr("刷新DNS缓存");
    dnsFlush.corePurpose = tr("清空本地DNS缓存，解决网站打不开、域名解析错误等问题。");
    dnsFlush.paramBreakdown = {
        {"resolvectl", tr("systemd DNS解析服务管理工具")},
        {"flush-caches", tr("清空DNS缓存")},
        {"||", tr("如果前面的命令失败，尝试备用命令")},
        {"systemd-resolve", tr("旧版systemd的DNS工具（备用）")}
    };
    dnsFlush.safetyLevel = SafetyLevel::Safe;
    dnsFlush.executionEffect = tr("DNS缓存被清空，下次访问网站时会重新查询DNS。");
    dnsFlush.commonPitfall = tr("安全操作。如果某些网站打不开，可以尝试刷新DNS。");
    dnsFlush.category = CommandCategory::Network;
    dnsFlush.supportedDistros = {"ubuntu", "fedora", "debian", "opensuse", "arch", "manjaro"};
    dnsFlush.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[dnsFlush.id] = dnsFlush;

    CommandMetadata pingTest;
    pingTest.id = "ping_test";
    pingTest.command = "ping -c 4 1.1.1.1";
    pingTest.needsAdmin = false;
    pingTest.friendlyName = tr("网络连通性测试");
    pingTest.corePurpose = tr("测试网络连接是否正常，通过ping Cloudflare DNS 服务器(1.1.1.1)检测网络连通性和延迟。");
    pingTest.paramBreakdown = {
        {"ping", tr("ICMP网络诊断工具")},
        {"-c 4", tr("只发送4个数据包")},
        {"1.1.1.1", tr("测试目标地址（Cloudflare DNS）")}
    };
    pingTest.safetyLevel = SafetyLevel::Safe;
    pingTest.executionEffect = tr("显示网络延迟和丢包率，可以判断网络是否正常。");
    pingTest.commonPitfall = tr("不需要管理员权限。如果ping不通说明网络连接有问题。");
    pingTest.category = CommandCategory::Network;
    pingTest.supportedDistros = {"ubuntu", "fedora", "debian", "opensuse"};
    pingTest.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[pingTest.id] = pingTest;

    // ========== 硬件驱动类 ==========

    CommandMetadata nvidiaInstallUbuntu;
    nvidiaInstallUbuntu.id = "nvidia_install_ubuntu";
    nvidiaInstallUbuntu.command = "ubuntu-drivers autoinstall";
    nvidiaInstallUbuntu.needsAdmin = true;
    nvidiaInstallUbuntu.friendlyName = tr("自动安装NVIDIA驱动");
    nvidiaInstallUbuntu.corePurpose = tr("自动检测并安装推荐的NVIDIA显卡驱动，适用于Ubuntu和Debian系统。");
    nvidiaInstallUbuntu.paramBreakdown = {
        {"ubuntu-drivers", tr("Ubuntu驱动管理工具")},
        {"autoinstall", tr("自动安装推荐的驱动")}
    };
    nvidiaInstallUbuntu.safetyLevel = SafetyLevel::Caution;
    nvidiaInstallUbuntu.executionEffect = tr("系统会自动下载并安装推荐版本的NVIDIA驱动，安装完成后需要重启。");
    nvidiaInstallUbuntu.commonPitfall = tr("⚠️ 安装完成后必须重启电脑才能生效！安装过程中不要中断。");
    nvidiaInstallUbuntu.category = CommandCategory::Hardware;
    nvidiaInstallUbuntu.supportedDistros = {"ubuntu", "mint"};
    nvidiaInstallUbuntu.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[nvidiaInstallUbuntu.id] = nvidiaInstallUbuntu;

    CommandMetadata nvidiaInstallArch;
    nvidiaInstallArch.id = "nvidia_install_arch";
    nvidiaInstallArch.command = "pacman -S --noconfirm nvidia nvidia-utils";
    nvidiaInstallArch.needsAdmin = true;
    nvidiaInstallArch.friendlyName = tr("安装NVIDIA驱动");
    nvidiaInstallArch.corePurpose = tr("安装NVIDIA显卡驱动，适用于Arch和Manjaro系统。");
    nvidiaInstallArch.paramBreakdown = {
        {"pacman", tr("Arch Linux包管理器")},
        {"-S", tr("安装软件包")},
        {"nvidia", tr("NVIDIA内核驱动模块")},
        {"nvidia-utils", tr("NVIDIA用户态驱动库")},
        {"--noconfirm", tr("自动确认")}
    };
    nvidiaInstallArch.safetyLevel = SafetyLevel::Caution;
    nvidiaInstallArch.executionEffect = tr("NVIDIA驱动被安装，需要重启后生效。");
    nvidiaInstallArch.commonPitfall = tr("⚠️ 安装完成后必须重启！如果使用自定义内核，需要安装对应的nvidia-lts包。");
    nvidiaInstallArch.category = CommandCategory::Hardware;
    nvidiaInstallArch.supportedDistros = {"arch", "manjaro"};
    nvidiaInstallArch.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[nvidiaInstallArch.id] = nvidiaInstallArch;

    CommandMetadata nvidiaInstallFedora;
    nvidiaInstallFedora.id = "nvidia_install_fedora";
    nvidiaInstallFedora.command = "dnf install akmod-nvidia -y";
    nvidiaInstallFedora.needsAdmin = true;
    nvidiaInstallFedora.friendlyName = tr("安装NVIDIA驱动");
    nvidiaInstallFedora.corePurpose = tr("安装NVIDIA显卡驱动（akmod版本），适用于Fedora系统。需要先启用RPM Fusion源。");
    nvidiaInstallFedora.paramBreakdown = {
        {"dnf", tr("Fedora包管理器")},
        {"install", tr("安装")},
        {"akmod-nvidia", tr("NVIDIA驱动（自动内核模块编译版本）")},
        {"-y", tr("自动确认")}
    };
    nvidiaInstallFedora.safetyLevel = SafetyLevel::Caution;
    nvidiaInstallFedora.executionEffect = tr("NVIDIA驱动被安装，需要重启后生效。");
    nvidiaInstallFedora.commonPitfall = tr("⚠️ 必须先启用 RPM Fusion 源！安装后重启生效。内核模块编译需要几分钟。");
    nvidiaInstallFedora.category = CommandCategory::Hardware;
    nvidiaInstallFedora.supportedDistros = {"fedora"};
    nvidiaInstallFedora.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[nvidiaInstallFedora.id] = nvidiaInstallFedora;

    // ========== 备份恢复类 ==========

    CommandMetadata timeshiftCreate;
    timeshiftCreate.id = "timeshift_create";
    timeshiftCreate.command = "timeshift --create --comments \"Manual snapshot\"";
    timeshiftCreate.needsAdmin = true;
    timeshiftCreate.friendlyName = tr("创建系统快照");
    timeshiftCreate.corePurpose = tr("使用Timeshift创建系统快照，用于系统出问题时恢复到当前状态。");
    timeshiftCreate.paramBreakdown = {
        {"timeshift", tr("Linux系统快照工具")},
        {"--create", tr("创建快照")},
        {"--comments", tr("快照备注")}
    };
    timeshiftCreate.safetyLevel = SafetyLevel::Safe;
    timeshiftCreate.executionEffect = tr("系统快照被创建，占用一定磁盘空间。系统出问题时可以恢复到此状态。");
    timeshiftCreate.commonPitfall = tr("需要先安装timeshift并配置好。快照会占用磁盘空间。");
    timeshiftCreate.category = CommandCategory::BackupRestore;
    timeshiftCreate.supportedDistros = {"ubuntu", "fedora", "debian", "opensuse"};
    timeshiftCreate.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[timeshiftCreate.id] = timeshiftCreate;

    CommandMetadata timeshiftList;
    timeshiftList.id = "timeshift_list";
    timeshiftList.command = "timeshift --list";
    timeshiftList.needsAdmin = true;
    timeshiftList.friendlyName = tr("查看系统快照列表");
    timeshiftList.corePurpose = tr("列出所有已创建的Timeshift系统快照及其详细信息。");
    timeshiftList.paramBreakdown = {
        {"timeshift", tr("系统快照工具")},
        {"--list", tr("列出所有快照")}
    };
    timeshiftList.safetyLevel = SafetyLevel::Safe;
    timeshiftList.executionEffect = tr("显示所有系统快照的列表，包括日期、标签和占用空间。");
    timeshiftList.commonPitfall = tr("需要管理员权限才能查看完整的快照信息。");
    timeshiftList.category = CommandCategory::BackupRestore;
    timeshiftList.supportedDistros = {"ubuntu", "fedora", "debian", "opensuse"};
    timeshiftList.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[timeshiftList.id] = timeshiftList;

    CommandMetadata timeshiftRestoreLatest;
    timeshiftRestoreLatest.id = "timeshift_restore_latest";
    timeshiftRestoreLatest.command = "timeshift --restore --snapshot $(timeshift --list | grep -E '^[0-9]' | tail -1 | awk '{print $1}') --yes";
    timeshiftRestoreLatest.needsAdmin = true;
    timeshiftRestoreLatest.friendlyName = tr("恢复到最新快照");
    timeshiftRestoreLatest.corePurpose = tr("将系统恢复到最近一次创建的Timeshift快照状态。");
    timeshiftRestoreLatest.paramBreakdown = {
        {"timeshift --restore", tr("恢复系统")},
        {"--snapshot", tr("指定快照")},
        {"$(timeshift --list ...)", tr("自动获取最新快照名称")},
        {"--yes", tr("自动确认")}
    };
    timeshiftRestoreLatest.safetyLevel = SafetyLevel::Dangerous;
    timeshiftRestoreLatest.executionEffect = tr("系统会被恢复到最新快照的状态，之后创建的所有系统级更改都会丢失。");
    timeshiftRestoreLatest.commonPitfall = tr("⚠️ 危险操作！恢复后系统会回到快照时的状态，个人文件不受影响但系统配置会回退。");
    timeshiftRestoreLatest.category = CommandCategory::BackupRestore;
    timeshiftRestoreLatest.supportedDistros = {"ubuntu", "fedora", "debian", "opensuse"};
    timeshiftRestoreLatest.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[timeshiftRestoreLatest.id] = timeshiftRestoreLatest;

    // ========== 双系统类 ==========

    CommandMetadata dualbootTimeFix;
    dualbootTimeFix.id = "dualboot_time_fix";
    dualbootTimeFix.command = "timedatectl set-local-rtc 1 --adjust-system-clock";
    dualbootTimeFix.needsAdmin = true;
    dualbootTimeFix.friendlyName = tr("修复双系统时间错乱");
    dualbootTimeFix.corePurpose = tr("将Linux的硬件时钟设置为本地时间，解决Windows和Linux双系统时间不一致的问题。");
    dualbootTimeFix.paramBreakdown = {
        {"timedatectl", tr("systemd时间管理工具")},
        {"set-local-rtc 1", tr("将硬件时钟设置为本地时间")},
        {"--adjust-system-clock", tr("基于设置调整系统时钟")}
    };
    dualbootTimeFix.safetyLevel = SafetyLevel::Safe;
    dualbootTimeFix.executionEffect = tr("Linux使用本地时间模式，与Windows保持一致，双系统时间不再错乱。");
    dualbootTimeFix.commonPitfall = tr("修改后建议重启进入Windows确认时间是否正确。");
    dualbootTimeFix.category = CommandCategory::DualBoot;
    dualbootTimeFix.supportedDistros = {"ubuntu", "fedora", "debian", "opensuse"};
    dualbootTimeFix.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[dualbootTimeFix.id] = dualbootTimeFix;

    CommandMetadata ntfsMountFix;
    ntfsMountFix.id = "ntfs_mount_fix";
    ntfsMountFix.command = "lsblk -o NAME,FSTYPE,LABEL,SIZE -n | grep -i ntfs";
    ntfsMountFix.needsAdmin = false;
    ntfsMountFix.friendlyName = tr("检测NTFS分区");
    ntfsMountFix.corePurpose = tr("列出系统中的NTFS分区，帮助定位Windows分区。检测到后请使用 sudo ntfsfix <分区路径> 修复（例如 sudo ntfsfix /dev/nvme0n1p2）。");
    ntfsMountFix.paramBreakdown = {
        {"lsblk", tr("列出块设备")},
        {"grep -i ntfs", tr("筛选NTFS分区")}
    };
    ntfsMountFix.safetyLevel = SafetyLevel::Safe;
    ntfsMountFix.executionEffect = tr("显示系统中的NTFS分区列表，用户可据此手动运行 ntfsfix 修复。");
    ntfsMountFix.commonPitfall = tr("本命令仅检测，不会修改任何分区。修复需手动执行 sudo ntfsfix <设备路径>。");
    ntfsMountFix.category = CommandCategory::DualBoot;
    ntfsMountFix.supportedDistros = {"ubuntu", "fedora", "debian", "opensuse"};
    ntfsMountFix.supportedDesktops = {"gnome", "kde", "xfce"};
    m_commands[ntfsMountFix.id] = ntfsMountFix;
}

CommandMetadata CommandMetadataManager::getCommand(const QString& id) const
{
    return m_commands.value(id);
}

QList<CommandMetadata> CommandMetadataManager::getAllCommands() const
{
    return m_commands.values();
}

QList<CommandMetadata> CommandMetadataManager::getCommandsByCategory(CommandCategory category) const
{
    QList<CommandMetadata> result;
    for (const auto& cmd : m_commands) {
        if (cmd.category == category) {
            result.append(cmd);
        }
    }
    return result;
}

QList<CommandMetadata> CommandMetadataManager::searchCommands(const QString& keyword) const
{
    QList<CommandMetadata> result;
    QString kw = keyword.toLower();
    for (const auto& cmd : m_commands) {
        if (cmd.friendlyName.toLower().contains(kw) ||
            cmd.command.toLower().contains(kw) ||
            cmd.corePurpose.toLower().contains(kw)) {
            result.append(cmd);
        }
    }
    return result;
}

QString CommandMetadataManager::safetyLevelText(SafetyLevel level) const
{
    switch (level) {
    case SafetyLevel::Safe: return tr("完全安全");
    case SafetyLevel::Caution: return tr("谨慎操作");
    case SafetyLevel::Dangerous: return tr("危险操作");
    }
    return tr("未知");
}

QString CommandMetadataManager::safetyLevelIcon(SafetyLevel level) const
{
    switch (level) {
    case SafetyLevel::Safe: return "✅";
    case SafetyLevel::Caution: return "⚠️";
    case SafetyLevel::Dangerous: return "❌";
    }
    return "❓";
}

QString CommandMetadataManager::categoryText(CommandCategory category) const
{
    switch (category) {
    case CommandCategory::SystemUpdate: return tr("系统更新");
    case CommandCategory::SystemCleanup: return tr("系统清理");
    case CommandCategory::DesktopRepair: return tr("桌面修复");
    case CommandCategory::PackageManager: return tr("包管理器");
    case CommandCategory::FileOperation: return tr("文件操作");
    case CommandCategory::Network: return tr("网络相关");
    case CommandCategory::Hardware: return tr("硬件驱动");
    case CommandCategory::BackupRestore: return tr("备份恢复");
    case CommandCategory::DualBoot: return tr("双系统");
    }
    return tr("其他");
}
