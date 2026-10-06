#include "GnomeExtensionWidget.h"
#include <QMessageBox>
#include <QTimer>
#include <QCheckBox>
#include <QProcess>
#include <QApplication>
#include <QDBusInterface>
#include <QDBusReply>
#include <QProcessEnvironment>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTemporaryFile>
#include <QUrl>
#include <QUrlQuery>
#include <QEventLoop>
#include <QDir>
#include <QDesktopServices>
#include <unistd.h>

GnomeExtensionWidget::GnomeExtensionWidget(QWidget* parent)
    : QWidget(parent)
    , m_statusLabel(nullptr)
    , m_networkManager(new QNetworkAccessManager(this))
{
    initExtensions();
    setupUI();
    // 构造完成后自动检测一次真实状态
    QTimer::singleShot(300, this, &GnomeExtensionWidget::onDetectStatusClicked);
}

QString GnomeExtensionWidget::runGnomeExtensionCmd(const QStringList& args, int timeoutMs)
{
    QProcess process;
    process.setProgram(QStringLiteral("gnome-extensions"));
    process.setArguments(args);
    // 确保使用用户会话的 D-Bus，避免 root 环境下找不到用户的 GNOME Shell
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!env.contains(QStringLiteral("DBUS_SESSION_BUS_ADDRESS"))) {
        // 尝试从用户运行时目录恢复
        QString uid = QString::fromUtf8(qgetenv("PKEXEC_UID"));
        if (uid.isEmpty()) uid = QString::number(getuid());
        env.insert(QStringLiteral("DBUS_SESSION_BUS_ADDRESS"),
                   QStringLiteral("unix:path=/run/user/%1/bus").arg(uid));
        env.insert(QStringLiteral("XDG_RUNTIME_DIR"),
                   QStringLiteral("/run/user/%1").arg(uid));
    }
    process.setProcessEnvironment(env);
    process.start(QIODevice::ReadOnly);
    if (!process.waitForStarted(timeoutMs)) return {};
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(1000);
        return {};
    }
    return QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
}

void GnomeExtensionWidget::detectExtensionState(GnomeExtension& ext)
{
    if (ext.uuid.isEmpty()) {
        ext.installed = false;
        ext.enabled = false;
        return;
    }
    // gnome-extensions info <uuid> 的输出包含 "State: ENABLED" / "State: DISABLED"
    // 若扩展未安装，输出为空或报错
    QString info = runGnomeExtensionCmd({QStringLiteral("info"), ext.uuid});
    if (info.isEmpty()) {
        ext.installed = false;
        ext.enabled = false;
        return;
    }
    ext.installed = true;
    ext.enabled = info.contains(QStringLiteral("ENABLED"), Qt::CaseInsensitive)
                   && !info.contains(QStringLiteral("DISABLED"), Qt::CaseInsensitive);
}

