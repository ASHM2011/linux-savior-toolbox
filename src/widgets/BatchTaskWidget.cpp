#include "BatchTaskWidget.h"
#include "core/SystemDetector.h"
#include "core/CommandMetadata.h"
#include <QMessageBox>
#include <QTimer>

BatchTaskWidget::BatchTaskWidget(QWidget* parent)
    : QWidget(parent)
    , m_availableList(nullptr)
    , m_selectedList(nullptr)
    , m_overallProgress(nullptr)
    , m_currentTaskLabel(nullptr)
    , m_progressCountLabel(nullptr)
    , m_executeCustomBtn(nullptr)
    , m_selectedPresetIndex(-1)
{
    initTasks();
    setupUI();
    initAvailableCommands();
}

void BatchTaskWidget::initTasks()
{
    auto sysDet = SystemDetector::instance();

    BatchTask systemMaintain;
    systemMaintain.id = "system_maintain";
    systemMaintain.icon = "🚀";
    systemMaintain.name = tr("一键系统维护");
    systemMaintain.description = tr("更新系统 + 清理缓存 + 清理无用依赖，建议每周执行一次");
    systemMaintain.commandIds = {
        sysDet->getUpdateCommandId(),
        sysDet->getCleanCacheCommandId(),
        "apt_autoremove"
    };
    systemMaintain.color = "#3b82f6";
    m_tasks.append(systemMaintain);

    BatchTask deepClean;
    deepClean.id = "deep_clean";
    deepClean.icon = "🧹";
    deepClean.name = tr("一键深度清理");
    deepClean.description = tr("包缓存 + 系统日志 + 缩略图 + 浏览器缓存，释放更多磁盘空间");
    deepClean.commandIds = {
        sysDet->getCleanCacheCommandId(),
        "apt_autoremove",
        "clean_system_logs",
        "clean_thumbnails",
        "rm_trash"
    };
    deepClean.color = "#10b981";
    m_tasks.append(deepClean);

    BatchTask desktopRepair;
    desktopRepair.id = "desktop_repair";
    desktopRepair.icon = "🔧";
    desktopRepair.name = tr("一键桌面修复");
    desktopRepair.description = tr("重启桌面 + 清理桌面缓存 + 修复图标，解决桌面各种小问题");
    desktopRepair.commandIds = {
        "gnome_shell_restart",
        "nautilus_restart",
        "ibus_restart",
        "clean_thumbnails"
    };
    desktopRepair.color = "#8b5cf6";
    m_tasks.append(desktopRepair);
}

void BatchTaskWidget::initAvailableCommands()
{
    auto mgr = CommandMetadataManager::instance();
    auto allCmds = mgr->getAllCommands();

    for (const auto& cmd : allCmds) {
        QListWidgetItem* item = new QListWidgetItem(cmd.friendlyName);
        item->setData(Qt::UserRole, cmd.id);
        QString icon = "⚙️";
        if (cmd.safetyLevel == SafetyLevel::Safe) icon = "✅";
        else if (cmd.safetyLevel == SafetyLevel::Caution) icon = "⚠️";
        else icon = "❌";
        item->setText(icon + "  " + cmd.friendlyName);
        m_availableList->addItem(item);
    }
}

