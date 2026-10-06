#ifndef EMERGENCYREPAIRWIDGET_H
#define EMERGENCYREPAIRWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>

class EmergencyRepairWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EmergencyRepairWidget(QWidget* parent = nullptr);

signals:
    void commandTriggered(const QString& commandId);
    void showCommandDetail(const QString& commandId);
    void executeInTerminal(const QString& commandId);

private slots:
    void onRepairClicked(const QString& commandId);
    void onInfoClicked(const QString& commandId);

private:
    void setupUI();
    QFrame* createRepairCard(const QString& icon, const QString& title,
                             const QString& desc, const QString& commandId,
                             bool highlighted = false);
    bool isPackageManagerLocked() const;
    bool confirmRepair(const QString& title, const QString& desc) const;
};

#endif
