#include "DualBootWidget.h"
#include "core/PackageManager.h"
#include <QMessageBox>
#include <QProcess>
#include <QTimer>
#include <QInputDialog>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QApplication>
#include <QStandardPaths>

DualBootWidget::DualBootWidget(QWidget* parent)
    : QWidget(parent)
    , m_isDualBoot(false)
    , m_forceShow(false)
    , m_defaultBootCombo(nullptr)
    , m_bootTimeoutSpin(nullptr)
    , m_bootMenuCheck(nullptr)
    , m_partitionListLayout(nullptr)
    , m_windowsStatusLabel(nullptr)
{
    m_isDualBoot = detectDualBoot();
    if (m_isDualBoot) {
        initPartitions();
        initBootEntries();
    }
    setupUI();
}

bool DualBootWidget::detectDualBoot()
{
    bool hasWindowsPartition = detectWindowsPartitions();
    bool hasWindowsBoot = detectWindowsBootEntry();
    return hasWindowsPartition || hasWindowsBoot;
}

bool DualBootWidget::detectWindowsPartitions()
{
    QProcess process;
    process.start("lsblk", QStringList() << "-o" << "NAME,FSTYPE,LABEL,MOUNTPOINT,SIZE" << "-n" << "-l");
    process.waitForFinished(3000);
    
    if (process.exitCode() != 0) {
        QProcess process2;
        process2.start("blkid", QStringList());
        process2.waitForFinished(3000);
        QString output = process2.readAllStandardOutput();
        return output.contains("ntfs", Qt::CaseInsensitive) ||
               output.contains("vfat", Qt::CaseInsensitive);
    }
    
    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    
    for (const QString& line : lines) {
        if (line.contains("ntfs", Qt::CaseInsensitive) ||
            line.contains("NTFS", Qt::CaseInsensitive)) {
            return true;
        }
    }
    
    QDir efiDir("/boot/efi/EFI");
    if (efiDir.exists()) {
        QStringList entries = efiDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& entry : entries) {
            if (entry.contains("Microsoft", Qt::CaseInsensitive) ||
                entry.contains("Windows", Qt::CaseInsensitive)) {
                return true;
            }
        }
    }
    
    return false;
}

bool DualBootWidget::detectWindowsBootEntry()
{
    QProcess process;
    process.start("efibootmgr", QStringList());
    process.waitForFinished(3000);
    
    if (process.exitCode() == 0) {
        QString output = process.readAllStandardOutput();
        if (output.contains("Windows", Qt::CaseInsensitive) ||
            output.contains("Microsoft", Qt::CaseInsensitive)) {
            return true;
        }
    }
    
    QFile grubCfg("/boot/grub/grub.cfg");
    if (grubCfg.open(QFile::ReadOnly)) {
        QString content = grubCfg.readAll();
        grubCfg.close();
        if (content.contains("Windows", Qt::CaseInsensitive) ||
            content.contains("chainloader", Qt::CaseInsensitive)) {
            return true;
        }
    }
    
    return false;
}

void DualBootWidget::initPartitions()
{
    m_partitions.clear();

    QProcess process;
    process.start("lsblk", QStringList()
        << "-o" << "NAME,FSTYPE,LABEL,MOUNTPOINT,SIZE"
        << "-n" << "-l" << "-p");
    if (!process.waitForFinished(5000)) return;

    QString output = QString::fromUtf8(process.readAllStandardOutput());
    const QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (const QString& line : lines) {
        // 按空白分割，保留足够字段
        QStringList parts = line.split(QRegularExpression("\\s{2,}"), Qt::SkipEmptyParts);
        if (parts.size() < 2) continue;

        PartitionInfo p;
        p.device = parts[0].trimmed();
        // 跳过没有文件系统的整盘设备
        if (parts.size() >= 2 && parts[1].trimmed().isEmpty()) continue;

        p.fsType = parts.size() >= 2 ? parts[1].trimmed() : QString();
        p.label = parts.size() >= 3 ? parts[2].trimmed() : QString();
        p.mountPoint = parts.size() >= 4 ? parts[3].trimmed() : QString();
        p.size = parts.size() >= 5 ? parts[4].trimmed() : QString();
        p.isMounted = !p.mountPoint.isEmpty();
        p.used = QString(); // lsblk 默认不显示已用空间，留空
        p.isWindows = (p.fsType.compare("ntfs", Qt::CaseInsensitive) == 0 ||
                       p.fsType.compare("vfat", Qt::CaseInsensitive) == 0 ||
                       p.label.contains("windows", Qt::CaseInsensitive) ||
                       p.label.contains("efi", Qt::CaseInsensitive));

        // 只添加有文件系统的分区
        if (!p.fsType.isEmpty()) {
            m_partitions.append(p);
        }
    }
}

