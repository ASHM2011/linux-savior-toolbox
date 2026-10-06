#ifndef LOGVIEWERWIDGET_H
#define LOGVIEWERWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QTimer>

class LogViewerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogViewerWidget(QWidget* parent = nullptr);

signals:
    void commandTriggered(const QString& commandId);

private slots:
    void onRefresh();
    void onFilterChanged();
    void onLogSourceChanged(int index);
    void onSearchTextChanged(const QString& text);
    void onAutoRefreshToggled(bool checked);
    void appendLogOutput(const QString& text);
    void appendLogError(const QString& text);
    void onCommandFinished(int exitCode);

private:
    void setupUI();
    void loadLogSources();
    void executeJournalCommand(const QString& args);
    QString getCurrentLogCommand() const;

    QComboBox* m_logSourceCombo;
    QComboBox* m_priorityCombo;
    QLineEdit* m_searchEdit;
    QPushButton* m_refreshBtn;
    QPushButton* m_autoRefreshBtn;
    QPushButton* m_exportBtn;
    QTextEdit* m_logView;
    QLabel* m_statusLabel;
    QTimer* m_autoRefreshTimer;

    bool m_isRefreshing;
};

#endif