void GnomeExtensionWidget::initExtensions()
{
    GnomeExtension ding;
    ding.id = "ding";
    ding.uuid = "ding@rastersoft.com";
    ding.name = "Desktop Icons NG (DING)";
    ding.description = tr("GNOME官方推荐，解决默认不显示桌面图标的问题");
    ding.icon = "🖥️";
    ding.stability = 5;
    ding.installed = false;
    ding.enabled = false;
    ding.category = tr("基础必备");
    ding.supportedVersions = "GNOME 42-46";
    m_extensions.append(ding);

    GnomeExtension appindicator;
    appindicator.id = "appindicator";
    appindicator.uuid = "appindicatorsupport@rgcjonas.gmail.com";
    appindicator.name = "AppIndicator Support";
    appindicator.description = tr("显示微信、QQ、网易云音乐等软件的系统托盘图标");
    appindicator.icon = "📌";
    appindicator.stability = 5;
    appindicator.installed = false;
    appindicator.enabled = false;
    appindicator.category = tr("基础必备");
    appindicator.supportedVersions = "GNOME 42-46";
    m_extensions.append(appindicator);

    GnomeExtension noannoyance;
    noannoyance.id = "noannoyance";
    noannoyance.uuid = "noannoyance@daase.net";
    noannoyance.name = "No Annoyance";
    noannoyance.description = tr("去掉\"窗口未响应\"的烦人提示");
    noannoyance.icon = "🚫";
    noannoyance.stability = 4;
    noannoyance.installed = false;
    noannoyance.enabled = false;
    noannoyance.category = tr("基础必备");
    noannoyance.supportedVersions = "GNOME 42-46";
    m_extensions.append(noannoyance);

    GnomeExtension dashtodock;
    dashtodock.id = "dashtodock";
    dashtodock.uuid = "dash-to-dock@micxgx.gmail.com";
    dashtodock.name = "Dash to Dock";
    dashtodock.description = tr("把左侧Dash变成可停靠的任务栏，类似Windows");
    dashtodock.icon = "📋";
    dashtodock.stability = 5;
    dashtodock.installed = false;
    dashtodock.enabled = false;
    dashtodock.category = tr("效率提升");
    dashtodock.supportedVersions = "GNOME 42-46";
    m_extensions.append(dashtodock);

    GnomeExtension clipboard;
    clipboard.id = "clipboard";
    clipboard.uuid = "clipboard-indicator@tudmotu.com";
    clipboard.name = "Clipboard Indicator";
    clipboard.description = tr("系统级剪贴板管理器，解决剪贴板内容丢失问题");
    clipboard.icon = "📋";
    clipboard.stability = 5;
    clipboard.installed = false;
    clipboard.enabled = false;
    clipboard.category = tr("效率提升");
    clipboard.supportedVersions = "GNOME 42-46";
    m_extensions.append(clipboard);

    GnomeExtension justperfection;
    justperfection.id = "justperfection";
    justperfection.uuid = "just-perfection-desktop@just-perfection";
    justperfection.name = "Just Perfection";
    justperfection.description = tr("GNOME最强自定义工具，隐藏任何不想要的界面元素");
    justperfection.icon = "⚙️";
    justperfection.stability = 4;
    justperfection.installed = false;
    justperfection.enabled = false;
    justperfection.category = tr("效率提升");
    justperfection.supportedVersions = "GNOME 42-46";
    m_extensions.append(justperfection);

    GnomeExtension blurmyshell;
    blurmyshell.id = "blurmyshell";
    blurmyshell.uuid = "blur-my-shell@aunetx";
    blurmyshell.name = "Blur My Shell";
    blurmyshell.description = tr("给面板、概览添加毛玻璃效果，不影响性能");
    blurmyshell.icon = "✨";
    blurmyshell.stability = 4;
    blurmyshell.installed = false;
    blurmyshell.enabled = false;
    blurmyshell.category = tr("美化增强");
    blurmyshell.supportedVersions = "GNOME 42-46";
    m_extensions.append(blurmyshell);

    GnomeExtension userthemes;
    userthemes.id = "userthemes";
    userthemes.uuid = "user-theme@gnome-shell-extensions.gcampax.github.com";
    userthemes.name = "User Themes";
    userthemes.description = tr("允许加载第三方主题，是GNOME美化的基础");
    userthemes.icon = "🎨";
    userthemes.stability = 5;
    userthemes.installed = false;
    userthemes.enabled = false;
    userthemes.category = tr("美化增强");
    userthemes.supportedVersions = "GNOME 42-46";
    m_extensions.append(userthemes);

    GnomeExtension magiclamp;
    magiclamp.id = "magiclamp";
    magiclamp.uuid = "magic-lamp@eonfluxor.github.io";
    magiclamp.name = "Compiz Magic Lamp";
    magiclamp.description = tr("给窗口最小化添加MacOS风格的魔法灯效果");
    magiclamp.icon = "💡";
    magiclamp.stability = 3;
    magiclamp.installed = false;
    magiclamp.enabled = false;
    magiclamp.category = tr("美化增强");
    magiclamp.supportedVersions = "GNOME 42-45";
    m_extensions.append(magiclamp);
}

void GnomeExtensionWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget* content = new QWidget();
    m_mainContentLayout = new QVBoxLayout(content);
    m_mainContentLayout->setContentsMargins(32, 24, 32, 24);
    m_mainContentLayout->setSpacing(24);

    QLabel* title = new QLabel(tr("🧩 GNOME扩展推荐"));
    title->setStyleSheet("font-size: 22px; font-weight: 700; color: #0f172a;");
    m_mainContentLayout->addWidget(title);

    QLabel* subtitle = new QLabel(tr("精选稳定好用的GNOME扩展，一键安装，小白也能轻松美化桌面"));
    subtitle->setStyleSheet("font-size: 13px; color: #64748b;");
    m_mainContentLayout->addWidget(subtitle);

    QFrame* statusBar = new QFrame();
    statusBar->setObjectName("card");
    QHBoxLayout* statusLayout = new QHBoxLayout(statusBar);
    statusLayout->setContentsMargins(16, 12, 16, 12);
    statusLayout->setSpacing(12);

    m_statusLabel = new QLabel(tr("📊 已检测到 3 个已安装扩展，2 个正在运行"));
    m_statusLabel->setStyleSheet("font-size: 13px; color: #475569;");
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch(1);

    QPushButton* detectBtn = new QPushButton(tr("🔄 检测状态"));
    detectBtn->setObjectName("secondaryBtn");
    detectBtn->setFixedHeight(32);
    detectBtn->setCursor(Qt::PointingHandCursor);
    connect(detectBtn, &QPushButton::clicked, this, &GnomeExtensionWidget::onDetectStatusClicked);
    statusLayout->addWidget(detectBtn);

    m_mainContentLayout->addWidget(statusBar);

    QStringList categories = {tr("🔧 基础必备（不装就没法用）"), tr("⚡ 效率提升（用了就回不去）"), tr("🎨 美化增强（好看又稳定）")};
    QStringList categoryKeys = {tr("基础必备"), tr("效率提升"), tr("美化增强")};

    for (int c = 0; c < categories.size(); c++) {
        QLabel* catTitle = new QLabel(categories[c]);
        catTitle->setStyleSheet("font-size: 15px; font-weight: 600; color: #0f172a; margin-top: 8px;");
        m_mainContentLayout->addWidget(catTitle);

        QGridLayout* grid = new QGridLayout();
        grid->setSpacing(12);

        int col = 0;
        int row = 0;
        for (const auto& ext : m_extensions) {
            if (ext.category != categoryKeys[c]) continue;

            QFrame* card = createExtensionCard(ext);
            m_cardList.append(card);
            grid->addWidget(card, row, col);
            col++;
            if (col >= 3) {
                col = 0;
                row++;
            }
        }

        m_mainContentLayout->addLayout(grid);
    }

    m_mainContentLayout->addStretch(1);

    QFrame* bottomBar = new QFrame();
    bottomBar->setStyleSheet(R"(
        QFrame {
            background-color: #ffffff;
            border-top: 1px solid #e2e8f0;
        }
    )");
    QHBoxLayout* bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(32, 12, 32, 12);
    bottomLayout->setSpacing(12);

    QPushButton* disableAllBtn = new QPushButton(tr("⏸️ 一键禁用所有扩展"));
    disableAllBtn->setObjectName("warningBtn");
    disableAllBtn->setFixedHeight(40);
    disableAllBtn->setCursor(Qt::PointingHandCursor);
    connect(disableAllBtn, &QPushButton::clicked, this, &GnomeExtensionWidget::onDisableAllClicked);
    bottomLayout->addWidget(disableAllBtn);

    bottomLayout->addStretch(1);

    QPushButton* restartBtn = new QPushButton(tr("🔄 重启GNOME Shell"));
    restartBtn->setObjectName("secondaryBtn");
    restartBtn->setFixedHeight(40);
    restartBtn->setCursor(Qt::PointingHandCursor);
    connect(restartBtn, &QPushButton::clicked, this, &GnomeExtensionWidget::onRestartShellClicked);
    bottomLayout->addWidget(restartBtn);

    m_mainContentLayout->addWidget(bottomBar);

    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);
}

