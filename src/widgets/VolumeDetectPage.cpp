#include "VolumeDetectPage.h"
#include "HelpDialog.h"

VolumeDetectPage::VolumeDetectPage(QWidget* parent)
    : QWidget(parent)
    , m_helpDialog(nullptr)
    , m_simTimer(nullptr)
    , m_simProgress(0)
    , m_simStage(0)
{
    m_stageTexts << tr("正在扫描磁盘设备")
                 << tr("正在校验文件系统")
                 << tr("检测全盘加密状态")
                 << tr("检测分区挂载异常");

    setupUI();
    startScan();
}

void VolumeDetectPage::setupUI()
{
    setStyleSheet(R"(
        VolumeDetectPage {
            background-color: #2a2a2e;
        }
        QFrame#detectCard {
            background-color: #3a3a3e;
            border-radius: 20px;
        }
        QLabel#detectTitle {
            font-size: 26px;
            font-weight: 600;
            color: #ffffff;
        }
        QLabel#detectSubtitle {
            font-size: 14px;
            color: #9a9aa0;
        }
        QProgressBar#detectProgress {
            background-color: #4a4a4e;
            border: none;
            border-radius: 999px;
            height: 10px;
            text-align: center;
        }
        QProgressBar#detectProgress::chunk {
            background-color: #0a84ff;
            border-radius: 999px;
        }
        QLabel#detectStatus {
            font-size: 13px;
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
    connect(m_helpBtn, &QPushButton::clicked, this, &VolumeDetectPage::onHelpClicked);
    topBarLayout->addWidget(m_helpBtn);
    topBarLayout->addStretch(1);

    mainLayout->addLayout(topBarLayout);

    m_cardFrame = new QFrame();
    m_cardFrame->setObjectName("detectCard");
    m_cardFrame->setFixedWidth(600);
    QVBoxLayout* cardLayout = new QVBoxLayout(m_cardFrame);
    cardLayout->setContentsMargins(48, 40, 48, 40);
    cardLayout->setSpacing(20);

    m_titleLabel = new QLabel(tr("正在检测系统磁盘与逻辑卷"));
    m_titleLabel->setObjectName("detectTitle");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(m_titleLabel);

    m_subtitleLabel = new QLabel(tr("请稍候，正在扫描您的存储设备"));
    m_subtitleLabel->setObjectName("detectSubtitle");
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(m_subtitleLabel);

    cardLayout->addSpacing(16);

    m_progressBar = new QProgressBar();
    m_progressBar->setObjectName("detectProgress");
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(10);
    cardLayout->addWidget(m_progressBar);

    m_statusLabel = new QLabel(tr("正在扫描磁盘设备"));
    m_statusLabel->setObjectName("detectStatus");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(m_statusLabel);

    cardLayout->addStretch(1);

    mainLayout->addWidget(m_cardFrame, 0, Qt::AlignCenter);
    mainLayout->addStretch(1);
}

void VolumeDetectPage::startScan()
{
    m_simProgress = 0;
    m_simStage = 0;

    m_simTimer = new QTimer(this);
    m_simTimer->setInterval(80);
    connect(m_simTimer, &QTimer::timeout, this, &VolumeDetectPage::onSimulationTick);
    m_simTimer->start();
}

void VolumeDetectPage::onSimulationTick()
{
    m_simProgress += 2;

    int newStage = 0;
    if (m_simProgress < 25) {
        newStage = 0;
    } else if (m_simProgress < 50) {
        newStage = 1;
    } else if (m_simProgress < 75) {
        newStage = 2;
    } else {
        newStage = 3;
    }

    if (newStage != m_simStage) {
        m_simStage = newStage;
    }

    onScanProgress(qMin(m_simProgress, 100), m_stageTexts[m_simStage]);

    if (m_simProgress >= 100) {
        m_simTimer->stop();
        onScanFinished();
    }
}

void VolumeDetectPage::onScanProgress(int percent, const QString& stage)
{
    m_progressBar->setValue(percent);
    m_statusLabel->setText(stage);
}

void VolumeDetectPage::onScanFinished()
{
    m_statusLabel->setText(tr("扫描完成"));
    QTimer::singleShot(1000, this, [this]() {
        emit scanCompleted();
    });
}

void VolumeDetectPage::onHelpClicked()
{
    if (!m_helpDialog) {
        m_helpDialog = new HelpDialog(this);
    }
    m_helpDialog->exec();
}
