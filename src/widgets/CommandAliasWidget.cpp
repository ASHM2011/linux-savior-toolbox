#include "CommandAliasWidget.h"
#include "core/SystemDetector.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QCheckBox>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QProcessEnvironment>
#include <QInputDialog>
#include <QFileInfo>
#include <QDateTime>

CommandAliasWidget::CommandAliasWidget(QWidget* parent)
    : QWidget(parent)
    , m_presetTable(nullptr)
    , m_aliasNameEdit(nullptr)
    , m_aliasCmdEdit(nullptr)
    , m_customTable(nullptr)
    , m_bashBtn(nullptr)
    , m_zshBtn(nullptr)
    , m_currentShellLabel(nullptr)
{
    setupUI();
}

void CommandAliasWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(32, 24, 32, 24);
    mainLayout->setSpacing(20);

    QLabel* title = new QLabel(tr("⌨️ 命令别名一键配置"));
    title->setStyleSheet("font-size: 22px; font-weight: 700; color: #0f172a;");
    mainLayout->addWidget(title);

    QLabel* subtitle = new QLabel(tr("让命令更好记更短，新手也能玩转终端"));
    subtitle->setStyleSheet("font-size: 13px; color: #64748b;");
    mainLayout->addWidget(subtitle);

    QLabel* currentShell = new QLabel();
    QString shell = detectCurrentShell();
    currentShell->setText(QString(tr("当前使用的 Shell：<b style='color: #2563eb;'>%1</b> （已为你高亮推荐）")).arg(shell));
    currentShell->setStyleSheet("font-size: 13px; color: #475569; padding: 8px 12px; background-color: #eff6ff; border-radius: 8px;");
    mainLayout->addWidget(currentShell);

    QLabel* presetTitle = new QLabel(tr("✨ 萌新常用预设（推荐）"));
    presetTitle->setStyleSheet("font-size: 15px; font-weight: 600; color: #0f172a; margin-top: 8px;");
    mainLayout->addWidget(presetTitle);

    QFrame* presetCard = new QFrame();
    presetCard->setObjectName("card");
    QVBoxLayout* presetLayout = new QVBoxLayout(presetCard);
    presetLayout->setContentsMargins(16, 16, 16, 16);
    presetLayout->setSpacing(12);

    m_presetTable = new QTableWidget(6, 4);
    m_presetTable->setHorizontalHeaderLabels({tr("启用"), tr("别名"), tr("对应命令"), tr("说明")});
    m_presetTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_presetTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_presetTable->verticalHeader()->setVisible(false);
    m_presetTable->horizontalHeader()->setStretchLastSection(true);
    m_presetTable->setColumnWidth(0, 60);
    m_presetTable->setColumnWidth(1, 100);
    m_presetTable->setColumnWidth(2, 280);

    setupPresetTable();

    presetLayout->addWidget(m_presetTable);
    mainLayout->addWidget(presetCard);

    QLabel* customTitle = new QLabel(tr("➕ 自定义别名"));
    customTitle->setStyleSheet("font-size: 15px; font-weight: 600; color: #0f172a; margin-top: 8px;");
    mainLayout->addWidget(customTitle);

    QFrame* customCard = new QFrame();
    customCard->setObjectName("card");
    QVBoxLayout* customLayout = new QVBoxLayout(customCard);
    customLayout->setContentsMargins(16, 16, 16, 16);
    customLayout->setSpacing(12);

    QHBoxLayout* inputLayout = new QHBoxLayout();
    inputLayout->setSpacing(12);

    QVBoxLayout* nameLayout = new QVBoxLayout();
    nameLayout->setSpacing(4);
    QLabel* nameLabel = new QLabel(tr("别名"));
    nameLabel->setStyleSheet("font-size: 12px; color: #64748b; font-weight: 500;");
    nameLayout->addWidget(nameLabel);
    m_aliasNameEdit = new QLineEdit();
    m_aliasNameEdit->setPlaceholderText(tr("例如: mycmd"));
    nameLayout->addWidget(m_aliasNameEdit);
    inputLayout->addLayout(nameLayout);

    QVBoxLayout* cmdLayout = new QVBoxLayout();
    cmdLayout->setSpacing(4);
    QLabel* cmdLabel = new QLabel(tr("对应命令"));
    cmdLabel->setStyleSheet("font-size: 12px; color: #64748b; font-weight: 500;");
    cmdLayout->addWidget(cmdLabel);
    m_aliasCmdEdit = new QLineEdit();
    m_aliasCmdEdit->setPlaceholderText(tr("例如: ls -lah"));
    cmdLayout->addWidget(m_aliasCmdEdit);
    inputLayout->addLayout(cmdLayout, 1);

    QPushButton* addBtn = new QPushButton(tr("➕ 添加"));
    addBtn->setFixedHeight(34);
    addBtn->setStyleSheet("margin-top: 18px;");
    addBtn->setCursor(Qt::PointingHandCursor);
    connect(addBtn, &QPushButton::clicked, this, &CommandAliasWidget::onAddAliasClicked);
    inputLayout->addWidget(addBtn);

    customLayout->addLayout(inputLayout);

    m_customTable = new QTableWidget(0, 3);
    m_customTable->setHorizontalHeaderLabels({tr("别名"), tr("对应命令"), tr("操作")});
    m_customTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_customTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_customTable->verticalHeader()->setVisible(false);
    m_customTable->horizontalHeader()->setStretchLastSection(false);
    m_customTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_customTable->setColumnWidth(0, 120);
    m_customTable->setColumnWidth(2, 120);

    setupCustomTable();

    customLayout->addWidget(m_customTable);
    mainLayout->addWidget(customCard);

    mainLayout->addStretch(1);

    QFrame* applyBar = new QFrame();
    applyBar->setObjectName("card");
    QHBoxLayout* applyLayout = new QHBoxLayout(applyBar);
    applyLayout->setContentsMargins(20, 16, 20, 16);
    applyLayout->setSpacing(12);

    QVBoxLayout* hintLayout = new QVBoxLayout();
    hintLayout->setSpacing(2);
    QLabel* applyHint = new QLabel(tr("💡 一键写入配置文件，永久生效"));
    applyHint->setStyleSheet("font-size: 13px; color: #475569;");
    hintLayout->addWidget(applyHint);
    m_currentShellLabel = new QLabel(QString(tr("检测到当前 Shell: %1")).arg(shell));
    m_currentShellLabel->setStyleSheet("font-size: 11px; color: #94a3b8;");
    hintLayout->addWidget(m_currentShellLabel);
    applyLayout->addLayout(hintLayout);
    applyLayout->addStretch(1);

    m_bashBtn = new QPushButton(tr("📝 写入 ~/.bashrc"));
    m_bashBtn->setObjectName("secondaryBtn");
    m_bashBtn->setFixedHeight(40);
    m_bashBtn->setCursor(Qt::PointingHandCursor);
    connect(m_bashBtn, &QPushButton::clicked, this, &CommandAliasWidget::onApplyBashClicked);
    applyLayout->addWidget(m_bashBtn);

    m_zshBtn = new QPushButton(tr("📝 写入 ~/.zshrc"));
    m_zshBtn->setFixedHeight(40);
    m_zshBtn->setCursor(Qt::PointingHandCursor);
    connect(m_zshBtn, &QPushButton::clicked, this, &CommandAliasWidget::onApplyZshClicked);
    applyLayout->addWidget(m_zshBtn);

    if (shell.contains("zsh", Qt::CaseInsensitive)) {
        m_zshBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #3b82f6;
                color: white;
                border: 2px solid #3b82f6;
                border-radius: 8px;
                padding: 8px 16px;
                font-weight: 600;
            }
            QPushButton:hover {
                background-color: #2563eb;
            }
        )");
    } else {
        m_bashBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #3b82f6;
                color: white;
                border: 2px solid #3b82f6;
                border-radius: 8px;
                padding: 8px 16px;
                font-weight: 600;
            }
            QPushButton:hover {
                background-color: #2563eb;
            }
        )");
    }

    mainLayout->addWidget(applyBar);
}

