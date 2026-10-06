#include "StartupManagerWidget.h"
#include <QTimer>
#include <QHeaderView>
#include <QMessageBox>
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QStandardPaths>
#include <QProcess>
#include <QDateTime>
#include <QRegularExpression>
#include <QPainter>
#include <QBrush>
#include <QColor>

StartupManagerWidget::StartupManagerWidget(QWidget* parent)
    : QWidget(parent)
    , m_searchEdit(nullptr)
    , m_refreshBtn(nullptr)
    , m_toggleBtn(nullptr)
    , m_openDirBtn(nullptr)
    , m_table(nullptr)
    , m_statusLabel(nullptr)
{
    setupUI();
    QTimer::singleShot(100, this, &StartupManagerWidget::refreshEntries);
}

void StartupManagerWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(32, 24, 32, 24);
    mainLayout->setSpacing(16);

    // 顶部卡片
    QFrame* headerCard = new QFrame();
    headerCard->setObjectName("card");
    QHBoxLayout* headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(24, 20, 24, 20);
    headerLayout->setSpacing(16);

    QVBoxLayout* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(4);

    QLabel* titleLabel = new QLabel(tr("🚀 启动项管理"));
    titleLabel->setObjectName("cardTitle");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: 600;");
    titleLayout->addWidget(titleLabel);

    QLabel* descLabel = new QLabel(tr("管理系统启动时自动运行的应用程序和服务"));
    descLabel->setStyleSheet("font-size: 13px; color: #64748b;");
    titleLayout->addWidget(descLabel);

    headerLayout->addLayout(titleLayout, 1);

    m_openDirBtn = new QPushButton(tr("📁 打开目录"));
    m_openDirBtn->setObjectName("secondaryBtn");
    m_openDirBtn->setMinimumHeight(36);
    m_openDirBtn->setCursor(Qt::PointingHandCursor);
    connect(m_openDirBtn, &QPushButton::clicked, this, &StartupManagerWidget::openAutostartDir);
    headerLayout->addWidget(m_openDirBtn);

    m_refreshBtn = new QPushButton(tr("🔄 刷新"));
    m_refreshBtn->setObjectName("secondaryBtn");
    m_refreshBtn->setMinimumHeight(36);
    m_refreshBtn->setCursor(Qt::PointingHandCursor);
    connect(m_refreshBtn, &QPushButton::clicked, this, &StartupManagerWidget::refreshEntries);
    headerLayout->addWidget(m_refreshBtn);

    m_toggleBtn = new QPushButton(tr("✓ 启用/禁用"));
    m_toggleBtn->setObjectName("primaryBtn");
    m_toggleBtn->setMinimumHeight(36);
    m_toggleBtn->setMinimumWidth(120);
    m_toggleBtn->setCursor(Qt::PointingHandCursor);
    m_toggleBtn->setEnabled(false);
    connect(m_toggleBtn, &QPushButton::clicked, this, &StartupManagerWidget::toggleSelectedEntry);
    headerLayout->addWidget(m_toggleBtn);

    mainLayout->addWidget(headerCard);

    // 搜索框
    QFrame* searchCard = new QFrame();
    searchCard->setObjectName("card");
    QHBoxLayout* searchLayout = new QHBoxLayout(searchCard);
    searchLayout->setContentsMargins(16, 12, 16, 12);
    searchLayout->setSpacing(12);

    QLabel* searchIcon = new QLabel("🔍");
    searchIcon->setStyleSheet("font-size: 16px;");
    searchLayout->addWidget(searchIcon);

    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText(tr("搜索启动项名称、命令或描述..."));
    m_searchEdit->setMinimumHeight(36);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &StartupManagerWidget::onSearchTextChanged);
    searchLayout->addWidget(m_searchEdit, 1);

    mainLayout->addWidget(searchCard);

    // 表格
    QFrame* tableCard = new QFrame();
    tableCard->setObjectName("card");
    QVBoxLayout* tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(0, 0, 0, 0);

    m_table = new QTableWidget();
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({tr("状态"), tr("名称"), tr("描述"), tr("命令"), tr("来源")});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this, &StartupManagerWidget::onSelectionChanged);

    tableLayout->addWidget(m_table);
    mainLayout->addWidget(tableCard, 1);

    // 状态栏
    QHBoxLayout* statusRow = new QHBoxLayout();
    m_statusLabel = new QLabel(tr("就绪"));
    m_statusLabel->setStyleSheet("font-size: 12px; color: #64748b;");
    statusRow->addWidget(m_statusLabel);
    statusRow->addStretch(1);
    mainLayout->addLayout(statusRow);
}

