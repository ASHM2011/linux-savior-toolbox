#include "MainWindow.h"
#include "core/CommandExecutor.h"
#include "core/CommandMetadata.h"
#include "core/SystemDetector.h"
#include "core/PackageManager.h"
#include "widgets/CommandDetailDialog.h"
#include "widgets/ProgressDialog.h"
#include "widgets/DashboardWidget.h"
#include "widgets/EmergencyRepairWidget.h"
#include "widgets/TerminalWidget.h"
#include "widgets/SystemCleanupWidget.h"
#include "widgets/BatchTaskWidget.h"
#include "widgets/GnomeExtensionWidget.h"
#include "widgets/BackupRestoreWidget.h"
#include "widgets/DiagnosticReportWidget.h"
#include "widgets/CommandAliasWidget.h"
#include "widgets/CommandReferenceWidget.h"
#include "widgets/DualBootWidget.h"
#include "widgets/SettingsWidget.h"
#include "widgets/ServiceManagerWidget.h"
#include "widgets/ProcessManagerWidget.h"
#include "widgets/NetworkDiagnosticWidget.h"
#include "widgets/LogViewerWidget.h"
#include "widgets/SystemInfoWidget.h"
#include "widgets/StartupManagerWidget.h"
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStatusBar>
#include <QTimer>
#include <QKeySequence>
#include <QLabel>
#include <QApplication>
#include <QEasingCurve>
#include <QResizeEvent>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_sidebar(nullptr)
    , m_topbar(nullptr)
    , m_contentStack(nullptr)
    , m_contentContainer(nullptr)
    , m_dashboard(nullptr)
    , m_emergency(nullptr)
    , m_terminal(nullptr)
    , m_cleanup(nullptr)
    , m_batchTask(nullptr)
    , m_extensions(nullptr)
    , m_backup(nullptr)
    , m_diagnostic(nullptr)
    , m_alias(nullptr)
    , m_reference(nullptr)
    , m_dualboot(nullptr)
    , m_settings(nullptr)
    , m_serviceManager(nullptr)
    , m_processManager(nullptr)
    , m_networkDiagnostic(nullptr)
    , m_logViewer(nullptr)
    , m_systemInfo(nullptr)
    , m_startupManager(nullptr)
    , m_systemDetected(false)
    , m_slideAnim(nullptr)
    , m_opacityAnim(nullptr)
    , m_animGroup(nullptr)
    , m_opacityEffect(nullptr)
    , m_isAnimating(false)
    , m_slideOffset(0)
    , m_pageOpacity(1.0)
    , m_darkTheme(false)
    , m_appSettings(nullptr)
    , m_previousIndex(0)
{
    setWindowTitle(tr("Linux Savior Toolbox v%1 - Linux萌新救星工具箱").arg(QCoreApplication::applicationVersion()));
    resize(1200, 800);
    setMinimumSize(860, 600);

    m_appSettings = new QSettings("LinuxSavior", "Toolbox", this);

    setupUI();
    setupConnections();
    setupShortcuts();
    loadTheme();
    
    statusBar()->showMessage(tr("正在检测系统信息..."));
    statusBar()->setStyleSheet("QStatusBar { font-weight: 500; }");
    
    auto detector = SystemDetector::instance();
    connect(detector, &SystemDetector::detectionFinished, this, &MainWindow::onSystemDetectionFinished);
    connect(detector, &SystemDetector::detectionProgress, this, [this](const QString& status) {
        statusBar()->showMessage(status);
    });
    detector->detectAllAsync();
    
    m_dashboard = new DashboardWidget();
    m_contentStack->addWidget(m_dashboard);
    m_pages["dashboard"] = m_dashboard;
    m_contentStack->setCurrentWidget(m_dashboard);
    
    QTimer::singleShot(0, this, [this]() {
        connectPageSignals(m_dashboard);
    });
}

MainWindow::~MainWindow()
{
    qDeleteAll(m_shortcuts);
}

