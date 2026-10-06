#include "ProcessManagerWidget.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QApplication>
#include <algorithm>

ProcessManagerWidget::ProcessManagerWidget(QWidget* parent)
    : QWidget(parent)
    , m_sortColumn(2)
    , m_sortOrder(Qt::DescendingOrder)
{
    setupUI();
    setupToolbar();
    setupTable();
    QTimer::singleShot(100, this, &ProcessManagerWidget::refreshProcesses);

    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(3000);
    connect(m_refreshTimer, &QTimer::timeout, this, &ProcessManagerWidget::refreshProcesses);
    m_refreshTimer->start();
}

void ProcessManagerWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(32, 24, 32, 24);
    mainLayout->setSpacing(16);

    QFrame* headerCard = new QFrame();
    headerCard->setObjectName("card");
    QHBoxLayout* headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(24, 20, 24, 20);
    headerLayout->setSpacing(16);

    QVBoxLayout* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(4);

    QLabel* titleLabel = new QLabel(tr("📊 进程管理器"));
    titleLabel->setStyleSheet("font-size: 18px; font-weight: 600; color: #0f172a;");
    titleLayout->addWidget(titleLabel);

    QLabel* descLabel = new QLabel(tr("查看和管理系统进程，终止或强制结束无响应的程序"));
    descLabel->setStyleSheet("font-size: 13px; color: #64748b;");
    titleLayout->addWidget(descLabel);

    headerLayout->addLayout(titleLayout, 1);
    mainLayout->addWidget(headerCard);
}

void ProcessManagerWidget::setupToolbar()
{
    QFrame* toolbarCard = new QFrame();
    toolbarCard->setObjectName("card");
    QVBoxLayout* toolbarLayout = new QVBoxLayout(toolbarCard);
    toolbarLayout->setContentsMargins(20, 16, 20, 16);
    toolbarLayout->setSpacing(12);

    QHBoxLayout* topRow = new QHBoxLayout();
    topRow->setSpacing(12);

    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText(tr("🔍 搜索进程名、PID或用户..."));
    m_searchEdit->setMinimumHeight(36);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &ProcessManagerWidget::onSearchTextChanged);
    topRow->addWidget(m_searchEdit, 1);

    m_refreshBtn = new QPushButton(tr("🔄 刷新"));
    m_refreshBtn->setMinimumHeight(36);
    m_refreshBtn->setMinimumWidth(100);
    m_refreshBtn->setCursor(Qt::PointingHandCursor);
    connect(m_refreshBtn, &QPushButton::clicked, this, &ProcessManagerWidget::refreshProcesses);
    topRow->addWidget(m_refreshBtn);

    toolbarLayout->addLayout(topRow);

    QHBoxLayout* btnRow = new QHBoxLayout();
    btnRow->setSpacing(8);

    m_terminateBtn = new QPushButton(tr("⏹️ 终止进程 (SIGTERM)"));
    m_terminateBtn->setEnabled(false);
    m_terminateBtn->setMinimumHeight(32);
    m_terminateBtn->setCursor(Qt::PointingHandCursor);
    connect(m_terminateBtn, &QPushButton::clicked, this, &ProcessManagerWidget::terminateProcess);
    btnRow->addWidget(m_terminateBtn);

    m_killBtn = new QPushButton(tr("💀 强制结束 (SIGKILL)"));
    m_killBtn->setEnabled(false);
    m_killBtn->setMinimumHeight(32);
    m_killBtn->setObjectName("dangerBtn");
    m_killBtn->setCursor(Qt::PointingHandCursor);
    connect(m_killBtn, &QPushButton::clicked, this, &ProcessManagerWidget::killProcess);
    btnRow->addWidget(m_killBtn);

    btnRow->addStretch(1);

    m_statusLabel = new QLabel(tr("就绪"));
    m_statusLabel->setStyleSheet("font-size: 12px; color: #64748b;");
    btnRow->addWidget(m_statusLabel);

    toolbarLayout->addLayout(btnRow);

    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(layout());
    mainLayout->addWidget(toolbarCard);
}

