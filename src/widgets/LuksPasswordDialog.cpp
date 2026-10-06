#include "LuksPasswordDialog.h"

LuksPasswordDialog::LuksPasswordDialog(QWidget* parent)
    : QDialog(parent)
    , m_passwordVisible(false)
{
    setWindowTitle(tr("解锁加密磁盘"));
    setFixedSize(480, 380);
    setModal(true);
    setupUI();
}

void LuksPasswordDialog::setPartitionInfo(const QString& deviceName, const QString& size)
{
    m_partitionInfoLabel->setText(QString("%1 · %2").arg(deviceName, size));
}

void LuksPasswordDialog::showError(const QString& message)
{
    m_errorLabel->setText(message);
    m_errorLabel->setVisible(true);
    m_passwordEdit->setStyleSheet(R"(
        QLineEdit {
            background-color: #3a3a3e;
            border: 1px solid #ff453a;
            border-radius: 12px;
            padding: 12px 44px 12px 16px;
            color: #ffffff;
            font-size: 14px;
        }
        QLineEdit:focus {
            border: 1px solid #ff453a;
            outline: none;
        }
    )");
}

void LuksPasswordDialog::setupUI()
{
    setStyleSheet(R"(
        QDialog {
            background-color: #2a2a2e;
        }
        QFrame#luksCard {
            background-color: #3a3a3e;
            border-radius: 20px;
        }
        QLabel#luksTitle {
            font-size: 22px;
            font-weight: 600;
            color: #ffffff;
        }
        QLabel#luksDesc {
            font-size: 13px;
            color: #b0b0b5;
            line-height: 1.5;
        }
        QLabel#partitionInfo {
            font-size: 12px;
            color: #9a9aa0;
            padding: 8px 12px;
            background-color: #4a4a4e;
            border-radius: 8px;
        }
        QLineEdit {
            background-color: #3a3a3e;
            border: 1px solid #5a5a5e;
            border-radius: 12px;
            padding: 12px 44px 12px 16px;
            color: #ffffff;
            font-size: 14px;
        }
        QLineEdit:focus {
            border: 1px solid #0a84ff;
            outline: none;
        }
        QPushButton#togglePwdBtn {
            background-color: transparent;
            color: #9a9aa0;
            border: none;
            padding: 4px 8px;
            font-size: 14px;
        }
        QPushButton#togglePwdBtn:hover {
            color: #ffffff;
        }
        QLabel#luksError {
            font-size: 12px;
            color: #ff453a;
        }
        QPushButton#unlockBtn {
            background-color: #0a84ff;
            color: #ffffff;
            border: none;
            border-radius: 12px;
            padding: 12px 28px;
            font-size: 14px;
            font-weight: 500;
        }
        QPushButton#unlockBtn:hover {
            background-color: #1a94ff;
        }
        QPushButton#unlockBtn:pressed {
            background-color: #0a74ef;
        }
        QPushButton#cancelLuksBtn {
            background-color: #4a4a4e;
            color: #ffffff;
            border: none;
            border-radius: 12px;
            padding: 12px 28px;
            font-size: 14px;
            font-weight: 500;
        }
        QPushButton#cancelLuksBtn:hover {
            background-color: #5a5a5e;
        }
        QPushButton#cancelLuksBtn:pressed {
            background-color: #3a3a3e;
        }
    )");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_cardFrame = new QFrame();
    m_cardFrame->setObjectName("luksCard");
    m_cardFrame->setFixedWidth(440);
    QVBoxLayout* cardLayout = new QVBoxLayout(m_cardFrame);
    cardLayout->setContentsMargins(32, 28, 32, 28);
    cardLayout->setSpacing(16);

    m_titleLabel = new QLabel(tr("解锁加密磁盘"));
    m_titleLabel->setObjectName("luksTitle");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(m_titleLabel);

    m_descLabel = new QLabel(tr("检测到加密分区，请输入磁盘加密密码以继续（只读模式挂载）"));
    m_descLabel->setObjectName("luksDesc");
    m_descLabel->setWordWrap(true);
    m_descLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(m_descLabel);

    m_partitionInfoLabel = new QLabel(tr("未指定分区"));
    m_partitionInfoLabel->setObjectName("partitionInfo");
    m_partitionInfoLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(m_partitionInfoLabel);

    cardLayout->addSpacing(8);

    QHBoxLayout* pwdLayout = new QHBoxLayout();
    pwdLayout->setContentsMargins(0, 0, 0, 0);
    pwdLayout->setSpacing(0);

    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(tr("请输入加密密码"));
    m_passwordEdit->setFixedHeight(48);
    pwdLayout->addWidget(m_passwordEdit, 1);

    m_toggleBtn = new QPushButton("👁");
    m_toggleBtn->setObjectName("togglePwdBtn");
    m_toggleBtn->setFixedSize(40, 48);
    m_toggleBtn->setCursor(Qt::PointingHandCursor);
    connect(m_toggleBtn, &QPushButton::clicked, this, &LuksPasswordDialog::onTogglePasswordVisibility);
    pwdLayout->addWidget(m_toggleBtn);

    cardLayout->addLayout(pwdLayout);

    m_errorLabel = new QLabel("");
    m_errorLabel->setObjectName("luksError");
    m_errorLabel->setVisible(false);
    m_errorLabel->setWordWrap(true);
    cardLayout->addWidget(m_errorLabel);

    cardLayout->addStretch(1);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);

    m_cancelBtn = new QPushButton(tr("取消"));
    m_cancelBtn->setObjectName("cancelLuksBtn");
    m_cancelBtn->setFixedHeight(44);
    connect(m_cancelBtn, &QPushButton::clicked, this, &LuksPasswordDialog::onCancelClicked);
    btnLayout->addWidget(m_cancelBtn, 1);

    m_unlockBtn = new QPushButton(tr("解锁"));
    m_unlockBtn->setObjectName("unlockBtn");
    m_unlockBtn->setFixedHeight(44);
    m_unlockBtn->setDefault(true);
    connect(m_unlockBtn, &QPushButton::clicked, this, &LuksPasswordDialog::onUnlockClicked);
    btnLayout->addWidget(m_unlockBtn, 1);

    cardLayout->addLayout(btnLayout);

    mainLayout->addWidget(m_cardFrame, 0, Qt::AlignCenter);
}

void LuksPasswordDialog::onUnlockClicked()
{
    QString password = m_passwordEdit->text();
    if (password.isEmpty()) {
        showError(tr("请输入密码"));
        return;
    }
    emit passwordEntered(password);
}

void LuksPasswordDialog::onCancelClicked()
{
    reject();
}

void LuksPasswordDialog::onTogglePasswordVisibility()
{
    m_passwordVisible = !m_passwordVisible;
    if (m_passwordVisible) {
        m_passwordEdit->setEchoMode(QLineEdit::Normal);
        m_toggleBtn->setText("🙈");
    } else {
        m_passwordEdit->setEchoMode(QLineEdit::Password);
        m_toggleBtn->setText("👁");
    }
}