QFrame* GnomeExtensionWidget::createExtensionCard(const GnomeExtension& ext)
{
    QFrame* card = new QFrame();
    card->setObjectName("card");
    card->setProperty("extId", ext.id);
    card->setStyleSheet(R"(
        QFrame#card {
            background-color: #ffffff;
            border: 1px solid #e2e8f0;
            border-radius: 12px;
        }
    )");

    QVBoxLayout* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(12);

    QLabel* iconLabel = new QLabel(ext.icon);
    iconLabel->setStyleSheet("font-size: 28px;");
    iconLabel->setFixedWidth(40);
    iconLabel->setAlignment(Qt::AlignCenter);
    headerLayout->addWidget(iconLabel);

    QVBoxLayout* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(2);

    QLabel* nameLabel = new QLabel(ext.name);
    nameLabel->setStyleSheet("font-size: 14px; font-weight: 600; color: #0f172a;");
    titleLayout->addWidget(nameLabel);

    QString stars;
    for (int i = 0; i < 5; i++) {
        stars += (i < ext.stability) ? "⭐" : "☆";
    }
    QLabel* starsLabel = new QLabel(stars + tr(" 稳定性评分"));
    starsLabel->setStyleSheet("font-size: 11px; color: #94a3b8;");
    titleLayout->addWidget(starsLabel);

    headerLayout->addLayout(titleLayout, 1);
    layout->addLayout(headerLayout);

    QLabel* descLabel = new QLabel(ext.description);
    descLabel->setStyleSheet("font-size: 12px; color: #64748b; line-height: 1.5;");
    descLabel->setWordWrap(true);
    layout->addWidget(descLabel);

    QLabel* versionLabel = new QLabel("📦 " + ext.supportedVersions);
    versionLabel->setStyleSheet("font-size: 11px; color: #94a3b8;");
    layout->addWidget(versionLabel);

    layout->addStretch(1);

    QHBoxLayout* toggleRow = new QHBoxLayout();
    toggleRow->setSpacing(8);

    QLabel* toggleLabel = new QLabel(tr("启用"));
    toggleLabel->setStyleSheet("font-size: 12px; color: #64748b;");
    toggleRow->addWidget(toggleLabel);

    QCheckBox* toggleCheck = new QCheckBox();
    toggleCheck->setProperty("extId", ext.id);
    toggleCheck->setChecked(ext.enabled);
    toggleCheck->setEnabled(ext.installed);
    toggleCheck->setCursor(Qt::PointingHandCursor);
    connect(toggleCheck, &QCheckBox::clicked, [this, ext](bool checked) {
        Q_UNUSED(checked);
        onToggleClicked(ext.id);
    });
    toggleRow->addWidget(toggleCheck);
    toggleRow->addStretch(1);

    if (ext.enabled) {
        QLabel* statusBadge = new QLabel(tr("运行中"));
        statusBadge->setObjectName("successBadge");
        statusBadge->setAlignment(Qt::AlignCenter);
        statusBadge->setFixedHeight(20);
        toggleRow->addWidget(statusBadge);
    } else if (ext.installed) {
        QLabel* statusBadge = new QLabel(tr("已安装"));
        statusBadge->setObjectName("badge");
        statusBadge->setAlignment(Qt::AlignCenter);
        statusBadge->setFixedHeight(20);
        toggleRow->addWidget(statusBadge);
    }

    layout->addLayout(toggleRow);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);

    QPushButton* actionBtn = new QPushButton();
    actionBtn->setFixedHeight(32);
    actionBtn->setCursor(Qt::PointingHandCursor);

    if (ext.installed) {
        actionBtn->setText(tr("卸载"));
        actionBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #fef2f2;
                color: #dc2626;
                border: none;
                border-radius: 6px;
                font-size: 12px;
                font-weight: 500;
                padding: 0 12px;
            }
            QPushButton:hover {
                background-color: #fee2e2;
            }
        )");
        connect(actionBtn, &QPushButton::clicked, [this, ext]() {
            onUninstallClicked(ext.id);
        });
    } else {
        actionBtn->setText(tr("一键安装"));
        actionBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #3b82f6;
                color: white;
                border: none;
                border-radius: 6px;
                font-size: 12px;
                font-weight: 500;
                padding: 0 12px;
            }
            QPushButton:hover {
                background-color: #2563eb;
            }
        )");
        connect(actionBtn, &QPushButton::clicked, [this, ext]() {
            onInstallClicked(ext.id);
        });
    }

    btnLayout->addWidget(actionBtn, 1);

    QPushButton* detailBtn = new QPushButton(tr("详情"));
    detailBtn->setFixedHeight(32);
    detailBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #f1f5f9;
            color: #64748b;
            border: none;
            border-radius: 6px;
            font-size: 12px;
            padding: 0 12px;
        }
        QPushButton:hover {
            background-color: #e2e8f0;
        }
    )");
    btnLayout->addWidget(detailBtn);

    layout->addLayout(btnLayout);

    return card;
}

