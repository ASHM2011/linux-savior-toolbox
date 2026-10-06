#include "ProgressDialog.h"

ProgressDialog::ProgressDialog(const QString& title, QWidget* parent)
    : QDialog(parent)
    , m_finished(false)
    , m_outputExpanded(false)
{
    setWindowTitle(title);
    setFixedWidth(520);
    setupUI();
}

void ProgressDialog::setupUI()
{
    setStyleSheet(R"(
        QDialog {
            background-color: #ffffff;
        }
        QLabel#progressTitle {
            font-size: 18px;
            font-weight: 700;
            color: #0f172a;
        }
        QLabel#progressStatus {
            font-size: 13px;
            color: #64748b;
        }
        QProgressBar {
            background-color: #e2e8f0;
            border: none;
            border-radius: 999px;
            height: 10px;
            text-align: center;
        }
        QProgressBar::chunk {
            background-color: #3b82f6;
            border-radius: 999px;
        }
        QTextEdit {
            background-color: #0f172a;
            color: #e2e8f0;
            border: none;
            border-radius: 8px;
            padding: 12px;
            font-family: 'Fira Code', Consolas, monospace;
            font-size: 12px;
        }
    )");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    m_titleLabel = new QLabel(windowTitle());
    m_titleLabel->setObjectName("progressTitle");
    mainLayout->addWidget(m_titleLabel);

    m_statusLabel = new QLabel(tr("准备中..."));
    m_statusLabel->setObjectName("progressStatus");
    mainLayout->addWidget(m_statusLabel);

    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    mainLayout->addWidget(m_progressBar);

    m_resultWidget = new QFrame();
    m_resultWidget->setVisible(false);
    QHBoxLayout* resultLayout = new QHBoxLayout(m_resultWidget);
    resultLayout->setContentsMargins(0, 0, 0, 0);
    resultLayout->setSpacing(12);

    m_resultIcon = new QLabel();
    m_resultIcon->setStyleSheet("font-size: 36px;");
    resultLayout->addWidget(m_resultIcon);

    m_resultText = new QLabel();
    m_resultText->setWordWrap(true);
    resultLayout->addWidget(m_resultText, 1);

    mainLayout->addWidget(m_resultWidget);

    m_outputFrame = new QFrame();
    m_outputFrame->setVisible(false);
    QVBoxLayout* outputFrameLayout = new QVBoxLayout(m_outputFrame);
    outputFrameLayout->setContentsMargins(0, 0, 0, 0);
    outputFrameLayout->setSpacing(8);

    QHBoxLayout* outputToggleLayout = new QHBoxLayout();
    QLabel* outputLabel = new QLabel(tr("📋 输出详情"));
    outputLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #334155;");
    outputToggleLayout->addWidget(outputLabel);
    outputToggleLayout->addStretch(1);
    m_toggleOutputBtn = new QPushButton(tr("展开 ▼"));
    m_toggleOutputBtn->setObjectName("secondaryBtn");
    m_toggleOutputBtn->setFixedSize(80, 28);
    m_toggleOutputBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #f1f5f9;
            color: #64748b;
            border: none;
            border-radius: 6px;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: #e2e8f0;
            color: #334155;
        }
    )");
    connect(m_toggleOutputBtn, &QPushButton::clicked, this, &ProgressDialog::onToggleOutput);
    outputToggleLayout->addWidget(m_toggleOutputBtn);
    outputFrameLayout->addLayout(outputToggleLayout);

    m_outputEdit = new QTextEdit();
    m_outputEdit->setReadOnly(true);
    m_outputEdit->setFixedHeight(0);
    outputFrameLayout->addWidget(m_outputEdit, 1);

    mainLayout->addWidget(m_outputFrame, 1);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch(1);

    m_cancelBtn = new QPushButton(tr("取消"));
    m_cancelBtn->setObjectName("secondaryBtn");
    m_cancelBtn->setFixedSize(100, 38);
    connect(m_cancelBtn, &QPushButton::clicked, this, &ProgressDialog::onCancelClicked);
    btnLayout->addWidget(m_cancelBtn);

    m_closeBtn = new QPushButton(tr("关闭"));
    m_closeBtn->setVisible(false);
    m_closeBtn->setFixedSize(100, 38);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(m_closeBtn);

    mainLayout->addLayout(btnLayout);
}

void ProgressDialog::setProgress(int percent, const QString& statusText)
{
    m_progressBar->setValue(percent);
    m_statusLabel->setText(statusText);

    if (!m_outputFrame->isVisible()) {
        m_outputFrame->setVisible(true);
    }
}

void ProgressDialog::setFinished(bool success, const QString& message)
{
    m_finished = true;

    if (success) {
        m_progressBar->setStyleSheet(R"(
            QProgressBar::chunk {
                background-color: #10b981;
                border-radius: 999px;
            }
        )");
        m_resultIcon->setText("✅");
        m_resultText->setText(message);
        m_resultText->setStyleSheet("font-size: 14px; font-weight: 600; color: #059669; line-height: 1.6;");
    } else {
        m_progressBar->setStyleSheet(R"(
            QProgressBar::chunk {
                background-color: #ef4444;
                border-radius: 999px;
            }
        )");
        m_resultIcon->setText("❌");
        m_resultText->setText(message);
        m_resultText->setStyleSheet("font-size: 14px; font-weight: 600; color: #dc2626; line-height: 1.6;");
    }

    m_progressBar->setValue(100);
    m_statusLabel->setText(success ? tr("执行完成") : tr("执行失败"));
    m_resultWidget->setVisible(true);

    if (!m_outputFrame->isVisible()) {
        m_outputFrame->setVisible(true);
    }

    m_cancelBtn->setVisible(false);
    m_closeBtn->setVisible(true);

    adjustSize();
}

void ProgressDialog::appendOutput(const QString& text)
{
    if (!m_outputFrame->isVisible()) {
        m_outputFrame->setVisible(true);
    }
    m_outputEdit->append(text);
    m_outputEdit->moveCursor(QTextCursor::End);
}

void ProgressDialog::onCancelClicked()
{
    emit cancelled();
    reject();
}

void ProgressDialog::onToggleOutput()
{
    m_outputExpanded = !m_outputExpanded;
    if (m_outputExpanded) {
        m_outputEdit->setFixedHeight(180);
        m_toggleOutputBtn->setText(tr("收起 ▲"));
    } else {
        m_outputEdit->setFixedHeight(0);
        m_toggleOutputBtn->setText(tr("展开 ▼"));
    }
    adjustSize();
}