void BatchTaskWidget::setupUI()
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

    QLabel* title = new QLabel(tr("📋 批量任务"));
    title->setStyleSheet("font-size: 22px; font-weight: 700; color: #0f172a;");
    contentLayout->addWidget(title);

    QLabel* subtitle = new QLabel(tr("把多个常用操作组合起来，一键执行，省时省力"));
    subtitle->setStyleSheet("font-size: 13px; color: #64748b;");
    contentLayout->addWidget(subtitle);

    QLabel* presetTitle = new QLabel(tr("✨ 预设批量任务"));
    presetTitle->setStyleSheet("font-size: 15px; font-weight: 600; color: #0f172a; margin-top: 8px;");
    contentLayout->addWidget(presetTitle);

    m_presetLayout = new QVBoxLayout();
    m_presetLayout->setSpacing(12);

    for (int i = 0; i < m_tasks.size(); i++) {
        m_presetLayout->addWidget(createPresetCard(m_tasks[i], i));
    }

    contentLayout->addLayout(m_presetLayout);

    QFrame* progressFrame = createProgressSection();
    contentLayout->addWidget(progressFrame);

    QLabel* customTitle = new QLabel(tr("🛠️ 自定义批量任务"));
    customTitle->setStyleSheet("font-size: 15px; font-weight: 600; color: #0f172a; margin-top: 8px;");
    contentLayout->addWidget(customTitle);

    QFrame* customFrame = new QFrame();
    customFrame->setObjectName("card");
    QVBoxLayout* customLayout = new QVBoxLayout(customFrame);
    customLayout->setContentsMargins(20, 16, 20, 16);
    customLayout->setSpacing(12);

    QHBoxLayout* listsLayout = new QHBoxLayout();
    listsLayout->setSpacing(12);

    QVBoxLayout* availableLayout = new QVBoxLayout();
    availableLayout->setSpacing(8);
    QLabel* availableLabel = new QLabel(tr("📦 可选命令"));
    availableLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #334155;");
    availableLayout->addWidget(availableLabel);
    m_availableList = new QListWidget();
    m_availableList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_availableList->setFixedHeight(220);
    connect(m_availableList, &QListWidget::itemDoubleClicked,
            this, &BatchTaskWidget::onAvailableItemDoubleClicked);
    availableLayout->addWidget(m_availableList, 1);
    listsLayout->addLayout(availableLayout, 1);

    QVBoxLayout* buttonLayout = new QVBoxLayout();
    buttonLayout->addStretch(1);

    QPushButton* addBtn = new QPushButton("→");
    addBtn->setObjectName("secondaryBtn");
    addBtn->setFixedSize(40, 36);
    addBtn->setCursor(Qt::PointingHandCursor);
    connect(addBtn, &QPushButton::clicked, this, &BatchTaskWidget::onAddCommandClicked);
    buttonLayout->addWidget(addBtn);

    QPushButton* removeBtn = new QPushButton("←");
    removeBtn->setObjectName("secondaryBtn");
    removeBtn->setFixedSize(40, 36);
    removeBtn->setCursor(Qt::PointingHandCursor);
    connect(removeBtn, &QPushButton::clicked, this, &BatchTaskWidget::onRemoveCommandClicked);
    buttonLayout->addWidget(removeBtn);

    buttonLayout->addSpacing(8);

    QPushButton* upBtn = new QPushButton("↑");
    upBtn->setObjectName("secondaryBtn");
    upBtn->setFixedSize(40, 36);
    upBtn->setCursor(Qt::PointingHandCursor);
    connect(upBtn, &QPushButton::clicked, this, &BatchTaskWidget::onMoveUpClicked);
    buttonLayout->addWidget(upBtn);

    QPushButton* downBtn = new QPushButton("↓");
    downBtn->setObjectName("secondaryBtn");
    downBtn->setFixedSize(40, 36);
    downBtn->setCursor(Qt::PointingHandCursor);
    connect(downBtn, &QPushButton::clicked, this, &BatchTaskWidget::onMoveDownClicked);
    buttonLayout->addWidget(downBtn);

    buttonLayout->addStretch(1);
    listsLayout->addLayout(buttonLayout);

    QVBoxLayout* selectedLayout = new QVBoxLayout();
    selectedLayout->setSpacing(8);
    QLabel* selectedLabel = new QLabel(tr("✅ 已选命令（可排序）"));
    selectedLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #334155;");
    selectedLayout->addWidget(selectedLabel);
    m_selectedList = new QListWidget();
    m_selectedList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_selectedList->setFixedHeight(220);
    m_selectedList->setDragDropMode(QAbstractItemView::InternalMove);
    connect(m_selectedList, &QListWidget::itemDoubleClicked,
            this, &BatchTaskWidget::onSelectedItemDoubleClicked);
    selectedLayout->addWidget(m_selectedList, 1);
    listsLayout->addLayout(selectedLayout, 1);

    customLayout->addLayout(listsLayout);

    QHBoxLayout* customBtnLayout = new QHBoxLayout();
    customBtnLayout->setSpacing(10);

    QPushButton* savePresetBtn = new QPushButton(tr("💾 保存为预设"));
    savePresetBtn->setObjectName("secondaryBtn");
    savePresetBtn->setFixedHeight(40);
    savePresetBtn->setCursor(Qt::PointingHandCursor);
    connect(savePresetBtn, &QPushButton::clicked, this, &BatchTaskWidget::onSavePresetClicked);
    customBtnLayout->addWidget(savePresetBtn);

    customBtnLayout->addStretch(1);

    m_executeCustomBtn = new QPushButton(tr("▶ 执行自定义任务"));
    m_executeCustomBtn->setFixedHeight(40);
    m_executeCustomBtn->setFixedWidth(160);
    m_executeCustomBtn->setCursor(Qt::PointingHandCursor);
    connect(m_executeCustomBtn, &QPushButton::clicked, this, &BatchTaskWidget::onExecuteCustomClicked);
    customBtnLayout->addWidget(m_executeCustomBtn);

    customLayout->addLayout(customBtnLayout);

    contentLayout->addWidget(customFrame);
    contentLayout->addStretch(1);

    scrollArea->setWidget(content);
    mainLayout->addWidget(scrollArea);
}