void DualBootWidget::initBootEntries()
{
    m_bootEntries.clear();

    // 尝试从 grub.cfg 解析启动项
    QFile grubCfg("/boot/grub/grub.cfg");
    if (grubCfg.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&grubCfg);
        int idx = 0;
        while (!in.atEnd() && m_bootEntries.size() < 10) {
            QString line = in.readLine().trimmed();
            if (line.startsWith("menuentry '") || line.startsWith("menuentry \"")) {
                // 提取菜单项名称
                int start = line.indexOf('\'');
                if (start < 0) start = line.indexOf('"');
                if (start >= 0) {
                    QChar quote = line[start];
                    int end = line.indexOf(quote, start + 1);
                    if (end > start) {
                        BootEntry e;
                        e.id = QString::number(idx);
                        e.name = line.mid(start + 1, end - start - 1);
                        e.isDefault = (idx == 0);
                        if (e.name.contains("windows", Qt::CaseInsensitive)) {
                            e.type = "Windows";
                        } else if (e.name.contains("uefi", Qt::CaseInsensitive) ||
                                   e.name.contains("setup", Qt::CaseInsensitive) ||
                                   e.name.contains("设置", Qt::CaseInsensitive)) {
                            e.type = "UEFI";
                        } else {
                            e.type = "Linux";
                        }
                        m_bootEntries.append(e);
                        idx++;
                    }
                }
            }
        }
        grubCfg.close();
    }

    // 如果 grub.cfg 解析失败，尝试从 EFI 目录检测
    if (m_bootEntries.isEmpty()) {
        QDir efiDir("/boot/efi/EFI");
        if (efiDir.exists()) {
            QStringList entries = efiDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (int i = 0; i < entries.size() && m_bootEntries.size() < 10; i++) {
                BootEntry e;
                e.id = QString::number(i);
                e.name = entries[i];
                e.isDefault = (i == 0);
                if (entries[i].contains("windows", Qt::CaseInsensitive) ||
                    entries[i].contains("Microsoft", Qt::CaseInsensitive)) {
                    e.type = "Windows";
                } else if (entries[i].contains("ubuntu", Qt::CaseInsensitive) ||
                           entries[i].contains("fedora", Qt::CaseInsensitive) ||
                           entries[i].contains("debian", Qt::CaseInsensitive)) {
                    e.type = "Linux";
                } else {
                    e.type = "UEFI";
                }
                m_bootEntries.append(e);
            }
        }
    }

    // 如果还是没有，至少添加一个 Linux 项
    if (m_bootEntries.isEmpty()) {
        BootEntry e;
        e.id = "0";
        e.name = tr("当前 Linux 系统");
        e.type = "Linux";
        e.isDefault = true;
        m_bootEntries.append(e);
    }
}

void DualBootWidget::setupUI()
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

    QLabel* title = new QLabel(tr("🔄 Windows 双系统工具箱"));
    title->setStyleSheet("font-size: 22px; font-weight: 700; color: #0f172a;");
    contentLayout->addWidget(title);

    QLabel* subtitle = new QLabel(tr("启动项管理 · 分区工具 · 跨系统文件访问，双系统用户一站式解决方案"));
    subtitle->setStyleSheet("font-size: 13px; color: #64748b;");
    contentLayout->addWidget(subtitle);

    contentLayout->addWidget(createStatusCard());

    if (m_isDualBoot || m_forceShow) {
        contentLayout->addWidget(createBootManagerSection());
        contentLayout->addWidget(createPartitionSection());
        contentLayout->addWidget(createFileAccessSection());

        QLabel* toolsTitle = new QLabel(tr("🛠️ 快速修复工具"));
        toolsTitle->setStyleSheet("font-size: 15px; font-weight: 600; color: #0f172a; margin-top: 8px;");
        contentLayout->addWidget(toolsTitle);

        QFrame* toolsFrame = new QFrame();
        toolsFrame->setObjectName("card");
        QVBoxLayout* toolsLayout = new QVBoxLayout(toolsFrame);
        toolsLayout->setContentsMargins(16, 16, 16, 16);
        toolsLayout->setSpacing(12);
        setupToolsLayout(toolsLayout);
        contentLayout->addWidget(toolsFrame);
    } else {
        contentLayout->addWidget(createNotFoundCard());
    }

    contentLayout->addStretch(1);

    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);
}

QFrame* DualBootWidget::createStatusCard()
{
    QFrame* card = new QFrame();
    card->setObjectName("card");
    QHBoxLayout* layout = new QHBoxLayout(card);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(16);

    QLabel* iconLabel = new QLabel("💻");
    iconLabel->setStyleSheet("font-size: 32px;");
    iconLabel->setFixedWidth(48);
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);

    QVBoxLayout* infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(4);

    QLabel* statusTitle = new QLabel();
    if (m_isDualBoot || m_forceShow) {
        statusTitle->setText(tr("检测到 Windows + Linux 双系统"));
    } else {
        statusTitle->setText(tr("未检测到 Windows 系统"));
    }
    statusTitle->setStyleSheet("font-size: 15px; font-weight: 600; color: #0f172a;");
    infoLayout->addWidget(statusTitle);

    m_windowsStatusLabel = new QLabel();
    if (m_isDualBoot || m_forceShow) {
        m_windowsStatusLabel->setText(tr("已找到 Windows 分区，可进行双系统相关操作"));
        m_windowsStatusLabel->setStyleSheet("font-size: 12px; color: #16a34a;");
    } else {
        m_windowsStatusLabel->setText(tr("当前系统似乎只有 Linux，若确定是双系统可点击强制显示"));
        m_windowsStatusLabel->setStyleSheet("font-size: 12px; color: #64748b;");
    }
    infoLayout->addWidget(m_windowsStatusLabel);

    layout->addLayout(infoLayout, 1);

    if (!m_isDualBoot && !m_forceShow) {
        QPushButton* detectBtn = new QPushButton(tr("重新检测"));
        detectBtn->setObjectName("secondaryBtn");
        detectBtn->setFixedHeight(36);
        detectBtn->setCursor(Qt::PointingHandCursor);
        connect(detectBtn, &QPushButton::clicked, this, &DualBootWidget::onRedetectClicked);
        layout->addWidget(detectBtn);

        QPushButton* forceBtn = new QPushButton(tr("强制显示"));
        forceBtn->setObjectName("secondaryBtn");
        forceBtn->setFixedHeight(36);
        forceBtn->setCursor(Qt::PointingHandCursor);
        connect(forceBtn, &QPushButton::clicked, this, &DualBootWidget::onForceShowClicked);
        layout->addWidget(forceBtn);
    }

    return card;
}

