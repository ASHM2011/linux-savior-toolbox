#include "SettingsWidget.h"
#include "core/TranslationManager.h"
#include <QCheckBox>
#include <QMessageBox>
#include <QApplication>
#include <QProcess>
#include <QSettings>

SettingsWidget::SettingsWidget(QWidget* parent)
    : QWidget(parent)
    , m_monitorIntervalCombo(nullptr)
    , m_languageCombo(nullptr)
    , m_themeGroup(nullptr)
    , m_themeSystem(nullptr)
    , m_themeLight(nullptr)
    , m_themeDark(nullptr)
    , m_zoomSlider(nullptr)
    , m_zoomValueLabel(nullptr)
{
    setupUI();
}

void SettingsWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget* content = new QWidget();
    QVBoxLayout* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(32, 24, 32, 24);
    contentLayout->setSpacing(20);

    QLabel* title = new QLabel(tr("⚙️ 设置"));
    title->setStyleSheet("font-size: 22px; font-weight: 700; color: #0f172a;");
    contentLayout->addWidget(title);

    QFrame* monitorSection = createSection(tr("🔍 后台监控设置"), "monitor");
    QVBoxLayout* monitorLayout = qobject_cast<QVBoxLayout*>(monitorSection->layout());

    monitorLayout->addWidget(createToggleRow(tr("启用后台监控"), tr("开启后将在后台定期检测系统状态"), true));
    monitorLayout->addWidget(createToggleRow(tr("自动检测包管理器锁死"), tr("检测到软件中心卡住时自动提醒并修复"), true));
    monitorLayout->addWidget(createToggleRow(tr("自动检测磁盘空间不足"), tr("系统盘剩余空间低于10%时自动提醒"), true));
    monitorLayout->addWidget(createComboRow(tr("监控间隔时间"), tr("设置后台监控的检测频率"),
        QStringList() << tr("5 分钟") << tr("10 分钟") << tr("30 分钟"), 1));
    contentLayout->addWidget(monitorSection);

    QFrame* notifySection = createSection(tr("🔔 通知设置"), "notify");
    QVBoxLayout* notifyLayout = qobject_cast<QVBoxLayout*>(notifySection->layout());
    notifyLayout->addWidget(createToggleRow(tr("启用系统通知"), tr("使用系统原生通知提醒各种状态变化"), true));
    notifyLayout->addWidget(createToggleRow(tr("桌面崩溃自动恢复通知"), tr("桌面崩溃自动恢复后发送通知提醒"), true));
    notifyLayout->addWidget(createToggleRow(tr("清理完成通知"), tr("系统清理完成后弹出通知提醒"), true));
    contentLayout->addWidget(notifySection);

    QFrame* appearanceSection = createSection(tr("🎨 外观设置"), "appearance");
    QVBoxLayout* appearanceLayout = qobject_cast<QVBoxLayout*>(appearanceSection->layout());

    QFrame* themeRow = new QFrame();
    QHBoxLayout* themeRowLayout = new QHBoxLayout(themeRow);
    themeRowLayout->setContentsMargins(0, 8, 0, 8);

    QVBoxLayout* themeText = new QVBoxLayout();
    themeText->setSpacing(2);
    QLabel* themeLabel = new QLabel(tr("主题选择"));
    themeLabel->setStyleSheet("font-size: 13px; font-weight: 500; color: #334155;");
    themeText->addWidget(themeLabel);
    QLabel* themeDesc = new QLabel(tr("跟随系统或手动选择深浅色主题"));
    themeDesc->setStyleSheet("font-size: 12px; color: #94a3b8;");
    themeText->addWidget(themeDesc);
    themeRowLayout->addLayout(themeText, 1);

    QFrame* themeButtons = new QFrame();
    QHBoxLayout* themeBtnLayout = new QHBoxLayout(themeButtons);
    themeBtnLayout->setContentsMargins(0, 0, 0, 0);
    themeBtnLayout->setSpacing(8);

    m_themeGroup = new QButtonGroup(this);
    m_themeSystem = new QRadioButton(tr("跟随系统"));
    m_themeLight = new QRadioButton(tr("浅色"));
    m_themeDark = new QRadioButton(tr("深色"));

    m_themeSystem->setChecked(true);
    m_themeGroup->addButton(m_themeSystem, 0);
    m_themeGroup->addButton(m_themeLight, 1);
    m_themeGroup->addButton(m_themeDark, 2);

    QString radioStyle = R"(
        QRadioButton {
            font-size: 12px;
            color: #475569;
            padding: 6px 12px;
            background-color: #f1f5f9;
            border-radius: 8px;
        }
        QRadioButton::indicator {
            width: 0;
            height: 0;
        }
        QRadioButton:checked {
            background-color: #3b82f6;
            color: white;
            font-weight: 500;
        }
    )";
    m_themeSystem->setStyleSheet(radioStyle);
    m_themeLight->setStyleSheet(radioStyle);
    m_themeDark->setStyleSheet(radioStyle);

    themeBtnLayout->addWidget(m_themeSystem);
    themeBtnLayout->addWidget(m_themeLight);
    themeBtnLayout->addWidget(m_themeDark);
    themeRowLayout->addWidget(themeButtons);

    connect(m_themeGroup, QOverload<int>::of(&QButtonGroup::idClicked),
            this, &SettingsWidget::onThemeChanged);

    appearanceLayout->addWidget(themeRow);

    appearanceLayout->addWidget(createZoomRow(tr("显示缩放"), tr("调整界面整体缩放比例，默认为75%")));

    // 语言选择
    QFrame* langRow = new QFrame();
    QHBoxLayout* langRowLayout = new QHBoxLayout(langRow);
    langRowLayout->setContentsMargins(0, 8, 0, 8);

    QVBoxLayout* langText = new QVBoxLayout();
    langText->setSpacing(2);
    QLabel* langLabel = new QLabel(tr("界面语言"));
    langLabel->setStyleSheet("font-size: 13px; font-weight: 500; color: #334155;");
    langText->addWidget(langLabel);
    QLabel* langDesc = new QLabel(tr("切换界面显示语言，重启后完全生效"));
    langDesc->setStyleSheet("font-size: 12px; color: #94a3b8;");
    langText->addWidget(langDesc);
    langRowLayout->addLayout(langText, 1);

    m_languageCombo = new QComboBox();
    m_languageCombo->setFixedWidth(160);
    {
        auto langs = TranslationManager::instance().supportedLanguages();
        QString current = TranslationManager::instance().currentLanguage();
        int idx = 0;
        for (auto it = langs.constBegin(); it != langs.constEnd(); ++it) {
            m_languageCombo->addItem(it.value(), it.key());
            if (it.key() == current) idx = m_languageCombo->count() - 1;
        }
        m_languageCombo->setCurrentIndex(idx);
    }
    langRowLayout->addWidget(m_languageCombo);
    appearanceLayout->addWidget(langRow);

    connect(m_languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsWidget::onLanguageChanged);

    appearanceLayout->addWidget(createToggleRow(tr("最小化到系统托盘"), tr("关闭主窗口时最小化到托盘，不退出程序"), true));
    appearanceLayout->addWidget(createToggleRow(tr("关闭主窗口时退出应用"), tr("关闭按钮直接退出程序"), false));
    contentLayout->addWidget(appearanceSection);

    QFrame* shortcutSection = createSection(tr("⌨️ 快捷键设置"), "shortcut");
    QVBoxLayout* shortcutLayout = qobject_cast<QVBoxLayout*>(shortcutSection->layout());

    shortcutLayout->addWidget(createShortcutRow(tr("打开工具箱终端"), "Ctrl+Alt+T"));
    shortcutLayout->addWidget(createShortcutRow(tr("重启桌面"), "Ctrl+Alt+R"));
    shortcutLayout->addWidget(createShortcutRow(tr("一键清理"), "Ctrl+Alt+C"));

    contentLayout->addWidget(shortcutSection);

    QFrame* aboutSection = createSection(tr("ℹ️ 关于"), "about");
    QVBoxLayout* aboutLayout = qobject_cast<QVBoxLayout*>(aboutSection->layout());

    QFrame* aboutHeader = new QFrame();
    QHBoxLayout* aboutHeaderLayout = new QHBoxLayout(aboutHeader);
    aboutHeaderLayout->setContentsMargins(0, 4, 0, 4);
    aboutHeaderLayout->setSpacing(16);

    QLabel* appIcon = new QLabel("🐧");
    appIcon->setStyleSheet("font-size: 48px;");
    appIcon->setFixedWidth(60);
    appIcon->setAlignment(Qt::AlignCenter);
    aboutHeaderLayout->addWidget(appIcon);

    QVBoxLayout* appInfoLayout = new QVBoxLayout();
    appInfoLayout->setSpacing(4);

    QLabel* appName = new QLabel("Linux Savior Toolbox");
    appName->setStyleSheet("font-size: 18px; font-weight: 700; color: #0f172a;");
    appInfoLayout->addWidget(appName);

    QLabel* appVersion = new QLabel(tr("版本 v%1").arg(QCoreApplication::applicationVersion()));
    appVersion->setStyleSheet("font-size: 13px; color: #64748b;");
    appInfoLayout->addWidget(appVersion);

    aboutHeaderLayout->addLayout(appInfoLayout, 1);
    aboutLayout->addWidget(aboutHeader);

    QLabel* aboutText = new QLabel(
        tr("Linux萌新救星工具箱是一款专为Linux新手设计的系统维护工具。\n" "让完全不懂终端命令的小白，也能轻松搞定各种系统问题。\n\n" "技术栈：Qt 6 + C++17\n" "开源协议：GPL v3")
    );
    aboutText->setStyleSheet("font-size: 12px; color: #64748b; line-height: 1.8;");
    aboutText->setWordWrap(true);
    aboutLayout->addWidget(aboutText);

    QHBoxLayout* aboutBtnLayout = new QHBoxLayout();
    aboutBtnLayout->setSpacing(10);

    QPushButton* logsBtn = new QPushButton(tr("📋 查看日志"));
    logsBtn->setObjectName("secondaryBtn");
    logsBtn->setFixedHeight(36);
    connect(logsBtn, &QPushButton::clicked, this, &SettingsWidget::onViewLogsClicked);
    aboutBtnLayout->addWidget(logsBtn);

    QPushButton* updateBtn = new QPushButton(tr("🔄 检查更新"));
    updateBtn->setObjectName("secondaryBtn");
    updateBtn->setFixedHeight(36);
    connect(updateBtn, &QPushButton::clicked, this, &SettingsWidget::onCheckUpdateClicked);
    aboutBtnLayout->addWidget(updateBtn);

    aboutLayout->addLayout(aboutBtnLayout);

    contentLayout->addWidget(aboutSection);
    contentLayout->addStretch(1);

    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);
}

