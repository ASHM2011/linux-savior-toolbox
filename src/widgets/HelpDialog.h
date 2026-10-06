#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>

class HelpDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HelpDialog(QWidget* parent = nullptr);

private slots:
    void onCloseClicked();

private:
    void setupUI();

    QLabel* m_titleLabel;
    QLabel* m_contentLabel1;
    QLabel* m_contentLabel2;
    QLabel* m_contentLabel3;
    QLabel* m_contentLabel4;
    QPushButton* m_closeBtn;
    QFrame* m_cardFrame;
};

#endif
