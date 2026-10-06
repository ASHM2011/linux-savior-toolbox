#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QShortcut>
#include <QMap>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QParallelAnimationGroup>
#include <QSettings>

#include "widgets/SidebarWidget.h"
#include "widgets/TopBarWidget.h"

class DashboardWidget;
class EmergencyRepairWidget;
class TerminalWidget;
class SystemCleanupWidget;
class BatchTaskWidget;
class GnomeExtensionWidget;
class BackupRestoreWidget;
class DiagnosticReportWidget;
class CommandAliasWidget;
class CommandReferenceWidget;
class DualBootWidget;
class SettingsWidget;
class ServiceManagerWidget;
class ProcessManagerWidget;
class NetworkDiagnosticWidget;
class LogViewerWidget;
class SystemInfoWidget;
class StartupManagerWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    Q_PROPERTY(int slideOffset READ slideOffset WRITE setSlideOffset)
    Q_PROPERTY(qreal pageOpacity READ pageOpacity WRITE setPageOpacity)

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    int slideOffset() const { return m_slideOffset; }
    void setSlideOffset(int offset);
    qreal pageOpacity() const { return m_pageOpacity; }
    void setPageOpacity(qreal opacity);
    void toggleTheme();

private slots:
    void onPageChanged(const QString& pageId);
    void onCommandTriggered(const QString& commandId);
    void onShowCommandDetail(const QString& commandId);
    void onExecuteInTerminal(const QString& commandId);
    void onBatchStarted(const QStringList& commandIds);
    void onSearchTextChanged(const QString& text);
    void onSystemDetectionFinished();
    void onThemeToggled();

private:
    void setupUI();
    void setupConnections();
    void setupShortcuts();
    QWidget* getOrCreatePage(const QString& pageId);
    void showProgressDialog(const QString& commandId);
    void connectPageSignals(QWidget* page);
    void animatePageSwitch(int fromIndex, int toIndex);
    void loadTheme();
    void applyTheme(bool dark);
    void resizeEvent(QResizeEvent* event) override;

    SidebarWidget* m_sidebar;
    TopBarWidget* m_topbar;
    QStackedWidget* m_contentStack;
    QFrame* m_contentContainer;

    QMap<QString, QWidget*> m_pages;
    
    DashboardWidget* m_dashboard;
    EmergencyRepairWidget* m_emergency;
    TerminalWidget* m_terminal;
    SystemCleanupWidget* m_cleanup;
    BatchTaskWidget* m_batchTask;
    GnomeExtensionWidget* m_extensions;
    BackupRestoreWidget* m_backup;
    DiagnosticReportWidget* m_diagnostic;
    CommandAliasWidget* m_alias;
    CommandReferenceWidget* m_reference;
    DualBootWidget* m_dualboot;
    SettingsWidget* m_settings;
    ServiceManagerWidget* m_serviceManager;
    ProcessManagerWidget* m_processManager;
    NetworkDiagnosticWidget* m_networkDiagnostic;
    LogViewerWidget* m_logViewer;
    SystemInfoWidget* m_systemInfo;
    StartupManagerWidget* m_startupManager;
    
    QList<QShortcut*> m_shortcuts;
    bool m_systemDetected;

    QPropertyAnimation* m_slideAnim;
    QPropertyAnimation* m_opacityAnim;
    QParallelAnimationGroup* m_animGroup;
    QGraphicsOpacityEffect* m_opacityEffect;
    bool m_isAnimating;
    int m_slideOffset;
    qreal m_pageOpacity;
    bool m_darkTheme;
    QSettings* m_appSettings;
    int m_previousIndex;
};

#endif
