#include "TerminalWidget.h"
#include "core/SystemDetector.h"
#include "core/CommandMetadata.h"
#include <QProcess>
#include <QDir>
#include <QStandardPaths>
#include <QFont>
#include <QFontDatabase>
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QSettings>
#include <QMessageBox>
#include <QTabBar>
#include <QActionGroup>
#include <QFontDialog>
#include <QInputDialog>
#include <QTimer>
#include <QShortcut>
#include <QKeyEvent>
#include <QRegularExpression>

TerminalWidget::TerminalWidget(QWidget* parent)
    : QWidget(parent)
    , m_tabWidget(nullptr)
    , m_currentTheme("Dracula")
    , m_currentFontFamily("Fira Code")
    , m_currentFontSize(12)
    , m_opacity(100)
    , m_tabCount(0)
    , m_autoRestart(true)
{
    loadSettings();
    setupUI();
    setupQuickCommands();
    newTab();
}

void TerminalWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    setupToolbar();

    QFrame* terminalContainer = new QFrame();
    terminalContainer->setObjectName("card");
    terminalContainer->setMinimumHeight(500);
    terminalContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    terminalContainer->setStyleSheet(R"(
        QFrame#card {
            margin: 0px 20px 20px 20px;
            border: 1px solid #e2e8f0;
            border-radius: 12px;
            overflow: hidden;
            background-color: #0f172a;
        }
    )");
    QVBoxLayout* termLayout = new QVBoxLayout(terminalContainer);
    termLayout->setContentsMargins(0, 0, 0, 0);
    termLayout->setSpacing(0);

#ifdef HAVE_QTERMWIDGET
    m_tabWidget = new QTabWidget();
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_tabWidget->setStyleSheet(R"(
        QTabWidget::pane {
            border: none;
            background-color: #0f172a;
        }
        QTabBar::tab {
            background-color: #1e293b;
            color: #94a3b8;
            padding: 8px 16px;
            border: none;
            border-right: 1px solid #334155;
            min-width: 100px;
            height: 36px;
            font-size: 12px;
        }
        QTabBar::tab:selected {
            background-color: #0f172a;
            color: #f1f5f9;
        }
        QTabBar::tab:hover:!selected {
            background-color: #334155;
            color: #e2e8f0;
        }
        QTabBar::close-button {
            image: none;
            width: 16px;
            height: 16px;
            border-radius: 8px;
            margin: 2px;
        }
        QTabBar::close-button:hover {
            background-color: #ef4444;
        }
    )");

    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &TerminalWidget::closeTab);
    connect(m_tabWidget, &QTabWidget::customContextMenuRequested, this, &TerminalWidget::onTabContextMenu);

    termLayout->addWidget(m_tabWidget);
#else
    QLabel* fallbackLabel = new QLabel(tr("终端模拟器需要 QTermWidget 库支持"));
    fallbackLabel->setAlignment(Qt::AlignCenter);
    fallbackLabel->setStyleSheet("color: #94a3b8; padding: 40px;");
    termLayout->addWidget(fallbackLabel);
#endif

    mainLayout->addWidget(terminalContainer, 1);
}

