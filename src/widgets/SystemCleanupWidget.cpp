#include "SystemCleanupWidget.h"
#include "core/SystemDetector.h"
#include "core/CommandExecutor.h"
#include "widgets/CommandDetailDialog.h"
#include <QTimer>
#include <QMessageBox>
#include <QDir>
#include <QProcess>
#include <cstdlib>
#include <ctime>

SystemCleanupWidget::SystemCleanupWidget(QWidget* parent)
    : QWidget(parent)
    , m_packageSizeLabel(nullptr)
    , m_logsSizeLabel(nullptr)
    , m_thumbnailSizeLabel(nullptr)
    , m_trashSizeLabel(nullptr)
    , m_browserSizeLabel(nullptr)
    , m_packageCleanBtn(nullptr)
    , m_packageProgressBar(nullptr)
    , m_packageResultLabel(nullptr)
    , m_logsCleanBtn(nullptr)
    , m_logsProgressBar(nullptr)
    , m_logsResultLabel(nullptr)
    , m_thumbnailCleanBtn(nullptr)
    , m_thumbnailProgressBar(nullptr)
    , m_thumbnailResultLabel(nullptr)
    , m_trashCleanBtn(nullptr)
    , m_trashProgressBar(nullptr)
    , m_trashResultLabel(nullptr)
    , m_browserCleanBtn(nullptr)
    , m_browserProgressBar(nullptr)
    , m_browserResultLabel(nullptr)
    , m_chromeCheckBox(nullptr)
    , m_firefoxCheckBox(nullptr)
{
    setupUI();
    QTimer::singleShot(200, this, &SystemCleanupWidget::scanRealSizes);
}

void SystemCleanupWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(32, 24, 32, 24);
    mainLayout->setSpacing(20);

    QLabel* title = new QLabel(tr("🧹 系统清理"));
    title->setStyleSheet("font-size: 22px; font-weight: 700; color: #0f172a;");
    mainLayout->addWidget(title);

    QLabel* subtitle = new QLabel(tr("安全清理系统垃圾，释放磁盘空间。所有清理项都经过安全验证。"));
    subtitle->setStyleSheet("font-size: 13px; color: #64748b;");
    mainLayout->addWidget(subtitle);

    m_tabWidget = new QTabWidget();
    m_tabWidget->addTab(createPackageCacheTab(), tr("📦 软件包缓存"));
    m_tabWidget->addTab(createLogsTab(), tr("📝 系统日志"));
    m_tabWidget->addTab(createThumbnailTab(), tr("🖼️ 缩略图缓存"));
    m_tabWidget->addTab(createTrashTab(), tr("🗑️ 回收站"));
    m_tabWidget->addTab(createBrowserCacheTab(), tr("🌐 浏览器缓存"));

    connect(m_tabWidget, &QTabWidget::currentChanged, this, &SystemCleanupWidget::onTabChanged);
    mainLayout->addWidget(m_tabWidget, 1);

    QFrame* bottomBar = new QFrame();
    bottomBar->setStyleSheet(R"(
        QFrame {
            background-color: #ffffff;
            border: 1px solid #e2e8f0;
            border-radius: 12px;
        }
    )");
    QHBoxLayout* bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(20, 12, 20, 12);
    bottomLayout->setSpacing(16);

    QVBoxLayout* sizeLayout = new QVBoxLayout();
    sizeLayout->setSpacing(2);

    QLabel* sizeLabel = new QLabel(tr("预计可释放空间"));
    sizeLabel->setStyleSheet("font-size: 12px; color: #64748b;");
    sizeLayout->addWidget(sizeLabel);

    m_totalSizeLabel = new QLabel(tr("扫描中..."));
    m_totalSizeLabel->setStyleSheet("font-size: 20px; font-weight: 700; color: #2563eb;");
    sizeLayout->addWidget(m_totalSizeLabel);

    bottomLayout->addLayout(sizeLayout);
    bottomLayout->addStretch(1);

    m_scanBtn = new QPushButton(tr("🔍 扫描"));
    m_scanBtn->setObjectName("secondaryBtn");
    m_scanBtn->setFixedSize(100, 40);
    connect(m_scanBtn, &QPushButton::clicked, this, &SystemCleanupWidget::onScanClicked);
    bottomLayout->addWidget(m_scanBtn);

    m_cleanBtn = new QPushButton(tr("🧹 一键清理选中项"));
    m_cleanBtn->setFixedHeight(40);
    m_cleanBtn->setCursor(Qt::PointingHandCursor);
    connect(m_cleanBtn, &QPushButton::clicked, this, &SystemCleanupWidget::onCleanClicked);
    bottomLayout->addWidget(m_cleanBtn);

    mainLayout->addWidget(bottomBar);
}