void MainWindow::setSlideOffset(int offset)
{
    m_slideOffset = offset;
    if (m_contentStack && m_contentStack->currentWidget()) {
        QWidget* w = m_contentStack->currentWidget();
        w->setGeometry(offset, 0, m_contentStack->width(), m_contentStack->height());
    }
}

void MainWindow::setPageOpacity(qreal opacity)
{
    m_pageOpacity = opacity;
    if (m_opacityEffect) {
        m_opacityEffect->setOpacity(opacity);
    }
}

void MainWindow::toggleTheme()
{
    m_darkTheme = !m_darkTheme;
    applyTheme(m_darkTheme);
    m_appSettings->setValue("ui/theme", m_darkTheme ? "dark" : "light");
    m_appSettings->sync();
    if (m_topbar) {
        m_topbar->updateThemeButton(m_darkTheme);
    }
}

void MainWindow::applyTheme(bool dark)
{
    m_darkTheme = dark;
    if (dark) {
        this->setProperty("theme", "dark");
    } else {
        this->setProperty("theme", "light");
    }
    this->style()->unpolish(this);
    this->style()->polish(this);
    this->update();
    QApplication::setPalette(QApplication::palette());
}

void MainWindow::loadTheme()
{
    QString theme = m_appSettings->value("ui/theme", "light").toString();
    m_darkTheme = (theme == "dark");
    applyTheme(m_darkTheme);
    if (m_topbar) {
        m_topbar->updateThemeButton(m_darkTheme);
    }
}

void MainWindow::setupUI()
{
    QWidget* centralWidget = new QWidget();
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_sidebar = new SidebarWidget();
    mainLayout->addWidget(m_sidebar);

    QVBoxLayout* rightLayout = new QVBoxLayout();
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    m_topbar = new TopBarWidget();
    rightLayout->addWidget(m_topbar);

    m_contentContainer = new QFrame();
    m_contentContainer->setObjectName("contentContainer");
    QHBoxLayout* contentContainerLayout = new QHBoxLayout(m_contentContainer);
    contentContainerLayout->setContentsMargins(24, 24, 24, 24);
    contentContainerLayout->setSpacing(0);

    m_contentStack = new QStackedWidget();
    m_contentStack->setObjectName("contentStack");
    m_contentStack->setStyleSheet("background-color: transparent;");

    m_opacityEffect = new QGraphicsOpacityEffect(m_contentStack);
    m_opacityEffect->setOpacity(1.0);
    m_contentStack->setGraphicsEffect(m_opacityEffect);

    contentContainerLayout->addWidget(m_contentStack);
    rightLayout->addWidget(m_contentContainer, 1);

    QFrame* rightFrame = new QFrame();
    rightFrame->setLayout(rightLayout);
    mainLayout->addWidget(rightFrame, 1);

    setCentralWidget(centralWidget);

    m_slideAnim = new QPropertyAnimation(this, "slideOffset", this);
    m_opacityAnim = new QPropertyAnimation(this, "pageOpacity", this);
    m_animGroup = new QParallelAnimationGroup(this);
    m_animGroup->addAnimation(m_opacityAnim);
}

void MainWindow::setupConnections()
{
    connect(m_sidebar, &SidebarWidget::pageChanged, this, &MainWindow::onPageChanged);
    connect(m_topbar, &TopBarWidget::searchTextChanged, this, &MainWindow::onSearchTextChanged);
    connect(m_topbar, &TopBarWidget::themeToggled, this, &MainWindow::onThemeToggled);

    auto executor = CommandExecutor::instance();
    connect(executor, &CommandExecutor::executionStarted, this, [this](const QString&, const QString& name) {
        statusBar()->showMessage(tr("正在执行: ") + name);
    });
    connect(executor, &CommandExecutor::executionFinished, this, [this](const QString&, bool success, const QString& msg) {
        statusBar()->showMessage(success ? "✅ " + msg : "❌ " + msg, 5000);
    });
}

