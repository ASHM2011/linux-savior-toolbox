#ifndef COMMANDMETADATA_H
#define COMMANDMETADATA_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QObject>

enum class SafetyLevel {
    Safe = 0,
    Caution = 1,
    Dangerous = 2
};

enum class CommandCategory {
    SystemUpdate,
    SystemCleanup,
    DesktopRepair,
    PackageManager,
    FileOperation,
    Network,
    Hardware,
    BackupRestore,
    DualBoot
};

struct CommandParam {
    QString param;
    QString description;
};

struct CommandMetadata {
    QString id;
    QString command;
    bool needsAdmin;
    QString friendlyName;
    QString corePurpose;
    QList<CommandParam> paramBreakdown;
    SafetyLevel safetyLevel;
    QString executionEffect;
    QString commonPitfall;
    CommandCategory category;
    QStringList supportedDistros;
    QStringList supportedDesktops;
};

class CommandMetadataManager : public QObject
{
    Q_OBJECT

public:
    static CommandMetadataManager* instance();

    CommandMetadata getCommand(const QString& id) const;
    QList<CommandMetadata> getAllCommands() const;
    QList<CommandMetadata> getCommandsByCategory(CommandCategory category) const;
    QList<CommandMetadata> searchCommands(const QString& keyword) const;

    QString safetyLevelText(SafetyLevel level) const;
    QString safetyLevelIcon(SafetyLevel level) const;
    QString categoryText(CommandCategory category) const;

private:
    CommandMetadataManager();
    void initCommands();

    QMap<QString, CommandMetadata> m_commands;
    static CommandMetadataManager* s_instance;
};

#endif