QFrame* SystemCleanupWidget::createInfoCard(const QString& title, const QString& size, const QString& count,
                                            const QString& descTitle, const QString& descText,
                                            const QString& buttonText, const QString& buttonObjectName,
                                            const QString& progressBarObjectName,
                                            QPushButton*& cleanBtn, QProgressBar*& progressBar, QLabel*& resultLabel,
                                            QLabel*& sizeValueOut)
{
    QFrame* infoCard = new QFrame();
    infoCard->setObjectName("card");
    QVBoxLayout* infoLayout = new QVBoxLayout(infoCard);
    infoLayout->setContentsMargins(20, 20, 20, 20);
    infoLayout->setSpacing(12);

    QHBoxLayout* sizeRow = new QHBoxLayout();
    QLabel* sizeLabel = new QLabel(title);
    sizeLabel->setStyleSheet("font-size: 14px; color: #475569; font-weight: 500;");
    sizeRow->addWidget(sizeLabel);
    sizeRow->addStretch(1);
    QLabel* sizeValue = new QLabel(size);
    sizeValueOut = sizeValue;
    QString color = "#f59e0b";
    if (buttonObjectName == "successBtn") color = "#10b981";
    if (buttonObjectName == "dangerBtn") color = "#ef4444";
    sizeValue->setStyleSheet(QString("font-size: 24px; font-weight: 700; color: %1;").arg(color));
    sizeRow->addWidget(sizeValue);
    infoLayout->addLayout(sizeRow);

    QLabel* countLabel = new QLabel(count);
    countLabel->setStyleSheet("font-size: 12px; color: #64748b;");
    infoLayout->addWidget(countLabel);

    QFrame* descFrame = new QFrame();
    QString bgColor = "#f0fdf4";
    QString textColor = "#15803d";
    QString titleColor = "#166534";
    if (buttonObjectName == "warningBtn") {
        bgColor = "#fffbeb";
        textColor = "#a16207";
        titleColor = "#92400e";
    }
    if (buttonObjectName == "dangerBtn") {
        bgColor = "#fef2f2";
        textColor = "#991b1b";
        titleColor = "#b91c1c";
    }
    descFrame->setStyleSheet(QString(R"(
        QFrame {
            background-color: %1;
            border-radius: 8px;
            padding: 12px;
        }
    )").arg(bgColor));
    QVBoxLayout* descLayout = new QVBoxLayout(descFrame);
    descLayout->setContentsMargins(12, 8, 12, 8);
    descLayout->setSpacing(6);

    QLabel* descTitleLabel = new QLabel(descTitle);
    descTitleLabel->setStyleSheet(QString("font-size: 12px; font-weight: 600; color: %1;").arg(titleColor));
    descLayout->addWidget(descTitleLabel);

    QLabel* descTextLabel = new QLabel(descText);
    descTextLabel->setStyleSheet(QString("font-size: 12px; color: %1; line-height: 1.5;").arg(textColor));
    descTextLabel->setWordWrap(true);
    descLayout->addWidget(descTextLabel);

    infoLayout->addWidget(descFrame);

    progressBar = new QProgressBar();
    progressBar->setObjectName(progressBarObjectName);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setFixedHeight(8);
    progressBar->setTextVisible(false);
    progressBar->hide();
    infoLayout->addWidget(progressBar);

    resultLabel = new QLabel();
    resultLabel->setStyleSheet("font-size: 12px; color: #10b981; font-weight: 500;");
    resultLabel->hide();
    infoLayout->addWidget(resultLabel);

    cleanBtn = new QPushButton(buttonText);
    cleanBtn->setObjectName(buttonObjectName);
    cleanBtn->setFixedHeight(42);
    cleanBtn->setCursor(Qt::PointingHandCursor);
    cleanBtn->setStyleSheet(QString(R"(
        QPushButton#%1 {
            font-size: 14px;
            font-weight: 600;
            border-radius: 8px;
        }
    )").arg(buttonObjectName));
    infoLayout->addWidget(cleanBtn);

    return infoCard;
}

QWidget* SystemCleanupWidget::createPackageCacheTab()
{
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(4, 8, 4, 8);
    layout->setSpacing(16);

    QFrame* infoCard = createInfoCard(
        tr("软件包缓存大小"), tr("扫描中..."), tr("已缓存软件包"),
        tr("💡 说明"), tr("清理软件包缓存不会影响已安装的软件，只是删除下载的安装包文件。下次安装软件时会重新下载。"),
        tr("🧹 清理软件包缓存"), "warningBtn", "warningBar",
        m_packageCleanBtn, m_packageProgressBar, m_packageResultLabel, m_packageSizeLabel
    );
    connect(m_packageCleanBtn, &QPushButton::clicked, this, &SystemCleanupWidget::onPackageCleanClicked);

    layout->addWidget(infoCard);
    layout->addStretch(1);

    return widget;
}

QWidget* SystemCleanupWidget::createLogsTab()
{
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(4, 8, 4, 8);
    layout->setSpacing(16);

    QFrame* infoCard = createInfoCard(
        tr("系统日志大小"), tr("扫描中..."), tr("仅清理: 7天前的旧日志"),
        tr("✅ 安全操作"), tr("清理旧日志不会影响系统运行，只保留最近7天的日志用于问题排查。"),
        tr("🧹 清理旧日志"), "successBtn", "successBar",
        m_logsCleanBtn, m_logsProgressBar, m_logsResultLabel, m_logsSizeLabel
    );
    connect(m_logsCleanBtn, &QPushButton::clicked, this, &SystemCleanupWidget::onLogsCleanClicked);

    layout->addWidget(infoCard);
    layout->addStretch(1);

    return widget;
}

QWidget* SystemCleanupWidget::createThumbnailTab()
{
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(4, 8, 4, 8);
    layout->setSpacing(16);

    QFrame* infoCard = createInfoCard(
        tr("缩略图缓存大小"), tr("扫描中..."), tr("缩略图缓存"),
        tr("💡 说明"), tr("清理缩略图缓存可以释放空间。清理后再次打开图片文件夹时会重新生成缩略图，速度会稍慢。"),
        tr("🧹 清理缩略图缓存"), "warningBtn", "warningBar",
        m_thumbnailCleanBtn, m_thumbnailProgressBar, m_thumbnailResultLabel, m_thumbnailSizeLabel
    );
    connect(m_thumbnailCleanBtn, &QPushButton::clicked, this, &SystemCleanupWidget::onThumbnailCleanClicked);

    layout->addWidget(infoCard);
    layout->addStretch(1);

    return widget;
}

QWidget* SystemCleanupWidget::createTrashTab()
{
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(4, 8, 4, 8);
    layout->setSpacing(16);

    QFrame* infoCard = new QFrame();
    infoCard->setObjectName("card");
    QVBoxLayout* infoLayout = new QVBoxLayout(infoCard);
    infoLayout->setContentsMargins(20, 20, 20, 20);
    infoLayout->setSpacing(12);

    QHBoxLayout* sizeRow = new QHBoxLayout();
    QLabel* sizeLabel = new QLabel(tr("回收站大小"));
    sizeLabel->setStyleSheet("font-size: 14px; color: #475569; font-weight: 500;");
    sizeRow->addWidget(sizeLabel);
    sizeRow->addStretch(1);
    m_trashSizeLabel = new QLabel(tr("扫描中..."));
    m_trashSizeLabel->setStyleSheet("font-size: 24px; font-weight: 700; color: #ef4444;");
    sizeRow->addWidget(m_trashSizeLabel);
    infoLayout->addLayout(sizeRow);

    QLabel* countLabel = new QLabel(tr("回收站内容"));
    countLabel->setStyleSheet("font-size: 12px; color: #64748b;");
    infoLayout->addWidget(countLabel);

    QFrame* warnFrame = new QFrame();
    warnFrame->setStyleSheet(R"(
        QFrame {
            background-color: #fef2f2;
            border-radius: 8px;
            padding: 12px;
            border-left: 4px solid #ef4444;
        }
    )");
    QVBoxLayout* warnLayout = new QVBoxLayout(warnFrame);
    warnLayout->setContentsMargins(12, 8, 12, 8);
    warnLayout->setSpacing(6);

    QLabel* warnTitle = new QLabel(tr("⚠️ 注意：永久删除，无法恢复！"));
    warnTitle->setStyleSheet("font-size: 12px; font-weight: 600; color: #b91c1c;");
    warnLayout->addWidget(warnTitle);

    QLabel* warnText = new QLabel(tr("清空回收站后，所有文件将被永久删除，无法恢复。请确认回收站中没有重要文件再执行清理。"));
    warnText->setStyleSheet("font-size: 12px; color: #991b1b; line-height: 1.5;");
    warnText->setWordWrap(true);
    warnLayout->addWidget(warnText);

    infoLayout->addWidget(warnFrame);

    m_trashProgressBar = new QProgressBar();
    m_trashProgressBar->setObjectName("dangerBar");
    m_trashProgressBar->setRange(0, 100);
    m_trashProgressBar->setValue(0);
    m_trashProgressBar->setFixedHeight(8);
    m_trashProgressBar->setTextVisible(false);
    m_trashProgressBar->hide();
    infoLayout->addWidget(m_trashProgressBar);

    m_trashResultLabel = new QLabel();
    m_trashResultLabel->setStyleSheet("font-size: 12px; color: #10b981; font-weight: 500;");
    m_trashResultLabel->hide();
    infoLayout->addWidget(m_trashResultLabel);

    m_trashCleanBtn = new QPushButton(tr("🗑️ 清空回收站"));
    m_trashCleanBtn->setObjectName("dangerBtn");
    m_trashCleanBtn->setFixedHeight(42);
    m_trashCleanBtn->setCursor(Qt::PointingHandCursor);
    m_trashCleanBtn->setStyleSheet(R"(
        QPushButton#dangerBtn {
            font-size: 14px;
            font-weight: 600;
            border-radius: 8px;
        }
    )");
    connect(m_trashCleanBtn, &QPushButton::clicked, this, &SystemCleanupWidget::onTrashCleanClicked);
    infoLayout->addWidget(m_trashCleanBtn);

    layout->addWidget(infoCard);
    layout->addStretch(1);

    return widget;
}