void CommandAliasWidget::setupPresetTable()
{
    QString distro = detectCurrentDistro();

    QString updateCmd, cleanCmd;
    QString updateDesc, cleanDesc;

    if (distro == "fedora" || distro == "rhel" || distro == "centos") {
        updateCmd = "sudo dnf upgrade --refresh -y";
        cleanCmd = "sudo dnf autoremove -y && sudo dnf clean all";
        updateDesc = tr("一键更新系统所有软件（DNF）");
        cleanDesc = tr("一键清理系统垃圾和缓存（DNF）");
    } else if (distro == "opensuse" || distro == "suse") {
        updateCmd = "sudo zypper refresh && sudo zypper up -y";
        cleanCmd = "sudo zypper cc -a && sudo zypper rm -u";
        updateDesc = tr("一键更新系统所有软件（Zypper）");
        cleanDesc = tr("一键清理系统垃圾和缓存（Zypper）");
    } else if (distro == "arch" || distro == "manjaro") {
        updateCmd = "sudo pacman -Syu --noconfirm";
        cleanCmd = "sudo pacman -Sc --noconfirm && sudo pacman -Rns $(pacman -Qtdq) --noconfirm";
        updateDesc = tr("一键更新系统所有软件（Pacman）");
        cleanDesc = tr("一键清理系统垃圾和缓存（Pacman）");
    } else {
        updateCmd = "sudo apt update && sudo apt upgrade -y";
        cleanCmd = "sudo apt autoremove -y && sudo apt clean";
        updateDesc = tr("一键更新系统所有软件（APT）");
        cleanDesc = tr("一键清理系统垃圾和缓存（APT）");
    }

    m_presetAliases = {"update", "clean", "ll", "rm", "..", "gs"};
    m_presetCommands = {
        updateCmd,
        cleanCmd,
        "ls -lah",
        "rm -i",
        "cd ..",
        "git status"
    };
    m_presetDescs = {
        updateDesc,
        cleanDesc,
        tr("详细列出文件（含隐藏文件）"),
        tr("带确认的安全删除，防止误删"),
        tr("快速返回上级目录"),
        tr("查看 Git 仓库状态")
    };

    int count = m_presetAliases.size();
    m_presetTable->setRowCount(count);

    for (int i = 0; i < count; i++) {
        m_presetEnabled.append(true);
        m_presetTable->setRowHeight(i, 44);

        QWidget* checkboxWidget = new QWidget();
        QHBoxLayout* checkboxLayout = new QHBoxLayout(checkboxWidget);
        checkboxLayout->setContentsMargins(0, 0, 0, 0);
        checkboxLayout->setAlignment(Qt::AlignCenter);
        QCheckBox* checkbox = new QCheckBox();
        checkbox->setChecked(true);
        checkboxLayout->addWidget(checkbox);
        m_presetTable->setCellWidget(i, 0, checkboxWidget);

        connect(checkbox, &QCheckBox::toggled, this, [this, i](bool checked) {
            onPresetCheckboxChanged(i, checked);
        });

        QTableWidgetItem* aliasItem = new QTableWidgetItem(m_presetAliases[i]);
        aliasItem->setFont(QFont("", -1, QFont::Bold));
        m_presetTable->setItem(i, 1, aliasItem);
        m_presetTable->setItem(i, 2, new QTableWidgetItem(m_presetCommands[i]));
        m_presetTable->setItem(i, 3, new QTableWidgetItem(m_presetDescs[i]));
    }
}

