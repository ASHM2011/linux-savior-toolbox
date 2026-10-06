#include "TopBarWidget.h"
#include "core/SystemDetector.h"

TopBarWidget::TopBarWidget(QWidget* parent)
    : QFrame(parent)
{
    setObjectName("topbar");
    setMinimumHeight(72);
    setupUI();
    updateSystemInfo();
}

void TopBarWidget::updateThemeButton(bool dark)
{
    if (m_themeBtn) {
        m_themeBtn->setText(dark ? "☀️" : "🌙");
        m_themeBtn->setToolTip(dark ? tr("切换到亮色主题") : tr("切换到暗色主题"));
    }
}

void TopBarWidget::refreshInfo()
{
    updateSystemInfo();
}

void TopBarWidget::setupUI()
{
    m_mainLayout = new QHBoxLayout(this);
    m_mainLayout->setContentsMargins(28, 14, 28, 14);
    m_mainLayout->setSpacing(14);

    m_searchEdit = new QLineEdit();
    m_searchEdit->setObjectName("searchInput");
    m_searchEdit->setPlaceholderText(tr("🔍  搜索命令、功能、问题..."));
    m_searchEdit->setMinimumWidth(380);
    m_searchEdit->setMinimumHeight(44);
    m_mainLayout->addWidget(m_searchEdit);
    m_mainLayout->addStretch(1);

    m_distroBadge = new QLabel();
    m_distroBadge->setObjectName("badge");
    m_distroBadge->setMinimumHeight(32);
    m_distroBadge->setAlignment(Qt::AlignCenter);
    m_mainLayout->addWidget(m_distroBadge);

    m_desktopBadge = new QLabel();
    m_desktopBadge->setObjectName("successBadge");
    m_desktopBadge->setMinimumHeight(32);
    m_desktopBadge->setAlignment(Qt::AlignCenter);
    m_mainLayout->addWidget(m_desktopBadge);

    m_kernelBadge = new QLabel();
    m_kernelBadge->setObjectName("purpleBadge");
    m_kernelBadge->setMinimumHeight(32);
    m_kernelBadge->setAlignment(Qt::AlignCenter);
    m_mainLayout->addWidget(m_kernelBadge);

    m_statusBadge = new QLabel(tr("🟢 运行正常"));
    m_statusBadge->setObjectName("successBadge");
    m_statusBadge->setMinimumHeight(32);
    m_statusBadge->setAlignment(Qt::AlignCenter);
    m_mainLayout->addWidget(m_statusBadge);

    m_themeBtn = new QPushButton("🌙");
    m_themeBtn->setObjectName("themeToggleBtn");
    m_themeBtn->setMinimumSize(44, 44);
    m_themeBtn->setMaximumSize(44, 44);
    m_themeBtn->setToolTip(tr("切换到暗色主题"));
    m_themeBtn->setCursor(Qt::PointingHandCursor);
    m_mainLayout->addWidget(m_themeBtn);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &TopBarWidget::onSearchChanged);
    connect(m_searchEdit, &QLineEdit::returnPressed, [this]() {
        emit searchSubmitted(m_searchEdit->text());
    });
    connect(m_themeBtn, &QPushButton::clicked, this, &TopBarWidget::onThemeClicked);
}

void TopBarWidget::updateSystemInfo()
{
    auto info = SystemDetector::instance()->getSystemInfo();
    
    if (info.detected) {
        m_distroBadge->setText(QString("📦 %1 %2").arg(info.distroName, info.distroVersion));
        m_desktopBadge->setText(QString("🖥️ %1").arg(info.desktopName));

        QString kernelShort = info.kernelVersion;
        if (kernelShort.length() > 12) {
            kernelShort = kernelShort.left(12) + "...";
        }
        m_kernelBadge->setText(QString(tr("⚙️ 内核 %1")).arg(kernelShort));
    } else {
        m_distroBadge->setText(tr("📦 检测中..."));
        m_desktopBadge->setText(tr("🖥️ 检测中..."));
        m_kernelBadge->setText("⚙️ ...");
    }
}

void TopBarWidget::onSearchChanged()
{
    emit searchTextChanged(m_searchEdit->text());
}

void TopBarWidget::onThemeClicked()
{
    emit themeToggled();
}
