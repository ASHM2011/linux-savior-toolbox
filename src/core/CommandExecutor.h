#ifndef COMMANDEXECUTOR_H
#define COMMANDEXECUTOR_H

#include <QObject>
#include <QString>
#include <QProcess>
#include <QPointer>
#include <QTimer>
#include <QRegularExpression>

enum class ExecutionState {
    Idle,
    Running,
    Success,
    Failed,
    Timeout
};

struct ExecutionResult {
    bool success;
    QString output;
    QString errorOutput;
    QString errorMessage;
    int exitCode;
    bool timedOut;
};

/**
 * 命令执行器（安全版，与 Taru 版同等级）
 * - 启动始终参数化：QProcess::start(program, args)，绝不走 shell 字符串拼接
 * - 最小化环境变量 PATH（固定为系统安全路径），防止 PATH 劫持
 * - 单条命令长度上限 2048 字符
 * - 无论是否提权，禁止用户把 sudo/pkexec/doas/su 前缀直接拼进命令：
 *      非提权：绕过权限隔离；提权：双重提权（pkexec bash -c "sudo ..."）
 * - 超时默认 5 分钟，先 SIGTERM，3s 后 SIGKILL
 */
class CommandExecutor : public QObject
{
    Q_OBJECT

public:
    static constexpr int kDefaultTimeoutMs = 5 * 60 * 1000;
    static constexpr int kMaxCommandLength = 2048;
    static constexpr int kKillWaitMs       = 3000;

    static CommandExecutor* instance();

    void executeCommand(const QString& commandId, int timeoutMs = kDefaultTimeoutMs);
    void executeRawCommand(const QString& command, bool needsAdmin = false, int timeoutMs = kDefaultTimeoutMs);
    void executeBatch(const QStringList& commandIds, int timeoutPerCommandMs = kDefaultTimeoutMs);

    bool isRunning() const;
    void cancel();
    void terminate();
    void kill();
    void setWorkingDirectory(const QString& dir);

    QString analyzeError(const QString& errorOutput) const;
    QString getSuggestionForError(const QString& errorOutput) const;

signals:
    void executionStarted(const QString& commandId, const QString& friendlyName);
    void executionProgress(int percent, const QString& statusText);
    void executionFinished(const QString& commandId, bool success, const QString& message);
    void outputReceived(const QString& text);
    void errorReceived(const QString& text);
    void batchStarted(int totalCount);
    void batchProgress(int currentIndex, const QString& currentCommandName);
    void batchFinished(bool allSuccess);
    void commandTimedOut(const QString& commandId);

private slots:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onTimeout();

private:
    CommandExecutor();
    ~CommandExecutor() override;

    /** 安全校验 + 规范化：返回空串代表非法（通过 errorReceived 同步说明） */
    QString sanitizeCommand(const QString& raw, bool needsAdmin) const;
    /** 真正启动进程：已经是干净的 command，按 needsAdmin 选择提权方式 */
    void startProcessInternal(const QString& cleanCommand, bool needsAdmin, int timeoutMs);

    QPointer<QProcess> m_process;
    QTimer* m_timeoutTimer;
    bool m_running;
    bool m_timedOut;
    QString m_currentCommandId;
    QString m_collectedOutput;
    QString m_collectedError;
    QString m_workingDirectory;
    QStringList m_batchQueue;
    int m_batchIndex;
    bool m_batchMode;
    bool m_batchAllSuccess;
    int m_batchTimeout;
    static CommandExecutor* s_instance;
};

#endif