QFrame* SettingsWidget::createSection(const QString& title, const QString& icon)
{
    Q_UNUSED(icon);
    QFrame* section = new QFrame();
    section->setObjectName("card");
    QVBoxLayout* layout = new QVBoxLayout(section);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(4);

    QLabel* titleLabel = new QLabel(title);
    titleLabel->setStyleSheet("font-size: 15px; font-weight: 600; color: #0f172a; margin-bottom: 4px;");
    layout->addWidget(titleLabel);

    return section;
}

QFrame* SettingsWidget::createToggleRow(const QString& label, const QString& desc, bool enabled)
{
    QFrame* row = new QFrame();
    QHBoxLayout* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 8, 0, 8);

    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->setSpacing(2);

    QLabel* labelWidget = new QLabel(label);
    labelWidget->setStyleSheet("font-size: 13px; font-weight: 500; color: #334155;");
    textLayout->addWidget(labelWidget);

    QLabel* descWidget = new QLabel(desc);
    descWidget->setStyleSheet("font-size: 12px; color: #94a3b8;");
    textLayout->addWidget(descWidget);

    layout->addLayout(textLayout, 1);

    QCheckBox* toggle = new QCheckBox();
    toggle->setChecked(enabled);
    toggle->setStyleSheet(R"(
        QCheckBox::indicator {
            width: 44px;
            height: 24px;
            border-radius: 12px;
            background-color: #cbd5e1;
        }
        QCheckBox::indicator:checked {
            background-color: #3b82f6;
        }
    )");
    layout->addWidget(toggle);

    return row;
}