QFrame* DualBootWidget::createBootManagerSection()
{
    QFrame* section = new QFrame();
    section->setObjectName("card");
    QVBoxLayout* layout = new QVBoxLayout(section);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);

    QHBoxLayout* headerRow = new QHBoxLayout();
    QLabel* headerIcon = new QLabel("⚙️");
    headerIcon->setStyleSheet("font-size: 20px;");
    headerRow->addWidget(headerIcon);
    QLabel* headerTitle = new QLabel(tr("启动项管理"));
    headerTitle->setStyleSheet("font-size: 15px; font-weight: 600; color: #0f172a;");
    headerRow->addWidget(headerTitle);
    headerRow->addStretch(1);
    layout->addLayout(headerRow);

    QHBoxLayout* defaultRow = new QHBoxLayout();
    defaultRow->setSpacing(12);
    QLabel* defaultLabel = new QLabel(tr("默认启动项:"));
    defaultLabel->setStyleSheet("font-size: 13px; color: #334155; min-width: 90px;");
    defaultRow->addWidget(defaultLabel);
    m_defaultBootCombo = new QComboBox();
    for (const auto& e : m_bootEntries) {
        m_defaultBootCombo->addItem(e.name, e.id);
        if (e.isDefault) {
            m_defaultBootCombo->setCurrentText(e.name);
        }
    }
    m_defaultBootCombo->setStyleSheet("font-size: 13px;");
    connect(m_defaultBootCombo, &QComboBox::currentTextChanged, this, &DualBootWidget::onDefaultBootChanged);
    defaultRow->addWidget(m_defaultBootCombo, 1);
    QPushButton* applyDefaultBtn = new QPushButton(tr("应用"));
    applyDefaultBtn->setFixedHeight(32);
    applyDefaultBtn->setCursor(Qt::PointingHandCursor);
    applyDefaultBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #3b82f6;
            color: white;
            border: none;
            border-radius: 6px;
            font-size: 12px;
            padding: 0 16px;
        }
        QPushButton:hover { background-color: #2563eb; }
    )");
    connect(applyDefaultBtn, &QPushButton::clicked, this, &DualBootWidget::onSetDefaultBootClicked);
    defaultRow->addWidget(applyDefaultBtn);
    layout->addLayout(defaultRow);

    QHBoxLayout* timeoutRow = new QHBoxLayout();
    timeoutRow->setSpacing(12);
    QLabel* timeoutLabel = new QLabel(tr("菜单超时:"));
    timeoutLabel->setStyleSheet("font-size: 13px; color: #334155; min-width: 90px;");
    timeoutRow->addWidget(timeoutLabel);
    m_bootTimeoutSpin = new QSpinBox();
    m_bootTimeoutSpin->setRange(0, 60);
    m_bootTimeoutSpin->setValue(10);
    m_bootTimeoutSpin->setSuffix(tr(" 秒"));
    m_bootTimeoutSpin->setStyleSheet("font-size: 13px;");
    connect(m_bootTimeoutSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &DualBootWidget::onBootTimeoutChanged);
    timeoutRow->addWidget(m_bootTimeoutSpin);
    timeoutRow->addStretch(1);
    layout->addLayout(timeoutRow);

    m_bootMenuCheck = new QCheckBox(tr("显示启动菜单"));
    m_bootMenuCheck->setChecked(true);
    m_bootMenuCheck->setStyleSheet("font-size: 13px; color: #334155;");
    layout->addWidget(m_bootMenuCheck);

    QHBoxLayout* btnRow = new QHBoxLayout();
    btnRow->setSpacing(10);

    QPushButton* repairBtn = new QPushButton(tr("🔧 修复GRUB引导"));
    repairBtn->setObjectName("secondaryBtn");
    repairBtn->setFixedHeight(36);
    repairBtn->setCursor(Qt::PointingHandCursor);
    connect(repairBtn, &QPushButton::clicked, this, &DualBootWidget::onRepairGrubClicked);
    btnRow->addWidget(repairBtn);

    QPushButton* updateGrubBtn = new QPushButton(tr("🔄 更新GRUB配置"));
    updateGrubBtn->setObjectName("secondaryBtn");
    updateGrubBtn->setFixedHeight(36);
    updateGrubBtn->setCursor(Qt::PointingHandCursor);
    connect(updateGrubBtn, &QPushButton::clicked, this, &DualBootWidget::onUpdateGrubClicked);
    btnRow->addWidget(updateGrubBtn);

    layout->addLayout(btnRow);

    return section;
}

