#include "ServiceManagerWidget.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollBar>
#include <QApplication>
#include <QRegularExpression>

ServiceManagerWidget::ServiceManagerWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    setupToolbar();
    setupTable();
    QTimer::singleShot(100, this, &ServiceManagerWidget::refreshServices);
}

void ServiceManagerWidget::setupUI()
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

    QLabel* titleLabel = new QLabel(tr("🔧 系统服务管理器"));
    titleLabel->setStyleSheet("font-size: 18px; font-weight: 600; color: #0f172a;");
    titleLayout->addWidget(titleLabel);

    QLabel* descLabel = new QLabel(tr("管理 systemd 服务，启动、停止、重启、启用或禁用系统服务"));
    descLabel->setStyleSheet("font-size: 13px; color: #64748b;");
    titleLayout->addWidget(descLabel);

    headerLayout->addLayout(titleLayout, 1);
    mainLayout->addWidget(headerCard);
}

void ServiceManagerWidget::setupToolbar()
{
    QFrame* toolbarCard = new QFrame();
    toolbarCard->setObjectName("card");
    QVBoxLayout* toolbarLayout = new QVBoxLayout(toolbarCard);
    toolbarLayout->setContentsMargins(20, 16, 20, 16);
    toolbarLayout->setSpacing(12);

    QHBoxLayout* topRow = new QHBoxLayout();
    topRow->setSpacing(12);

    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText(tr("🔍 搜索服务名称或描述..."));
    m_searchEdit->setMinimumHeight(36);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &ServiceManagerWidget::onSearchTextChanged);
    topRow->addWidget(m_searchEdit, 1);

    m_refreshBtn = new QPushButton(tr("🔄 刷新"));
    m_refreshBtn->setMinimumHeight(36);
    m_refreshBtn->setMinimumWidth(100);
    m_refreshBtn->setCursor(Qt::PointingHandCursor);
    connect(m_refreshBtn, &QPushButton::clicked, this, &ServiceManagerWidget::refreshServices);
    topRow->addWidget(m_refreshBtn);

    toolbarLayout->addLayout(topRow);

    QHBoxLayout* btnRow = new QHBoxLayout();
    btnRow->setSpacing(8);

    m_startBtn = new QPushButton(tr("▶️ 启动"));
    m_startBtn->setEnabled(false);
    m_startBtn->setMinimumHeight(32);
    m_startBtn->setCursor(Qt::PointingHandCursor);
    connect(m_startBtn, &QPushButton::clicked, this, &ServiceManagerWidget::startService);
    btnRow->addWidget(m_startBtn);

    m_stopBtn = new QPushButton(tr("⏹️ 停止"));
    m_stopBtn->setEnabled(false);
    m_stopBtn->setMinimumHeight(32);
    m_stopBtn->setCursor(Qt::PointingHandCursor);
    connect(m_stopBtn, &QPushButton::clicked, this, &ServiceManagerWidget::stopService);
    btnRow->addWidget(m_stopBtn);

    m_restartBtn = new QPushButton(tr("🔄 重启"));
    m_restartBtn->setEnabled(false);
    m_restartBtn->setMinimumHeight(32);
    m_restartBtn->setCursor(Qt::PointingHandCursor);
    connect(m_restartBtn, &QPushButton::clicked, this, &ServiceManagerWidget::restartService);
    btnRow->addWidget(m_restartBtn);

    QFrame* separator = new QFrame();
    separator->setFrameShape(QFrame::VLine);
    separator->setStyleSheet("color: #e2e8f0;");
    btnRow->addWidget(separator);

    m_enableBtn = new QPushButton(tr("✅ 启用"));
    m_enableBtn->setEnabled(false);
    m_enableBtn->setMinimumHeight(32);
    m_enableBtn->setCursor(Qt::PointingHandCursor);
    connect(m_enableBtn, &QPushButton::clicked, this, &ServiceManagerWidget::enableService);
    btnRow->addWidget(m_enableBtn);

    m_disableBtn = new QPushButton(tr("❌ 禁用"));
    m_disableBtn->setEnabled(false);
    m_disableBtn->setMinimumHeight(32);
    m_disableBtn->setCursor(Qt::PointingHandCursor);
    connect(m_disableBtn, &QPushButton::clicked, this, &ServiceManagerWidget::disableService);
    btnRow->addWidget(m_disableBtn);

    btnRow->addStretch(1);

    m_statusLabel = new QLabel(tr("就绪"));
    m_statusLabel->setStyleSheet("font-size: 12px; color: #64748b;");
    btnRow->addWidget(m_statusLabel);

    toolbarLayout->addLayout(btnRow);

    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(layout());
    mainLayout->addWidget(toolbarCard);
}