void ProcessManagerWidget::setupTable()
{
    QFrame* tableCard = new QFrame();
    tableCard->setObjectName("card");
    QVBoxLayout* tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(0, 0, 0, 0);

    m_table = new QTableWidget();
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({"PID", tr("用户"), "CPU %", tr("内存 %"), tr("命令")});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(false);
    m_table->setSortingEnabled(true);
    m_table->setStyleSheet(R"(
        QTableWidget {
            background-color: #ffffff;
            border: none;
            gridline-color: transparent;
            font-size: 13px;
        }
        QTableWidget::item {
            padding: 6px 12px;
            border-bottom: 1px solid #f1f5f9;
        }
        QTableWidget::item:selected {
            background-color: #eff6ff;
            color: #1e40af;
        }
        QHeaderView::section {
            background-color: #f8fafc;
            padding: 10px 12px;
            border: none;
            border-bottom: 2px solid #e2e8f0;
            font-weight: 600;
            color: #475569;
            font-size: 12px;
        }
        QHeaderView::section:hover {
            background-color: #f1f5f9;
        }
        QTableWidget::item:alternate {
            background-color: #fafbfc;
        }
    )");

    QHeaderView* horizontalHeader = m_table->horizontalHeader();
    horizontalHeader->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    horizontalHeader->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    horizontalHeader->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    horizontalHeader->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    horizontalHeader->setSectionResizeMode(4, QHeaderView::Stretch);

    connect(m_table, &QTableWidget::itemSelectionChanged, this, &ProcessManagerWidget::onProcessSelectionChanged);
    connect(horizontalHeader, &QHeaderView::sectionClicked, this, &ProcessManagerWidget::sortByColumn);

    tableLayout->addWidget(m_table);

    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(layout());
    mainLayout->addWidget(tableCard, 1);
}

void ProcessManagerWidget::refreshProcesses()
{
    loadProcesses();
}

void ProcessManagerWidget::loadProcesses()
{
    m_allProcesses.clear();

    QProcess process;
    process.start("ps", {"-eo", "pid,user,pcpu,pmem,comm", "--no-headers"});
    process.waitForFinished(3000);

    QString output = QString::fromUtf8(process.readAllStandardOutput());
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;

        QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() >= 5) {
            ProcessInfo info;
            info.pid = parts[0].toInt();
            info.user = parts[1];
            info.cpuPercent = parts[2].toDouble();
            info.memPercent = parts[3].toDouble();

            int cmdStart = trimmed.indexOf(parts[3]) + parts[3].length();
            info.command = trimmed.mid(cmdStart).trimmed();

            m_allProcesses.append(info);
        }
    }

    QProcess cmdlineProcess;
    cmdlineProcess.start("ps", {"-eo", "pid,args", "--no-headers"});
    cmdlineProcess.waitForFinished(3000);
    QString cmdlineOutput = QString::fromUtf8(cmdlineProcess.readAllStandardOutput());
    QStringList cmdlineLines = cmdlineOutput.split('\n', Qt::SkipEmptyParts);

    QMap<int, QString> fullCommands;
    for (const QString& line : cmdlineLines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;
        int spacePos = trimmed.indexOf(' ');
        if (spacePos > 0) {
            bool ok;
            int pid = trimmed.left(spacePos).toInt(&ok);
            if (ok) {
                fullCommands[pid] = trimmed.mid(spacePos + 1).trimmed();
            }
        }
    }

    for (ProcessInfo& info : m_allProcesses) {
        if (fullCommands.contains(info.pid)) {
            info.command = fullCommands[info.pid];
        }
    }

    filterProcesses(m_searchEdit->text());
}

bool compareProcesses(const ProcessInfo& a, const ProcessInfo& b, int column, Qt::SortOrder order)
{
    bool ascending = (order == Qt::AscendingOrder);
    switch (column) {
        case 0: return ascending ? (a.pid < b.pid) : (a.pid > b.pid);
        case 1: return ascending ? (a.user < b.user) : (a.user > b.user);
        case 2: return ascending ? (a.cpuPercent < b.cpuPercent) : (a.cpuPercent > b.cpuPercent);
        case 3: return ascending ? (a.memPercent < b.memPercent) : (a.memPercent > b.memPercent);
        case 4: return ascending ? (a.command < b.command) : (a.command > b.command);
        default: return ascending ? (a.pid < b.pid) : (a.pid > b.pid);
    }
}