void TerminalWidget::setupToolbar()
{
    QFrame* toolbar = new QFrame();
    toolbar->setObjectName("card");
    toolbar->setStyleSheet(R"(
        QFrame#card {
            margin: 16px 20px 12px 20px;
            background-color: #ffffff;
            border: 1px solid #e2e8f0;
            border-radius: 12px;
        }
    )");
    QHBoxLayout* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(16, 10, 16, 10);
    toolbarLayout->setSpacing(8);

    m_newTabBtn = new QPushButton(tr("➕ 新标签"));
    m_newTabBtn->setObjectName("secondaryBtn");
    m_newTabBtn->setMinimumHeight(34);
    connect(m_newTabBtn, &QPushButton::clicked, this, &TerminalWidget::newTab);
    toolbarLayout->addWidget(m_newTabBtn);

    QFrame* separator1 = new QFrame();
    separator1->setFrameShape(QFrame::VLine);
    separator1->setStyleSheet("color: #e2e8f0;");
    toolbarLayout->addWidget(separator1);

    QLabel* quickLabel = new QLabel(tr("快捷:"));
    quickLabel->setStyleSheet("color: #64748b; font-size: 12px;");
    toolbarLayout->addWidget(quickLabel);

    m_quickCmdLayout = new QHBoxLayout();
    m_quickCmdLayout->setSpacing(6);
    toolbarLayout->addLayout(m_quickCmdLayout, 1);

    m_clearBtn = new QPushButton(tr("🧹 清屏"));
    m_clearBtn->setObjectName("secondaryBtn");
    m_clearBtn->setMinimumHeight(34);
    connect(m_clearBtn, &QPushButton::clicked, this, &TerminalWidget::onClearClicked);
    toolbarLayout->addWidget(m_clearBtn);

    m_copyBtn = new QPushButton(tr("📋 复制"));
    m_copyBtn->setObjectName("secondaryBtn");
    m_copyBtn->setMinimumHeight(34);
    connect(m_copyBtn, &QPushButton::clicked, this, &TerminalWidget::onCopyClicked);
    toolbarLayout->addWidget(m_copyBtn);

    m_pasteBtn = new QPushButton(tr("📎 粘贴"));
    m_pasteBtn->setObjectName("secondaryBtn");
    m_pasteBtn->setMinimumHeight(34);
    connect(m_pasteBtn, &QPushButton::clicked, this, &TerminalWidget::onPasteClicked);
    toolbarLayout->addWidget(m_pasteBtn);

    m_settingsBtn = new QPushButton("⚙️");
    m_settingsBtn->setObjectName("secondaryBtn");
    m_settingsBtn->setFixedSize(38, 34);
    connect(m_settingsBtn, &QPushButton::clicked, this, &TerminalWidget::onSettingsClicked);
    toolbarLayout->addWidget(m_settingsBtn);

    setupSettingsMenu();

    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(layout());
    if (mainLayout) {
        mainLayout->addWidget(toolbar);
    }
}

void TerminalWidget::setupSettingsMenu()
{
    m_settingsMenu = new QMenu(this);

    QAction* newTabAct = new QAction(tr("➕ 新建标签页"), this);
    newTabAct->setShortcut(QKeySequence("Ctrl+Shift+T"));
    connect(newTabAct, &QAction::triggered, this, &TerminalWidget::newTab);
    m_settingsMenu->addAction(newTabAct);

    QAction* closeTabAct = new QAction(tr("✖️ 关闭当前标签"), this);
    closeTabAct->setShortcut(QKeySequence("Ctrl+Shift+W"));
    connect(closeTabAct, &QAction::triggered, this, [this]() {
        if (m_tabWidget) closeTab(m_tabWidget->currentIndex());
    });
    m_settingsMenu->addAction(closeTabAct);

    QAction* renameAct = new QAction(tr("✏️ 重命名标签"), this);
    connect(renameAct, &QAction::triggered, this, &TerminalWidget::renameTab);
    m_settingsMenu->addAction(renameAct);

    m_settingsMenu->addSeparator();

    QAction* autoRestartAct = new QAction(tr("🔄 Shell退出自动重启"), this);
    autoRestartAct->setCheckable(true);
    autoRestartAct->setChecked(m_autoRestart);
    connect(autoRestartAct, &QAction::toggled, this, [this](bool checked) {
        m_autoRestart = checked;
    });
    m_settingsMenu->addAction(autoRestartAct);

    m_settingsMenu->addSeparator();

    m_themeMenu = m_settingsMenu->addMenu(tr("🎨 终端主题"));

    QStringList themes = {
        "Dracula", "Monokai", "Solarized Dark", "Solarized Light",
        "WhiteOnBlack", "GreenOnBlack", "BlackOnWhite", "Tango",
        "Linux", "XTerm", "Rxvt", "Konsole"
    };

    QActionGroup* themeGroup = new QActionGroup(this);
    themeGroup->setExclusive(true);

    for (const QString& theme : themes) {
        QAction* act = new QAction(theme, this);
        act->setCheckable(true);
        if (theme == m_currentTheme) act->setChecked(true);
        connect(act, &QAction::triggered, this, [this, theme]() {
            applyTheme(theme);
            m_currentTheme = theme;
            saveSettings();
        });
        themeGroup->addAction(act);
        m_themeMenu->addAction(act);
    }

    m_settingsMenu->addSeparator();

    QAction* fontAct = new QAction(tr("🔤 字体设置"), this);
    connect(fontAct, &QAction::triggered, this, &TerminalWidget::onFontChanged);
    m_settingsMenu->addAction(fontAct);

    QMenu* fontSizeMenu = m_settingsMenu->addMenu(tr("📏 字体大小"));
    QActionGroup* sizeGroup = new QActionGroup(this);
    for (int size : {9, 10, 11, 12, 13, 14, 16, 18, 20, 24}) {
        QAction* act = new QAction(QString::number(size) + " pt", this);
        act->setCheckable(true);
        if (size == m_currentFontSize) act->setChecked(true);
        connect(act, &QAction::triggered, this, [this, size]() {
            m_currentFontSize = size;
            saveSettings();
#ifdef HAVE_QTERMWIDGET
            QFont font(m_currentFontFamily, size);
            if (!font.exactMatch()) {
                font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
                font.setPointSize(size);
            }
            for (int i = 0; i < m_tabWidget->count(); i++) {
                QTermWidget* term = qobject_cast<QTermWidget*>(m_tabWidget->widget(i));
                if (term) term->setTerminalFont(font);
            }
#endif
        });
        sizeGroup->addAction(act);
        fontSizeMenu->addAction(act);
    }

    m_settingsMenu->addSeparator();

    QMenu* opacityMenu = m_settingsMenu->addMenu(tr("🌫️ 透明度"));
    QActionGroup* opacityGroup = new QActionGroup(this);
    for (int op : {100, 95, 90, 85, 80, 75, 70, 60, 50}) {
        QAction* act = new QAction(QString::number(op) + "%", this);
        act->setCheckable(true);
        if (op == m_opacity) act->setChecked(true);
        connect(act, &QAction::triggered, this, [this, op]() {
            m_opacity = op;
            saveSettings();
#ifdef HAVE_QTERMWIDGET
            for (int i = 0; i < m_tabWidget->count(); i++) {
                QTermWidget* term = qobject_cast<QTermWidget*>(m_tabWidget->widget(i));
                if (term) {
                    term->setTerminalOpacity(op / 100.0);
                }
            }
#endif
        });
        opacityGroup->addAction(act);
        opacityMenu->addAction(act);
    }

    m_settingsMenu->addSeparator();

    QAction* copyAct = new QAction(tr("📋 复制选中"), this);
    copyAct->setShortcut(QKeySequence("Ctrl+Shift+C"));
    connect(copyAct, &QAction::triggered, this, &TerminalWidget::onCopyClicked);
    m_settingsMenu->addAction(copyAct);

    QAction* pasteAct = new QAction(tr("📎 粘贴"), this);
    pasteAct->setShortcut(QKeySequence("Ctrl+Shift+V"));
    connect(pasteAct, &QAction::triggered, this, &TerminalWidget::onPasteClicked);
    m_settingsMenu->addAction(pasteAct);

    QAction* clearAct = new QAction(tr("🧹 清屏"), this);
    clearAct->setShortcut(QKeySequence("Ctrl+L"));
    connect(clearAct, &QAction::triggered, this, &TerminalWidget::onClearClicked);
    m_settingsMenu->addAction(clearAct);
}