void GnomeExtensionWidget::updateExtensionCard(const QString& extId)
{
    Q_UNUSED(extId);
}

void GnomeExtensionWidget::refreshAllCards()
{
    int installedCount = 0;
    int enabledCount = 0;
    for (const auto& ext : m_extensions) {
        if (ext.installed) installedCount++;
        if (ext.enabled) enabledCount++;
    }
    m_statusLabel->setText(QString(tr("📊 已检测到 %1 个已安装扩展，%2 个正在运行"))
                               .arg(installedCount).arg(enabledCount));
}

QString GnomeExtensionWidget::getGnomeShellVersion()
{
    QProcess process;
    process.start(QStringLiteral("gnome-shell"), {QStringLiteral("--version")});
    if (!process.waitForFinished(3000)) return {};
    QString out = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
    // 输出格式: "GNOME Shell 46.0" 或 "GNOME Shell 46.3.1"
    static const QRegularExpression re(QStringLiteral("(\\d+)\\.\\d+"));
    auto m = re.match(out);
    return m.hasMatch() ? m.captured(1) : QString();
}

void GnomeExtensionWidget::onInstallClicked(const QString& extId)
{
    GnomeExtension* ext = nullptr;
    for (auto& e : m_extensions) {
        if (e.id == extId) { ext = &e; break; }
    }
    if (!ext) return;

    if (ext->installed) {
        QMessageBox::information(this, tr("已安装"),
            QString(tr("扩展 \"%1\" 已经安装。")).arg(ext->name));
        return;
    }

    // 检查 gnome-extensions 命令是否可用
    if (runGnomeExtensionCmd({QStringLiteral("--help")}, 2000).isEmpty() &&
        !QFile::exists(QStringLiteral("/usr/bin/gnome-extensions"))) {
        QMessageBox::warning(this, tr("缺少依赖"),
            tr("系统未安装 gnome-extensions 命令行工具。\n\n"
               "请先安装：\n"
               "  Debian/Ubuntu: sudo apt install gnome-shell-extension-prefs\n"
               "  Fedora:        sudo dnf install gnome-extensions-app\n"
               "  Arch:          sudo pacman -S gnome-shell-extensions"));
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("确认安装"),
        QString(tr("确定要安装扩展 \"%1\" 吗？\n\n"
                   "将从 extensions.gnome.org 下载并安装。\n"
                   "安装后需要重启 GNOME Shell 才能生效。")).arg(ext->name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (reply != QMessageBox::Yes) return;

    downloadAndInstallExtension(*ext);
}

void GnomeExtensionWidget::downloadAndInstallExtension(const GnomeExtension& ext)
{
    QString shellVersion = getGnomeShellVersion();
    if (shellVersion.isEmpty()) {
        QMessageBox::warning(this, tr("安装失败"),
            tr("无法获取 GNOME Shell 版本。\n\n请确认已安装 gnome-shell。"));
        return;
    }

    m_statusLabel->setText(tr("🔄 正在获取 %1 的下载链接...").arg(ext.name));
    QApplication::processEvents();

    // 1. 查询 extensions.gnome.org API 获取下载地址
    QUrl infoUrl = QUrl(QStringLiteral("https://extensions.gnome.org/extension-info/"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("uuid"), ext.uuid);
    query.addQueryItem(QStringLiteral("shell_version"), shellVersion);
    infoUrl.setQuery(query);

    QNetworkRequest request(infoUrl);
    request.setRawHeader("User-Agent", "LinuxSaviorToolbox/1.0");

    QNetworkReply* infoReply = m_networkManager->get(request);
    QEventLoop loop;
    connect(infoReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (infoReply->error() != QNetworkReply::NoError) {
        infoReply->deleteLater();
        // 网络失败，引导用户手动安装
        showManualInstallGuide(ext);
        return;
    }

    QByteArray data = infoReply->readAll();
    infoReply->deleteLater();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        showManualInstallGuide(ext);
        return;
    }

    QJsonObject obj = doc.object();
    QString downloadUrl = obj.value(QStringLiteral("download_url")).toString();
    if (downloadUrl.isEmpty()) {
        // 该扩展不支持当前 shell 版本
        QMessageBox::warning(this, tr("不兼容"),
            QString(tr("扩展 \"%1\" 不支持当前 GNOME Shell %2 版本。\n\n"
                       "请在 extensions.gnome.org 查看可用版本。")).arg(ext.name).arg(shellVersion));
        return;
    }

    if (downloadUrl.startsWith(QStringLiteral("/"))) {
        downloadUrl = QStringLiteral("https://extensions.gnome.org") + downloadUrl;
    }

    // 2. 下载 zip 文件
    m_statusLabel->setText(tr("⬇️ 正在下载 %1...").arg(ext.name));
    QApplication::processEvents();

    QTemporaryFile* tmpZip = new QTemporaryFile(QDir::tempPath() + "/gnome_ext_XXXXXX.zip");
    if (!tmpZip->open()) {
        delete tmpZip;
        showManualInstallGuide(ext);
        return;
    }
    QString zipPath = tmpZip->fileName();
    tmpZip->close();
    delete tmpZip;

    QNetworkRequest dlRequest = QNetworkRequest(QUrl(downloadUrl));
    dlRequest.setRawHeader("User-Agent", "LinuxSaviorToolbox/1.0");
    QNetworkReply* dlReply = m_networkManager->get(dlRequest);

    QEventLoop dlLoop;
    connect(dlReply, &QNetworkReply::finished, &dlLoop, &QEventLoop::quit);
    dlLoop.exec();

    if (dlReply->error() != QNetworkReply::NoError) {
        dlReply->deleteLater();
        QFile::remove(zipPath);
        showManualInstallGuide(ext);
        return;
    }

    QFile zipFile(zipPath);
    if (!zipFile.open(QIODevice::WriteOnly)) {
        dlReply->deleteLater();
        showManualInstallGuide(ext);
        return;
    }
    zipFile.write(dlReply->readAll());
    zipFile.close();
    dlReply->deleteLater();

    // 3. 本地安装（gnome-extensions install 只接受本地文件路径）
    m_statusLabel->setText(tr("📦 正在安装 %1...").arg(ext.name));
    QApplication::processEvents();

    QString installOut = runGnomeExtensionCmd({QStringLiteral("install"), zipPath}, 30000);
    Q_UNUSED(installOut);
    QFile::remove(zipPath);

    // 刷新状态
    GnomeExtension* target = nullptr;
    for (auto& e : m_extensions) {
        if (e.id == ext.id) { target = &e; break; }
    }
    if (target) {
        detectExtensionState(*target);
    }
    refreshAllCards();

    if (target && target->installed) {
        QMessageBox::information(this, tr("安装成功"),
            QString(tr("扩展 \"%1\" 安装成功！\n\n请点击「重启 GNOME Shell」使扩展生效。")).arg(ext.name));
    } else {
        showManualInstallGuide(ext);
    }
}

void GnomeExtensionWidget::showManualInstallGuide(const GnomeExtension& ext)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("手动安装指引"));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText(QString(tr("自动安装 \"%1\" 失败。\n\n"
                              "请通过以下方式手动安装：\n\n"
                              "1. 用浏览器打开 GNOME 扩展页面：\n"
                              "   https://extensions.gnome.org/extension/?uuid=%2\n\n"
                              "2. 点击页面上的开关按钮即可安装\n"
                              "（需先安装浏览器扩展和原生连接器）\n\n"
                              "或使用命令行安装：\n"
                              "   gnome-extensions install <下载的zip文件>"))
                     .arg(ext.name, ext.uuid));

    QPushButton* openBtn = msgBox.addButton(tr("打开扩展页面"), QMessageBox::ActionRole);
    msgBox.addButton(QMessageBox::Ok);
    msgBox.exec();

    if (msgBox.clickedButton() == openBtn) {
        QDesktopServices::openUrl(QUrl(
            QStringLiteral("https://extensions.gnome.org/extension/?uuid=%1").arg(ext.uuid)));
    }
}

void GnomeExtensionWidget::onToggleClicked(const QString& extId)
{
    GnomeExtension* ext = nullptr;
    for (auto& e : m_extensions) {
        if (e.id == extId) { ext = &e; break; }
    }
    if (!ext || !ext->installed) return;

    QString action = ext->enabled ? QStringLiteral("disable") : QStringLiteral("enable");
    runGnomeExtensionCmd({action, ext->uuid});
    detectExtensionState(*ext);
    refreshAllCards();
}

void GnomeExtensionWidget::onUninstallClicked(const QString& extId)
{
    GnomeExtension* ext = nullptr;
    for (auto& e : m_extensions) {
        if (e.id == extId) { ext = &e; break; }
    }
    if (!ext || !ext->installed) return;

    QMessageBox::StandardButton reply = QMessageBox::warning(
        this, tr("确认卸载"),
        QString(tr("确定要卸载扩展 \"%1\" 吗？\n\n卸载后扩展的配置也会被清除。")).arg(ext->name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    runGnomeExtensionCmd({QStringLiteral("uninstall"), ext->uuid});
    detectExtensionState(*ext);
    refreshAllCards();

    if (!ext->installed) {
        QMessageBox::information(this, tr("卸载成功"),
            QString(tr("扩展 \"%1\" 已成功卸载！")).arg(ext->name));
    } else {
        QMessageBox::warning(this, tr("卸载失败"),
            tr("卸载操作未能完成，请手动检查。"));
    }
}

void GnomeExtensionWidget::onDisableAllClicked()
{
    int enabledCount = 0;
    for (const auto& ext : m_extensions) {
        if (ext.enabled) enabledCount++;
    }
    if (enabledCount == 0) {
        QMessageBox::information(this, tr("提示"), tr("当前没有正在运行的扩展。"));
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("确认禁用"),
        QString(tr("确定要禁用所有 %1 个正在运行的扩展吗？\n\n此操作不会卸载扩展，只是暂时禁用。")).arg(enabledCount),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    for (auto& ext : m_extensions) {
        if (ext.enabled) {
            runGnomeExtensionCmd({QStringLiteral("disable"), ext.uuid});
        }
    }
    onDetectStatusClicked();
    QMessageBox::information(this, tr("已禁用"),
        tr("所有扩展已被禁用。\n\n建议重启 GNOME Shell 以使更改完全生效。"));
}

void GnomeExtensionWidget::onRestartShellClicked()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("重启GNOME Shell"),
        tr("确定要重启 GNOME Shell 吗？\n\n重启后屏幕会短暂闪烁，所有正在运行的窗口会保留。"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    // 通过 D-Bus 调用 org.gnome.Shell.Eval 执行 Meta.restart
    bool ok = false;
    QDBusInterface iface(QStringLiteral("org.gnome.Shell"),
                         QStringLiteral("/org/gnome/Shell"),
                         QStringLiteral("org.gnome.Shell"),
                         QDBusConnection::sessionBus());
    if (iface.isValid()) {
        QDBusReply<QString> reply2 = iface.call(QStringLiteral("Eval"),
            QStringLiteral("Meta.restart(_('Restarting…'))"));
        ok = reply2.isValid();
    }
    if (!ok) {
        // 回退：尝试 busctl 命令
        QProcess p;
        p.start(QStringLiteral("busctl"),
            QStringList() << QStringLiteral("--user") << QStringLiteral("call")
                          << QStringLiteral("org.gnome.Shell")
                          << QStringLiteral("/org/gnome/Shell")
                          << QStringLiteral("org.gnome.Shell") << QStringLiteral("Eval")
                          << QStringLiteral("s") << QStringLiteral("Meta.restart('Restarting…')"));
        ok = p.waitForFinished(3000) && p.exitCode() == 0;
    }

    if (ok) {
        QTimer::singleShot(1500, [this]() {
            QMessageBox::information(this, tr("重启完成"), tr("GNOME Shell 已重启完成！"));
        });
    } else {
        QMessageBox::warning(this, tr("重启失败"),
            tr("无法通过 D-Bus 重启 GNOME Shell。\n\n请尝试：按 Alt+F2，输入 r，回车。\n（X11 会话有效；Wayland 下需注销重登录。）"));
    }
}

void GnomeExtensionWidget::onDetectStatusClicked()
{
    m_statusLabel->setText(tr("🔍 正在检测扩展状态..."));
    QApplication::processEvents();

    for (auto& ext : m_extensions) {
        detectExtensionState(ext);
    }
    refreshAllCards();

    int installed = 0, enabled = 0;
    for (const auto& ext : m_extensions) {
        if (ext.installed) installed++;
        if (ext.enabled) enabled++;
    }
    m_statusLabel->setText(QString(tr("📊 已检测到 %1 个已安装扩展，%2 个正在运行"))
                               .arg(installed).arg(enabled));
}
