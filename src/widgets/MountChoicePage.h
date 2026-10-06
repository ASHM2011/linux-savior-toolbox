#ifndef MOUNTCHOICEPAGE_H
#define MOUNTCHOICEPAGE_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>

class HelpDialog;

class MountChoicePage : public QWidget
{
    Q_OBJECT

public:
    explicit MountChoicePage(QWidget* parent = nullptr);

    void setDetectionSummary(int diskCount, int partitionCount, int encryptedCount);

signals:
    void mountRequested();
    void skipMountRequested();

private slots:
    void onHelpClicked();
    void onMountClicked();
    void onSkipClicked();

private:
    void setupUI();

    QFrame* m_cardFrame;
    QLabel* m_titleLabel;
    QLabel* m_subtitleLabel;
    QLabel* m_summaryLabel;
    QPushButton* m_mountBtn;
    QPushButton* m_skipBtn;
    QPushButton* m_helpBtn;

    HelpDialog* m_helpDialog;

    int m_diskCount;
    int m_partitionCount;
    int m_encryptedCount;
};

#endif
