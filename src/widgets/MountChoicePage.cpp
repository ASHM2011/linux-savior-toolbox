#include "MountChoicePage.h"
#include "HelpDialog.h"

MountChoicePage::MountChoicePage(QWidget* parent)
    : QWidget(parent)
    , m_helpDialog(nullptr)
    , m_diskCount(2)
    , m_partitionCount(5)
    , m_encryptedCount(1)
{
    setupUI();
}

void MountChoicePage::setDetectionSummary(int diskCount, int partitionCount, int encryptedCount)
{
    m_diskCount = diskCount;
    m_partitionCount = partitionCount;
    m_encryptedCount = encryptedCount;

    QString summary = QString(tr("已检测到 %1 个磁盘，%2 个分区，%3 个加密分区"))
                          .arg(m_diskCount)
                          .arg(m_partitionCount)
                          .arg(m_encryptedCount);
    m_summaryLabel->setText(summary);
}

void MountChoicePage::setupUI()
{
    setStyleSheet(R"(
        MountChoicePage {
            background-color: #2a2a2e;
        }
        QFrame#choiceCard {
            background-color: #3a3a3e;
            border-radius: 20px;
        }
        QLabel#choiceTitle {
            font-size: 26px;
            font-weight: 600;
            color: #ffffff;
        }
        QLabel#choiceSubtitle {
            font-size: 14px;
            color: #9a9aa0;
        }
        QLabel#choiceSummary {
            font-size: 13px;
            color: #b0b0b5;
            padding: 10px 16px;
            background-color: #4a4a4e;
            border-radius: 10px;
        }
        QPushButton#mountBtn {
            background-color: #0a84ff;
            color: #ffffff;
            border: none;
            border-radius: 16px;
            padding: 20px 28px;
            text-align: left;
        }
        QPushButton#mountBtn:hover {
            background-color: #1a94ff;
        }
        QPushButton#mountBtn:pressed {
            background-color: #0a74ef;
        }
        QPushButton#skipBtn {
            background-color: #4a4a4e;
            color: #ffffff;
            border: none;
            border-radius: 16px;
            padding: 20px 28px;
            text-align: left;
        }
        QPushButton#skipBtn:hover {
            background-color: #5a5a5e;
        }
        QPushButton#skipBtn:pressed {
            background-color: #3a3a3e;
        }
        QLabel#btnTitle {
            font-size: 17px;
            font-weight: 600;
            color: #ffffff;
        }
        QLabel#btnDesc {
            font-size: 12px;
            color: #b0b0b5;
        }
        QPushButton#helpBtn {
            background-color: transparent;
            color: #9a9aa0;
            border: 1px solid #4a4a4e;
            border-radius: 10px;
            padding: 8px 20px;
            font-size: 13px;
            font-weight: 500;
        }
        QPushButton#helpBtn:hover {
            background-color: #4a4a4e;
            color: #ffffff;
        }
        QPushButton#helpBtn:pressed {
            background-color: #3a3a3e;
        }
    )");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QHBoxLayout* topBarLayout = new QHBoxLayout();
    topBarLayout->setContentsMargins(24, 20, 24, 0);
    topBarLayout->setSpacing(0);

    m_helpBtn = new QPushButton(tr("帮助"));
    m_helpBtn->setObjectName("helpBtn");
    m_helpBtn->setFixedHeight(36);
    connect(m_helpBtn, &QPushButton::clicked, this, &MountChoicePage::onHelpClicked);
    topBarLayout->addWidget(m_helpBtn);
    topBarLayout->addStretch(1);

    mainLayout->addLayout(topBarLayout);

    m_cardFrame = new QFrame();
    m_cardFrame->setObjectName("choiceCard");
    m_cardFrame->setFixedWidth(640);
    QVBoxLayout* cardLayout = new QVBoxLayout(m_cardFrame);
    cardLayout->setContentsMargins(40, 36, 40, 36);
    cardLayout->setSpacing(16);

    m_titleLabel = new QLabel(tr("选择操作模式"));
    m_titleLabel->setObjectName("choiceTitle");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(m_titleLabel);

    m_subtitleLabel = new QLabel(tr("请选择您需要的操作方式"));
    m_subtitleLabel->setObjectName("choiceSubtitle");
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(m_subtitleLabel);

    cardLayout->addSpacing(8);

    m_summaryLabel = new QLabel();
    m_summaryLabel->setObjectName("choiceSummary");
    m_summaryLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(m_summaryLabel);

    setDetectionSummary(m_diskCount, m_partitionCount, m_encryptedCount);

    cardLayout->addSpacing(16);

    m_mountBtn = new QPushButton();
    m_mountBtn->setObjectName("mountBtn");
    m_mountBtn->setFixedSize(500, 100);
    QVBoxLayout* mountBtnLayout = new QVBoxLayout(m_mountBtn);
    mountBtnLayout->setContentsMargins(24, 16, 24, 16);
    mountBtnLayout->setSpacing(4);
    mountBtnLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QLabel* mountTitle = new QLabel(tr("我需要磁盘挂载"));
    mountTitle->setObjectName("btnTitle");
    mountTitle->setAlignment(Qt::AlignLeft);
    mountBtnLayout->addWidget(mountTitle);

    QLabel* mountDesc = new QLabel(tr("用于系统修复、快照还原、数据备份"));
    mountDesc->setObjectName("btnDesc");
    mountDesc->setAlignment(Qt::AlignLeft);
    mountBtnLayout->addWidget(mountDesc);

    connect(m_mountBtn, &QPushButton::clicked, this, &MountChoicePage::onMountClicked);
    cardLayout->addWidget(m_mountBtn, 0, Qt::AlignHCenter);

    m_skipBtn = new QPushButton();
    m_skipBtn->setObjectName("skipBtn");
    m_skipBtn->setFixedSize(500, 100);
    QVBoxLayout* skipBtnLayout = new QVBoxLayout(m_skipBtn);
    skipBtnLayout->setContentsMargins(24, 16, 24, 16);
    skipBtnLayout->setSpacing(4);
    skipBtnLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QLabel* skipTitle = new QLabel(tr("无需磁盘挂载，直接进入急救桌面"));
    skipTitle->setObjectName("btnTitle");
    skipTitle->setAlignment(Qt::AlignLeft);
    skipBtnLayout->addWidget(skipTitle);

    QLabel* skipDesc = new QLabel(tr("仅使用内置工具，不访问主系统数据"));
    skipDesc->setObjectName("btnDesc");
    skipDesc->setAlignment(Qt::AlignLeft);
    skipBtnLayout->addWidget(skipDesc);

    connect(m_skipBtn, &QPushButton::clicked, this, &MountChoicePage::onSkipClicked);
    cardLayout->addWidget(m_skipBtn, 0, Qt::AlignHCenter);

    cardLayout->addStretch(1);

    mainLayout->addWidget(m_cardFrame, 0, Qt::AlignCenter);
    mainLayout->addStretch(1);
}

void MountChoicePage::onHelpClicked()
{
    if (!m_helpDialog) {
        m_helpDialog = new HelpDialog(this);
    }
    m_helpDialog->exec();
}

void MountChoicePage::onMountClicked()
{
    emit mountRequested();
}

void MountChoicePage::onSkipClicked()
{
    emit skipMountRequested();
}