void TerminalWidget::setupQuickCommands()
{
    auto sysDetector = SystemDetector::instance();
    QStringList cmdIds;
    QStringList cmdNames;

    cmdIds << sysDetector->getUpdateCommandId();
    cmdNames << tr("🔄 更新系统");

    cmdIds << sysDetector->getCleanCacheCommandId();
    cmdNames << tr("🧹 清理缓存");

    if (sysDetector->isGNOME()) {
        cmdIds << "gnome_shell_restart";
    } else if (sysDetector->isKDE()) {
        cmdIds << "plasma_restart";
    } else {
        cmdIds << "gnome_shell_restart";
    }
    cmdNames << tr("🖥️ 重启桌面");

    cmdIds << "rm_trash";
    cmdNames << tr("🗑️ 清空回收站");

    for (int i = 0; i < cmdIds.size() && i < 4; i++) {
        QPushButton* btn = new QPushButton(cmdNames[i]);
        btn->setObjectName("secondaryBtn");
        btn->setMinimumHeight(34);
        btn->setCursor(Qt::PointingHandCursor);

        QString cmdId = cmdIds[i];
        connect(btn, &QPushButton::clicked, this, [this, cmdId]() {
            onQuickCommandClicked(cmdId);
        });

        m_quickCmdButtons.append(btn);
        m_quickCmdLayout->addWidget(btn);
    }

    m_quickCmdLayout->addStretch();
}