QWidget* SystemCleanupWidget::createBrowserCacheTab()
{
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(4, 8, 4, 8);
    layout->setSpacing(16);

    QFrame* infoCard = new QFrame();
    infoCard->setObjectName("card");
    QVBoxLayout* infoLayout = new QVBoxLayout(infoCard);
    infoLayout->setContentsMargins(20, 20, 20, 20);
    infoLayout->setSpacing(12);

    QHBoxLayout* sizeRow = new QHBoxLayout();
    QLabel* sizeLabel = new QLabel(tr("浏览器缓存大小"));
    sizeLabel->setStyleSheet("font-size: 14px; color: #475569; font-weight: 500;");
    sizeRow->addWidget(sizeLabel);
    sizeRow->addStretch(1);
    m_browserSizeLabel = new QLabel(tr("扫描中..."));
    m_browserSizeLabel->setStyleSheet("font-size: 24px; font-weight: 700; color: #f59e0b;");
    sizeRow->addWidget(m_browserSizeLabel);
    infoLayout->addLayout(sizeRow);

    QLabel* countLabel = new QLabel(tr("已检测浏览器缓存"));
    countLabel->setStyleSheet("font-size: 12px; color: #64748b;");
    infoLayout->addWidget(countLabel);

    QFrame* browserSelectFrame = new QFrame();
    browserSelectFrame->setStyleSheet(R"(
        QFrame {
            background-color: #f8fafc;
            border-radius: 8px;
            padding: 12px;
        }
    )");
    QVBoxLayout* browserLayout = new QVBoxLayout(browserSelectFrame);
    browserLayout->setContentsMargins(12, 8, 12, 8);
    browserLayout->setSpacing(8);

    QLabel* selectTitle = new QLabel(tr("🌐 选择要清理的浏览器："));
    selectTitle->setStyleSheet("font-size: 12px; font-weight: 600; color: #475569;");
    browserLayout->addWidget(selectTitle);

    m_chromeCheckBox = new QCheckBox(tr("Google Chrome"));
    m_chromeCheckBox->setChecked(true);
    m_chromeCheckBox->setStyleSheet("font-size: 12px;");
    browserLayout->addWidget(m_chromeCheckBox);

    m_firefoxCheckBox = new QCheckBox(tr("Mozilla Firefox"));
    m_firefoxCheckBox->setChecked(true);
    m_firefoxCheckBox->setStyleSheet("font-size: 12px;");
    browserLayout->addWidget(m_firefoxCheckBox);

    infoLayout->addWidget(browserSelectFrame);

    QFrame* descFrame = new QFrame();
    descFrame->setStyleSheet(R"(
        QFrame {
            background-color: #fffbeb;
            border-radius: 8px;
            padding: 12px;
        }
    )");
    QVBoxLayout* descLayout = new QVBoxLayout(descFrame);
    descLayout->setContentsMargins(12, 8, 12, 8);
    descLayout->setSpacing(6);

    QLabel* descTitle = new QLabel(tr("💡 说明"));
    descTitle->setStyleSheet("font-size: 12px; font-weight: 600; color: #92400e;");
    descLayout->addWidget(descTitle);

    QLabel* descText = new QLabel(tr("清理浏览器缓存会删除网页的临时文件（图片、脚本等），但不会删除书签、密码和浏览历史。下次访问网页时加载速度可能稍慢。"));
    descText->setStyleSheet("font-size: 12px; color: #a16207; line-height: 1.5;");
    descText->setWordWrap(true);
    descLayout->addWidget(descText);

    infoLayout->addWidget(descFrame);

    m_browserProgressBar = new QProgressBar();
    m_browserProgressBar->setObjectName("warningBar");
    m_browserProgressBar->setRange(0, 100);
    m_browserProgressBar->setValue(0);
    m_browserProgressBar->setFixedHeight(8);
    m_browserProgressBar->setTextVisible(false);
    m_browserProgressBar->hide();
    infoLayout->addWidget(m_browserProgressBar);

    m_browserResultLabel = new QLabel();
    m_browserResultLabel->setStyleSheet("font-size: 12px; color: #10b981; font-weight: 500;");
    m_browserResultLabel->hide();
    infoLayout->addWidget(m_browserResultLabel);

    m_browserCleanBtn = new QPushButton(tr("🧹 清理浏览器缓存"));
    m_browserCleanBtn->setObjectName("warningBtn");
    m_browserCleanBtn->setFixedHeight(42);
    m_browserCleanBtn->setCursor(Qt::PointingHandCursor);
    m_browserCleanBtn->setStyleSheet(R"(
        QPushButton#warningBtn {
            font-size: 14px;
            font-weight: 600;
            border-radius: 8px;
        }
    )");
    connect(m_browserCleanBtn, &QPushButton::clicked, this, &SystemCleanupWidget::onBrowserCleanClicked);
    infoLayout->addWidget(m_browserCleanBtn);

    layout->addWidget(infoCard);
    layout->addStretch(1);

    return widget;
}