void ServiceManagerWidget::setupTable()
{
    QFrame* tableCard = new QFrame();
    tableCard->setObjectName("card");
    QVBoxLayout* tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(0, 0, 0, 0);

    m_table = new QTableWidget();
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({tr("服务名称"), tr("加载状态"), tr("活动状态"), tr("子状态"), tr("描述")});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setShowGrid(false);
    m_table->setStyleSheet(R"(
        QTableWidget {
            background-color: #ffffff;
            border: none;
            gridline-color: transparent;
            font-size: 13px;
        }
        QTableWidget::item {
            padding: 8px 12px;
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

    connect(m_table, &QTableWidget::itemSelectionChanged, this, &ServiceManagerWidget::onServiceSelectionChanged);

    tableLayout->addWidget(m_table);

    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(layout());
    mainLayout->addWidget(tableCard, 1);
}

void ServiceManagerWidget::refreshServices()
{
    m_statusLabel->setText(tr("正在加载服务列表..."));
    QApplication::setOverrideCursor(Qt::WaitCursor);
    loadServices();
    QApplication::restoreOverrideCursor();
}

void ServiceManagerWidget::loadServices()
{
    m_allServices.clear();

    QProcess process;
    process.start("systemctl", {"list-units", "--type=service", "--all", "--no-pager", "--no-legend"});
    process.waitForFinished(5000);

    QString output = QString::fromUtf8(process.readAllStandardOutput());
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (const QString& line : lines) {
        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() >= 4) {
            ServiceInfo info;
            info.name = parts[0];
            info.loadState = parts[1];
            info.activeState = parts[2];
            info.subState = parts[3];

            int descStart = line.indexOf(parts[3]) + parts[3].length();
            info.description = line.mid(descStart).trimmed();

            m_allServices.append(info);
        }
    }

    filterServices(m_searchEdit->text());
    m_statusLabel->setText(QString(tr("共 %1 个服务")).arg(m_allServices.size()));
}

void ServiceManagerWidget::filterServices(const QString& filter)
{
    m_table->setRowCount(0);

    QString lowerFilter = filter.toLower().trimmed();
    QList<ServiceInfo> filtered;

    if (lowerFilter.isEmpty()) {
        filtered = m_allServices;
    } else {
        for (const ServiceInfo& info : m_allServices) {
            if (info.name.toLower().contains(lowerFilter) ||
                info.description.toLower().contains(lowerFilter)) {
                filtered.append(info);
            }
        }
    }

    m_table->setRowCount(filtered.size());

    for (int i = 0; i < filtered.size(); ++i) {
        const ServiceInfo& info = filtered[i];

        QTableWidgetItem* nameItem = new QTableWidgetItem(info.name);
        nameItem->setData(Qt::UserRole, info.name);
        m_table->setItem(i, 0, nameItem);

        QTableWidgetItem* loadItem = new QTableWidgetItem(info.loadState);
        if (info.loadState == "loaded") {
            loadItem->setForeground(QColor("#059669"));
        } else if (info.loadState == "not-found") {
            loadItem->setForeground(QColor("#dc2626"));
        }
        m_table->setItem(i, 1, loadItem);

        QTableWidgetItem* activeItem = new QTableWidgetItem(info.activeState);
        if (info.activeState == "active") {
            activeItem->setForeground(QColor("#059669"));
            activeItem->setText("🟢 " + info.activeState);
        } else if (info.activeState == "inactive") {
            activeItem->setForeground(QColor("#6b7280"));
            activeItem->setText("⚪ " + info.activeState);
        } else if (info.activeState == "failed") {
            activeItem->setForeground(QColor("#dc2626"));
            activeItem->setText("🔴 " + info.activeState);
        }
        m_table->setItem(i, 2, activeItem);

        QTableWidgetItem* subItem = new QTableWidgetItem(info.subState);
        m_table->setItem(i, 3, subItem);

        QTableWidgetItem* descItem = new QTableWidgetItem(info.description);
        m_table->setItem(i, 4, descItem);
    }

    m_statusLabel->setText(QString(tr("显示 %1 / %2 个服务")).arg(filtered.size()).arg(m_allServices.size()));
}

void ServiceManagerWidget::onSearchTextChanged(const QString& text)
{
    filterServices(text);
}

void ServiceManagerWidget::onServiceSelectionChanged()
{
    updateButtonStates();
}

void ServiceManagerWidget::updateButtonStates()
{
    bool hasSelection = m_table->selectedItems().size() > 0;
    m_startBtn->setEnabled(hasSelection);
    m_stopBtn->setEnabled(hasSelection);
    m_restartBtn->setEnabled(hasSelection);
    m_enableBtn->setEnabled(hasSelection);
    m_disableBtn->setEnabled(hasSelection);
}

QString getSelectedServiceName(QTableWidget* table)
{
    auto items = table->selectedItems();
    if (items.isEmpty()) return QString();
    return items.first()->data(Qt::UserRole).toString();
}

void ServiceManagerWidget::startService()
{
    QString serviceName = getSelectedServiceName(m_table);
    if (serviceName.isEmpty()) return;

    auto ret = QMessageBox::question(this, tr("启动服务"),
        QString(tr("确定要启动服务「%1」吗？")).arg(serviceName),
        QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        executeServiceCommand(serviceName, "start");
    }
}

void ServiceManagerWidget::stopService()
{
    QString serviceName = getSelectedServiceName(m_table);
    if (serviceName.isEmpty()) return;

    auto ret = QMessageBox::question(this, tr("停止服务"),
        QString(tr("确定要停止服务「%1」吗？")).arg(serviceName),
        QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        executeServiceCommand(serviceName, "stop");
    }
}

void ServiceManagerWidget::restartService()
{
    QString serviceName = getSelectedServiceName(m_table);
    if (serviceName.isEmpty()) return;

    auto ret = QMessageBox::question(this, tr("重启服务"),
        QString(tr("确定要重启服务「%1」吗？")).arg(serviceName),
        QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        executeServiceCommand(serviceName, "restart");
    }
}

void ServiceManagerWidget::enableService()
{
    QString serviceName = getSelectedServiceName(m_table);
    if (serviceName.isEmpty()) return;

    auto ret = QMessageBox::question(this, tr("启用服务"),
        QString(tr("确定要开机启用服务「%1」吗？")).arg(serviceName),
        QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        executeServiceCommand(serviceName, "enable");
    }
}

void ServiceManagerWidget::disableService()
{
    QString serviceName = getSelectedServiceName(m_table);
    if (serviceName.isEmpty()) return;

    auto ret = QMessageBox::question(this, tr("禁用服务"),
        QString(tr("确定要开机禁用服务「%1」吗？")).arg(serviceName),
        QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        executeServiceCommand(serviceName, "disable");
    }
}

void ServiceManagerWidget::executeServiceCommand(const QString& serviceName, const QString& action)
{
    m_statusLabel->setText(QString(tr("正在%1服务...")).arg(action));
    QApplication::setOverrideCursor(Qt::WaitCursor);

    QProcess process;
    // 使用 pkexec 提权（与应用其他部分一致），不再硬编码或存储 sudo 密码
    process.start(QStringLiteral("pkexec"),
                  QStringList() << QStringLiteral("systemctl") << action << serviceName);

    if (!process.waitForStarted(3000)) {
        QApplication::restoreOverrideCursor();
        QMessageBox::critical(this, tr("执行失败"),
            tr("无法启动 pkexec，请确认已安装 polkit 权限框架。"));
        return;
    }
    process.waitForFinished(8000);

    QApplication::restoreOverrideCursor();

    QString error = QString::fromUtf8(process.readAllStandardError());
    bool success = (process.exitCode() == 0);

    if (success) {
        m_statusLabel->setText(QString(tr("%1 服务成功")).arg(action));
        QMessageBox::information(this, tr("操作成功"),
            QString(tr("服务「%1」%2 成功！")).arg(serviceName, action));
    } else {
        m_statusLabel->setText(tr("操作失败"));
        QMessageBox::warning(this, tr("操作失败"),
            QString(tr("服务「%1」%2 失败：\n%3")).arg(serviceName, action, error));
    }

    QTimer::singleShot(500, this, &ServiceManagerWidget::refreshServices);
}
