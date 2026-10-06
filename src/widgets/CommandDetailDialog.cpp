#include "CommandDetailDialog.h"
#include "core/PermissionManager.h"
#include <QFont>
#include <QGuiApplication>
#include <QClipboard>
#include <QToolTip>
#include <QHeaderView>

CommandDetailDialog::CommandDetailDialog(const QString& commandId, QWidget* parent)
    : QDialog(parent)
    , m_commandId(commandId)
{
    m_cmd = CommandMetadataManager::instance()->getCommand(commandId);
    setWindowTitle(tr("命令详情"));
    setFixedWidth(600);
    setupUI();
    loadCommandData();

    setStyleSheet(R"(
        QDialog {
            background-color: #ffffff;
        }
        QLabel#dialogTitle {
            font-size: 20px;
            font-weight: 700;
            color: #0f172a;
        }
        QLabel#sectionTitle {
            font-size: 13px;
            font-weight: 600;
            color: #334155;
            margin-top: 4px;
        }
        QLabel#bodyText {
            font-size: 13px;
            color: #475569;
            line-height: 1.6;
        }
        QFrame#commandBox {
            background-color: #0f172a;
            border-radius: 8px;
            padding: 12px 16px;
        }
        QLabel#commandText {
            font-family: 'Fira Code', 'Source Code Pro', Consolas, monospace;
            font-size: 13px;
            color: #e2e8f0;
        }
        QTableWidget {
            background-color: #ffffff;
            border: 1px solid #e2e8f0;
            border-radius: 10px;
            gridline-color: #f1f5f9;
            outline: none;
        }
        QTableWidget::item {
            padding: 10px 12px;
            border-bottom: 1px solid #f1f5f9;
        }
        QHeaderView::section {
            background-color: #f8fafc;
            color: #475569;
            font-weight: 600;
            padding: 10px 12px;
            border: none;
            border-bottom: 1px solid #e2e8f0;
            text-align: left;
            font-size: 12px;
        }
    )");
}

void CommandDetailDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    QHBoxLayout* titleRow = new QHBoxLayout();
    titleRow->setSpacing(12);

    m_titleLabel = new QLabel();
    m_titleLabel->setObjectName("dialogTitle");
    titleRow->addWidget(m_titleLabel, 1);

    m_safetyBadge = new QLabel();
    m_safetyBadge->setStyleSheet(R"(
        padding: 6px 14px;
        border-radius: 999px;
        font-size: 12px;
        font-weight: 600;
    )");
    titleRow->addWidget(m_safetyBadge);

    mainLayout->addLayout(titleRow);

    QFrame* cmdBox = new QFrame();
    cmdBox->setObjectName("commandBox");
    QHBoxLayout* cmdLayout = new QHBoxLayout(cmdBox);
    cmdLayout->setContentsMargins(0, 0, 0, 0);

    m_commandLabel = new QLabel();
    m_commandLabel->setObjectName("commandText");
    m_commandLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    cmdLayout->addWidget(m_commandLabel, 1);

    QPushButton* copyBtn = new QPushButton("📋");
    copyBtn->setFixedSize(32, 32);
    copyBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #1e293b;
            color: #94a3b8;
            border: none;
            border-radius: 6px;
            font-size: 14px;
        }
        QPushButton:hover {
            background-color: #334155;
            color: #e2e8f0;
        }
    )");
    connect(copyBtn, &QPushButton::clicked, [this]() {
        QGuiApplication::clipboard()->setText(m_cmd.command);
        QToolTip::showText(QCursor::pos(), tr("已复制到剪贴板"), this);
    });
    cmdLayout->addWidget(copyBtn);

    mainLayout->addWidget(cmdBox);

    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget* scrollContent = new QWidget();
    QVBoxLayout* scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(0, 0, 0, 0);
    scrollLayout->setSpacing(16);

    QLabel* purposeTitle = new QLabel(tr("💡 核心作用"));
    purposeTitle->setObjectName("sectionTitle");
    scrollLayout->addWidget(purposeTitle);

    m_purposeLabel = new QLabel();
    m_purposeLabel->setObjectName("bodyText");
    m_purposeLabel->setWordWrap(true);
    scrollLayout->addWidget(m_purposeLabel);

    QLabel* paramsTitle = new QLabel(tr("🔍 逐参数拆解"));
    paramsTitle->setObjectName("sectionTitle");
    scrollLayout->addWidget(paramsTitle);

    m_paramTable = new QTableWidget();
    m_paramTable->setColumnCount(2);
    m_paramTable->setHorizontalHeaderLabels(QStringList() << tr("参数") << tr("说明"));
    m_paramTable->verticalHeader()->setVisible(false);
    m_paramTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_paramTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_paramTable->horizontalHeader()->setStretchLastSection(true);
    m_paramTable->setFixedHeight(160);
    scrollLayout->addWidget(m_paramTable);

    QLabel* effectTitle = new QLabel(tr("✨ 执行效果"));
    effectTitle->setObjectName("sectionTitle");
    scrollLayout->addWidget(effectTitle);

    m_effectLabel = new QLabel();
    m_effectLabel->setObjectName("bodyText");
    m_effectLabel->setWordWrap(true);
    scrollLayout->addWidget(m_effectLabel);

    QLabel* pitfallTitle = new QLabel(tr("⚠️ 萌新常见坑点"));
    pitfallTitle->setObjectName("sectionTitle");
    scrollLayout->addWidget(pitfallTitle);

    m_pitfallLabel = new QLabel();
    m_pitfallLabel->setWordWrap(true);
    m_pitfallLabel->setStyleSheet(R"(
        padding: 12px;
        background-color: #fef2f2;
        border-radius: 8px;
        border-left: 4px solid #ef4444;
        color: #b91c1c;
        font-size: 13px;
        line-height: 1.6;
        font-weight: 500;
    )");
    scrollLayout->addWidget(m_pitfallLabel);

    scrollLayout->addStretch(1);
    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);

    QFrame* btnFrame = new QFrame();
    QHBoxLayout* btnLayout = new QHBoxLayout(btnFrame);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(10);

    m_closeBtn = new QPushButton(tr("关闭"));
    m_closeBtn->setObjectName("secondaryBtn");
    m_closeBtn->setFixedHeight(40);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(m_closeBtn);

    m_backgroundBtn = new QPushButton(tr("⏱️ 后台执行"));
    m_backgroundBtn->setObjectName("secondaryBtn");
    m_backgroundBtn->setFixedHeight(40);
    connect(m_backgroundBtn, &QPushButton::clicked, this, &CommandDetailDialog::onExecuteInBackgroundClicked);
    btnLayout->addWidget(m_backgroundBtn);

    m_terminalBtn = new QPushButton(tr("📺 在终端中执行"));
    m_terminalBtn->setObjectName("secondaryBtn");
    m_terminalBtn->setFixedHeight(40);
    connect(m_terminalBtn, &QPushButton::clicked, this, &CommandDetailDialog::onExecuteInTerminalClicked);
    btnLayout->addWidget(m_terminalBtn);

    m_executeBtn = new QPushButton(tr("⚡ 一键执行"));
    m_executeBtn->setFixedHeight(40);
    connect(m_executeBtn, &QPushButton::clicked, this, &CommandDetailDialog::onExecuteClicked);
    btnLayout->addWidget(m_executeBtn);

    mainLayout->addWidget(btnFrame);
}

