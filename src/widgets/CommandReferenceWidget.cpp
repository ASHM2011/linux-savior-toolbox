#include "CommandReferenceWidget.h"
#include "widgets/CommandDetailDialog.h"
#include "core/CommandMetadata.h"
#include "core/SystemDetector.h"
#include <QListWidgetItem>
#include <QSpacerItem>

CommandReferenceWidget::CommandReferenceWidget(QWidget* parent)
    : QWidget(parent)
    , m_searchEdit(nullptr)
    , m_categoryCombo(nullptr)
    , m_safetyCombo(nullptr)
    , m_categoryList(nullptr)
    , m_scrollArea(nullptr)
    , m_gridContainer(nullptr)
    , m_gridLayout(nullptr)
    , m_currentCategory(static_cast<CommandCategory>(-1))
    , m_currentSafetyFilter(static_cast<SafetyLevel>(-1))
    , m_searchKeyword("")
{
    setupUI();
    updateCommandDisplay();
}

void CommandReferenceWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(32, 24, 32, 24);
    mainLayout->setSpacing(16);

    QLabel* title = new QLabel(tr("📖 Linux命令速查表"));
    title->setStyleSheet("font-size: 22px; font-weight: 700; color: #0f172a;");
    mainLayout->addWidget(title);

    QLabel* subtitle = new QLabel(tr("300+常用命令，大白话解释，新手也能看懂。点击命令查看详细说明。"));
    subtitle->setStyleSheet("font-size: 13px; color: #64748b;");
    mainLayout->addWidget(subtitle);

    QFrame* filterFrame = setupFilters();
    mainLayout->addWidget(filterFrame);

    QHBoxLayout* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(16);

    setupCategoryList();
    contentLayout->addWidget(m_categoryList, 0);

    setupCommandGrid();
    contentLayout->addWidget(m_scrollArea, 1);

    mainLayout->addLayout(contentLayout, 1);
}

QFrame* CommandReferenceWidget::setupFilters()
{
    QFrame* filterFrame = new QFrame();
    filterFrame->setObjectName("card");
    QHBoxLayout* filterLayout = new QHBoxLayout(filterFrame);
    filterLayout->setContentsMargins(16, 12, 16, 12);
    filterLayout->setSpacing(12);

    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText(tr("🔍 搜索命令，支持拼音和模糊搜索..."));
    m_searchEdit->setFixedHeight(40);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &CommandReferenceWidget::onSearchChanged);
    filterLayout->addWidget(m_searchEdit, 2);

    m_categoryCombo = new QComboBox();
    m_categoryCombo->setFixedHeight(40);
    m_categoryCombo->addItem(tr("📋 全部分类"));
    m_categoryCombo->addItem(tr("🔄 系统更新"));
    m_categoryCombo->addItem(tr("🧹 系统清理"));
    m_categoryCombo->addItem(tr("🖥️ 桌面修复"));
    m_categoryCombo->addItem(tr("📦 包管理器"));
    m_categoryCombo->addItem(tr("📁 文件操作"));
    m_categoryCombo->addItem(tr("🌐 网络"));
    m_categoryCombo->addItem(tr("🔧 硬件驱动"));
    m_categoryCombo->addItem(tr("💾 备份恢复"));
    m_categoryCombo->addItem(tr("🪟 双系统"));
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CommandReferenceWidget::onCategoryChanged);
    filterLayout->addWidget(m_categoryCombo, 1);

    m_safetyCombo = new QComboBox();
    m_safetyCombo->setFixedHeight(40);
    m_safetyCombo->addItem(tr("🛡️ 全部安全等级"));
    m_safetyCombo->addItem(tr("✅ 安全"));
    m_safetyCombo->addItem(tr("⚠️ 谨慎"));
    m_safetyCombo->addItem(tr("❌ 危险"));
    connect(m_safetyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CommandReferenceWidget::onSafetyFilterChanged);
    filterLayout->addWidget(m_safetyCombo, 1);

    return filterFrame;
}

void CommandReferenceWidget::setupCategoryList()
{
    m_categoryList = new QListWidget();
    m_categoryList->setFixedWidth(200);

    QStringList categories = {
        tr("📋 全部命令"),
        tr("🔄 系统更新"),
        tr("🧹 系统清理"),
        tr("🖥️ 桌面修复"),
        tr("📦 包管理器"),
        tr("📁 文件操作"),
        tr("🌐 网络"),
        tr("🔧 硬件驱动"),
        tr("💾 备份恢复"),
        tr("🪟 双系统")
    };

    for (const QString& cat : categories) {
        QListWidgetItem* item = new QListWidgetItem(cat);
        m_categoryList->addItem(item);
    }

    m_categoryList->setCurrentRow(0);
    connect(m_categoryList, &QListWidget::itemClicked,
            this, &CommandReferenceWidget::onCategoryListClicked);
}

