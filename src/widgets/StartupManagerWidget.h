#ifndef STARTUPMANAGERWIDGET_H
#define STARTUPMANAGERWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>

struct StartupEntry {
    QString fileName;       // desktop 文件名（如 firefox.desktop）
    QString filePath;       // 生效的文件完整路径
    QString name;           // Name=
    QString comment;        // Comment=
    QString exec;           // Exec=
    bool enabled;           // 是否启用
    bool isUserEntry;       // 是否来自用户目录 ~/.config/autostart
};

class StartupManagerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StartupManagerWidget(QWidget* parent = nullptr);

private slots:
    void refreshEntries();
    void onSearchTextChanged(const QString& text);
    void onSelectionChanged();
    void toggleSelectedEntry();
    void openAutostartDir();

private:
    void setupUI();
    void loadEntries();
    void populateTable(const QString& filter);
    void updateButtonStates();
    void setEntryEnabled(const StartupEntry& entry, bool enabled);
    QString userAutostartDir() const;

    QLineEdit* m_searchEdit;
    QPushButton* m_refreshBtn;
    QPushButton* m_toggleBtn;
    QPushButton* m_openDirBtn;
    QTableWidget* m_table;
    QLabel* m_statusLabel;

    QList<StartupEntry> m_entries;
};

#endif
