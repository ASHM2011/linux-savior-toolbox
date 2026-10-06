#include "PermissionManager.h"
#include "CommandMetadata.h"

PermissionManager* PermissionManager::s_instance = nullptr;

PermissionManager* PermissionManager::instance()
{
    if (!s_instance) {
        s_instance = new PermissionManager();
    }
    return s_instance;
}

PermissionManager::PermissionManager()
{
    initForbiddenCommands();
}

void PermissionManager::initForbiddenCommands()
{
    m_forbiddenAdminCommands["gnome_shell_restart"] = true;
    m_forbiddenAdminCommands["nautilus_restart"] = true;
    m_forbiddenAdminCommands["rm_trash"] = true;
    m_forbiddenAdminCommands["ibus_restart"] = true;
    m_forbiddenAdminCommands["plasma_restart"] = true;
    m_forbiddenAdminCommands["xfce4_restart"] = true;
    m_forbiddenAdminCommands["dolphin_restart"] = true;
    m_forbiddenAdminCommands["fix_trash_permission"] = true;
    m_forbiddenAdminCommands["clean_thumbnails"] = true;
    m_forbiddenAdminCommands["clean_browser_cache"] = true;
    m_forbiddenAdminCommands["ping_test"] = true;
    m_forbiddenAdminCommands["dnf_update_only"] = true;
}

PermissionCheckResult PermissionManager::checkCommandPermission(const QString& commandId) const
{
    if (isForbiddenAdminCommand(commandId)) {
        return PermissionCheckResult::ForbiddenAdmin;
    }

    auto cmd = CommandMetadataManager::instance()->getCommand(commandId);
    if (cmd.needsAdmin) {
        return PermissionCheckResult::NeedsAdmin;
    }

    return PermissionCheckResult::Allowed;
}

bool PermissionManager::commandNeedsAdmin(const QString& commandId) const
{
    auto cmd = CommandMetadataManager::instance()->getCommand(commandId);
    return cmd.needsAdmin && !isForbiddenAdminCommand(commandId);
}

bool PermissionManager::isForbiddenAdminCommand(const QString& commandId) const
{
    return m_forbiddenAdminCommands.contains(commandId) && m_forbiddenAdminCommands[commandId];
}

QString PermissionManager::getPermissionWarning(const QString& commandId) const
{
    if (isForbiddenAdminCommand(commandId)) {
        return tr("此操作绝对不需要管理员权限！如果使用sudo执行可能会导致权限问题。");
    }
    auto cmd = CommandMetadataManager::instance()->getCommand(commandId);
    if (cmd.needsAdmin) {
        return tr("此操作需要管理员权限，执行时会弹出密码确认框。");
    }
    return tr("此操作不需要管理员权限，可直接执行。");
}