void CommandDetailDialog::loadCommandData()
{
    auto mgr = CommandMetadataManager::instance();

    m_titleLabel->setText(m_cmd.friendlyName);

    QString safetyText = mgr->safetyLevelIcon(m_cmd.safetyLevel) + " " + mgr->safetyLevelText(m_cmd.safetyLevel);
    m_safetyBadge->setText(safetyText);

    QString safetyStyle;
    switch (m_cmd.safetyLevel) {
    case SafetyLevel::Safe:
        safetyStyle = "background-color: #d1fae5; color: #059669;";
        break;
    case SafetyLevel::Caution:
        safetyStyle = "background-color: #fef3c7; color: #d97706;";
        break;
    case SafetyLevel::Dangerous:
        safetyStyle = "background-color: #fee2e2; color: #dc2626;";
        break;
    }
    m_safetyBadge->setStyleSheet(m_safetyBadge->styleSheet() + safetyStyle);

    m_commandLabel->setText(m_cmd.command);
    m_purposeLabel->setText(m_cmd.corePurpose);
    m_effectLabel->setText(m_cmd.executionEffect);
    m_pitfallLabel->setText(m_cmd.commonPitfall);

    m_paramTable->setRowCount(m_cmd.paramBreakdown.size());
    for (int i = 0; i < m_cmd.paramBreakdown.size(); i++) {
        const auto& param = m_cmd.paramBreakdown[i];
        QTableWidgetItem* paramItem = new QTableWidgetItem(param.param);
        paramItem->setFont(QFont("'Fira Code', monospace"));
        paramItem->setForeground(QBrush(QColor("#2563eb")));
        paramItem->setFont(QFont(paramItem->font().family(), -1, QFont::DemiBold));
        m_paramTable->setItem(i, 0, paramItem);

        QTableWidgetItem* descItem = new QTableWidgetItem(param.description);
        descItem->setForeground(QBrush(QColor("#64748b")));
        m_paramTable->setItem(i, 1, descItem);
    }
    m_paramTable->resizeRowsToContents();

    int tableHeight = 0;
    for (int i = 0; i < m_paramTable->rowCount(); i++) {
        tableHeight += m_paramTable->rowHeight(i);
    }
    tableHeight += m_paramTable->horizontalHeader()->height();
    tableHeight += 4;
    m_paramTable->setFixedHeight(qMin(tableHeight, 200));

    auto perm = PermissionManager::instance()->checkCommandPermission(m_commandId);
    if (perm == PermissionCheckResult::ForbiddenAdmin) {
        m_executeBtn->setToolTip(tr("此操作不需要管理员权限"));
    } else if (perm == PermissionCheckResult::NeedsAdmin) {
        m_executeBtn->setToolTip(tr("🛡️ 此操作需要管理员权限"));
    }
}

void CommandDetailDialog::onExecuteClicked()
{
    emit executeRequested(m_commandId);
    accept();
}

void CommandDetailDialog::onExecuteInTerminalClicked()
{
    emit executeInTerminalRequested(m_commandId);
    accept();
}

void CommandDetailDialog::onExecuteInBackgroundClicked()
{
    emit executeInBackgroundRequested(m_commandId);
    accept();
}