void MainWindow::setupShortcuts()
{
    QShortcut* quitShortcut = new QShortcut(QKeySequence("Ctrl+Q"), this);
    connect(quitShortcut, &QShortcut::activated, this, &QWidget::close);
    m_shortcuts.append(quitShortcut);
    
    QShortcut* searchShortcut = new QShortcut(QKeySequence("Ctrl+F"), this);
    connect(searchShortcut, &QShortcut::activated, m_topbar, [this]() {
        if (m_topbar) {
            QWidget* searchWidget = m_topbar->findChild<QWidget*>("searchInput");
            if (searchWidget) searchWidget->setFocus();
        }
    });
    m_shortcuts.append(searchShortcut);
    
    for (int i = 1; i <= 9; i++) {
        QShortcut* numShortcut = new QShortcut(QKeySequence(QString("Alt+%1").arg(i)), this);
        int pageIndex = i - 1;
        connect(numShortcut, &QShortcut::activated, this, [this, pageIndex]() {
            QStringList pageOrder = {"dashboard", "emergency", "systemInfo", "terminal", "cleanup", "startup",
                                     "extensions", "backup", "diagnostic", "logs", "alias", "reference",
                                     "dualboot", "services", "processes", "network", "settings"};
            if (pageIndex < pageOrder.size()) {
                onPageChanged(pageOrder[pageIndex]);
                if (m_sidebar) {
                    m_sidebar->setCurrentPage(pageOrder[pageIndex]);
                }
            }
        });
        m_shortcuts.append(numShortcut);
    }
    
    QShortcut* newTabShortcut = new QShortcut(QKeySequence("Ctrl+T"), this);
    connect(newTabShortcut, &QShortcut::activated, this, [this]() {
        onPageChanged("terminal");
        if (m_sidebar) m_sidebar->setCurrentPage("terminal");
        QTimer::singleShot(350, this, [this]() {
            if (m_terminal) m_terminal->newTab();
        });
    });
    m_shortcuts.append(newTabShortcut);
    
    QShortcut* closeTabShortcut = new QShortcut(QKeySequence("Ctrl+W"), this);
    connect(closeTabShortcut, &QShortcut::activated, this, [this]() {
        onPageChanged("dashboard");
        if (m_sidebar) m_sidebar->setCurrentPage("dashboard");
    });
    m_shortcuts.append(closeTabShortcut);
    
    QShortcut* helpShortcut = new QShortcut(QKeySequence("F1"), this);
    connect(helpShortcut, &QShortcut::activated, this, [this]() {
        QMessageBox::information(this, tr("快捷键帮助"),
            tr("快捷键列表：\n\n"
               "Ctrl+F - 搜索\n"
               "Ctrl+Q - 退出\n"
               "Ctrl+T - 新建终端标签\n"
               "Alt+1~9 - 切换页面\n"
               "F1 - 显示帮助\n"
               "Ctrl+Shift+C - 终端复制\n"
               "Ctrl+Shift+V - 终端粘贴\n"
               "Ctrl+L - 清屏")
        );
    });
    m_shortcuts.append(helpShortcut);
}