QString CommandAliasWidget::detectCurrentDistro() const
{
    auto info = SystemDetector::instance()->getSystemInfo();
    return info.osReleaseId.toLower();
}

void CommandAliasWidget::setupCustomTable()
{
}

QString CommandAliasWidget::detectCurrentShell() const
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString shell = env.value("SHELL", "");
    if (shell.isEmpty()) {
        return "bash";
    }
    QFileInfo fileInfo(shell);
    return fileInfo.fileName();
}

QString CommandAliasWidget::generateAliasContent() const
{
    QString content;
    QTextStream stream(&content);

    stream << "\n";
    stream << tr("# ============ Linux 萌新救星工具箱 - 命令别名 ============\n");
    stream << tr("# 由 Linux Savior Toolbox 自动生成\n");
    stream << tr("# 生成时间: ") << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << "\n";
    stream << "\n";

    stream << tr("# --- 预设别名 ---\n");
    for (int i = 0; i < m_presetEnabled.size() && i < m_presetAliases.size(); i++) {
        if (m_presetEnabled[i]) {
            stream << "alias " << m_presetAliases[i] << "='" << m_presetCommands[i] << "'\n";
        }
    }

    if (!m_customAliases.isEmpty()) {
        stream << tr("\n# --- 自定义别名 ---\n");
        for (const auto& alias : m_customAliases) {
            stream << "alias " << alias.first << "='" << alias.second << "'\n";
        }
    }

    stream << "\n# ========================================================\n";
    stream << "\n";

    return content;
}