void SystemCleanupWidget::simulateCleanup(QProgressBar* progressBar, QPushButton* cleanBtn, QLabel* resultLabel,
                                          const QString& commandId)
{
    progressBar->show();
    progressBar->setValue(0);
    cleanBtn->setEnabled(false);
    cleanBtn->setText(tr("清理中..."));
    resultLabel->hide();

    emit commandTriggered(commandId);

    int currentValue = 0;
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this, progressBar, cleanBtn, resultLabel, timer, &currentValue]() {
        currentValue += 12;
        if (currentValue >= 100) {
            currentValue = 100;
            progressBar->setValue(currentValue);
            timer->stop();
            timer->deleteLater();

            progressBar->hide();
            cleanBtn->setEnabled(true);
            cleanBtn->setText(tr("✅ 已清理完成"));
            cleanBtn->setObjectName("successBtn");
            cleanBtn->setStyleSheet(R"(
                QPushButton#successBtn {
                    background-color: #10b981;
                    color: white;
                    font-size: 14px;
                    font-weight: 600;
                    border-radius: 8px;
                }
                QPushButton#successBtn:hover {
                    background-color: #059669;
                }
            )");
            resultLabel->setText(tr("🎉 清理完成！"));
            resultLabel->show();
            // 清理后重新扫描大小
            QTimer::singleShot(500, this, &SystemCleanupWidget::scanRealSizes);
        } else {
            progressBar->setValue(currentValue);
        }
    });
    timer->start(80);
}