void MainWindow::connectPageSignals(QWidget* page)
{
    if (!page) return;

    if (auto dashboard = qobject_cast<DashboardWidget*>(page)) {
        connect(dashboard, &DashboardWidget::commandTriggered, this, &MainWindow::onCommandTriggered);
        connect(dashboard, &DashboardWidget::showCommandDetail, this, &MainWindow::onShowCommandDetail);
        connect(dashboard, &DashboardWidget::executeInTerminal, this, &MainWindow::onExecuteInTerminal);
    } else if (auto emergency = qobject_cast<EmergencyRepairWidget*>(page)) {
        connect(emergency, &EmergencyRepairWidget::commandTriggered, this, &MainWindow::onCommandTriggered);
        connect(emergency, &EmergencyRepairWidget::showCommandDetail, this, &MainWindow::onShowCommandDetail);
        connect(emergency, &EmergencyRepairWidget::executeInTerminal, this, &MainWindow::onExecuteInTerminal);
    } else if (auto terminal = qobject_cast<TerminalWidget*>(page)) {
        connect(terminal, &TerminalWidget::commandTriggered, this, &MainWindow::onCommandTriggered);
    } else if (auto cleanup = qobject_cast<SystemCleanupWidget*>(page)) {
        connect(cleanup, &SystemCleanupWidget::commandTriggered, this, &MainWindow::onCommandTriggered);
        connect(cleanup, &SystemCleanupWidget::showCommandDetail, this, &MainWindow::onShowCommandDetail);
    } else if (auto batchTask = qobject_cast<BatchTaskWidget*>(page)) {
        connect(batchTask, &BatchTaskWidget::batchStarted, this, &MainWindow::onBatchStarted);
        connect(batchTask, &BatchTaskWidget::commandTriggered, this, &MainWindow::onCommandTriggered);
    } else if (auto reference = qobject_cast<CommandReferenceWidget*>(page)) {
        connect(reference, &CommandReferenceWidget::showCommandDetail, this, &MainWindow::onShowCommandDetail);
    } else if (auto serviceMgr = qobject_cast<ServiceManagerWidget*>(page)) {
        connect(serviceMgr, &ServiceManagerWidget::commandTriggered, this, &MainWindow::onCommandTriggered);
    } else if (auto processMgr = qobject_cast<ProcessManagerWidget*>(page)) {
        connect(processMgr, &ProcessManagerWidget::commandTriggered, this, &MainWindow::onCommandTriggered);
    } else if (auto networkDiag = qobject_cast<NetworkDiagnosticWidget*>(page)) {
        connect(networkDiag, &NetworkDiagnosticWidget::commandTriggered, this, &MainWindow::onCommandTriggered);
    } else if (auto logViewer = qobject_cast<LogViewerWidget*>(page)) {
        connect(logViewer, &LogViewerWidget::commandTriggered, this, &MainWindow::onCommandTriggered);
    } else if (auto settings = qobject_cast<SettingsWidget*>(page)) {
        connect(settings, &SettingsWidget::navigateTo, this, [this](const QString& pageId) {
            onPageChanged(pageId);
            if (m_sidebar) m_sidebar->setCurrentPage(pageId);
        });
    }
}

QWidget* MainWindow::getOrCreatePage(const QString& pageId)
{
    if (m_pages.contains(pageId)) {
        return m_pages[pageId];
    }
    
    QWidget* page = nullptr;
    
    if (pageId == "emergency") {
        m_emergency = new EmergencyRepairWidget();
        page = m_emergency;
    } else if (pageId == "systemInfo") {
        m_systemInfo = new SystemInfoWidget();
        page = m_systemInfo;
    } else if (pageId == "terminal") {
        m_terminal = new TerminalWidget();
        page = m_terminal;
    } else if (pageId == "cleanup") {
        m_cleanup = new SystemCleanupWidget();
        page = m_cleanup;
    } else if (pageId == "startup") {
        m_startupManager = new StartupManagerWidget();
        page = m_startupManager;
    } else if (pageId == "batch") {
        m_batchTask = new BatchTaskWidget();
        page = m_batchTask;
    } else if (pageId == "extensions") {
        m_extensions = new GnomeExtensionWidget();
        page = m_extensions;
    } else if (pageId == "backup") {
        m_backup = new BackupRestoreWidget();
        page = m_backup;
    } else if (pageId == "diagnostic") {
        m_diagnostic = new DiagnosticReportWidget();
        page = m_diagnostic;
    } else if (pageId == "logs") {
        m_logViewer = new LogViewerWidget();
        page = m_logViewer;
    } else if (pageId == "alias") {
        m_alias = new CommandAliasWidget();
        page = m_alias;
    } else if (pageId == "reference") {
        m_reference = new CommandReferenceWidget();
        page = m_reference;
    } else if (pageId == "dualboot") {
        m_dualboot = new DualBootWidget();
        page = m_dualboot;
    } else if (pageId == "settings") {
        m_settings = new SettingsWidget();
        page = m_settings;
    } else if (pageId == "services") {
        m_serviceManager = new ServiceManagerWidget();
        page = m_serviceManager;
    } else if (pageId == "processes") {
        m_processManager = new ProcessManagerWidget();
        page = m_processManager;
    } else if (pageId == "network") {
        m_networkDiagnostic = new NetworkDiagnosticWidget();
        page = m_networkDiagnostic;
    }
    
    if (page) {
        m_contentStack->addWidget(page);
        m_pages[pageId] = page;
        QTimer::singleShot(0, this, [this, page]() {
            connectPageSignals(page);
        });
    }
    
    return page;
}