#ifdef HAVE_QTERMWIDGET
QTermWidget* TerminalWidget::createTerminal(const QString& tabTitle)
{
    QTermWidget* terminal = new QTermWidget(0);
    terminal->setColorScheme(m_currentTheme);
    
    QString shellPath = QStandardPaths::findExecutable("bash");
    if (shellPath.isEmpty()) {
        shellPath = QStandardPaths::findExecutable("zsh");
    }
    if (shellPath.isEmpty()) {
        shellPath = "/bin/sh";
    }
    terminal->setShellProgram(shellPath);
    // 去除 /etc/profile 与 ~/.profile 的加载（非登录式 shell 仍然会读 .bashrc，但降低来自 profile 侧的 PATH 劫持风险）
    terminal->setArgs({QStringLiteral("--noprofile")});
    terminal->setHistorySize(10000);
    terminal->setScrollBarPosition(QTermWidget::ScrollBarRight);
    terminal->setTerminalOpacity(m_opacity / 100.0);
    terminal->setTerminalSizeHint(true);
    terminal->setMinimumSize(QSize(600, 400));
    terminal->setBlinkingCursor(true);
    
    QFont font(m_currentFontFamily, m_currentFontSize);
    if (!font.exactMatch()) {
        font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        font.setPointSize(m_currentFontSize);
        m_currentFontFamily = font.family();
    }
    terminal->setTerminalFont(font);

    connect(terminal, &QTermWidget::finished, this, &TerminalWidget::onTerminalFinished);
    connect(terminal, &QTermWidget::copyAvailable, this, &TerminalWidget::onTerminalCopyAvailable);

    QString title = tabTitle.isEmpty() ? QString(tr("终端 %1")).arg(m_tabCount) : tabTitle;
    int index = m_tabWidget->addTab(terminal, title);
    m_tabWidget->setCurrentIndex(index);
    
    terminal->startShellProgram();
    terminal->setFocus();

    return terminal;
}

QTermWidget* TerminalWidget::currentTerminal() const
{
    if (!m_tabWidget || m_tabWidget->count() == 0) return nullptr;
    return qobject_cast<QTermWidget*>(m_tabWidget->currentWidget());
}

void TerminalWidget::restartTerminal(int index)
{
    if (index < 0 || index >= m_tabWidget->count()) return;
    
    QString tabTitle = m_tabWidget->tabText(index);
    QWidget* oldWidget = m_tabWidget->widget(index);
    
    m_tabWidget->removeTab(index);
    if (oldWidget) oldWidget->deleteLater();
    
    QTermWidget* newTerm = createTerminal(tabTitle + tr(" (重启)"));
    m_tabWidget->setCurrentWidget(newTerm);
}
#endif

void TerminalWidget::newTab()
{
#ifdef HAVE_QTERMWIDGET
    m_tabCount++;
    createTerminal();
#endif
}

void TerminalWidget::closeTab(int index)
{
#ifdef HAVE_QTERMWIDGET
    m_autoRestart = false;
    if (m_tabWidget->count() <= 1) {
        m_autoRestart = true;
        newTab();
    }
    QWidget* widget = m_tabWidget->widget(index);
    m_tabWidget->removeTab(index);
    if (widget) widget->deleteLater();
    m_autoRestart = true;
#endif
}

void TerminalWidget::renameTab()
{
#ifdef HAVE_QTERMWIDGET
    bool ok;
    QString text = QInputDialog::getText(this, tr("重命名标签"), tr("输入新名称:"), QLineEdit::Normal,
                                          m_tabWidget->tabText(m_tabWidget->currentIndex()), &ok);
    if (ok && !text.isEmpty()) {
        m_tabWidget->setTabText(m_tabWidget->currentIndex(), text);
    }
#endif
}

void TerminalWidget::onTabContextMenu(const QPoint& pos)
{
    QMenu menu(this);
    menu.addAction(tr("➕ 新建标签页"), this, &TerminalWidget::newTab);
    menu.addAction(tr("✏️ 重命名"), this, &TerminalWidget::renameTab);
#ifdef HAVE_QTERMWIDGET
    menu.addSeparator();
    menu.addAction(tr("🔄 重启终端"), this, [this]() {
        if (m_tabWidget) restartTerminal(m_tabWidget->currentIndex());
    });
#endif
    menu.addSeparator();
    menu.addAction(tr("✖️ 关闭标签"), this, [this]() {
        if (m_tabWidget) closeTab(m_tabWidget->currentIndex());
    });
    menu.exec(m_tabWidget->tabBar()->mapToGlobal(pos));
}

void TerminalWidget::onQuickCommandClicked(const QString& cmdId)
{
    emit commandTriggered(cmdId);
}

void TerminalWidget::onClearClicked()
{
#ifdef HAVE_QTERMWIDGET
    auto term = currentTerminal();
    if (term) term->sendText("clear\n");
#endif
}

void TerminalWidget::onCopyClicked()
{
#ifdef HAVE_QTERMWIDGET
    auto term = currentTerminal();
    if (term) term->copyClipboard();
#endif
}

void TerminalWidget::onPasteClicked()
{
#ifdef HAVE_QTERMWIDGET
    auto term = currentTerminal();
    if (term) term->pasteClipboard();
#endif
}