QFrame* DualBootWidget::createPartitionSection()
{
    QFrame* section = new QFrame();
    section->setObjectName("card");
    QVBoxLayout* layout = new QVBoxLayout(section);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);

    QHBoxLayout* headerRow = new QHBoxLayout();
    QLabel* headerIcon = new QLabel("💾");
    headerIcon->setStyleSheet("font-size: 20px;");
    headerRow->addWidget(headerIcon);
    QLabel* headerTitle = new QLabel(tr("分区管理"));
    headerTitle->setStyleSheet("font-size: 15px; font-weight: 600; color: #0f172a;");
    headerRow->addWidget(headerTitle);
    headerRow->addStretch(1);
    layout->addLayout(headerRow);

    m_partitionListLayout = new QVBoxLayout();
    m_partitionListLayout->setSpacing(8);

    for (const auto& p : m_partitions) {
        QFrame* partCard = new QFrame();
        partCard->setStyleSheet(R"(
            QFrame {
                background-color: #f8fafc;
                border: 1px solid #e2e8f0;
                border-radius: 8px;
            }
        )");
        QHBoxLayout* partLayout = new QHBoxLayout(partCard);
        partLayout->setContentsMargins(12, 10, 12, 10);
        partLayout->setSpacing(10);

        QString partIcon = p.isWindows ? "🪟" : "🐧";
        QLabel* iconLbl = new QLabel(partIcon);
        iconLbl->setStyleSheet("font-size: 20px;");
        iconLbl->setFixedWidth(28);
        iconLbl->setAlignment(Qt::AlignCenter);
        partLayout->addWidget(iconLbl);

        QVBoxLayout* partInfo = new QVBoxLayout();
        partInfo->setSpacing(2);

        QHBoxLayout* nameRow = new QHBoxLayout();
        nameRow->setSpacing(8);
        QLabel* nameLbl = new QLabel(p.label);
        nameLbl->setStyleSheet("font-size: 13px; font-weight: 600; color: #0f172a;");
        nameRow->addWidget(nameLbl);
        QLabel* devLbl = new QLabel(p.device);
        devLbl->setStyleSheet("font-size: 11px; color: #94a3b8;");
        nameRow->addWidget(devLbl);
        if (p.isMounted) {
            QLabel* mountBadge = new QLabel(tr("已挂载"));
            mountBadge->setStyleSheet(R"(
                QLabel {
                    background-color: #dcfce7;
                    color: #166534;
                    border-radius: 4px;
                    padding: 1px 6px;
                    font-size: 10px;
                    font-weight: 600;
                }
            )");
            nameRow->addWidget(mountBadge);
        }
        nameRow->addStretch(1);
        partInfo->addLayout(nameRow);

        QHBoxLayout* detailRow = new QHBoxLayout();
        detailRow->setSpacing(16);
        QLabel* fsLbl = new QLabel(tr("文件系统: ") + p.fsType);
        fsLbl->setStyleSheet("font-size: 11px; color: #64748b;");
        detailRow->addWidget(fsLbl);
        QLabel* sizeLbl = new QLabel(tr("大小: ") + p.size);
        sizeLbl->setStyleSheet("font-size: 11px; color: #64748b;");
        detailRow->addWidget(sizeLbl);
        QLabel* usedLbl = new QLabel(tr("已用: ") + (p.used.isEmpty() ? tr("未知") : p.used));
        usedLbl->setStyleSheet("font-size: 11px; color: #64748b;");
        detailRow->addWidget(usedLbl);
        if (p.isMounted && !p.mountPoint.isEmpty()) {
            QLabel* mpLbl = new QLabel(tr("挂载点: ") + p.mountPoint);
            mpLbl->setStyleSheet("font-size: 11px; color: #64748b;");
            detailRow->addWidget(mpLbl);
        }
        detailRow->addStretch(1);
        partInfo->addLayout(detailRow);

        partLayout->addLayout(partInfo, 1);

        if (p.isWindows && !p.isMounted) {
            QPushButton* mountBtn = new QPushButton(tr("挂载"));
            mountBtn->setFixedHeight(30);
            mountBtn->setCursor(Qt::PointingHandCursor);
            mountBtn->setStyleSheet(R"(
                QPushButton {
                    background-color: #dbeafe;
                    color: #1d4ed8;
                    border: none;
                    border-radius: 6px;
                    font-size: 11px;
                    padding: 0 12px;
                }
                QPushButton:hover { background-color: #bfdbfe; }
            )");
            QString dev = p.device;
            connect(mountBtn, &QPushButton::clicked, [this, dev]() {
                onMountPartition(dev);
            });
            partLayout->addWidget(mountBtn);
        } else if (p.isWindows && p.isMounted && p.mountPoint != "/boot/efi") {
            QPushButton* unmountBtn = new QPushButton(tr("卸载"));
            unmountBtn->setFixedHeight(30);
            unmountBtn->setCursor(Qt::PointingHandCursor);
            unmountBtn->setStyleSheet(R"(
                QPushButton {
                    background-color: #fee2e2;
                    color: #dc2626;
                    border: none;
                    border-radius: 6px;
                    font-size: 11px;
                    padding: 0 12px;
                }
                QPushButton:hover { background-color: #fecaca; }
            )");
            QString dev = p.device;
            connect(unmountBtn, &QPushButton::clicked, [this, dev]() {
                onUnmountPartition(dev);
            });
            partLayout->addWidget(unmountBtn);
        }

        m_partitionListLayout->addWidget(partCard);
    }

    layout->addLayout(m_partitionListLayout);

    return section;
}

QFrame* DualBootWidget::createFileAccessSection()
{
    QFrame* section = new QFrame();
    section->setObjectName("card");
    QVBoxLayout* layout = new QVBoxLayout(section);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);

    QHBoxLayout* headerRow = new QHBoxLayout();
    QLabel* headerIcon = new QLabel("📁");
    headerIcon->setStyleSheet("font-size: 20px;");
    headerRow->addWidget(headerIcon);
    QLabel* headerTitle = new QLabel(tr("跨系统文件访问"));
    headerTitle->setStyleSheet("font-size: 15px; font-weight: 600; color: #0f172a;");
    headerRow->addWidget(headerTitle);
    headerRow->addStretch(1);
    layout->addLayout(headerRow);

    QFrame* infoFrame = new QFrame();
    infoFrame->setStyleSheet(R"(
        QFrame {
            background-color: #eff6ff;
            border-radius: 8px;
        }
    )");
    QVBoxLayout* infoLayout = new QVBoxLayout(infoFrame);
    infoLayout->setContentsMargins(14, 12, 14, 12);
    infoLayout->setSpacing(8);

    QLabel* infoTitle = new QLabel(tr("💡 NTFS 读写支持"));
    infoTitle->setStyleSheet("font-size: 13px; font-weight: 600; color: #1d4ed8;");
    infoLayout->addWidget(infoTitle);

    QLabel* infoDesc = new QLabel(tr("通过 ntfs-3g 驱动，可以在 Linux 中读写 Windows 的 NTFS 分区文件。支持创建、修改、删除文件。"));
    infoDesc->setStyleSheet("font-size: 12px; color: #3b82f6;");
    infoDesc->setWordWrap(true);
    infoLayout->addWidget(infoDesc);

    layout->addWidget(infoFrame);

    QHBoxLayout* btnRow = new QHBoxLayout();
    btnRow->setSpacing(10);

    QPushButton* openWinBtn = new QPushButton(tr("📂 打开 Windows 文件"));
    openWinBtn->setFixedHeight(38);
    openWinBtn->setCursor(Qt::PointingHandCursor);
    openWinBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #3b82f6;
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 13px;
            font-weight: 500;
        }
        QPushButton:hover { background-color: #2563eb; }
    )");
    connect(openWinBtn, &QPushButton::clicked, this, &DualBootWidget::onOpenWindowsFiles);
    btnRow->addWidget(openWinBtn);

    QPushButton* installNtfsBtn = new QPushButton(tr("安装 ntfs-3g"));
    installNtfsBtn->setObjectName("secondaryBtn");
    installNtfsBtn->setFixedHeight(38);
    installNtfsBtn->setCursor(Qt::PointingHandCursor);
    connect(installNtfsBtn, &QPushButton::clicked, this, &DualBootWidget::onInstallNTFSClicked);
    btnRow->addWidget(installNtfsBtn);

    btnRow->addStretch(1);
    layout->addLayout(btnRow);

    QFrame* tipFrame = new QFrame();
    tipFrame->setStyleSheet(R"(
        QFrame {
            background-color: #fefce8;
            border-radius: 8px;
        }
    )");
    QHBoxLayout* tipLayout = new QHBoxLayout(tipFrame);
    tipLayout->setContentsMargins(12, 10, 12, 10);
    tipLayout->setSpacing(8);
    QLabel* tipIcon = new QLabel("⚠️");
    tipIcon->setStyleSheet("font-size: 16px;");
    tipLayout->addWidget(tipIcon);
    QLabel* tipText = new QLabel(tr("提示：修改 Windows 系统分区文件时请谨慎操作，避免损坏系统文件！"));
    tipText->setStyleSheet("font-size: 11px; color: #a16207;");
    tipText->setWordWrap(true);
    tipLayout->addWidget(tipText, 1);
    layout->addWidget(tipFrame);

    return section;
}

