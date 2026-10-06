#ifndef TOPBARWIDGET_H
#define TOPBARWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QHBoxLayout>
#include <QFrame>
#include <QPushButton>

class TopBarWidget : public QFrame
{
    Q_OBJECT

public:
    explicit TopBarWidget(QWidget* parent = nullptr);
    void updateSystemInfo();
    void updateThemeButton(bool dark);
    void refreshInfo();

signals:
    void searchTextChanged(const QString& text);
    void searchSubmitted(const QString& text);
    void themeToggled();

private slots:
    void onSearchChanged();
    void onThemeClicked();

private:
    void setupUI();

    QLineEdit* m_searchEdit;
    QLabel* m_distroBadge;
    QLabel* m_desktopBadge;
    QLabel* m_kernelBadge;
    QLabel* m_statusBadge;
    QPushButton* m_themeBtn;
    QPushButton* m_notifBtn;
    QHBoxLayout* m_mainLayout;
};

#endif
