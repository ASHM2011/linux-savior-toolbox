#ifndef GNOMEEXTENSIONWIDGET_H
#define GNOMEEXTENSIONWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QScrollArea>
#include <QGridLayout>
#include <QCheckBox>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>

struct GnomeExtension {
    QString id;          // 内部简称
    QString uuid;        // GNOME 扩展真实 UUID（gnome-extensions 命令使用）
    QString name;
    QString description;
    QString icon;
    int stability;
    bool installed;
    bool enabled;
    QString category;
    QString supportedVersions;
};

class GnomeExtensionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GnomeExtensionWidget(QWidget* parent = nullptr);

signals:
    void installExtension(const QString& extId);
    void removeExtension(const QString& extId);

private slots:
    void onInstallClicked(const QString& extId);
    void onToggleClicked(const QString& extId);
    void onUninstallClicked(const QString& extId);
    void onDisableAllClicked();
    void onRestartShellClicked();
    void onDetectStatusClicked();

private:
    void setupUI();
    void initExtensions();
    QFrame* createExtensionCard(const GnomeExtension& ext);
    void updateExtensionCard(const QString& extId);
    void refreshAllCards();

    // 执行 gnome-extensions 命令，返回标准输出
    QString runGnomeExtensionCmd(const QStringList& args, int timeoutMs = 5000);
    // 检测单个扩展的安装/启用状态
    void detectExtensionState(GnomeExtension& ext);
    // 获取当前 GNOME Shell 版本（如 "46"）
    QString getGnomeShellVersion();
    // 从 extensions.gnome.org 下载扩展 zip 并安装
    void downloadAndInstallExtension(const GnomeExtension& ext);
    // 显示手动安装引导（网络失败时的回退方案）
    void showManualInstallGuide(const GnomeExtension& ext);

    QVBoxLayout* m_mainContentLayout;
    QList<GnomeExtension> m_extensions;
    QList<QFrame*> m_cardList;
    QLabel* m_statusLabel;
    QNetworkAccessManager* m_networkManager;
};

#endif
