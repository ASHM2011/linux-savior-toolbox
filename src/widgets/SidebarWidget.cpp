#include "SidebarWidget.h"
#include "core/SystemDetector.h"
#include <QCoreApplication>
#include <QListWidgetItem>
#include <QFont>
#include <QIcon>
#include <QTimer>

SidebarWidget::SidebarWidget(QWidget* parent)
    : QFrame(parent)
{
    setObjectName("sidebar");
    setFixedWidth(240);
    setupUI();
    loadNavItems();
    filterByDesktop();
}

void SidebarWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 20, 0, 20);
    m_mainLayout->setSpacing(8);

    QWidget* header = new QWidget();
    QVBoxLayout* headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(24, 0, 20, 12);
    headerLayout->setSpacing(4);

    m_appNameLabel = new QLabel(tr("🛠️ 萌新工具箱"));
    m_appNameLabel->setStyleSheet("font-size: 18px; font-weight: 700; color: #0f172a;");
    headerLayout->addWidget(m_appNameLabel);

    m_versionLabel = new QLabel("v" + QCoreApplication::applicationVersion());
    m_versionLabel->setStyleSheet("font-size: 11px; color: #94a3b8; font-weight: 500;");
    headerLayout->addWidget(m_versionLabel);

    m_mainLayout->addWidget(header);

    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color: #e2e8f0; background-color: #e2e8f0; max-height: 1px; margin: 0 16px;");
    m_mainLayout->addWidget(line);

    m_navList = new QListWidget();
    m_navList->setObjectName("navList");
    m_navList->setFrameShape(QFrame::NoFrame);
    m_navList->setSpacing(2);
    m_navList->setAlternatingRowColors(false);
    m_navList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_navList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_navList->setStyleSheet(R"(
        QListWidget {
            background-color: transparent;
            border: none;
            padding: 8px 12px;
            outline: none;
        }
        QListWidget::item {
            padding: 12px 14px;
            border-radius: 12px;
            margin: 3px 0;
            color: #64748b;
            font-size: 13px;
            font-weight: 500;
        }
        QListWidget::item:hover {
            background-color: #f1f5f9;
            color: #334155;
        }
        QListWidget::item:selected {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3b82f6, stop:1 #2563eb);
            color: #ffffff;
            font-weight: 600;
        }
    )");

    m_mainLayout->addWidget(m_navList, 1);

    connect(m_navList, &QListWidget::itemClicked, this, &SidebarWidget::onItemClicked);
}

void SidebarWidget::loadNavItems()
{
    m_allItems = {
        {"dashboard", "🏠", tr("首页仪表盘"), tr("核心功能"), true, true, true},
        {"emergency", "🚨", tr("紧急修复"), tr("核心功能"), true, true, true},
        {"systemInfo", "🖥️", tr("系统信息"), tr("核心功能"), true, true, true},
        {"terminal", "💻", tr("终端模拟器"), tr("核心功能"), true, true, true},
        {"cleanup", "🧹", tr("系统清理"), tr("核心功能"), true, true, true},
        {"startup", "🚀", tr("启动项管理"), tr("核心功能"), true, true, true},
        {"batch", "📋", tr("批量任务"), tr("核心功能"), true, true, true},
        {"extensions", "🧩", tr("GNOME扩展"), tr("桌面专属"), true, false, false},
        {"backup", "💾", tr("备份恢复"), tr("实用工具"), true, true, true},
        {"diagnostic", "🩺", tr("系统诊断"), tr("实用工具"), true, true, true},
        {"logs", "📜", tr("日志查看器"), tr("实用工具"), true, true, true},
        {"alias", "⌨️", tr("命令别名"), tr("实用工具"), true, true, true},
        {"reference", "📖", tr("命令速查"), tr("实用工具"), true, true, true},
        {"dualboot", "🪟", tr("双系统工具"), tr("实用工具"), true, true, true},
        {"services", "🔧", tr("服务管理"), tr("系统监控"), true, true, true},
        {"processes", "📊", tr("进程管理"), tr("系统监控"), true, true, true},
        {"network", "🌐", tr("网络诊断"), tr("系统监控"), true, true, true},
        {"settings", "⚙️", tr("设置"), tr("其他"), true, true, true}
    };
}

void SidebarWidget::refreshNavItems()
{
    QString currentPage;
    QListWidgetItem* current = m_navList->currentItem();
    if (current) {
        currentPage = current->data(Qt::UserRole).toString();
    }
    
    filterByDesktop();
    
    if (!currentPage.isEmpty()) {
        setCurrentPage(currentPage);
    }
}

void SidebarWidget::filterByDesktop()
{
    m_navList->clear();
    auto sysDetector = SystemDetector::instance();
    
    bool isGnome = sysDetector->isGNOME();
    bool isKde = sysDetector->isKDE();
    bool isXfce = sysDetector->isXFCE();
    bool detected = sysDetector->isDetected();

    QString currentCategory;
    for (const auto& item : m_allItems) {
        bool show = true;
        
        if (detected) {
            show = false;
            if (isGnome && item.showForGNOME) show = true;
            if (isKde && item.showForKDE) show = true;
            if (isXfce && item.showForXFCE) show = true;
        }

        if (!show) continue;

        if (item.category != currentCategory) {
            currentCategory = item.category;
            QListWidgetItem* catItem = new QListWidgetItem("  " + currentCategory);
            catItem->setFlags(Qt::NoItemFlags);
            catItem->setForeground(QColor("#94a3b8"));
            QFont font = catItem->font();
            font.setPointSize(10);
            font.setBold(true);
            catItem->setFont(font);
            catItem->setData(Qt::UserRole, QString());
            m_navList->addItem(catItem);
        }

        QListWidgetItem* navItem = new QListWidgetItem("  " + item.icon + "  " + item.label);
        navItem->setData(Qt::UserRole, item.id);
        m_navList->addItem(navItem);
    }

    if (m_navList->count() > 0) {
        for (int i = 0; i < m_navList->count(); i++) {
            QListWidgetItem* it = m_navList->item(i);
            if (it->flags() & Qt::ItemIsSelectable) {
                m_navList->setCurrentRow(i);
                break;
            }
        }
    }
}

void SidebarWidget::setCurrentPage(const QString& pageId)
{
    for (int i = 0; i < m_navList->count(); i++) {
        QListWidgetItem* it = m_navList->item(i);
        if (it->data(Qt::UserRole).toString() == pageId) {
            m_navList->setCurrentRow(i);
            break;
        }
    }
}

void SidebarWidget::onItemClicked(QListWidgetItem* item)
{
    if (!(item->flags() & Qt::ItemIsSelectable)) return;

    QString pageId = item->data(Qt::UserRole).toString();
    emit pageChanged(pageId);
}
