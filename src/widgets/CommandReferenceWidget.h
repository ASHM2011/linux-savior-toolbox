#ifndef COMMANDREFERENCEWIDGET_H
#define COMMANDREFERENCEWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QListWidget>
#include <QScrollArea>
#include <QComboBox>
#include <QGridLayout>
#include "core/CommandMetadata.h"

class CommandReferenceWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CommandReferenceWidget(QWidget* parent = nullptr);

signals:
    void commandTriggered(const QString& commandId);
    void executeInTerminal(const QString& commandId);
    void showCommandDetail(const QString& commandId);

private slots:
    void onSearchChanged(const QString& text);
    void onCategoryChanged(int index);
    void onSafetyFilterChanged(int index);
    void onCategoryListClicked(QListWidgetItem* item);
    void onDetailClicked(const QString& commandId);
    void onExecuteClicked(const QString& commandId);
    void onTerminalClicked(const QString& commandId);

private:
    void setupUI();
    QFrame* setupFilters();
    void setupCategoryList();
    void setupCommandGrid();
    QFrame* createCommandCard(const CommandMetadata& cmd);
    void updateCommandDisplay();
    QList<CommandMetadata> getFilteredCommands() const;

    QLineEdit* m_searchEdit;
    QComboBox* m_categoryCombo;
    QComboBox* m_safetyCombo;
    QListWidget* m_categoryList;
    QScrollArea* m_scrollArea;
    QWidget* m_gridContainer;
    QGridLayout* m_gridLayout;

    CommandCategory m_currentCategory;
    SafetyLevel m_currentSafetyFilter;
    QString m_searchKeyword;
};

#endif