void StartupManagerWidget::refreshEntries()
{
    m_statusLabel->setText(tr("正在加载启动项..."));
    m_entries.clear();
    loadEntries();
    populateTable(m_searchEdit ? m_searchEdit->text() : "");
    int enabledCount = 0;
    for (const auto& e : m_entries) {
        if (e.enabled) enabledCount++;
    }
    m_statusLabel->setText(QString(tr("共 %1 个启动项，%2 个已启用")).arg(m_entries.size()).arg(enabledCount));
    updateButtonStates();
}

void StartupManagerWidget::onSearchTextChanged(const QString& text)
{
    populateTable(text);
}

void StartupManagerWidget::onSelectionChanged()
{
    updateButtonStates();
}

void StartupManagerWidget::toggleSelectedEntry()
{
    int row = m_table->currentRow();
    if (row < 0) return;
    QString fileName = m_table->item(row, 1)->data(Qt::UserRole).toString();
    StartupEntry* target = nullptr;
    for (auto& e : m_entries) {
        if (e.fileName == fileName) { target = &e; break; }
    }
    if (!target) return;

    bool newState = !target->enabled;
    setEntryEnabled(*target, newState);
    refreshEntries();
}

void StartupManagerWidget::openAutostartDir()
{
    QString path = userAutostartDir();
    QDir().mkpath(path);
    QProcess::startDetached("xdg-open", {path});
}

void StartupManagerWidget::loadEntries()
{
    QStringList searchDirs;
    searchDirs << userAutostartDir();
    searchDirs << "/etc/xdg/autostart";
    searchDirs << "/usr/share/applications";

    QSet<QString> seen;
    for (const QString& dirPath : searchDirs) {
        QDir dir(dirPath);
        if (!dir.exists()) continue;
        QStringList files = dir.entryList(QStringList() << "*.desktop", QDir::Files | QDir::NoDotAndDotDot);
        for (const QString& f : files) {
            if (seen.contains(f)) continue;
            seen.insert(f);
            QString fullPath = dir.filePath(f);
            StartupEntry entry;
            entry.fileName = f;
            entry.filePath = fullPath;
            entry.isUserEntry = (dirPath == userAutostartDir());
            QFile file(fullPath);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
            QTextStream in(&file);
            bool inDesktopEntry = false;
            bool hidden = false;
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (line.startsWith("[") && line.endsWith("]")) {
                    inDesktopEntry = (line == "[Desktop Entry]");
                    continue;
                }
                if (!inDesktopEntry) continue;
                int eq = line.indexOf('=');
                if (eq < 0) continue;
                QString key = line.left(eq).trimmed();
                QString val = line.mid(eq + 1).trimmed();
                if (key == "Name") entry.name = val;
                else if (key == "Comment") entry.comment = val;
                else if (key == "Exec") entry.exec = val;
                else if (key == "X-GNOME-Autostart-enabled") entry.enabled = (val.toLower() == "true");
                else if (key == "Hidden") hidden = (val.toLower() == "true");
            }
            if (entry.name.isEmpty()) entry.name = f;
            entry.enabled = entry.enabled && !hidden;
            if (dirPath == "/usr/share/applications") {
                // /usr/share/applications 下的默认不算 autostart，除非有用户覆盖
                continue;
            }
            if (hidden && !entry.isUserEntry) {
                continue;
            }
            m_entries.append(entry);
        }
    }
}

