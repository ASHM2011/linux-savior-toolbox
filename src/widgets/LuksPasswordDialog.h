#ifndef LUKSPASSWORDDIALOG_H
#define LUKSPASSWORDDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>

class LuksPasswordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LuksPasswordDialog(QWidget* parent = nullptr);

    void setPartitionInfo(const QString& deviceName, const QString& size);
    void showError(const QString& message);

signals:
    void passwordEntered(const QString& password);

private slots:
    void onUnlockClicked();
    void onCancelClicked();
    void onTogglePasswordVisibility();

private:
    void setupUI();

    QFrame* m_cardFrame;
    QLabel* m_titleLabel;
    QLabel* m_descLabel;
    QLabel* m_partitionInfoLabel;
    QLineEdit* m_passwordEdit;
    QPushButton* m_toggleBtn;
    QLabel* m_errorLabel;
    QPushButton* m_unlockBtn;
    QPushButton* m_cancelBtn;

    bool m_passwordVisible;
};

#endif