QFrame* BatchTaskWidget::createPresetCard(const BatchTask& task, int index)
{
    QFrame* card = new QFrame();
    card->setObjectName("card");
    card->setStyleSheet(QString(R"(
        QFrame#card {
            background-color: #ffffff;
            border: 2px solid #e2e8f0;
            border-radius: 12px;
        }
        QFrame#card:hover {
            border-color: %1;
        }
    )").arg(task.color));
    card->setCursor(Qt::PointingHandCursor);
    card->setProperty("taskIndex", index);

    QHBoxLayout* layout = new QHBoxLayout(card);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(16);

    QLabel* iconLabel = new QLabel(task.icon);
    iconLabel->setStyleSheet("font-size: 36px;");
    iconLabel->setFixedWidth(50);
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);

    QVBoxLayout* textLayout = new QVBoxLayout();
    textLayout->setSpacing(6);

    QLabel* nameLabel = new QLabel(task.name);
    nameLabel->setStyleSheet("font-size: 16px; font-weight: 600; color: #0f172a;");
    textLayout->addWidget(nameLabel);

    QLabel* descLabel = new QLabel(task.description);
    descLabel->setStyleSheet("font-size: 13px; color: #64748b;");
    descLabel->setWordWrap(true);
    textLayout->addWidget(descLabel);

    QStringList steps;
    for (const QString& cmdId : task.commandIds) {
        auto cmd = CommandMetadataManager::instance()->getCommand(cmdId);
        if (!cmd.friendlyName.isEmpty()) {
            steps.append(cmd.friendlyName);
        }
    }
    QLabel* stepsLabel = new QLabel(tr("包含: ") + steps.join(" → "));
    stepsLabel->setStyleSheet("font-size: 12px; color: #94a3b8;");
    textLayout->addWidget(stepsLabel);

    layout->addLayout(textLayout, 1);

    QPushButton* startBtn = new QPushButton(tr("▶ 执行"));
    startBtn->setFixedHeight(38);
    startBtn->setFixedWidth(100);
    startBtn->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: white;
            border: none;
            border-radius: 8px;
            font-weight: 500;
        }
        QPushButton:hover {
            opacity: 0.9;
        }
    )").arg(task.color));
    startBtn->setCursor(Qt::PointingHandCursor);
    connect(startBtn, &QPushButton::clicked, [this, index]() {
        m_selectedPresetIndex = index;
        const auto& task = m_tasks[index];

        int total = task.commandIds.size();
        m_overallProgress->setRange(0, total);
        m_overallProgress->setValue(0);
        m_progressCountLabel->setText(QString("0 / %1").arg(total));
        m_currentTaskLabel->setText(tr("准备执行..."));

        for (int i = 0; i < total; i++) {
            QTimer::singleShot((i + 1) * 800, [this, i, total, task]() {
                m_overallProgress->setValue(i + 1);
                m_progressCountLabel->setText(QString("%1 / %2").arg(i + 1).arg(total));
                if (i < total - 1) {
                    auto cmd = CommandMetadataManager::instance()->getCommand(task.commandIds[i + 1]);
                    m_currentTaskLabel->setText(tr("正在执行: ") + cmd.friendlyName);
                } else {
                    m_currentTaskLabel->setText(tr("✅ 全部执行完成！"));
                }
            });
        }

        emit batchStarted(task.commandIds);
    });
    layout->addWidget(startBtn);

    return card;
}