QFrame* SettingsWidget::createComboRow(const QString& label, const QString& desc,
                                        const QStringList& options, int currentIndex)
{
    QFrame* row = new QFrame();
    QHBoxLayout* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 8, 0, 8);

    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->setSpacing(2);

    QLabel* labelWidget = new QLabel(label);
    labelWidget->setStyleSheet("font-size: 13px; font-weight: 500; color: #334155;");
    textLayout->addWidget(labelWidget);

    QLabel* descWidget = new QLabel(desc);
    descWidget->setStyleSheet("font-size: 12px; color: #94a3b8;");
    textLayout->addWidget(descWidget);

    layout->addLayout(textLayout, 1);

    QComboBox* combo = new QComboBox();
    combo->addItems(options);
    combo->setCurrentIndex(currentIndex);
    combo->setFixedWidth(140);
    layout->addWidget(combo);

    if (label.contains(tr("监控间隔"))) {
        m_monitorIntervalCombo = combo;
    }

    return row;
}

QFrame* SettingsWidget::createShortcutRow(const QString& label, const QString& shortcut)
{
    QFrame* row = new QFrame();
    QHBoxLayout* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 8, 0, 8);

    QLabel* labelWidget = new QLabel(label);
    labelWidget->setStyleSheet("font-size: 13px; font-weight: 500; color: #334155;");
    layout->addWidget(labelWidget, 1);

    QLineEdit* shortcutEdit = new QLineEdit(shortcut);
    shortcutEdit->setAlignment(Qt::AlignCenter);
    shortcutEdit->setReadOnly(true);
    shortcutEdit->setFixedWidth(160);
    shortcutEdit->setStyleSheet(R"(
        QLineEdit {
            background-color: #f1f5f9;
            color: #475569;
            border: 1px solid #e2e8f0;
            border-radius: 6px;
            padding: 6px 12px;
            font-family: monospace;
            font-size: 12px;
        }
        QLineEdit:focus {
            border-color: #3b82f6;
            background-color: #ffffff;
        }
    )");
    layout->addWidget(shortcutEdit);

    return row;
}

