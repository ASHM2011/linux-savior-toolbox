#include "EmergencyRepairWidget.h"
#include "widgets/CommandDetailDialog.h"
#include "core/SystemDetector.h"
#include "core/CommandMetadata.h"
#include <QMessageBox>
#include <QFile>
#include <QScrollArea>

EmergencyRepairWidget::EmergencyRepairWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void EmergencyRepairWidget::setupUI()
{
    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget* contentWidget = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setContentsMargins(32, 24, 32, 24);
    mainLayout->setSpacing(24);

    QLabel* title = new QLabel(tr("🚨 紧急修复专区"));
    title->setStyleSheet("font-size: 24px; font-weight: 700; color: #0f172a;");
    title->setMinimumHeight(32);
    mainLayout->addWidget(title);

    QLabel* subtitle = new QLabel(tr("系统出问题了？先点这里一键修复，90%的问题都能解决"));
    subtitle->setStyleSheet("font-size: 14px; color: #64748b;");
    subtitle->setMinimumHeight(22);
    mainLayout->addWidget(subtitle);

    QFrame* warningFrame = new QFrame();
    warningFrame->setStyleSheet(R"(
        QFrame {
            background-color: #fef2f2;
            border: 1px solid #fecaca;
            border-radius: 12px;
            padding: 14px 20px;
        }
    )");
    warningFrame->setMinimumHeight(56);
    QHBoxLayout* warnLayout = new QHBoxLayout(warningFrame);
    warnLayout->setContentsMargins(0, 0, 0, 0);
    warnLayout->setSpacing(12);

    QLabel* warnIcon = new QLabel("⚠️");
    warnIcon->setStyleSheet("font-size: 22px;");
    warnIcon->setFixedWidth(36);
    warnLayout->addWidget(warnIcon);

    QLabel* warnText = new QLabel(tr("所有修复操作均经过安全验证，不会破坏你的系统。放心使用！"));
    warnText->setStyleSheet("font-size: 13px; color: #b91c1c;");
    warnText->setWordWrap(true);
    warnText->setMinimumHeight(20);
    warnLayout->addWidget(warnText, 1);

    mainLayout->addWidget(warningFrame);

    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setSpacing(20);

    auto sysDet = SystemDetector::instance();
    QString unlockCmd = sysDet->getUnlockCommandId();
    QString desktopRestartCmd = sysDet->isGNOME() ? "gnome_shell_restart" :
                               sysDet->isKDE() ? "plasma_restart" : "xfce4_restart";

    bool pkgLocked = isPackageManagerLocked();

    gridLayout->addWidget(createRepairCard(
        "🔑", tr("修复sudo后遗症"),
        tr("用sudo打开图形程序后，桌面图标打不开了？点这个修复用户目录权限。"),
        "chown_home", false
    ), 0, 0);

    gridLayout->addWidget(createRepairCard(
        "🗑️", tr("修复回收站权限"),
        tr("回收站无法清空、文件删不掉？一键修复回收站目录权限。"),
        "fix_trash_permission", false
    ), 0, 1);

    gridLayout->addWidget(createRepairCard(
        "🔓", tr("修复包管理器锁死"),
        tr("软件中心卡住、提示\"无法获得锁\"？强制解锁包管理器。"),
        unlockCmd, pkgLocked
    ), 1, 0);

    gridLayout->addWidget(createRepairCard(
        "🖥️", tr("桌面崩溃恢复"),
        tr("桌面卡住了、图标消失了？不用重启电脑，一键重启桌面。"),
        desktopRestartCmd, false
    ), 1, 1);

    mainLayout->addLayout(gridLayout);

    QLabel* bottomTip = new QLabel(tr("✨ 所有修复功能均经过安全验证，不会破坏你的系统"));
    bottomTip->setStyleSheet("font-size: 12px; color: #94a3b8;");
    bottomTip->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(bottomTip);

    mainLayout->addStretch(1);

    scrollArea->setWidget(contentWidget);
    outerLayout->addWidget(scrollArea);
}