QFrame* BatchTaskWidget::createProgressSection()
{
    QFrame* frame = new QFrame();
    frame->setObjectName("card");
    QVBoxLayout* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(10);

    QHBoxLayout* titleRow = new QHBoxLayout();
    QLabel* titleLabel = new QLabel(tr("📊 执行进度"));
    titleLabel->setStyleSheet("font-size: 14px; font-weight: 600; color: #0f172a;");
    titleRow->addWidget(titleLabel);
    titleRow->addStretch(1);
    m_progressCountLabel = new QLabel("0 / 0");
    m_progressCountLabel->setStyleSheet("font-size: 12px; color: #64748b;");
    titleRow->addWidget(m_progressCountLabel);
    layout->addLayout(titleRow);

    m_overallProgress = new QProgressBar();
    m_overallProgress->setRange(0, 100);
    m_overallProgress->setValue(0);
    m_overallProgress->setFixedHeight(10);
    m_overallProgress->setTextVisible(false);
    layout->addWidget(m_overallProgress);

    m_currentTaskLabel = new QLabel(tr("等待执行..."));
    m_currentTaskLabel->setStyleSheet("font-size: 13px; color: #475569;");
    layout->addWidget(m_currentTaskLabel);

    return frame;
}

void BatchTaskWidget::onPresetTaskClicked(int index)
{
    Q_UNUSED(index);
}

void BatchTaskWidget::onAddCommandClicked()
{
    auto selectedItems = m_availableList->selectedItems();
    for (auto item : selectedItems) {
        QListWidgetItem* newItem = new QListWidgetItem(item->text());
        newItem->setData(Qt::UserRole, item->data(Qt::UserRole));
        m_selectedList->addItem(newItem);
    }
}

void BatchTaskWidget::onRemoveCommandClicked()
{
    auto selectedItems = m_selectedList->selectedItems();
    for (auto item : selectedItems) {
        delete m_selectedList->takeItem(m_selectedList->row(item));
    }
}

void BatchTaskWidget::onMoveUpClicked()
{
    int currentRow = m_selectedList->currentRow();
    if (currentRow > 0) {
        QListWidgetItem* item = m_selectedList->takeItem(currentRow);
        m_selectedList->insertItem(currentRow - 1, item);
        m_selectedList->setCurrentRow(currentRow - 1);
    }
}

void BatchTaskWidget::onMoveDownClicked()
{
    int currentRow = m_selectedList->currentRow();
    if (currentRow < m_selectedList->count() - 1) {
        QListWidgetItem* item = m_selectedList->takeItem(currentRow);
        m_selectedList->insertItem(currentRow + 1, item);
        m_selectedList->setCurrentRow(currentRow + 1);
    }
}

void BatchTaskWidget::onExecuteCustomClicked()
{
    if (m_selectedList->count() == 0) {
        QMessageBox::information(this, tr("提示"), tr("请先添加至少一个命令到已选列表。"));
        return;
    }

    QStringList commandIds;
    for (int i = 0; i < m_selectedList->count(); i++) {
        commandIds.append(m_selectedList->item(i)->data(Qt::UserRole).toString());
    }

    int total = commandIds.size();
    m_overallProgress->setRange(0, total);
    m_overallProgress->setValue(0);
    m_progressCountLabel->setText(QString("0 / %1").arg(total));
    m_currentTaskLabel->setText(tr("准备执行..."));

    for (int i = 0; i < total; i++) {
        QTimer::singleShot((i + 1) * 800, [this, i, total, commandIds]() {
            m_overallProgress->setValue(i + 1);
            m_progressCountLabel->setText(QString("%1 / %2").arg(i + 1).arg(total));
            if (i < total - 1) {
                auto cmd = CommandMetadataManager::instance()->getCommand(commandIds[i + 1]);
                m_currentTaskLabel->setText(tr("正在执行: ") + cmd.friendlyName);
            } else {
                m_currentTaskLabel->setText(tr("✅ 全部执行完成！"));
            }
        });
    }

    emit batchStarted(commandIds);
}

void BatchTaskWidget::onSavePresetClicked()
{
    if (m_selectedList->count() == 0) {
        QMessageBox::information(this, tr("提示"), tr("请先添加至少一个命令。"));
        return;
    }
    QMessageBox::information(this, tr("保存预设"), tr("保存预设功能开发中..."));
}

void BatchTaskWidget::onAvailableItemDoubleClicked(QListWidgetItem* item)
{
    QListWidgetItem* newItem = new QListWidgetItem(item->text());
    newItem->setData(Qt::UserRole, item->data(Qt::UserRole));
    m_selectedList->addItem(newItem);
}

void BatchTaskWidget::onSelectedItemDoubleClicked(QListWidgetItem* item)
{
    delete m_selectedList->takeItem(m_selectedList->row(item));
}