qint64 SystemCleanupWidget::getDirSize(const QString& path)
{
    if (!QDir(path).exists()) return 0;
    QProcess du;
    du.start("du", QStringList() << "-sb" << path);
    if (!du.waitForFinished(5000)) return 0;
    QString output = QString::fromUtf8(du.readAllStandardOutput()).trimmed();
    bool ok = false;
    qint64 size = output.section('\t', 0, 0).toLongLong(&ok);
    return ok ? size : 0;
}

void SystemCleanupWidget::scanRealSizes()
{
    QString home = QDir::homePath();

    // 软件包缓存：根据发行版检测
    qint64 pkgSize = 0;
    QStringList pkgDirs = {"/var/cache/apt/archives", "/var/cache/dnf",
                           "/var/cache/zypp", "/var/cache/pacman/pkg"};
    for (const QString& d : pkgDirs) {
        pkgSize += getDirSize(d);
    }
    if (m_packageSizeLabel) m_packageSizeLabel->setText(formatSize(pkgSize));

    // 系统日志
    qint64 logSize = getDirSize("/var/log/journal");
    if (m_logsSizeLabel) m_logsSizeLabel->setText(formatSize(logSize));

    // 缩略图缓存
    qint64 thumbSize = getDirSize(home + "/.cache/thumbnails");
    if (m_thumbnailSizeLabel) m_thumbnailSizeLabel->setText(formatSize(thumbSize));

    // 回收站
    qint64 trashSize = getDirSize(home + "/.local/share/Trash");
    if (m_trashSizeLabel) m_trashSizeLabel->setText(formatSize(trashSize));

    // 浏览器缓存
    qint64 browserSize = getDirSize(home + "/.cache/google-chrome")
                       + getDirSize(home + "/.config/google-chrome/Default/Cache")
                       + getDirSize(home + "/.cache/mozilla")
                       + getDirSize(home + "/.mozilla/firefox");
    if (m_browserSizeLabel) m_browserSizeLabel->setText(formatSize(browserSize));

    // 总计
    qint64 total = pkgSize + logSize + thumbSize + trashSize + browserSize;
    if (m_totalSizeLabel) m_totalSizeLabel->setText(formatSize(total));
}

