#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QScrollArea>
#include <QComboBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QLineEdit>
#include <QSlider>

class SettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget* parent = nullptr);

signals:
    void navigateTo(const QString& pageId);

private slots:
    void onThemeChanged(int id);
    void onViewLogsClicked();
    void onCheckUpdateClicked();
    void onZoomSliderChanged(int value);
    void onLanguageChanged(int index);

private:
    void setupUI();
    QFrame* createSection(const QString& title, const QString& icon);
    QFrame* createToggleRow(const QString& label, const QString& desc, bool enabled);
    QFrame* createComboRow(const QString& label, const QString& desc, const QStringList& options, int currentIndex);
    QFrame* createShortcutRow(const QString& label, const QString& shortcut);
    QFrame* createZoomRow(const QString& label, const QString& desc);

    QComboBox* m_monitorIntervalCombo;
    QComboBox* m_languageCombo;
    QButtonGroup* m_themeGroup;
    QRadioButton* m_themeSystem;
    QRadioButton* m_themeLight;
    QRadioButton* m_themeDark;
    QSlider* m_zoomSlider;
    QLabel* m_zoomValueLabel;
};

#endif
