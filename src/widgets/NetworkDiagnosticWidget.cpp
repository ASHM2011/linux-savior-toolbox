#include "NetworkDiagnosticWidget.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QApplication>
#include <QScrollBar>
#include <QRegularExpression>
#include <QTextCursor>
#include <algorithm>

NetworkDiagnosticWidget::NetworkDiagnosticWidget(QWidget* parent)
    : QWidget(parent)
    , m_pingProcess(nullptr)
{
    setupUI();
    QTimer::singleShot(100, this, &NetworkDiagnosticWidget::refreshNetworkInfo);
}

void NetworkDiagnosticWidget::setupUI()
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

    QLabel* titleLabel = new QLabel(tr("🌐 网络诊断工具"));
    titleLabel->setStyleSheet("font-size: 18px; font-weight: 600; color: #0f172a;");
    titleLayout->addWidget(titleLabel);

    QLabel* descLabel = new QLabel(tr("查看网络接口、Ping测试、检查监听端口"));
    descLabel->setStyleSheet("font-size: 13px; color: #64748b;");
    titleLayout->addWidget(descLabel);

    headerLayout->addLayout(titleLayout, 1);
    mainLayout->addWidget(headerCard);

    m_tabWidget = new QTabWidget();
    m_tabWidget->setStyleSheet(R"(
        QTabWidget::pane {
            border: none;
            background-color: transparent;
        }
        QTabBar::tab {
            background-color: #f1f5f9;
            padding: 10px 20px;
            margin-right: 4px;
            border-top-left-radius: 8px;
            border-top-right-radius: 8px;
            font-size: 13px;
            font-weight: 500;
            color: #64748b;
        }
        QTabBar::tab:selected {
            background-color: #ffffff;
            color: #2563eb;
        }
        QTabBar::tab:hover:!selected {
            background-color: #e2e8f0;
        }
    )");

    setupInterfaceTab();
    setupPingTab();
    setupPortsTab();

    mainLayout->addWidget(m_tabWidget, 1);
}