void MainWindow::animatePageSwitch(int fromIndex, int toIndex)
{
    if (m_isAnimating || fromIndex == toIndex) return;
    m_isAnimating = true;
    m_previousIndex = fromIndex;

    // 仅使用透明度淡入淡出，避免操作子控件几何位置与 QStackedLayout 冲突导致错位
    m_opacityAnim->setDuration(150);
    m_opacityAnim->setStartValue(1.0);
    m_opacityAnim->setEndValue(0.0);
    m_opacityAnim->setEasingCurve(QEasingCurve::OutCubic);

    m_animGroup->disconnect();
    m_animGroup->start();

    connect(m_animGroup, &QParallelAnimationGroup::finished, this, [this, toIndex]() {
        m_contentStack->setCurrentIndex(toIndex);

        m_pageOpacity = 0.0;
        m_opacityEffect->setOpacity(0.0);

        QPropertyAnimation* fadeIn = new QPropertyAnimation(this, "pageOpacity");
        fadeIn->setDuration(180);
        fadeIn->setStartValue(0.0);
        fadeIn->setEndValue(1.0);
        fadeIn->setEasingCurve(QEasingCurve::OutCubic);

        connect(fadeIn, &QPropertyAnimation::finished, this, [this]() {
            m_isAnimating = false;
            m_pageOpacity = 1.0;
            // 确保当前页几何位置正确
            if (m_contentStack->currentWidget()) {
                QWidget* w = m_contentStack->currentWidget();
                w->setGeometry(0, 0, m_contentStack->width(), m_contentStack->height());
            }
        });

        fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
    }, Qt::SingleShotConnection);
}

void MainWindow::onSystemDetectionFinished()
{
    m_systemDetected = true;
    statusBar()->showMessage(tr("系统运行正常 | 所有功能已就绪"), 5000);
}

void MainWindow::onPageChanged(const QString& pageId)
{
    QWidget* page = getOrCreatePage(pageId);
    if (page) {
        int targetIndex = m_contentStack->indexOf(page);
        int currentIndex = m_contentStack->currentIndex();
        
        if (targetIndex != currentIndex && targetIndex >= 0) {
            animatePageSwitch(currentIndex, targetIndex);
        } else if (targetIndex >= 0) {
            m_contentStack->setCurrentWidget(page);
            // 直接切换时强制复位，防止上一次动画残留的偏移导致错位
            if (m_contentStack->currentWidget()) {
                QWidget* w = m_contentStack->currentWidget();
                w->setGeometry(0, 0, m_contentStack->width(), m_contentStack->height());
            }
        }
    }
}

void MainWindow::onThemeToggled()
{
    toggleTheme();
}

void MainWindow::onCommandTriggered(const QString& commandId)
{
    auto cmd = CommandMetadataManager::instance()->getCommand(commandId);
    if (cmd.id.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("未知命令"));
        return;
    }

    if (cmd.safetyLevel == SafetyLevel::Dangerous || cmd.safetyLevel == SafetyLevel::Caution) {
        auto ret = QMessageBox::warning(this, tr("⚠️ 操作确认"),
            QString(tr("你确定要执行「%1」吗？\n\n%2\n\n%3"))
                .arg(cmd.friendlyName, cmd.corePurpose, cmd.commonPitfall),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);

        if (ret != QMessageBox::Yes) return;
    }

    showProgressDialog(commandId);
}

