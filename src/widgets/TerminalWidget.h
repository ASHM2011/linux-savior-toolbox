#ifndef TERMINALWIDGET_H
#define TERMINALWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QStringList>
#include <QTabWidget>
#include <QAction>
#include <QMenu>
#include <QProcess>
#include <QKeyEvent>

#ifdef HAVE_QTERMWIDGET
#ifdef QTERMWIDGET6
#include <qtermwidget6/qtermwidget.h>
#else
#include <qtermwidget5/qtermwidget.h>
#endif
#endif

class TerminalWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TerminalWidget(QWidget* parent = nullptr);
    void executeCommand(const QString& commandId);
    void sendText(const QString& text);

public slots:
    void newTab();
    void closeTab(int index);
    void renameTab();

signals:
    void commandTriggered(const QString& commandId);
    void executeInTerminal(const QString& commandId);

private slots:
    void onQuickCommandClicked(const QString& cmdId);
    void onClearClicked();
    void onCopyClicked();
    void onPasteClicked();
    void onTabContextMenu(const QPoint& pos);
    void onFontChanged();
    void onSettingsClicked();
#ifdef HAVE_QTERMWIDGET
    void onTerminalFinished();
    void onTerminalCopyAvailable(bool available);
#endif

private:
    void setupUI();
    void setupQuickCommands();
    void setupToolbar();
    void setupSettingsMenu();
    void loadSettings();
    void saveSettings();
    void applyTheme(const QString& themeName);
    QStringList getQuickCommands() const;
    QString getQuickCommandIds() const;
#ifdef HAVE_QTERMWIDGET
    QTermWidget* createTerminal(const QString& tabTitle = QString());
    QTermWidget* currentTerminal() const;
    void restartTerminal(int index);
#endif

    QTabWidget* m_tabWidget;
    QHBoxLayout* m_quickCmdLayout;
    QList<QPushButton*> m_quickCmdButtons;
    QPushButton* m_newTabBtn;
    QPushButton* m_settingsBtn;
    QPushButton* m_clearBtn;
    QPushButton* m_copyBtn;
    QPushButton* m_pasteBtn;

    QMenu* m_settingsMenu;
    QMenu* m_themeMenu;
    QMenu* m_fontMenu;

    QString m_currentTheme;
    QString m_currentFontFamily;
    int m_currentFontSize;
    int m_opacity;
    int m_tabCount;
    bool m_autoRestart;
};

#endif