void NetworkDiagnosticWidget::setupInterfaceTab()
{
    m_interfaceTab = new QWidget();
    QVBoxLayout* tabLayout = new QVBoxLayout(m_interfaceTab);
    tabLayout->setContentsMargins(0, 16, 0, 0);
    tabLayout->setSpacing(12);

    QFrame* toolbarCard = new QFrame();
    toolbarCard->setObjectName("card");
    QHBoxLayout* toolbarLayout = new QHBoxLayout(toolbarCard);
    toolbarLayout->setContentsMargins(20, 12, 20, 12);

    QLabel* tabTitle = new QLabel(tr("📡 网络接口"));
    tabTitle->setStyleSheet("font-size: 14px; font-weight: 600; color: #0f172a;");
    toolbarLayout->addWidget(tabTitle);

    toolbarLayout->addStretch(1);

    m_interfaceStatusLabel = new QLabel(tr("就绪"));
    m_interfaceStatusLabel->setStyleSheet("font-size: 12px; color: #64748b;");
    toolbarLayout->addWidget(m_interfaceStatusLabel);

    m_refreshInterfacesBtn = new QPushButton(tr("🔄 刷新"));
    m_refreshInterfacesBtn->setMinimumHeight(32);
    m_refreshInterfacesBtn->setCursor(Qt::PointingHandCursor);
    connect(m_refreshInterfacesBtn, &QPushButton::clicked, this, &NetworkDiagnosticWidget::refreshNetworkInfo);
    toolbarLayout->addWidget(m_refreshInterfacesBtn);

    tabLayout->addWidget(toolbarCard);

    QFrame* tableCard = new QFrame();
    tableCard->setObjectName("card");
    QVBoxLayout* tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(0, 0, 0, 0);

    m_interfaceTable = new QTableWidget();
    m_interfaceTable->setColumnCount(5);
    m_interfaceTable->setHorizontalHeaderLabels({tr("接口名称"), tr("IPv4地址"), tr("IPv6地址"), tr("MAC地址"), tr("状态")});
    m_interfaceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_interfaceTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_interfaceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_interfaceTable->setAlternatingRowColors(true);
    m_interfaceTable->verticalHeader()->setVisible(false);
    m_interfaceTable->setShowGrid(false);
    m_interfaceTable->setStyleSheet(R"(
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

    QHeaderView* hHeader = m_interfaceTable->horizontalHeader();
    hHeader->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    hHeader->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    hHeader->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    hHeader->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    hHeader->setSectionResizeMode(4, QHeaderView::Stretch);

    tableLayout->addWidget(m_interfaceTable);
    tabLayout->addWidget(tableCard, 1);

    m_tabWidget->addTab(m_interfaceTab, tr("🔌 网络接口"));
}

void NetworkDiagnosticWidget::setupPingTab()
{
    m_pingTab = new QWidget();
    QVBoxLayout* tabLayout = new QVBoxLayout(m_pingTab);
    tabLayout->setContentsMargins(0, 16, 0, 0);
    tabLayout->setSpacing(12);

    QFrame* toolbarCard = new QFrame();
    toolbarCard->setObjectName("card");
    QVBoxLayout* toolbarLayout = new QVBoxLayout(toolbarCard);
    toolbarLayout->setContentsMargins(20, 16, 20, 16);
    toolbarLayout->setSpacing(12);

    QHBoxLayout* inputRow = new QHBoxLayout();
    inputRow->setSpacing(12);

    QLabel* pingLabel = new QLabel(tr("📍 目标地址:"));
    pingLabel->setStyleSheet("font-size: 13px; font-weight: 500; color: #334155;");
    inputRow->addWidget(pingLabel);

    m_pingAddressEdit = new QLineEdit();
    m_pingAddressEdit->setPlaceholderText(tr("输入域名或IP地址，例如: baidu.com 或 8.8.8.8"));
    m_pingAddressEdit->setMinimumHeight(36);
    m_pingAddressEdit->setText("baidu.com");
    inputRow->addWidget(m_pingAddressEdit, 1);

    m_startPingBtn = new QPushButton(tr("▶️ 开始Ping"));
    m_startPingBtn->setMinimumHeight(36);
    m_startPingBtn->setMinimumWidth(120);
    m_startPingBtn->setCursor(Qt::PointingHandCursor);
    connect(m_startPingBtn, &QPushButton::clicked, this, &NetworkDiagnosticWidget::startPing);
    inputRow->addWidget(m_startPingBtn);

    m_stopPingBtn = new QPushButton(tr("⏹️ 停止"));
    m_stopPingBtn->setMinimumHeight(36);
    m_stopPingBtn->setMinimumWidth(100);
    m_stopPingBtn->setEnabled(false);
    m_stopPingBtn->setCursor(Qt::PointingHandCursor);
    connect(m_stopPingBtn, &QPushButton::clicked, this, &NetworkDiagnosticWidget::stopPing);
    inputRow->addWidget(m_stopPingBtn);

    toolbarLayout->addLayout(inputRow);

    QHBoxLayout* statusRow = new QHBoxLayout();
    m_pingStatusLabel = new QLabel(tr("就绪，输入地址后点击开始"));
    m_pingStatusLabel->setStyleSheet("font-size: 12px; color: #64748b;");
    statusRow->addWidget(m_pingStatusLabel);
    statusRow->addStretch(1);
    toolbarLayout->addLayout(statusRow);

    tabLayout->addWidget(toolbarCard);

    QFrame* outputCard = new QFrame();
    outputCard->setObjectName("card");
    QVBoxLayout* outputLayout = new QVBoxLayout(outputCard);
    outputLayout->setContentsMargins(0, 0, 0, 0);

    m_pingOutput = new QTextEdit();
    m_pingOutput->setReadOnly(true);
    m_pingOutput->setFont(QFont("monospace", 10));
    m_pingOutput->setStyleSheet(R"(
        QTextEdit {
            background-color: #1e293b;
            color: #e2e8f0;
            border: none;
            padding: 16px;
            font-family: 'Consolas', 'Monaco', monospace;
        }
    )");
    m_pingOutput->setPlaceholderText(tr("Ping 输出将显示在这里..."));

    outputLayout->addWidget(m_pingOutput);
    tabLayout->addWidget(outputCard, 1);

    m_tabWidget->addTab(m_pingTab, tr("📶 Ping测试"));
}

void NetworkDiagnosticWidget::setupPortsTab()
{
    m_portsTab = new QWidget();
    QVBoxLayout* tabLayout = new QVBoxLayout(m_portsTab);
    tabLayout->setContentsMargins(0, 16, 0, 0);
    tabLayout->setSpacing(12);

    QFrame* toolbarCard = new QFrame();
    toolbarCard->setObjectName("card");
    QHBoxLayout* toolbarLayout = new QHBoxLayout(toolbarCard);
    toolbarLayout->setContentsMargins(20, 12, 20, 12);

    QLabel* tabTitle = new QLabel(tr("🔌 监听端口"));
    tabTitle->setStyleSheet("font-size: 14px; font-weight: 600; color: #0f172a;");
    toolbarLayout->addWidget(tabTitle);

    toolbarLayout->addStretch(1);

    m_portsStatusLabel = new QLabel(tr("就绪"));
    m_portsStatusLabel->setStyleSheet("font-size: 12px; color: #64748b;");
    toolbarLayout->addWidget(m_portsStatusLabel);

    m_refreshPortsBtn = new QPushButton(tr("🔄 刷新"));
    m_refreshPortsBtn->setMinimumHeight(32);
    m_refreshPortsBtn->setCursor(Qt::PointingHandCursor);
    connect(m_refreshPortsBtn, &QPushButton::clicked, this, &NetworkDiagnosticWidget::refreshListeningPorts);
    toolbarLayout->addWidget(m_refreshPortsBtn);

    tabLayout->addWidget(toolbarCard);

    QFrame* tableCard = new QFrame();
    tableCard->setObjectName("card");
    QVBoxLayout* tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(0, 0, 0, 0);

    m_portsTable = new QTableWidget();
    m_portsTable->setColumnCount(4);
    m_portsTable->setHorizontalHeaderLabels({tr("协议"), tr("端口"), tr("本地地址"), tr("进程/程序")});
    m_portsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_portsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_portsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_portsTable->setAlternatingRowColors(true);
    m_portsTable->verticalHeader()->setVisible(false);
    m_portsTable->setShowGrid(false);
    m_portsTable->setStyleSheet(R"(
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

    QHeaderView* hHeader = m_portsTable->horizontalHeader();
    hHeader->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    hHeader->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    hHeader->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    hHeader->setSectionResizeMode(3, QHeaderView::Stretch);

    tableLayout->addWidget(m_portsTable);
    tabLayout->addWidget(tableCard, 1);

    m_tabWidget->addTab(m_portsTab, tr("🚪 监听端口"));
}

void NetworkDiagnosticWidget::refreshNetworkInfo()
{
    loadInterfaces();
    refreshListeningPorts();
}

void NetworkDiagnosticWidget::loadInterfaces()
{
    m_interfaceStatusLabel->setText(tr("正在加载接口信息..."));
    QApplication::setOverrideCursor(Qt::WaitCursor);

    QList<NetworkInterface> interfaces;

    QProcess process;
    process.start("ip", {"-o", "-4", "addr", "show"});
    process.waitForFinished(3000);
    QString ipv4Output = QString::fromUtf8(process.readAllStandardOutput());

    QProcess ipv6Process;
    ipv6Process.start("ip", {"-o", "-6", "addr", "show"});
    ipv6Process.waitForFinished(3000);
    QString ipv6Output = QString::fromUtf8(ipv6Process.readAllStandardOutput());

    QProcess linkProcess;
    linkProcess.start("ip", {"-o", "link", "show"});
    linkProcess.waitForFinished(3000);
    QString linkOutput = QString::fromUtf8(linkProcess.readAllStandardOutput());

    QMap<QString, NetworkInterface> ifaceMap;

    QStringList linkLines = linkOutput.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : linkLines) {
        QRegularExpression re("^\\d+: ([^:]+):.*state ([^ ]+)");
        QRegularExpressionMatch match = re.match(line.trimmed());
        if (match.hasMatch()) {
            NetworkInterface iface;
            iface.name = match.captured(1);
            iface.state = match.captured(2);
            iface.ipv4 = "-";
            iface.ipv6 = "-";
            iface.mac = "-";
            ifaceMap[iface.name] = iface;
        }
    }

    QStringList ipv4Lines = ipv4Output.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : ipv4Lines) {
        QStringList parts = line.trimmed().split(QRegularExpression("\\s+"));
        if (parts.size() >= 4) {
            QString name = parts[1];
            if (ifaceMap.contains(name)) {
                ifaceMap[name].ipv4 = parts[3];
            }
        }
    }

    QStringList ipv6Lines = ipv6Output.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : ipv6Lines) {
        QStringList parts = line.trimmed().split(QRegularExpression("\\s+"));
        if (parts.size() >= 4 && parts[2] == "inet6") {
            QString name = parts[1];
            if (ifaceMap.contains(name)) {
                ifaceMap[name].ipv6 = parts[3];
            }
        }
    }

    for (auto it = ifaceMap.begin(); it != ifaceMap.end(); ++it) {
        interfaces.append(it.value());
    }

    m_interfaceTable->setRowCount(interfaces.size());
    for (int i = 0; i < interfaces.size(); ++i) {
        const NetworkInterface& iface = interfaces[i];

        QTableWidgetItem* nameItem = new QTableWidgetItem("🔌 " + iface.name);
        m_interfaceTable->setItem(i, 0, nameItem);

        QTableWidgetItem* ipv4Item = new QTableWidgetItem(iface.ipv4);
        if (iface.ipv4 != "-") {
            ipv4Item->setForeground(QColor("#059669"));
        }
        m_interfaceTable->setItem(i, 1, ipv4Item);

        QTableWidgetItem* ipv6Item = new QTableWidgetItem(iface.ipv6);
        m_interfaceTable->setItem(i, 2, ipv6Item);

        QTableWidgetItem* macItem = new QTableWidgetItem(iface.mac);
        m_interfaceTable->setItem(i, 3, macItem);

        QTableWidgetItem* stateItem = new QTableWidgetItem();
        if (iface.state == "UP") {
            stateItem->setText("🟢 " + iface.state);
            stateItem->setForeground(QColor("#059669"));
        } else if (iface.state == "DOWN") {
            stateItem->setText("🔴 " + iface.state);
            stateItem->setForeground(QColor("#dc2626"));
        } else {
            stateItem->setText(iface.state);
        }
        m_interfaceTable->setItem(i, 4, stateItem);
    }

    QApplication::restoreOverrideCursor();
    m_interfaceStatusLabel->setText(QString(tr("共 %1 个网络接口")).arg(interfaces.size()));
}