void CommandReferenceWidget::setupCommandGrid()
{
    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    m_gridContainer = new QWidget();
    m_gridLayout = new QGridLayout(m_gridContainer);
    m_gridLayout->setContentsMargins(4, 4, 4, 4);
    m_gridLayout->setSpacing(16);
    m_gridLayout->setAlignment(Qt::AlignTop);

    m_scrollArea->setWidget(m_gridContainer);
}

QFrame* CommandReferenceWidget::createCommandCard(const CommandMetadata& cmd)
{
    QFrame* card = new QFrame();
    card->setObjectName("card");
    card->setCursor(Qt::PointingHandCursor);

    QVBoxLayout* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(8);

    QLabel* nameLabel = new QLabel(cmd.friendlyName);
    nameLabel->setStyleSheet("font-size: 15px; font-weight: 600; color: #0f172a;");
    nameLabel->setWordWrap(true);
    headerLayout->addWidget(nameLabel, 1);

    headerLayout->addStretch(1);

    if (cmd.needsAdmin) {
        QLabel* adminLabel = new QLabel("🛡️");
        adminLabel->setToolTip(tr("需要管理员权限"));
        adminLabel->setStyleSheet("font-size: 14px;");
        headerLayout->addWidget(adminLabel);
    }

    layout->addLayout(headerLayout);

    QFrame* cmdFrame = new QFrame();
    cmdFrame->setStyleSheet(R"(
        QFrame {
            background-color: #0f172a;
            border-radius: 6px;
            padding: 8px 12px;
        }
    )");
    QHBoxLayout* cmdLayout = new QHBoxLayout(cmdFrame);
    cmdLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* cmdLabel = new QLabel(cmd.command);
    cmdLabel->setStyleSheet(R"(
        font-family: 'Fira Code', 'Source Code Pro', Consolas, monospace;
        font-size: 11px;
        color: #94a3b8;
    )");
    cmdLabel->setWordWrap(true);
    cmdLayout->addWidget(cmdLabel, 1);

    layout->addWidget(cmdFrame);

    QHBoxLayout* badgeLayout = new QHBoxLayout();
    badgeLayout->setSpacing(8);

    auto mgr = CommandMetadataManager::instance();
    QString safetyIcon = mgr->safetyLevelIcon(cmd.safetyLevel);
    QString safetyText = mgr->safetyLevelText(cmd.safetyLevel);

    QLabel* safetyBadge = new QLabel(safetyIcon + " " + safetyText);
    QString safetyStyle = "padding: 4px 10px; border-radius: 999px; font-size: 11px; font-weight: 500;";
    switch (cmd.safetyLevel) {
    case SafetyLevel::Safe:
        safetyStyle += "background-color: #d1fae5; color: #059669;";
        break;
    case SafetyLevel::Caution:
        safetyStyle += "background-color: #fef3c7; color: #d97706;";
        break;
    case SafetyLevel::Dangerous:
        safetyStyle += "background-color: #fee2e2; color: #dc2626;";
        break;
    }
    safetyBadge->setStyleSheet(safetyStyle);
    badgeLayout->addWidget(safetyBadge);

    badgeLayout->addStretch(1);

    layout->addLayout(badgeLayout);

    QLabel* descLabel = new QLabel(cmd.corePurpose);
    descLabel->setStyleSheet("font-size: 12px; color: #64748b; line-height: 1.5;");
    descLabel->setWordWrap(true);
    descLabel->setFixedHeight(36);
    layout->addWidget(descLabel);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);

    QPushButton* detailBtn = new QPushButton(tr("❓ 详情"));
    detailBtn->setObjectName("secondaryBtn");
    detailBtn->setFixedHeight(34);
    detailBtn->setCursor(Qt::PointingHandCursor);
    QString cmdId = cmd.id;
    connect(detailBtn, &QPushButton::clicked, [this, cmdId]() {
        onDetailClicked(cmdId);
    });
    btnLayout->addWidget(detailBtn);

    QPushButton* terminalBtn = new QPushButton(tr("💻 终端"));
    terminalBtn->setObjectName("secondaryBtn");
    terminalBtn->setFixedHeight(34);
    terminalBtn->setCursor(Qt::PointingHandCursor);
    connect(terminalBtn, &QPushButton::clicked, [this, cmdId]() {
        onTerminalClicked(cmdId);
    });
    btnLayout->addWidget(terminalBtn);

    QPushButton* execBtn = new QPushButton(tr("▶️ 执行"));
    execBtn->setFixedHeight(34);
    execBtn->setCursor(Qt::PointingHandCursor);
    if (cmd.safetyLevel == SafetyLevel::Dangerous) {
        execBtn->setObjectName("dangerBtn");
    } else if (cmd.safetyLevel == SafetyLevel::Caution) {
        execBtn->setObjectName("warningBtn");
    }
    connect(execBtn, &QPushButton::clicked, [this, cmdId]() {
        onExecuteClicked(cmdId);
    });
    btnLayout->addWidget(execBtn);

    layout->addLayout(btnLayout);

    return card;
}