void MainWindow::onShowCommandDetail(const QString& commandId)
{
    CommandDetailDialog dlg(commandId, this);
    connect(&dlg, &CommandDetailDialog::executeRequested, this, &MainWindow::onCommandTriggered);
    connect(&dlg, &CommandDetailDialog::executeInTerminalRequested, this, &MainWindow::onExecuteInTerminal);
    dlg.exec();
}

void MainWindow::onExecuteInTerminal(const QString& commandId)
{
    m_sidebar->setCurrentPage("terminal");
    onPageChanged("terminal");
    QTimer::singleShot(350, this, [this, commandId]() {
        if (m_terminal) {
            m_terminal->executeCommand(commandId);
        }
    });
}

void MainWindow::onBatchStarted(const QStringList& commandIds)
{
    if (commandIds.isEmpty()) return;

    QStringList names;
    for (const QString& id : commandIds) {
        auto cmd = CommandMetadataManager::instance()->getCommand(id);
        if (!cmd.friendlyName.isEmpty()) {
            names.append(cmd.friendlyName);
        }
    }

    auto ret = QMessageBox::question(this, tr("批量任务确认"),
        QString(tr("即将执行以下 %1 个操作：\n\n%2\n\n确定要开始吗？"))
            .arg(commandIds.size())
            .arg(names.join("\n")),
        QMessageBox::Yes | QMessageBox::No);

    if (ret != QMessageBox::Yes) return;

    ProgressDialog dlg(tr("批量任务执行中"), this);
    dlg.setWindowTitle(tr("批量任务"));
    auto executor = CommandExecutor::instance();

    bool* finished = new bool(false);

    connect(executor, &CommandExecutor::batchStarted, &dlg, [&dlg](int total) {
        dlg.setProgress(0, QString(tr("共 %1 个任务")).arg(total));
    });

    connect(executor, &CommandExecutor::batchProgress, &dlg, [&dlg, commandIds](int idx, const QString& name) {
        int percent = (idx * 100) / commandIds.size();
        dlg.setProgress(percent, QString(tr("正在执行 %1/%2: %3"))
            .arg(idx + 1).arg(commandIds.size()).arg(name));
    });

    connect(executor, &CommandExecutor::outputReceived, &dlg, &ProgressDialog::appendOutput);

    connect(executor, &CommandExecutor::batchFinished, &dlg, [&dlg, finished](bool success) {
        *finished = true;
        dlg.setProgress(100, tr("全部完成"));
        dlg.setFinished(success, success ? tr("所有任务执行成功！") : tr("部分任务执行失败"));
    });

    connect(&dlg, &ProgressDialog::cancelled, executor, &CommandExecutor::cancel);

    QTimer::singleShot(200, [executor, commandIds]() {
        executor->executeBatch(commandIds);
    });

    dlg.exec();

    delete finished;
}

void MainWindow::showProgressDialog(const QString& commandId)
{
    auto cmd = CommandMetadataManager::instance()->getCommand(commandId);
    ProgressDialog dlg(cmd.friendlyName, this);

    auto executor = CommandExecutor::instance();

    connect(executor, &CommandExecutor::executionProgress, &dlg, &ProgressDialog::setProgress);
    connect(executor, &CommandExecutor::outputReceived, &dlg, &ProgressDialog::appendOutput);
    connect(executor, &CommandExecutor::executionFinished,
            &dlg, [&dlg](const QString&, bool success, const QString& msg) {
                dlg.setFinished(success, msg);
            });
    connect(&dlg, &ProgressDialog::cancelled, executor, &CommandExecutor::cancel);

    QTimer::singleShot(200, [executor, commandId]() {
        executor->executeCommand(commandId);
    });

    dlg.exec();
}

void MainWindow::onSearchTextChanged(const QString& text)
{
    Q_UNUSED(text);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    // 窗口缩放时强制当前页回到正确位置和尺寸，避免动画偏移残留导致错位
    if (m_contentStack && m_contentStack->currentWidget()) {
        QWidget* w = m_contentStack->currentWidget();
        w->setGeometry(0, 0, m_contentStack->width(), m_contentStack->height());
    }
}