QFrame* DualBootWidget::createToolCard(const QString& icon, const QString& title,
                                       const QString& desc, const QString& command,
                                       const QString& btnText, const QString& btnObjectName,
                                       const char* slot)
{
    Q_UNUSED(command);
    QFrame* card = new QFrame();
    card->setStyleSheet(R"(
        QFrame {
            background-color: #f8fafc;
            border: 1px solid #e2e8f0;
            border-radius: 10px;
        }
        QFrame:hover {
            background-color: #f1f5f9;
            border-color: #cbd5e1;
        }
    )");

    QHBoxLayout* layout = new QHBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(14);

    QLabel* iconLabel = new QLabel(icon);
    iconLabel->setStyleSheet("font-size: 28px;");
    iconLabel->setFixedWidth(40);
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);

    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->setSpacing(4);

    QLabel* titleLabel = new QLabel(title);
    titleLabel->setStyleSheet("font-size: 14px; font-weight: 600; color: #0f172a;");
    textLayout->addWidget(titleLabel);

    QLabel* descLabel = new QLabel(desc);
    descLabel->setStyleSheet("font-size: 12px; color: #64748b;");
    descLabel->setWordWrap(true);
    textLayout->addWidget(descLabel);

    layout->addLayout(textLayout, 1);

    QPushButton* actionBtn = new QPushButton(btnText);
    actionBtn->setObjectName(btnObjectName);
    actionBtn->setFixedHeight(36);
    actionBtn->setCursor(Qt::PointingHandCursor);
    connect(actionBtn, SIGNAL(clicked()), this, slot);
    layout->addWidget(actionBtn);

    return card;
}

QFrame* DualBootWidget::createNotFoundCard()
{
    QFrame* card = new QFrame();
    card->setObjectName("card");
    QVBoxLayout* layout = new QVBoxLayout(card);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(16);
    layout->setAlignment(Qt::AlignCenter);

    QLabel* iconLabel = new QLabel("🔍");
    iconLabel->setStyleSheet("font-size: 48px;");
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);

    QLabel* titleLabel = new QLabel(tr("未检测到 Windows 系统"));
    titleLabel->setStyleSheet("font-size: 16px; font-weight: 600; color: #0f172a;");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    QLabel* descLabel = new QLabel(tr("当前系统可能不是双系统配置。\n如果您确定安装了 Windows，可以点击上方按钮强制显示工具。"));
    descLabel->setStyleSheet("font-size: 13px; color: #64748b;");
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setWordWrap(true);
    layout->addWidget(descLabel);

    return card;
}

void DualBootWidget::setupToolsLayout(QVBoxLayout* layout)
{
    layout->addWidget(createToolCard(
        "⏰", tr("修复双系统时间不一致"),
        tr("Windows 和 Linux 显示时间相差 8 小时？一键修复时区设置问题。"),
        "fix_time", tr("立即修复"), "secondaryBtn",
        SLOT(onFixTimeClicked())
    ));

    layout->addWidget(createToolCard(
        "🔧", tr("修复 GRUB 引导"),
        tr("重装系统后 GRUB 丢失？Windows 覆盖了引导？一键修复启动项。"),
        "fix_grub", tr("修复引导"), "secondaryBtn",
        SLOT(onFixGrubClicked())
    ));

    layout->addWidget(createToolCard(
        "📂", tr("安装 NTFS 读写支持"),
        tr("让 Linux 可以读写 Windows 的 NTFS 分区文件，实现数据互通。"),
        "install_ntfs", tr("立即安装"), "secondaryBtn",
        SLOT(onInstallNTFSClicked())
    ));

    layout->addWidget(createToolCard(
        "⚙️", tr("启动项管理"),
        tr("设置默认启动系统、调整等待时间、管理 GRUB 菜单。"),
        "boot_manage", tr("打开设置"), "secondaryBtn",
        SLOT(onBootManageClicked())
    ));
}