void NetworkDiagnosticWidget::startPing()
{
    QString address = m_pingAddressEdit->text().trimmed();
    if (address.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("请输入目标地址"));
        return;
    }

    m_pingOutput->clear();
    appendPingOutput(QString(tr("正在 Ping %1...\n")).arg(address));

    m_startPingBtn->setEnabled(false);
    m_stopPingBtn->setEnabled(true);
    m_pingAddressEdit->setEnabled(false);
    m_pingStatusLabel->setText(tr("正在Ping..."));

    m_pingProcess = new QProcess(this);
    m_pingProcess->setProgram("ping");
    m_pingProcess->setArguments({"-c", "10", address});

    connect(m_pingProcess, &QProcess::readyReadStandardOutput, this, &NetworkDiagnosticWidget::onPingReadyRead);
    connect(m_pingProcess, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &NetworkDiagnosticWidget::onPingFinished);

    m_pingProcess->start();
}

void NetworkDiagnosticWidget::stopPing()
{
    if (m_pingProcess && m_pingProcess->state() != QProcess::NotRunning) {
        m_pingProcess->terminate();
        if (!m_pingProcess->waitForFinished(2000)) {
            m_pingProcess->kill();
        }
    }
}

void NetworkDiagnosticWidget::onPingReadyRead()
{
    if (m_pingProcess) {
        QString output = QString::fromLocal8Bit(m_pingProcess->readAllStandardOutput());
        appendPingOutput(output);
    }
}