bool CommandAliasWidget::writeToFile(const QString& filePath, const QString& content)
{
    QFile file(filePath);
    if (!file.open(QFile::Append | QFile::Text)) {
        return false;
    }
    QTextStream out(&file);
    out << content;
    file.close();
    return true;
}

void CommandAliasWidget::onApplyBashClicked()
{
    QString bashrcPath = QDir::homePath() + "/.bashrc";

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("确认写入"),
        QString(tr("确定要将别名配置写入 %1 吗？\n\n" "将写入以下内容：\n" "- %2 个启用的预设别名\n" "- %3 个自定义别名\n\n" "写入后将永久生效，打开新终端即可使用。"))
            .arg(bashrcPath)
            .arg(m_presetEnabled.count(true))
            .arg(m_customAliases.size()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes
    );

    if (reply != QMessageBox::Yes) {
        return;
    }

    QString content = generateAliasContent();
    if (writeToFile(bashrcPath, content)) {
        QMessageBox::information(this, tr("配置成功"),
            QString(tr("命令别名已写入 %1\n\n" "生效方法：\n" "1. 打开新的终端窗口\n" "2. 或者执行 source ~/.bashrc")).arg(bashrcPath));
    } else {
        QMessageBox::warning(this, tr("写入失败"),
            QString(tr("无法写入 %1，请检查文件权限。")).arg(bashrcPath));
    }
}

void CommandAliasWidget::onApplyZshClicked()
{
    QString zshrcPath = QDir::homePath() + "/.zshrc";

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("确认写入"),
        QString(tr("确定要将别名配置写入 %1 吗？\n\n" "将写入以下内容：\n" "- %2 个启用的预设别名\n" "- %3 个自定义别名\n\n" "写入后将永久生效，打开新终端即可使用。"))
            .arg(zshrcPath)
            .arg(m_presetEnabled.count(true))
            .arg(m_customAliases.size()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes
    );

    if (reply != QMessageBox::Yes) {
        return;
    }

    QString content = generateAliasContent();
    if (writeToFile(zshrcPath, content)) {
        QMessageBox::information(this, tr("配置成功"),
            QString(tr("命令别名已写入 %1\n\n" "生效方法：\n" "1. 打开新的终端窗口\n" "2. 或者执行 source ~/.zshrc")).arg(zshrcPath));
    } else {
        QMessageBox::warning(this, tr("写入失败"),
            QString(tr("无法写入 %1，请检查文件权限。")).arg(zshrcPath));
    }
}

