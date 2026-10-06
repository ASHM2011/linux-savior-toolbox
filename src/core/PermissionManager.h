#ifndef PERMISSIONMANAGER_H
#define PERMISSIONMANAGER_H

#include <QObject>
#include <QString>
#include <QMap>

enum class PermissionCheckResult {
    Allowed,
    ForbiddenAdmin,
    NeedsAdmin
};

class PermissionManager : public QObject
{
    Q_OBJECT

public:
    static PermissionManager* instance();

    PermissionCheckResult checkCommandPermission(const QString& commandId) const;
    bool commandNeedsAdmin(const QString& commandId) const;
    bool isForbiddenAdminCommand(const QString& commandId) const;

    QString getPermissionWarning(const QString& commandId) const;

private:
    PermissionManager();
    void initForbiddenCommands();

    QMap<QString, bool> m_forbiddenAdminCommands;
    static PermissionManager* s_instance;
};

#endif