void NetworkDiagnosticWidget::onPingFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);

    m_startPingBtn->setEnabled(true);
    m_stopPingBtn->setEnabled(false);
    m_pingAddressEdit->setEnabled(true);
    m_pingStatusLabel->setText(tr("Ping完成"));

    appendPingOutput(tr("\n✅ Ping 已完成\n"));

    if (m_pingProcess) {
        m_pingProcess->deleteLater();
        m_pingProcess = nullptr;
    }
}

void NetworkDiagnosticWidget::appendPingOutput(const QString& text, bool isError)
{
    QTextCursor cursor = m_pingOutput->textCursor();
    cursor.movePosition(QTextCursor::End);

    QTextCharFormat format;
    if (isError) {
        format.setForeground(QColor("#f87171"));
    } else if (text.contains("time=")) {
        format.setForeground(QColor("#4ade80"));
    } else if (text.contains("Destination Host Unreachable") || text.contains("100% packet loss")) {
        format.setForeground(QColor("#f87171"));
    } else {
        format.setForeground(QColor("#e2e8f0"));
    }

    cursor.setCharFormat(format);
    cursor.insertText(text);

    m_pingOutput->setTextCursor(cursor);
    m_pingOutput->ensureCursorVisible();
}

void NetworkDiagnosticWidget::refreshListeningPorts()
{
    m_portsStatusLabel->setText(tr("正在加载监听端口..."));
    QApplication::setOverrideCursor(Qt::WaitCursor);

    QList<ListeningPort> ports;

    QProcess process;
    process.start(QStringLiteral("pkexec"),
                  QStringList() << QStringLiteral("ss") << QStringLiteral("-tulnp"));
    process.waitForFinished(8000);

    QString output = QString::fromUtf8(process.readAllStandardOutput());
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (int i = 1; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();
        if (line.isEmpty()) continue;

        QStringList parts = line.split(QRegularExpression("\\s+"));
        if (parts.size() < 5) continue;

        ListeningPort port;
        port.protocol = parts[0];

        QString localAddr = parts[4];
        int colonPos = localAddr.lastIndexOf(':');
        if (colonPos > 0) {
            port.localAddress = localAddr.left(colonPos);
            QString portStr = localAddr.mid(colonPos + 1);
            port.port = portStr.toInt();
        }

        if (parts.size() >= 6) {
            QString usersPart = "";
            for (int j = 6; j < parts.size(); ++j) {
                usersPart += parts[j] + " ";
            }
            QRegularExpression usersRe("users:\\(\\(\"([^\"]+)\"");
            QRegularExpressionMatch match = usersRe.match(usersPart);
            if (match.hasMatch()) {
                port.pidProgram = match.captured(1);
            } else {
                port.pidProgram = "-";
            }
        } else {
            port.pidProgram = "-";
        }

        ports.append(port);
    }

    std::sort(ports.begin(), ports.end(), [](const ListeningPort& a, const ListeningPort& b) {
        return a.port < b.port;
    });

    m_portsTable->setRowCount(ports.size());
    for (int i = 0; i < ports.size(); ++i) {
        const ListeningPort& port = ports[i];

        QTableWidgetItem* protoItem = new QTableWidgetItem(port.protocol.toUpper());
        if (port.protocol.startsWith("tcp")) {
            protoItem->setForeground(QColor("#2563eb"));
        } else {
            protoItem->setForeground(QColor("#059669"));
        }
        m_portsTable->setItem(i, 0, protoItem);

        QTableWidgetItem* portItem = new QTableWidgetItem(QString::number(port.port));
        portItem->setTextAlignment(Qt::AlignCenter);
        m_portsTable->setItem(i, 1, portItem);

        QTableWidgetItem* addrItem = new QTableWidgetItem(port.localAddress);
        m_portsTable->setItem(i, 2, addrItem);

        QTableWidgetItem* progItem = new QTableWidgetItem(port.pidProgram);
        m_portsTable->setItem(i, 3, progItem);
    }

    QApplication::restoreOverrideCursor();
    m_portsStatusLabel->setText(QString(tr("共 %1 个监听端口")).arg(ports.size()));
}