void StartupManagerWidget::populateTable(const QString& filter)
{
    m_table->setRowCount(0);
    QString f = filter.trimmed().toLower();
    int row = 0;
    for (const auto& e : m_entries) {
        if (!f.isEmpty()) {
            bool match = e.name.toLower().contains(f)
                      || e.comment.toLower().contains(f)
                      || e.exec.toLower().contains(f)
                      || e.fileName.toLower().contains(f);
            if (!match) continue;
        }
        m_table->insertRow(row);

        QLabel* statusLabel = new QLabel();
        statusLabel->setAlignment(Qt::AlignCenter);
        statusLabel->setContentsMargins(8, 0, 8, 0);
        statusLabel->setText(e.enabled ? tr("🟢 已启用") : tr("⚪ 已禁用"));
        m_table->setCellWidget(row, 0, statusLabel);

        QTableWidgetItem* nameItem = new QTableWidgetItem(e.name);
        nameItem->setData(Qt::UserRole, e.fileName);
        nameItem->setFont(QFont(nameItem->font().family(), -1, QFont::Medium));
        m_table->setItem(row, 1, nameItem);

        m_table->setItem(row, 2, new QTableWidgetItem(e.comment));
        m_table->setItem(row, 3, new QTableWidgetItem(e.exec));

        QLabel* srcLabel = new QLabel(e.isUserEntry ? tr("👤 用户") : tr("🔒 系统"));
        srcLabel->setAlignment(Qt::AlignCenter);
        srcLabel->setStyleSheet(e.isUserEntry ? "color: #0891b2; font-size: 12px;" : "color: #64748b; font-size: 12px;");
        m_table->setCellWidget(row, 4, srcLabel);

        m_table->setRowHeight(row, 36);
        row++;
    }
}

void StartupManagerWidget::updateButtonStates()
{
    bool hasSelection = m_table && m_table->currentRow() >= 0;
    if (m_toggleBtn) m_toggleBtn->setEnabled(hasSelection);
}

void StartupManagerWidget::setEntryEnabled(const StartupEntry& entry, bool enabled)
{
    QString targetDir = userAutostartDir();
    QDir().mkpath(targetDir);
    QString targetPath = targetDir + "/" + entry.fileName;

    // 如果原文件是系统级别的，先复制过来再修改
    if (!entry.isUserEntry && !QFile::exists(targetPath)) {
        QFile::copy(entry.filePath, targetPath);
    }

    // 读取现有内容
    QString content;
    QString readPath = QFile::exists(targetPath) ? targetPath : entry.filePath;
    QFile inFile(readPath);
    if (inFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&inFile);
        content = in.readAll();
        inFile.close();
    }

    // 修改或添加 X-GNOME-Autostart-enabled 和 Hidden
    QStringList lines = content.split('\n');
    bool inDesktop = false;
    bool foundStartEnabled = false;
    bool foundHidden = false;
    for (QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.startsWith("[") && trimmed.endsWith("]")) {
            inDesktop = (trimmed == "[Desktop Entry]");
            continue;
        }
        if (!inDesktop) continue;
        int eq = line.indexOf('=');
        if (eq < 0) continue;
        QString key = line.left(eq).trimmed();
        if (key == "X-GNOME-Autostart-enabled") {
            line = "X-GNOME-Autostart-enabled=" + QString(enabled ? "true" : "false");
            foundStartEnabled = true;
        } else if (key == "Hidden") {
            line = "Hidden=" + QString(enabled ? "false" : "true");
            foundHidden = true;
        }
    }

    // 如果没找到，在 [Desktop Entry] 末尾添加
    if (!foundStartEnabled || !foundHidden) {
        QStringList newLines;
        bool inSection = false;
        bool inserted = false;
        for (const QString& line : lines) {
            QString trimmed = line.trimmed();
            if (trimmed.startsWith("[") && trimmed.endsWith("]")) {
                if (inSection && !inserted) {
                    if (!foundStartEnabled) newLines.append("X-GNOME-Autostart-enabled=" + QString(enabled ? "true" : "false"));
                    if (!foundHidden) newLines.append("Hidden=" + QString(enabled ? "false" : "true"));
                    inserted = true;
                }
                inSection = (trimmed == "[Desktop Entry]");
            }
            newLines.append(line);
        }
        if (!inserted) {
            if (!foundStartEnabled) newLines.append("X-GNOME-Autostart-enabled=" + QString(enabled ? "true" : "false"));
            if (!foundHidden) newLines.append("Hidden=" + QString(enabled ? "false" : "true"));
        }
        lines = newLines;
    }

    QFile outFile(targetPath);
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QTextStream out(&outFile);
        out << lines.join('\n');
        outFile.close();
    }
}

QString StartupManagerWidget::userAutostartDir() const
{
    QString config = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    return config + "/autostart";
}