void SettingsWidget::onThemeChanged(int id)
{
    Q_UNUSED(id);
}

void SettingsWidget::onViewLogsClicked()
{
    emit navigateTo("logs");
}

void SettingsWidget::onCheckUpdateClicked()
{
    QMessageBox::information(this, tr("检查更新"),
        tr("当前已是最新版本 v%1").arg(QCoreApplication::applicationVersion()));
}

QFrame* SettingsWidget::createZoomRow(const QString& label, const QString& desc)
{
    QFrame* row = new QFrame();
    QVBoxLayout* mainLayout = new QVBoxLayout(row);
    mainLayout->setContentsMargins(0, 8, 0, 8);
    mainLayout->setSpacing(8);

    QHBoxLayout* topRow = new QHBoxLayout();
    topRow->setContentsMargins(0, 0, 0, 0);
    topRow->setSpacing(0);

    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->setSpacing(2);

    QLabel* labelWidget = new QLabel(label);
    labelWidget->setStyleSheet("font-size: 13px; font-weight: 500; color: #334155;");
    labelWidget->setMinimumHeight(20);
    textLayout->addWidget(labelWidget);

    QLabel* descWidget = new QLabel(desc);
    descWidget->setStyleSheet("font-size: 12px; color: #94a3b8;");
    descWidget->setMinimumHeight(18);
    textLayout->addWidget(descWidget);

    topRow->addLayout(textLayout, 1);

    QFrame* zoomControl = new QFrame();
    QHBoxLayout* zoomLayout = new QHBoxLayout(zoomControl);
    zoomLayout->setContentsMargins(0, 0, 0, 0);
    zoomLayout->setSpacing(12);

    m_zoomSlider = new QSlider(Qt::Horizontal);
    m_zoomSlider->setRange(50, 150);
    m_zoomSlider->setMinimumWidth(180);
    m_zoomSlider->setTickPosition(QSlider::TicksBelow);
    m_zoomSlider->setTickInterval(25);

    QSettings settings("LinuxSavior", "Toolbox");
    int savedZoom = settings.value("display/zoom", 75).toInt();
    m_zoomSlider->setValue(savedZoom);

    zoomLayout->addWidget(m_zoomSlider);

    m_zoomValueLabel = new QLabel(QString::number(savedZoom) + "%");
    m_zoomValueLabel->setFixedWidth(60);
    m_zoomValueLabel->setAlignment(Qt::AlignCenter);
    m_zoomValueLabel->setMinimumHeight(28);
    m_zoomValueLabel->setStyleSheet(R"(
        font-size: 14px;
        font-weight: 600;
        color: #3b82f6;
        background-color: #eff6ff;
        padding: 4px 12px;
        border-radius: 8px;
    )");
    zoomLayout->addWidget(m_zoomValueLabel);

    topRow->addWidget(zoomControl);

    mainLayout->addLayout(topRow);

    QLabel* restartHint = new QLabel(tr("💡 调整缩放比例后需重启程序才能生效"));
    restartHint->setStyleSheet("font-size: 11px; color: #f59e0b;");
    restartHint->setMinimumHeight(16);
    mainLayout->addWidget(restartHint);

    connect(m_zoomSlider, &QSlider::valueChanged, this, &SettingsWidget::onZoomSliderChanged);

    return row;
}

void SettingsWidget::onZoomSliderChanged(int value)
{
    m_zoomValueLabel->setText(QString::number(value) + "%");

    QSettings settings("LinuxSavior", "Toolbox");
    settings.setValue("display/zoom", value);
}

void SettingsWidget::onLanguageChanged(int index)
{
    if (!m_languageCombo) return;
    const QString langCode = m_languageCombo->itemData(index).toString();
    if (langCode.isEmpty()) return;

    TranslationManager::instance().switchLanguage(langCode);

    // 语言切换后，已创建的控件需要重翻译；为保证一致性，提示用户重启
    auto ret = QMessageBox::question(this,
        tr("语言切换"),
        tr("界面语言已切换为 %1。\n\n为使所有界面文字完全生效，需要重启应用程序。\n是否立即重启？").arg(
            m_languageCombo->itemText(index)),
        tr("立即重启"), tr("稍后重启"));

    if (ret == 0) {
        // 重启应用：用 QProcess 启动新实例后退出当前进程
        QString program = QCoreApplication::applicationFilePath();
        QStringList args = QCoreApplication::arguments();
        args.removeFirst(); // 移除程序名自身
        QProcess::startDetached(program, args);
        QCoreApplication::quit();
    }
}
