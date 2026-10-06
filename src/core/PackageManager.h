#ifndef PACKAGEMANAGER_H
#define PACKAGEMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>

#include "SystemDetector.h"

class PackageManager : public QObject
{
    Q_OBJECT

public:
    enum class Action {
        UpdateSystem,
        CleanCache,
        Autoremove,
        InstallPackage,
        RemovePackage,
        FixDependencies,
        UnlockPackageManager,
        CheckForUpdates
    };
    Q_ENUM(Action)

    static PackageManager* instance();

    explicit PackageManager(QObject *parent = nullptr);
    ~PackageManager() override;

    void updateSystem();
    void cleanCache();
    void autoremove();
    void installPackage(const QString &packageName);
    void removePackage(const QString &packageName);
    void fixDependencies();
    void unlockPackageManager();
    void checkForUpdates();
    void getInstalledPackages();

    bool isRunning() const;
    QString commandForAction(Action action, const QString &packageName = QString()) const;

signals:
    void outputReady(const QString &output);
    void errorReady(const QString &error);
    void finished(int exitCode);
    void started();
    void installedPackagesReady(const QStringList &packages);

private slots:
    void onOutputReceived(const QString &output);
    void onErrorReceived(const QString &error);
    void onExecutionFinished(const QString &commandId, bool success, const QString &message);

private:
    void executeAction(Action action, const QString &packageName = QString());
    void buildCommandMap();
    void parseInstalledPackages(const QString &output);

    static PackageManager* s_instance;

    SystemDetector* m_systemDetector;

    QMap<Action, QString> m_commandMap;
    QStringList m_accumulatedOutput;
    bool m_parsingInstalledPackages;
};

#endif