void TerminalWidget::onSettingsClicked()
{
    m_settingsMenu->exec(m_settingsBtn->mapToGlobal(QPoint(0, m_settingsBtn->height())));
}

void TerminalWidget::applyTheme(const QString& themeName)
{
#ifdef HAVE_QTERMWIDGET
    for (int i = 0; i < m_tabWidget->count(); i++) {
        QTermWidget* term = qobject_cast<QTermWidget*>(m_tabWidget->widget(i));
        if (term) term->setColorScheme(themeName);
    }
#endif
}

void TerminalWidget::onFontChanged()
{
#ifdef HAVE_QTERMWIDGET
    bool ok;
    QFont font = QFontDialog::getFont(&ok, QFont(m_currentFontFamily, m_currentFontSize), this, tr("选择终端字体"));
    if (ok) {
        m_currentFontFamily = font.family();
        m_currentFontSize = font.pointSize();
        saveSettings();
        for (int i = 0; i < m_tabWidget->count(); i++) {
            QTermWidget* term = qobject_cast<QTermWidget*>(m_tabWidget->widget(i));
            if (term) term->setTerminalFont(font);
        }
    }
#endif
}

#ifdef HAVE_QTERMWIDGET
void TerminalWidget::onTerminalFinished()
{
    QTermWidget* term = qobject_cast<QTermWidget*>(sender());
    if (!term) return;
    
    int idx = m_tabWidget->indexOf(term);
    if (idx < 0) return;
    
    if (m_autoRestart && m_tabWidget->count() > 0) {
        QTimer::singleShot(100, this, [this, idx]() {
            restartTerminal(idx);
        });
    } else {
        closeTab(idx);
    }
}

void TerminalWidget::onTerminalCopyAvailable(bool available)
{
    m_copyBtn->setEnabled(available);
}
#endif

void TerminalWidget::executeCommand(const QString& commandId)
{
#ifdef HAVE_QTERMWIDGET
    auto cmd = CommandMetadataManager::instance()->getCommand(commandId);
    if (cmd.command.isEmpty()) return;

    if (m_tabWidget->count() == 0) newTab();

    auto term = currentTerminal();
    if (term) {
        term->setFocus();
        term->sendText(cmd.command + "\n");
    }
#endif
}

void TerminalWidget::sendText(const QString& text)
{
#ifdef HAVE_QTERMWIDGET
    auto term = currentTerminal();
    if (!term) return;
    // ---------- 安全校验：程序化 sendText 入口同样禁止提权前缀 & 限制长度 ----------
    const QString trimmed = text.trimmed();
    if (!trimmed.isEmpty()) {
        static const QRegularExpression kPrefixRe(
            QStringLiteral(R"(^(sudo|pkexec|doas|su)\b)"),
            QRegularExpression::CaseInsensitiveOption);
        if (kPrefixRe.match(trimmed).hasMatch()) {
            qWarning() << tr("[安全策略] sendText 禁止 sudo/pkexec/doas/su 前缀，已拦截。");
            return;
        }
        if (trimmed.length() > 2048) {
            qWarning() << tr("[安全限制] sendText 文本过长（上限 2048 字符），已截断。");
            term->sendText(text.left(2048) + "\n");
            return;
        }
    }
    term->sendText(text);
#endif
}

void TerminalWidget::loadSettings()
{
    QSettings settings("LinuxSavior", "Toolbox");
    settings.beginGroup("terminal");
    m_currentTheme = settings.value("theme", "Dracula").toString();
    m_currentFontFamily = settings.value("fontFamily", "Fira Code").toString();
    m_currentFontSize = settings.value("fontSize", 12).toInt();
    m_opacity = settings.value("opacity", 100).toInt();
    m_autoRestart = settings.value("autoRestart", true).toBool();
    settings.endGroup();
}

void TerminalWidget::saveSettings()
{
    QSettings settings("LinuxSavior", "Toolbox");
    settings.beginGroup("terminal");
    settings.setValue("theme", m_currentTheme);
    settings.setValue("fontFamily", m_currentFontFamily);
    settings.setValue("fontSize", m_currentFontSize);
    settings.setValue("opacity", m_opacity);
    settings.setValue("autoRestart", m_autoRestart);
    settings.endGroup();
}

QStringList TerminalWidget::getQuickCommands() const
{
    return QStringList()
        << tr("更新系统")
        << tr("清理缓存")
        << tr("重启桌面")
        << tr("清空回收站");
}

QString TerminalWidget::getQuickCommandIds() const
{
    return QString();
}