void CommandReferenceWidget::updateCommandDisplay()
{
    QLayoutItem* child;
    while ((child = m_gridLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    QList<CommandMetadata> filtered = getFilteredCommands();

    if (filtered.isEmpty()) {
        QLabel* emptyLabel = new QLabel(tr("🔍 没有找到匹配的命令\n\n试试其他关键词或分类吧~"));
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("font-size: 14px; color: #94a3b8; padding: 60px;");
        m_gridLayout->addWidget(emptyLabel, 0, 0, 1, 2);
        return;
    }

    int col = 0;
    int row = 0;
    for (const auto& cmd : filtered) {
        QFrame* card = createCommandCard(cmd);
        m_gridLayout->addWidget(card, row, col);
        col++;
        if (col >= 2) {
            col = 0;
            row++;
        }
    }
}

QList<CommandMetadata> CommandReferenceWidget::getFilteredCommands() const
{
    auto mgr = CommandMetadataManager::instance();
    auto sysDetector = SystemDetector::instance();
    QList<CommandMetadata> result;

    QList<CommandMetadata> source;
    if (m_currentCategory >= CommandCategory::SystemUpdate && m_currentCategory <= CommandCategory::DualBoot) {
        source = mgr->getCommandsByCategory(m_currentCategory);
    } else {
        source = mgr->getAllCommands();
    }

    QString currentDistro = sysDetector->getSystemInfo().osReleaseId.toLower();

    for (const auto& cmd : source) {
        if (m_currentSafetyFilter >= SafetyLevel::Safe && m_currentSafetyFilter <= SafetyLevel::Dangerous) {
            if (cmd.safetyLevel != m_currentSafetyFilter) {
                continue;
            }
        }

        if (!cmd.supportedDistros.isEmpty()) {
            bool distroMatch = false;
            for (const QString& distro : cmd.supportedDistros) {
                if (distro.toLower() == currentDistro) {
                    distroMatch = true;
                    break;
                }
            }
            if (!distroMatch) {
                continue;
            }
        }

        if (!m_searchKeyword.isEmpty()) {
            QString kw = m_searchKeyword.toLower();
            if (!cmd.friendlyName.toLower().contains(kw) &&
                !cmd.command.toLower().contains(kw) &&
                !cmd.corePurpose.toLower().contains(kw)) {
                continue;
            }
        }

        result.append(cmd);
    }

    return result;
}

void CommandReferenceWidget::onSearchChanged(const QString& text)
{
    m_searchKeyword = text;
    updateCommandDisplay();
}

void CommandReferenceWidget::onCategoryChanged(int index)
{
    if (index == 0) {
        m_currentCategory = static_cast<CommandCategory>(-1);
    } else {
        m_currentCategory = static_cast<CommandCategory>(index - 1);
    }

    if (m_categoryList) {
        m_categoryList->blockSignals(true);
        m_categoryList->setCurrentRow(index);
        m_categoryList->blockSignals(false);
    }

    updateCommandDisplay();
}

void CommandReferenceWidget::onSafetyFilterChanged(int index)
{
    if (index == 0) {
        m_currentSafetyFilter = static_cast<SafetyLevel>(-1);
    } else {
        m_currentSafetyFilter = static_cast<SafetyLevel>(index - 1);
    }
    updateCommandDisplay();
}

void CommandReferenceWidget::onCategoryListClicked(QListWidgetItem* item)
{
    int row = m_categoryList->row(item);
    if (m_categoryCombo) {
        m_categoryCombo->blockSignals(true);
        m_categoryCombo->setCurrentIndex(row);
        m_categoryCombo->blockSignals(false);
    }

    if (row == 0) {
        m_currentCategory = static_cast<CommandCategory>(-1);
    } else {
        m_currentCategory = static_cast<CommandCategory>(row - 1);
    }

    updateCommandDisplay();
}

void CommandReferenceWidget::onDetailClicked(const QString& commandId)
{
    CommandDetailDialog dialog(commandId, this);
    connect(&dialog, &CommandDetailDialog::executeRequested, this, &CommandReferenceWidget::commandTriggered);
    connect(&dialog, &CommandDetailDialog::executeInTerminalRequested, this, &CommandReferenceWidget::executeInTerminal);
    dialog.exec();
}

void CommandReferenceWidget::onExecuteClicked(const QString& commandId)
{
    emit commandTriggered(commandId);
}

void CommandReferenceWidget::onTerminalClicked(const QString& commandId)
{
    emit executeInTerminal(commandId);
}