void ProcessManagerWidget::filterProcesses(const QString& filter)
{
    QString lowerFilter = filter.toLower().trimmed();
    QList<ProcessInfo> filtered;

    if (lowerFilter.isEmpty()) {
        filtered = m_allProcesses;
    } else {
        for (const ProcessInfo& info : m_allProcesses) {
            if (QString::number(info.pid).contains(lowerFilter) ||
                info.user.toLower().contains(lowerFilter) ||
                info.command.toLower().contains(lowerFilter)) {
                filtered.append(info);
            }
        }
    }

    std::sort(filtered.begin(), filtered.end(), [this](const ProcessInfo& a, const ProcessInfo& b) {
        return compareProcesses(a, b, m_sortColumn, m_sortOrder);
    });

    m_table->setSortingEnabled(false);
    m_table->setRowCount(filtered.size());

    for (int i = 0; i < filtered.size(); ++i) {
        const ProcessInfo& info = filtered[i];

        QTableWidgetItem* pidItem = new QTableWidgetItem(QString::number(info.pid));
        pidItem->setData(Qt::UserRole, info.pid);
        pidItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(i, 0, pidItem);

        QTableWidgetItem* userItem = new QTableWidgetItem(info.user);
        m_table->setItem(i, 1, userItem);

        QTableWidgetItem* cpuItem = new QTableWidgetItem(QString::number(info.cpuPercent, 'f', 1));
        cpuItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (info.cpuPercent > 50) {
            cpuItem->setForeground(QColor("#dc2626"));
        } else if (info.cpuPercent > 20) {
            cpuItem->setForeground(QColor("#d97706"));
        }
        m_table->setItem(i, 2, cpuItem);

        QTableWidgetItem* memItem = new QTableWidgetItem(QString::number(info.memPercent, 'f', 1));
        memItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (info.memPercent > 50) {
            memItem->setForeground(QColor("#dc2626"));
        } else if (info.memPercent > 20) {
            memItem->setForeground(QColor("#d97706"));
        }
        m_table->setItem(i, 3, memItem);

        QTableWidgetItem* cmdItem = new QTableWidgetItem(info.command);
        m_table->setItem(i, 4, cmdItem);
    }

    m_table->setSortingEnabled(true);
    m_statusLabel->setText(QString(tr("显示 %1 / %2 个进程")).arg(filtered.size()).arg(m_allProcesses.size()));
}

void ProcessManagerWidget::onSearchTextChanged(const QString& text)
{
    filterProcesses(text);
}

void ProcessManagerWidget::onProcessSelectionChanged()
{
    updateButtonStates();
}

void ProcessManagerWidget::updateButtonStates()
{
    bool hasSelection = m_table->selectedItems().size() > 0;
    m_terminateBtn->setEnabled(hasSelection);
    m_killBtn->setEnabled(hasSelection);
}

void ProcessManagerWidget::sortByColumn(int column)
{
    if (m_sortColumn == column) {
        m_sortOrder = (m_sortOrder == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
    } else {
        m_sortColumn = column;
        m_sortOrder = Qt::DescendingOrder;
    }
    filterProcesses(m_searchEdit->text());
}

int getSelectedPid(QTableWidget* table)
{
    auto items = table->selectedItems();
    if (items.isEmpty()) return -1;
    return items.first()->data(Qt::UserRole).toInt();
}

QString getSelectedCommand(QTableWidget* table)
{
    auto items = table->selectedItems();
    if (items.size() < 5) return QString();
    return table->item(table->currentRow(), 4)->text();
}

void ProcessManagerWidget::terminateProcess()
{
    int pid = getSelectedPid(m_table);
    if (pid <= 0) return;

    QString cmd = getSelectedCommand(m_table);

    auto ret = QMessageBox::question(this, tr("终止进程"),
        QString(tr("确定要发送 SIGTERM 信号终止进程 PID %1 (%2) 吗？\n\n进程将有机会正常退出。"))
            .arg(pid).arg(cmd.left(50)),
        QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        executeKillCommand(pid, 15);
    }
}

void ProcessManagerWidget::killProcess()
{
    int pid = getSelectedPid(m_table);
    if (pid <= 0) return;

    QString cmd = getSelectedCommand(m_table);

    auto ret = QMessageBox::warning(this, tr("强制结束进程"),
        QString(tr("⚠️ 确定要发送 SIGKILL 信号强制结束 PID %1 (%2) 吗？\n\n" "这将立即终止进程，可能导致数据丢失！"))
            .arg(pid).arg(cmd.left(50)),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        executeKillCommand(pid, 9);
    }
}

void ProcessManagerWidget::executeKillCommand(int pid, int signal)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);

    QProcess process;
    process.start(QStringLiteral("pkexec"),
                  QStringList() << QStringLiteral("kill")
                                << ("-" + QString::number(signal))
                                << QString::number(pid));
    process.waitForFinished(8000);

    QApplication::restoreOverrideCursor();

    bool success = (process.exitCode() == 0);
    QString error = QString::fromUtf8(process.readAllStandardError());

    if (success) {
        QString action = (signal == 9) ? tr("强制结束") : tr("终止");
        m_statusLabel->setText(QString(tr("进程 %1 已%2")).arg(pid).arg(action));
        QMessageBox::information(this, tr("操作成功"),
            QString(tr("进程 PID %1 已成功%2！")).arg(pid).arg(action));
    } else {
        m_statusLabel->setText(tr("操作失败"));
        QMessageBox::warning(this, tr("操作失败"),
            QString(tr("操作进程 PID %1 失败：\n%2")).arg(pid).arg(error));
    }

    QTimer::singleShot(300, this, &ProcessManagerWidget::refreshProcesses);
}
