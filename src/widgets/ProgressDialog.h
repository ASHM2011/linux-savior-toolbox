#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QTimer>
#include <QFrame>

class ProgressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProgressDialog(const QString& title, QWidget* parent = nullptr);

public slots:
    void setProgress(int percent, const QString& statusText);
    void setFinished(bool success, const QString& message);
    void appendOutput(const QString& text);

signals:
    void cancelled();

private slots:
    void onCancelClicked();
    void onToggleOutput();

private:
    void setupUI();

    QLabel* m_titleLabel;
    QLabel* m_statusLabel;
    QProgressBar* m_progressBar;
    QTextEdit* m_outputEdit;
    QPushButton* m_cancelBtn;
    QPushButton* m_closeBtn;
    QPushButton* m_toggleOutputBtn;
    QLabel* m_resultIcon;
    QLabel* m_resultText;
    QFrame* m_resultWidget;
    QFrame* m_outputFrame;
    bool m_finished;
    bool m_outputExpanded;
};

#endif
