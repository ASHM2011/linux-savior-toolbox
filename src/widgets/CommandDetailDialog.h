#ifndef COMMANDDETAILDIALOG_H
#define COMMANDDETAILDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QTextEdit>
#include <QTableWidget>
#include "core/CommandMetadata.h"

class CommandDetailDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CommandDetailDialog(const QString& commandId, QWidget* parent = nullptr);

signals:
    void executeRequested(const QString& commandId);
    void executeInTerminalRequested(const QString& commandId);
    void executeInBackgroundRequested(const QString& commandId);

private slots:
    void onExecuteClicked();
    void onExecuteInTerminalClicked();
    void onExecuteInBackgroundClicked();

private:
    void setupUI();
    void loadCommandData();

    QString m_commandId;
    CommandMetadata m_cmd;

    QLabel* m_titleLabel;
    QLabel* m_safetyBadge;
    QLabel* m_commandLabel;
    QLabel* m_purposeLabel;
    QLabel* m_effectLabel;
    QLabel* m_pitfallLabel;
    QTableWidget* m_paramTable;
    QPushButton* m_executeBtn;
    QPushButton* m_terminalBtn;
    QPushButton* m_backgroundBtn;
    QPushButton* m_closeBtn;
};

#endif
