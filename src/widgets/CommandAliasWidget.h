#ifndef COMMANDALIASWIDGET_H
#define COMMANDALIASWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QTableWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QStringList>
#include <QPair>

class CommandAliasWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CommandAliasWidget(QWidget* parent = nullptr);

private slots:
    void onApplyBashClicked();
    void onApplyZshClicked();
    void onAddAliasClicked();
    void onEditAliasClicked(int row);
    void onDeleteAliasClicked(int row);
    void onPresetCheckboxChanged(int row, bool checked);

private:
    void setupUI();
    void setupPresetTable();
    void setupCustomTable();
    QString detectCurrentShell() const;
    QString detectCurrentDistro() const;
    QString generateAliasContent() const;
    bool writeToFile(const QString& filePath, const QString& content);

    QTableWidget* m_presetTable;
    QLineEdit* m_aliasNameEdit;
    QLineEdit* m_aliasCmdEdit;
    QTableWidget* m_customTable;
    QPushButton* m_bashBtn;
    QPushButton* m_zshBtn;
    QLabel* m_currentShellLabel;

    QList<QPair<QString, QString>> m_customAliases;
    QList<bool> m_presetEnabled;
    QStringList m_presetAliases;
    QStringList m_presetCommands;
    QStringList m_presetDescs;
};

#endif