void DualBootWidget::onFixTimeClicked()
{
    auto reply = QMessageBox::question(this, tr("修复时间不一致"),
        tr("Windows 和 Linux 时间不一致通常是因为 Windows 使用本地时间，\n"
           "而 Linux 使用 UTC 时间导致的。\n\n"
           "本操作将把 Linux 设置为使用本地时间，与 Windows 保持一致。\n"
           "确定继续吗？"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    QProcess proc;
    proc.start("pkexec", QStringList() << "timedatectl" << "set-local-rtc" << "1" << "--adjust-system-clock");
    proc.waitForFinished(10000);

    QApplication::restoreOverrideCursor();

    if (proc.exitCode() == 0) {
        QMessageBox::information(this, tr("修复完成"),
            tr("时间不一致问题已修复！\n\n已执行 timedatectl set-local-rtc 1 命令。\n重启后生效。"));
    } else {
        QString err = QString::fromUtf8(proc.readAllStandardError());
        QMessageBox::warning(this, tr("修复失败"),
            tr("执行 timedatectl 失败：\n%1").arg(err.left(300)));
    }
}

void DualBootWidget::onInstallNTFSClicked()
{
    auto reply = QMessageBox::question(this, tr("安装 NTFS 读写支持"),
        tr("将安装 ntfs-3g 软件包，使 Linux 能够读写 Windows 的 NTFS 分区。\n\n确定安装吗？"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes);

    if (reply == QMessageBox::Yes) {
        // 通过 PackageManager 真实安装 ntfs-3g
        PackageManager::instance()->installPackage("ntfs-3g");
    }
}

void DualBootWidget::onFixGrubClicked()
{
    auto reply = QMessageBox::question(this, tr("修复 GRUB 引导"),
        tr("将重新安装 GRUB 引导加载程序并更新启动项配置。\n\n"
           "这通常可以解决以下问题：\n"
           "• 重装 Windows 后无法进入 Linux\n"
           "• 启动菜单中缺少某些系统选项\n"
           "• GRUB 损坏无法启动\n\n"
           "确定继续吗？"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    // 检测启动设备：EFI 系统使用 /boot/efi 所在磁盘，BIOS 使用 / 所在磁盘
    QString bootDevice;
    QProcess lsblk;
    lsblk.start("lsblk", QStringList() << "-o" << "NAME,MOUNTPOINT" << "-n" << "-l" << "-p");
    if (lsblk.waitForFinished(3000)) {
        QString output = QString::fromUtf8(lsblk.readAllStandardOutput());
        QStringList lines = output.split('\n');
        for (const QString& line : lines) {
            if (line.contains("/boot/efi") || line.contains("/boot ")) {
                QString dev = line.section(' ', 0, 0).trimmed();
                // 去掉分区号得到磁盘设备
                bootDevice = dev;
                // nvme0n1p1 -> nvme0n1, sda1 -> sda
                static QRegularExpression re("(nvme[0-9]+n[0-9]+|sd[a-z]+|mmcblk[0-9]+|vd[a-z]+)p?[0-9]+$");
                auto m = re.match(dev);
                if (m.hasMatch()) bootDevice = m.captured(1);
                break;
            }
        }
        // 回退：用根分区所在磁盘
        if (bootDevice.isEmpty()) {
            for (const QString& line : lines) {
                if (line.trimmed().endsWith(" /")) {
                    QString dev = line.section(' ', 0, 0).trimmed();
                    static QRegularExpression re("(nvme[0-9]+n[0-9]+|sd[a-z]+|mmcblk[0-9]+|vd[a-z]+)p?[0-9]+$");
                    auto m = re.match(dev);
                    if (m.hasMatch()) bootDevice = m.captured(1);
                    break;
                }
            }
        }
    }

    if (bootDevice.isEmpty()) {
        QMessageBox::warning(this, tr("无法确定启动设备"),
            tr("无法自动检测启动设备。\n\n请手动执行：\n"
               "  sudo grub-install /dev/sdX\n"
               "  sudo update-grub\n\n"
               "（将 /dev/sdX 替换为你的系统盘，如 /dev/sda 或 /dev/nvme0n1）"));
        return;
    }

    QMessageBox::information(this, tr("修复中"),
        tr("正在修复 GRUB 引导...\n\n设备: %1\n\n请在弹出的授权窗口中输入密码。").arg(bootDevice));

    QApplication::setOverrideCursor(Qt::WaitCursor);

    // 执行 grub-install
    QProcess installProc;
    installProc.start("pkexec", QStringList() << "grub-install" << bootDevice);
    installProc.waitForFinished(60000);
    int installCode = installProc.exitCode();

    if (installCode != 0) {
        QApplication::restoreOverrideCursor();
        QString err = QString::fromUtf8(installProc.readAllStandardError());
        QMessageBox::warning(this, tr("GRUB 安装失败"),
            tr("grub-install 执行失败：\n%1\n\n"
               "可能原因：\n"
               "• EFI 系统需确保 /boot/efi 已挂载\n"
               "• 磁盘设备检测错误\n"
               "• 请检查输出并手动修复").arg(err.left(400)));
        return;
    }

    // 执行 update-grub（或 grub2-mkconfig）
    QProcess updateProc;
    if (QFile::exists("/usr/sbin/update-grub")) {
        updateProc.start("pkexec", QStringList() << "update-grub");
    } else {
        updateProc.start("pkexec", QStringList() << "grub2-mkconfig" << "-o" << "/boot/grub2/grub.cfg");
    }
    updateProc.waitForFinished(60000);

    QApplication::restoreOverrideCursor();

    if (updateProc.exitCode() == 0) {
        QMessageBox::information(this, tr("修复成功"),
            tr("GRUB 引导已修复！\n\n重启后即可看到完整的启动菜单。"));
    } else {
        QString err = QString::fromUtf8(updateProc.readAllStandardError());
        QMessageBox::warning(this, tr("更新 GRUB 配置失败"),
            tr("grub-install 成功，但更新配置失败：\n%1").arg(err.left(400)));
    }
}

void DualBootWidget::onBootManageClicked()
{
    QMessageBox::information(this, tr("启动项管理"),
        tr("启动项管理功能已在上方提供。\n\n" "您可以：\n" "• 设置默认启动系统\n" "• 调整菜单等待时间\n" "• 修复 GRUB 引导\n" "• 更新 GRUB 配置"));
}

void DualBootWidget::onRedetectClicked()
{
    QMessageBox::information(this, tr("重新检测"),
        tr("正在重新检测双系统...\n\n" "正在扫描磁盘分区和启动项..."));

    QTimer::singleShot(1500, [this]() {
        bool detected = detectDualBoot();
        if (detected && !m_isDualBoot) {
            m_isDualBoot = true;
            initPartitions();
            initBootEntries();
            setupUI();
            QMessageBox::information(this, tr("检测结果"),
                tr("检测成功！已找到 Windows 系统。"));
        } else if (detected) {
            QMessageBox::information(this, tr("检测结果"),
                tr("检测确认：已找到 Windows 系统。"));
        } else {
            QMessageBox::information(this, tr("检测结果"),
                tr("未检测到 Windows 系统。\n\n" "如果您确定安装了 Windows，可以点击「强制显示」按钮使用工具。"));
        }
    });
}

void DualBootWidget::onForceShowClicked()
{
    m_forceShow = true;
    if (m_partitions.isEmpty()) {
        initPartitions();
    }
    if (m_bootEntries.isEmpty()) {
        initBootEntries();
    }
    setupUI();
}

void DualBootWidget::onMountPartition(const QString& device)
{
    // 找到分区信息
    PartitionInfo* target = nullptr;
    for (auto& p : m_partitions) {
        if (p.device == device) { target = &p; break; }
    }
    if (!target) return;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("挂载分区"),
        QString(tr("确定要挂载分区 %1 吗？\n\n"
                   "文件系统: %2\n"
                   "挂载后可以在文件管理器中访问该分区的文件。"))
            .arg(device, target->fsType),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes
    );

    if (reply != QMessageBox::Yes) return;

    // 生成挂载点：/mnt/<label或设备名>
    QString mountLabel = target->label.isEmpty()
        ? device.section('/', -1)
        : target->label.toLower().replace(QRegularExpression("[^a-z0-9]"), "_");
    QString mountPoint = "/mnt/" + mountLabel;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    // 1. 创建挂载点目录
    QProcess mkdirProc;
    mkdirProc.start("pkexec", QStringList() << "mkdir" << "-p" << mountPoint);
    mkdirProc.waitForFinished(5000);

    // 2. 挂载分区
    QProcess mountProc;
    QStringList args;
    args << "mount";
    if (!target->fsType.isEmpty() && target->fsType.compare("ntfs", Qt::CaseInsensitive) == 0) {
        args << "-t" << "ntfs-3g";
    } else if (!target->fsType.isEmpty() && target->fsType.compare("vfat", Qt::CaseInsensitive) == 0) {
        args << "-t" << "vfat";
    }
    args << device << mountPoint;
    mountProc.start("pkexec", args);
    mountProc.waitForFinished(15000);

    QApplication::restoreOverrideCursor();

    if (mountProc.exitCode() == 0) {
        target->isMounted = true;
        target->mountPoint = mountPoint;
        setupUI();
        QMessageBox::information(this, tr("挂载成功"),
            QString(tr("分区 %1 已成功挂载！\n\n挂载点: %2")).arg(device, mountPoint));
    } else {
        QString err = QString::fromUtf8(mountProc.readAllStandardError());
        QMessageBox::warning(this, tr("挂载失败"),
            QString(tr("挂载分区 %1 失败：\n%2\n\n"
                       "可能原因：\n"
                       "• NTFS 分区需先安装 ntfs-3g\n"
                       "• Windows 未完全关机（快速启动）\n"
                       "• 分区已损坏"))
                .arg(device, err.left(300)));
    }
}

void DualBootWidget::onUnmountPartition(const QString& device)
{
    PartitionInfo* target = nullptr;
    for (auto& p : m_partitions) {
        if (p.device == device) { target = &p; break; }
    }
    if (!target || !target->isMounted) return;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("卸载分区"),
        QString(tr("确定要卸载分区 %1 吗？\n\n挂载点: %2\n\n卸载后将无法再访问该分区的文件。"))
            .arg(device, target->mountPoint),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply != QMessageBox::Yes) return;

    QApplication::setOverrideCursor(Qt::WaitCursor);

    QProcess umountProc;
    umountProc.start("pkexec", QStringList() << "umount" << target->mountPoint);
    umountProc.waitForFinished(10000);

    QApplication::restoreOverrideCursor();

    if (umountProc.exitCode() == 0) {
        target->isMounted = false;
        target->mountPoint = "";
        setupUI();
        QMessageBox::information(this, tr("卸载成功"),
            QString(tr("分区 %1 已卸载。")).arg(device));
    } else {
        QString err = QString::fromUtf8(umountProc.readAllStandardError());
        QMessageBox::warning(this, tr("卸载失败"),
            tr("卸载失败：\n%1\n\n可能有文件正在被使用，请关闭相关窗口后重试。").arg(err.left(300)));
    }
}

void DualBootWidget::onOpenWindowsFiles()
{
    PartitionInfo* target = nullptr;
    for (auto& p : m_partitions) {
        if (p.isWindows && p.isMounted && p.mountPoint != "/boot/efi") {
            target = &p;
            break;
        }
    }

    if (!target) {
        QMessageBox::information(this, tr("提示"),
            tr("当前没有已挂载的 Windows 分区。\n\n请先在上方分区管理中挂载 Windows 分区。"));
        return;
    }

    // 用文件管理器打开挂载点
    QProcess::startDetached("xdg-open", QStringList() << target->mountPoint);
}

void DualBootWidget::onRepairGrubClicked()
{
    onFixGrubClicked();
}

void DualBootWidget::onUpdateGrubClicked()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("更新 GRUB 配置"),
        tr("更新 GRUB 配置会重新扫描所有系统并生成新的启动菜单。\n\n确定继续吗？"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes
    );

    if (reply != QMessageBox::Yes) return;

    QMessageBox::information(this, tr("更新中"),
        tr("正在更新 GRUB 配置...\n\n请在弹出的授权窗口中输入密码。"));

    QApplication::setOverrideCursor(Qt::WaitCursor);

    QProcess proc;
    if (QFile::exists("/usr/sbin/update-grub")) {
        proc.start("pkexec", QStringList() << "update-grub");
    } else {
        proc.start("pkexec", QStringList() << "grub2-mkconfig" << "-o" << "/boot/grub2/grub.cfg");
    }
    proc.waitForFinished(60000);

    QApplication::restoreOverrideCursor();

    if (proc.exitCode() == 0) {
        QMessageBox::information(this, tr("更新成功"),
            tr("GRUB 配置已更新！\n\n启动菜单已刷新。"));
    } else {
        QString err = QString::fromUtf8(proc.readAllStandardError());
        QMessageBox::warning(this, tr("更新失败"),
            tr("GRUB 配置更新失败：\n%1").arg(err.left(400)));
    }
}

void DualBootWidget::onSetDefaultBootClicked()
{
    QString selected = m_defaultBootCombo ? m_defaultBootCombo->currentText() : "";
    if (selected.isEmpty()) return;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("设置默认启动项"),
        QString(tr("确定要将 \"%1\" 设置为默认启动项吗？\n\n"
                   "将修改 /etc/default/grub 并更新 GRUB 配置。\n"
                   "下次开机时将自动启动该系统。")).arg(selected),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes
    );

    if (reply != QMessageBox::Yes) return;

    // 读取 /etc/default/grub，修改 GRUB_DEFAULT
    QFile grubFile("/etc/default/grub");
    if (!grubFile.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("操作失败"),
            tr("无法读取 /etc/default/grub。\n请检查文件权限。"));
        return;
    }
    QString content = QString::fromUtf8(grubFile.readAll());
    grubFile.close();

    // 用 grub-set-default 更安全；但先尝试修改配置文件
    // 构造要写入的临时文件
    QString newContent = content;
    QRegularExpression defaultRe("^GRUB_DEFAULT=.*$", QRegularExpression::MultilineOption);
    if (defaultRe.match(newContent).hasMatch()) {
        newContent.replace(defaultRe, QString("GRUB_DEFAULT=\"%1\"").arg(selected));
    } else {
        newContent += QString("\nGRUB_DEFAULT=\"%1\"\n").arg(selected);
    }

    // 写入临时文件，再用 pkexec cp 覆盖
    QString tmpPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/grub_default_tmp";
    QFile tmpFile(tmpPath);
    if (!tmpFile.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("操作失败"), tr("无法写入临时文件。"));
        return;
    }
    tmpFile.write(newContent.toUtf8());
    tmpFile.close();

    QApplication::setOverrideCursor(Qt::WaitCursor);

    // 备份原文件并覆盖
    QProcess cpProc;
    cpProc.start("pkexec", QStringList() << "cp" << tmpPath << "/etc/default/grub");
    cpProc.waitForFinished(10000);

    if (cpProc.exitCode() != 0) {
        QApplication::restoreOverrideCursor();
        QFile::remove(tmpPath);
        QMessageBox::warning(this, tr("操作失败"),
            tr("无法写入 /etc/default/grub。\n请确认已授权。"));
        return;
    }

    QFile::remove(tmpPath);

    // 更新 GRUB 配置
    QProcess updateProc;
    if (QFile::exists("/usr/sbin/update-grub")) {
        updateProc.start("pkexec", QStringList() << "update-grub");
    } else {
        updateProc.start("pkexec", QStringList() << "grub2-mkconfig" << "-o" << "/boot/grub2/grub.cfg");
    }
    updateProc.waitForFinished(60000);

    QApplication::restoreOverrideCursor();

    for (auto& e : m_bootEntries) {
        e.isDefault = (e.name == selected);
    }

    if (updateProc.exitCode() == 0) {
        QMessageBox::information(this, tr("设置成功"),
            QString(tr("默认启动项已设置为：%1\n\n重启后生效。")).arg(selected));
    } else {
        QMessageBox::warning(this, tr("部分成功"),
            tr("已修改 /etc/default/grub，但更新 GRUB 配置失败。\n请手动运行 update-grub。"));
    }
}

void DualBootWidget::onDefaultBootChanged(const QString& entry)
{
    Q_UNUSED(entry);
    // 仅改变下拉框选择，实际应用由「应用」按钮触发
}

void DualBootWidget::onBootTimeoutChanged(int timeout)
{
    Q_UNUSED(timeout);
    // 仅改变数值，实际应用由「应用」按钮触发（此处保留钩子供未来实现）
}