void CommandAliasWidget::onAddAliasClicked()
{
    QString name = m_aliasNameEdit->text().trimmed();
    QString cmd = m_aliasCmdEdit->text().trimmed();

    if (name.isEmpty() || cmd.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("请填写别名和对应命令"));
        return;
    }

    for (const auto& alias : m_customAliases) {
        if (alias.first == name) {
            QMessageBox::warning(this, tr("提示"), QString(tr("别名 \"%1\" 已存在，请使用其他名称。")).arg(name));
            return;
        }
    }

    m_customAliases.append(qMakePair(name, cmd));

    int row = m_customTable->rowCount();
    m_customTable->insertRow(row);
    m_customTable->setRowHeight(row, 40);
    m_customTable->setItem(row, 0, new QTableWidgetItem(name));
    m_customTable->setItem(row, 1, new QTableWidgetItem(cmd));

    QWidget* btnWidget = new QWidget();
    QHBoxLayout* btnLayout = new QHBoxLayout(btnWidget);
    btnLayout->setContentsMargins(4, 0, 4, 0);
    btnLayout->setSpacing(4);
    btnLayout->setAlignment(Qt::AlignCenter);

    QPushButton* editBtn = new QPushButton(tr("✏️ 编辑"));
    editBtn->setObjectName("secondaryBtn");
    editBtn->setFixedHeight(28);
    editBtn->setCursor(Qt::PointingHandCursor);
    editBtn->setStyleSheet("font-size: 12px; padding: 4px 10px;");
    connect(editBtn, &QPushButton::clicked, this, [this, row]() {
        onEditAliasClicked(row);
    });
    btnLayout->addWidget(editBtn);

    QPushButton* deleteBtn = new QPushButton(tr("🗑️ 删除"));
    deleteBtn->setObjectName("dangerBtn");
    deleteBtn->setFixedHeight(28);
    deleteBtn->setCursor(Qt::PointingHandCursor);
    deleteBtn->setStyleSheet("font-size: 12px; padding: 4px 10px;");
    connect(deleteBtn, &QPushButton::clicked, this, [this, row]() {
        onDeleteAliasClicked(row);
    });
    btnLayout->addWidget(deleteBtn);

    m_customTable->setCellWidget(row, 2, btnWidget);

    m_aliasNameEdit->clear();
    m_aliasCmdEdit->clear();
}

void CommandAliasWidget::onEditAliasClicked(int row)
{
    if (row < 0 || row >= m_customAliases.size()) {
        return;
    }

    QString oldName = m_customAliases[row].first;
    QString oldCmd = m_customAliases[row].second;

    bool ok;
    QString newName = QInputDialog::getText(this, tr("编辑别名"), tr("别名:"), QLineEdit::Normal, oldName, &ok);
    if (!ok || newName.trimmed().isEmpty()) {
        return;
    }

    QString newCmd = QInputDialog::getText(this, tr("编辑别名"), tr("对应命令:"), QLineEdit::Normal, oldCmd, &ok);
    if (!ok || newCmd.trimmed().isEmpty()) {
        return;
    }

    newName = newName.trimmed();
    newCmd = newCmd.trimmed();

    for (int i = 0; i < m_customAliases.size(); i++) {
        if (i != row && m_customAliases[i].first == newName) {
            QMessageBox::warning(this, tr("提示"), QString(tr("别名 \"%1\" 已存在，请使用其他名称。")).arg(newName));
            return;
        }
    }

    m_customAliases[row] = qMakePair(newName, newCmd);
    m_customTable->item(row, 0)->setText(newName);
    m_customTable->item(row, 1)->setText(newCmd);
}

void CommandAliasWidget::onDeleteAliasClicked(int row)
{
    if (row < 0 || row >= m_customAliases.size()) {
        return;
    }

    QString name = m_customAliases[row].first;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("确认删除"),
        QString(tr("确定要删除别名 \"%1\" 吗？")).arg(name),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply != QMessageBox::Yes) {
        return;
    }

    m_customAliases.removeAt(row);
    m_customTable->removeRow(row);
}

void CommandAliasWidget::onPresetCheckboxChanged(int row, bool checked)
{
    if (row >= 0 && row < m_presetEnabled.size()) {
        m_presetEnabled[row] = checked;
    }
}
