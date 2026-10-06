#include "HelpDialog.h"

HelpDialog::HelpDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("关于本恢复环境"));
    setFixedSize(500, 350);
    setModal(true);
    setupUI();
}

void HelpDialog::setupUI()
{
    setStyleSheet(R"(
        QDialog {
            background-color: #2a2a2e;
        }
        QFrame#helpCard {
            background-color: #3a3a3e;
            border-radius: 18px;
        }
        QLabel#helpTitle {
            font-size: 20px;
            font-weight: 600;
            color: #ffffff;
        }
        QLabel#helpContent {
            font-size: 13px;
            color: #b0b0b5;
            line-height: 1.6;
        }
        QPushButton#helpCloseBtn {
            background-color: #4a4a4e;
            color: #ffffff;
            border: none;
            border-radius: 12px;
            padding: 10px 24px;
            font-size: 14px;
            font-weight: 500;
        }
        QPushButton#helpCloseBtn:hover {
            background-color: #5a5a5e;
        }
        QPushButton#helpCloseBtn:pressed {
            background-color: #3a3a3e;
        }
    )");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_cardFrame = new QFrame();
    m_cardFrame->setObjectName("helpCard");
    QVBoxLayout* cardLayout = new QVBoxLayout(m_cardFrame);
    cardLayout->setContentsMargins(32, 28, 32, 28);
    cardLayout->setSpacing(16);

    m_titleLabel = new QLabel(tr("关于本恢复环境"));
    m_titleLabel->setObjectName("helpTitle");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(m_titleLabel);

    cardLayout->addSpacing(8);

    m_contentLabel1 = new QLabel(tr("本环境为独立急救系统，运行在独立的恢复分区中。"));
    m_contentLabel1->setObjectName("helpContent");
    m_contentLabel1->setWordWrap(true);
    cardLayout->addWidget(m_contentLabel1);

    m_contentLabel2 = new QLabel(tr("所有操作均不会主动篡改主系统业务数据。"));
    m_contentLabel2->setObjectName("helpContent");
    m_contentLabel2->setWordWrap(true);
    cardLayout->addWidget(m_contentLabel2);

    m_contentLabel3 = new QLabel(tr("磁盘挂载仅用于故障修复和数据备份。"));
    m_contentLabel3->setObjectName("helpContent");
    m_contentLabel3->setWordWrap(true);
    cardLayout->addWidget(m_contentLabel3);

    m_contentLabel4 = new QLabel(tr("即使不知道主系统 ROOT 密码，也可以完成常规运维修复。"));
    m_contentLabel4->setObjectName("helpContent");
    m_contentLabel4->setWordWrap(true);
    cardLayout->addWidget(m_contentLabel4);

    cardLayout->addStretch(1);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch(1);

    m_closeBtn = new QPushButton(tr("关闭"));
    m_closeBtn->setObjectName("helpCloseBtn");
    m_closeBtn->setFixedHeight(40);
    connect(m_closeBtn, &QPushButton::clicked, this, &HelpDialog::onCloseClicked);
    btnLayout->addWidget(m_closeBtn);

    btnLayout->addStretch(1);
    cardLayout->addLayout(btnLayout);

    mainLayout->addWidget(m_cardFrame, 0, Qt::AlignCenter);
}

void HelpDialog::onCloseClicked()
{
    accept();
}
