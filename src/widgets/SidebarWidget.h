#ifndef SIDEBARWIDGET_H
#define SIDEBARWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QFrame>

struct NavItem {
    QString id;
    QString icon;
    QString label;
    QString category;
    bool showForGNOME;
    bool showForKDE;
    bool showForXFCE;
};

class SidebarWidget : public QFrame
{
    Q_OBJECT

public:
    explicit SidebarWidget(QWidget* parent = nullptr);
    void setCurrentPage(const QString& pageId);
    void refreshNavItems();

signals:
    void pageChanged(const QString& pageId);

private slots:
    void onItemClicked(QListWidgetItem* item);

private:
    void setupUI();
    void loadNavItems();
    void filterByDesktop();

    QListWidget* m_navList;
    QLabel* m_appNameLabel;
    QLabel* m_versionLabel;
    QVBoxLayout* m_mainLayout;
    QList<NavItem> m_allItems;
};

#endif