QFrame* EmergencyRepairWidget::createRepairCard(const QString& icon,
    const QString& title, const QString& desc, const QString& commandId, bool highlighted)
{
    QFrame* card = new QFrame();
    card->setObjectName("card");
    card->setMinimumHeight(200);

    QString cardStyle = R"(
        QFrame#card {
            background-color: #ffffff;
            border: 1px solid #e2e8f0;
            border-radius: 14px;
        }
        QFrame#card:hover {
            border-color: #cbd5e1;
        }
    )";

    if (highlighted) {
        cardStyle = R"(
            QFrame#card {
                background-color: #fffbeb;
                border: 2px solid #f59e0b;
                border-radius: 14px;
            }
            QFrame#card:hover {
                border-color: #d97706;
            }
        )";
    }

    card->setStyleSheet(cardStyle);

    QVBoxLayout* layout = new QVBoxLayout(card);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    QHBoxLayout* iconRow = new QHBoxLayout();
    iconRow->setSpacing(10);

    QLabel* iconLabel = new QLabel(icon);
    iconLabel->setStyleSheet("font-size: 40px;");
    iconLabel->setMinimumHeight(44);
    iconRow->addWidget(iconLabel);

    if (highlighted) {
        QLabel* highlightBadge = new QLabel(tr("⚠️ 检测到异常"));
        highlightBadge->setStyleSheet(R"(
            background-color: #fef3c7;
            color: #d97706;
            padding: 6px 12px;
            border-radius: 999px;
            font-size: 12px;
            font-weight: 600;
        )");
        highlightBadge->setMinimumHeight(24);
        iconRow->addWidget(highlightBadge);
    }

    iconRow->addStretch(1);
    layout->addLayout(iconRow);

    QLabel* titleLabel = new QLabel(title);
    titleLabel->setStyleSheet("font-size: 17px; font-weight: 600; color: #0f172a;");
    titleLabel->setMinimumHeight(26);
    titleLabel->setWordWrap(true);
    layout->addWidget(titleLabel);

    QLabel* descLabel = new QLabel(desc);
    descLabel->setStyleSheet("font-size: 13px; color: #64748b; line-height: 1.6;");
    descLabel->setWordWrap(true);
    descLabel->setMinimumHeight(40);
    layout->addWidget(descLabel);

    layout->addStretch(1);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);

    QPushButton* infoBtn = new QPushButton("❓");
    infoBtn->setFixedSize(44, 44);
    infoBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #f1f5f9;
            color: #64748b;
            border: none;
            border-radius: 10px;
            font-size: 16px;
        }
        QPushButton:hover {
            background-color: #e2e8f0;
            color: #334155;
        }
    )");
    infoBtn->setToolTip(tr("查看详细说明"));
    connect(infoBtn, &QPushButton::clicked, [this, commandId]() {
        onInfoClicked(commandId);
    });
    btnLayout->addWidget(infoBtn);

    QPushButton* actionBtn = new QPushButton(tr("🛠️ 一键修复"));
    actionBtn->setMinimumHeight(46);
    actionBtn->setCursor(Qt::PointingHandCursor);
    actionBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #3b82f6;
            color: white;
            border: none;
            border-radius: 10px;
            padding: 0 20px;
            font-weight: 500;
            font-size: 14px;
        }
        QPushButton:hover {
            background-color: #2563eb;
        }
        QPushButton:pressed {
            background-color: #1d4ed8;
        }
    )");
    connect(actionBtn, &QPushButton::clicked, [this, commandId, title, desc]() {
        if (confirmRepair(title, desc)) {
            onRepairClicked(commandId);
        }
    });
    btnLayout->addWidget(actionBtn, 1);

    layout->addLayout(btnLayout);

    return card;
}

bool EmergencyRepairWidget::isPackageManagerLocked() const
{
    auto sysDet = SystemDetector::instance();

    if (sysDet->isUbuntu() || sysDet->distroName() == "Debian") {
        return QFile::exists("/var/lib/dpkg/lock") ||
               QFile::exists("/var/lib/dpkg/lock-frontend") ||
               QFile::exists("/var/lib/apt/lists/lock");
    } else if (sysDet->isFedora()) {
        return QFile::exists("/var/cache/dnf/packages.lock.sqlite");
    } else if (sysDet->isOpenSUSE()) {
        return QFile::exists("/var/lib/rpm/.rpm.lock");
    }

    return false;
}

bool EmergencyRepairWidget::confirmRepair(const QString& title, const QString& desc) const
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("确认修复"));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setText(QString(tr("确定要执行 \"%1\" 吗？")).arg(title));
    msgBox.setInformativeText(desc);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    QPushButton* yesBtn = qobject_cast<QPushButton*>(msgBox.button(QMessageBox::Yes));
    if (yesBtn) {
        yesBtn->setText(tr("✅ 确认修复"));
    }
    QPushButton* noBtn = qobject_cast<QPushButton*>(msgBox.button(QMessageBox::No));
    if (noBtn) {
        noBtn->setText(tr("取消"));
    }

    return msgBox.exec() == QMessageBox::Yes;
}

void EmergencyRepairWidget::onRepairClicked(const QString& commandId)
{
    emit commandTriggered(commandId);
}

void EmergencyRepairWidget::onInfoClicked(const QString& commandId)
{
    CommandDetailDialog dlg(commandId, this);
    connect(&dlg, &CommandDetailDialog::executeRequested, this, &EmergencyRepairWidget::commandTriggered);
    connect(&dlg, &CommandDetailDialog::executeInTerminalRequested, this, &EmergencyRepairWidget::executeInTerminal);
    dlg.exec();
}
