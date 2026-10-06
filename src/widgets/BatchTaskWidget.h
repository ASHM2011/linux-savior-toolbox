#ifndef BATCHTASKWIDGET_H
#define BATCHTASKWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QProgressBar>
#include <QListWidget>
#include <QScrollArea>

struct BatchTask {
    QString id;
    QString icon;
    QString name;
    QString description;
    QStringList commandIds;
    QString color;
};

class BatchTaskWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BatchTaskWidget(QWidget* parent = nullptr);

signals:
    void batchStarted(const QStringList& commandIds);
    void commandTriggered(const QString& commandId);

private slots:
    void onPresetTaskClicked(int index);
    void onAddCommandClicked();
    void onRemoveCommandClicked();
    void onMoveUpClicked();
    void onMoveDownClicked();
    void onExecuteCustomClicked();
    void onSavePresetClicked();
    void onAvailableItemDoubleClicked(QListWidgetItem* item);
    void onSelectedItemDoubleClicked(QListWidgetItem* item);

private:
    void setupUI();
    void initTasks();
    void initAvailableCommands();
    QFrame* createPresetCard(const BatchTask& task, int index);
    QFrame* createProgressSection();

    QList<BatchTask> m_tasks;
    QVBoxLayout* m_presetLayout;
    QListWidget* m_availableList;
    QListWidget* m_selectedList;
    QProgressBar* m_overallProgress;
    QLabel* m_currentTaskLabel;
    QLabel* m_progressCountLabel;
    QPushButton* m_executeCustomBtn;
    int m_selectedPresetIndex;
};

#endif
