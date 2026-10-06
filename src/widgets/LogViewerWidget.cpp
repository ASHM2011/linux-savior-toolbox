#include "LogViewerWidget.h"
#include "core/CommandExecutor.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextCursor>
#include <QProcess>
#include <QFile>

LogViewerWidget::LogViewerWidget(QWidget* parent)
    : QWidget(parent)
    , m_isRefreshing(false)
{
    setupUI();
    loadLogSources();
    QTimer::singleShot(100, this, &LogViewerWidget::onRefresh);
}

void LogViewerWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(16);

    QLabel* titleLabel = new QLabel(tr("📜 系统日志查看器"));
    titleLabel->setObjectName("titleLabel");
    mainLayout->addWidget(titleLabel);

    QLabel* subtitleLabel = new QLabel(tr("查看和过滤系统日志，支持systemd journal、内核日志、Xorg日志等"));
    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setWordWrap(true);
    mainLayout->addWidget(subtitleLabel);

    QFrame* toolbarCard = new QFrame();
    toolbarCard->setObjectName("card");
    QHBoxLayout* toolbarLayout = new QHBoxLayout(toolbarCard);
    toolbarLayout->setContentsMargins(16, 12, 16, 12);
    toolbarLayout->setSpacing(12);

    QLabel* sourceLabel = new QLabel(tr("日志来源:"));
    sourceLabel->setStyleSheet("font-weight: 600; color: #475569;");
    toolbarLayout->addWidget(sourceLabel);

    m_logSourceCombo = new QComboBox();
    m_logSourceCombo->setMinimumWidth(200);
    toolbarLayout->addWidget(m_logSourceCombo);

    QLabel* priorityLabel = new QLabel(tr("优先级:"));
    priorityLabel->setStyleSheet("font-weight: 600; color: #475569;");
    toolbarLayout->addWidget(priorityLabel);

    m_priorityCombo = new QComboBox();
    m_priorityCombo->addItem(tr("全部"), "all");
    m_priorityCombo->addItem(tr("紧急 (emerg)"), "0");
    m_priorityCombo->addItem(tr("警告 (alert)"), "1");
    m_priorityCombo->addItem(tr("严重 (crit)"), "2");
    m_priorityCombo->addItem(tr("错误 (err)"), "3");
    m_priorityCombo->addItem(tr("警告 (warning)"), "4");
    m_priorityCombo->addItem(tr("通知 (notice)"), "5");
    m_priorityCombo->addItem(tr("信息 (info)"), "6");
    m_priorityCombo->addItem(tr("调试 (debug)"), "7");
    m_priorityCombo->setMinimumWidth(140);
    toolbarLayout->addWidget(m_priorityCombo);

    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText(tr("🔍 搜索日志内容..."));
    m_searchEdit->setObjectName("searchInput");
    toolbarLayout->addWidget(m_searchEdit, 1);

    m_refreshBtn = new QPushButton(tr("🔄 刷新"));
    toolbarLayout->addWidget(m_refreshBtn);

    m_autoRefreshBtn = new QPushButton(tr("⏸️ 自动刷新"));
    m_autoRefreshBtn->setCheckable(true);
    m_autoRefreshBtn->setObjectName("secondaryBtn");
    toolbarLayout->addWidget(m_autoRefreshBtn);

    m_exportBtn = new QPushButton(tr("💾 导出"));
    m_exportBtn->setObjectName("secondaryBtn");
    toolbarLayout->addWidget(m_exportBtn);

    mainLayout->addWidget(toolbarCard);

    QFrame* logCard = new QFrame();
    logCard->setObjectName("card");
    QVBoxLayout* logLayout = new QVBoxLayout(logCard);
    logLayout->setContentsMargins(0, 0, 0, 0);

    m_logView = new QTextEdit();
    m_logView->setReadOnly(true);
    m_logView->setFont(QFont("Fira Code, JetBrains Mono, Source Code Pro, Consolas, monospace", 11));
    m_logView->setStyleSheet(R"(
        QTextEdit {
            background-color: #020617;
            color: #e2e8f0;
            border: none;
            border-radius: 16px;
            padding: 16px;
        }
    )");
    logLayout->addWidget(m_logView);

    mainLayout->addWidget(logCard, 1);

    m_statusLabel = new QLabel(tr("就绪"));
    m_statusLabel->setStyleSheet("color: #64748b; font-size: 12px; padding: 4px 0;");
    mainLayout->addWidget(m_statusLabel);

    m_autoRefreshTimer = new QTimer(this);
    m_autoRefreshTimer->setInterval(5000);

    connect(m_logSourceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LogViewerWidget::onLogSourceChanged);
    connect(m_priorityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LogViewerWidget::onFilterChanged);
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &LogViewerWidget::onSearchTextChanged);
    connect(m_refreshBtn, &QPushButton::clicked,
            this, &LogViewerWidget::onRefresh);
    connect(m_autoRefreshBtn, &QPushButton::toggled,
            this, &LogViewerWidget::onAutoRefreshToggled);
    connect(m_exportBtn, &QPushButton::clicked, this, [this]() {
        QString fileName = QFileDialog::getSaveFileName(this,
            tr("导出日志"), "system-log.txt", tr("文本文件 (*.txt);;所有文件 (*)"));
        if (!fileName.isEmpty()) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                file.write(m_logView->toPlainText().toUtf8());
                file.close();
                QMessageBox::information(this, tr("导出成功"), tr("日志已成功导出"));
            }
        }
    });
    connect(m_autoRefreshTimer, &QTimer::timeout, this, &LogViewerWidget::onRefresh);
}