void SystemCleanupWidget::onScanClicked()
{
    m_scanBtn->setEnabled(false);
    m_scanBtn->setText(tr("扫描中..."));

    QTimer::singleShot(100, [this]() {
        scanRealSizes();
        m_scanBtn->setEnabled(true);
        m_scanBtn->setText(tr("🔍 重新扫描"));
    });
}

void SystemCleanupWidget::onCleanClicked()
{
    emit commandTriggered(SystemDetector::instance()->getCleanCacheCommandId());
}

void SystemCleanupWidget::onTabChanged(int index)
{
    Q_UNUSED(index);
}

void SystemCleanupWidget::onPackageCleanClicked()
{
    QString cmdId = SystemDetector::instance()->getCleanCacheCommandId();
    simulateCleanup(m_packageProgressBar, m_packageCleanBtn, m_packageResultLabel, cmdId);
}

void SystemCleanupWidget::onLogsCleanClicked()
{
    simulateCleanup(m_logsProgressBar, m_logsCleanBtn, m_logsResultLabel, "clean_system_logs");
}

void SystemCleanupWidget::onThumbnailCleanClicked()
{
    simulateCleanup(m_thumbnailProgressBar, m_thumbnailCleanBtn, m_thumbnailResultLabel, "clean_thumbnails");
}

void SystemCleanupWidget::onTrashCleanClicked()
{
    QMessageBox::StandardButton reply = QMessageBox::warning(
        this,
        tr("确认清空回收站"),
        tr("⚠️ 确定要清空回收站吗？\n\n此操作将永久删除所有文件，无法恢复！"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        simulateCleanup(m_trashProgressBar, m_trashCleanBtn, m_trashResultLabel, "rm_trash");
    }
}

void SystemCleanupWidget::onBrowserCleanClicked()
{
    if (!m_chromeCheckBox->isChecked() && !m_firefoxCheckBox->isChecked()) {
        QMessageBox::information(this, tr("提示"), tr("请至少选择一个浏览器进行清理。"));
        return;
    }

    simulateCleanup(m_browserProgressBar, m_browserCleanBtn, m_browserResultLabel, "clean_browser_cache");
}

QString SystemCleanupWidget::formatSize(qint64 bytes)
{
    if (bytes < 1024) {
        return QString("%1 B").arg(bytes);
    } else if (bytes < 1024 * 1024) {
        return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    } else if (bytes < 1024 * 1024 * 1024) {
        return QString("%1 MB").arg(bytes / (1024.0 * 1024), 0, 'f', 1);
    } else {
        return QString("%1 GB").arg(bytes / (1024.0 * 1024 * 1024), 0, 'f', 1);
    }
}