void LogViewerWidget::loadLogSources()
{
    m_logSourceCombo->addItem(tr("系统日志 (最近1小时)"), "--since \"1 hour ago\"");
    m_logSourceCombo->addItem(tr("系统日志 (最近启动)"), "-b -0");
    m_logSourceCombo->addItem(tr("系统日志 (上次启动)"), "-b -1");
    m_logSourceCombo->addItem(tr("内核日志 (dmesg)"), "-k");
    m_logSourceCombo->addItem(tr("认证日志 (auth)"), "SYSLOG_IDENTIFIER=sshd SYSLOG_IDENTIFIER=sudo -u _gdm");
    m_logSourceCombo->addItem(tr("系统服务错误"), "-p err");
    m_logSourceCombo->addItem(tr("Xorg日志"), "--since \"1 day ago\" _COMM=Xorg");
    m_logSourceCombo->addItem(tr("NetworkManager日志"), "-u NetworkManager");
    m_logSourceCombo->addItem(tr("DNF/PackageKit日志"), "-u dnf -u PackageKit");
    m_logSourceCombo->addItem(tr("所有日志 (最近1000行)"), "-n 1000");
}

QString LogViewerWidget::getCurrentLogCommand() const
{
    QString sourceArgs = m_logSourceCombo->currentData().toString();
    QString priority = m_priorityCombo->currentData().toString();

    QString cmd = "journalctl --no-pager -o short-precise ";

    if (!sourceArgs.isEmpty()) {
        cmd += sourceArgs + " ";
    }

    if (priority != "all") {
        cmd += "-p " + priority + " ";
    }

    return cmd;
}

void LogViewerWidget::executeJournalCommand(const QString& args)
{
    if (m_isRefreshing) return;
    m_isRefreshing = true;
    m_refreshBtn->setEnabled(false);
    m_statusLabel->setText(tr("正在加载日志..."));
    m_logView->clear();

    QString cmd = "journalctl --no-pager -o short-precise " + args;

    auto executor = CommandExecutor::instance();
    QProcess* process = new QProcess(this);

    connect(process, &QProcess::readyReadStandardOutput, this, [this, process]() {
        QByteArray data = process->readAllStandardOutput();
        appendLogOutput(QString::fromUtf8(data));
    });

    connect(process, &QProcess::readyReadStandardError, this, [this, process]() {
        QByteArray data = process->readAllStandardError();
        appendLogError(QString::fromUtf8(data));
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process](int exitCode, QProcess::ExitStatus) {
        onCommandFinished(exitCode);
        process->deleteLater();
    });

    QStringList argsList;
    argsList << "-c" << cmd;
    process->start("/bin/sh", argsList);
}

void LogViewerWidget::onRefresh()
{
    QString sourceArgs = m_logSourceCombo->currentData().toString();
    QString priority = m_priorityCombo->currentData().toString();

    QString fullArgs;
    if (!sourceArgs.isEmpty()) {
        fullArgs += sourceArgs + " ";
    }
    if (priority != "all") {
        fullArgs += "-p " + priority + " ";
    }

    executeJournalCommand(fullArgs);
}

void LogViewerWidget::onFilterChanged()
{
    onRefresh();
}

void LogViewerWidget::onLogSourceChanged(int index)
{
    Q_UNUSED(index);
    onRefresh();
}

void LogViewerWidget::onSearchTextChanged(const QString& text)
{
    if (text.isEmpty()) {
        m_logView->moveCursor(QTextCursor::Start);
        return;
    }

    m_logView->moveCursor(QTextCursor::Start);
    bool found = m_logView->find(text);
    if (!found) {
        m_logView->moveCursor(QTextCursor::Start);
    }
}

void LogViewerWidget::onAutoRefreshToggled(bool checked)
{
    if (checked) {
        m_autoRefreshBtn->setText(tr("▶️ 自动刷新中"));
        m_autoRefreshTimer->start();
    } else {
        m_autoRefreshBtn->setText(tr("⏸️ 自动刷新"));
        m_autoRefreshTimer->stop();
    }
}

void LogViewerWidget::appendLogOutput(const QString& text)
{
    QTextCursor cursor = m_logView->textCursor();
    cursor.movePosition(QTextCursor::End);

    QStringList lines = text.split('\n');
    for (const QString& line : lines) {
        QString formattedLine = line;
        QTextCharFormat format;

        if (line.contains("error", Qt::CaseInsensitive) ||
            line.contains("failed", Qt::CaseInsensitive) ||
            line.contains("critical", Qt::CaseInsensitive) ||
            line.contains("emerg", Qt::CaseInsensitive)) {
            format.setForeground(QColor("#f87171"));
        } else if (line.contains("warning", Qt::CaseInsensitive) ||
                   line.contains("warn", Qt::CaseInsensitive)) {
            format.setForeground(QColor("#fbbf24"));
        } else if (line.contains("notice", Qt::CaseInsensitive) ||
                   line.contains("info", Qt::CaseInsensitive)) {
            format.setForeground(QColor("#60a5fa"));
        } else {
            format.setForeground(QColor("#94a3b8"));
        }

        cursor.insertText(formattedLine + "\n", format);
    }

    QScrollBar* sb = m_logView->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void LogViewerWidget::appendLogError(const QString& text)
{
    QTextCursor cursor = m_logView->textCursor();
    cursor.movePosition(QTextCursor::End);
    QTextCharFormat format;
    format.setForeground(QColor("#f87171"));
    cursor.insertText(text, format);
}

void LogViewerWidget::onCommandFinished(int exitCode)
{
    m_isRefreshing = false;
    m_refreshBtn->setEnabled(true);
    if (exitCode == 0) {
        m_statusLabel->setText(QString(tr("✅ 日志加载完成，共 %1 行"))
            .arg(m_logView->document()->blockCount()));
    } else {
        m_statusLabel->setText(QString(tr("❌ 加载日志失败 (退出码: %1)")).arg(exitCode));
    }
}
