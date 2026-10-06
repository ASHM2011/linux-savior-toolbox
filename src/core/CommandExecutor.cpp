#include "CommandExecutor.h"
#include "CommandMetadata.h"
#include "PermissionManager.h"
#include "SystemDetector.h"
#include <QDebug>
#include <QEventLoop>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>

CommandExecutor* CommandExecutor::s_instance = nullptr;

CommandExecutor* CommandExecutor::instance()
{
    static CommandExecutor inst;
    if (!s_instance) s_instance = &inst;
    return s_instance;
}

CommandExecutor::CommandExecutor()
    : m_process(nullptr)
    , m_timeoutTimer(new QTimer(this))
    , m_running(false)
    , m_timedOut(false)
    , m_batchIndex(0)
    , m_batchMode(false)
    , m_batchAllSuccess(true)
    , m_batchTimeout(kDefaultTimeoutMs)
{
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &CommandExecutor::onTimeout);
}

CommandExecutor::~CommandExecutor()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->blockSignals(true);
        m_process->terminate();
        if (!m_process->waitForFinished(kKillWaitMs)) {
            m_process->kill();
            m_process->waitForFinished(1000);
        }
    }
}

QString CommandExecutor::sanitizeCommand(const QString& raw, bool needsAdmin) const
{
    Q_UNUSED(needsAdmin);
    QString t = raw.trimmed();
    if (t.isEmpty() || t.length() > kMaxCommandLength) {
        return {};
    }
    // 无论是否提权，禁止 sudo/pkexec/doas/su 前缀：
    //  needsAdmin=false: 绕过权限隔离
    //  needsAdmin=true:  pkexec bash -c "sudo ..." 双重提权，容易出问题
    static const QRegularExpression kPrefixRe(
        QStringLiteral(R"(^(sudo|pkexec|doas|su)\b)"),
        QRegularExpression::CaseInsensitiveOption);
    if (kPrefixRe.match(t).hasMatch()) {
        return {};
    }
    return t;
}

void CommandExecutor::startProcessInternal(const QString& cleanCommand, bool needsAdmin, int timeoutMs)
{
    // --- 每次启动都新建一个 QProcess，避免状态残留（尤其是队列/批处理模式） ---
    if (m_process) {
        m_process->deleteLater();
        m_process.clear();
    }
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    // --- 最小化环境变量：防止 PATH 劫持 / 语言环境导致解析差异 ---
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("PATH"),
               QStringLiteral("/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"));
    env.insert(QStringLiteral("LANG"),   QStringLiteral("C.UTF-8"));
    env.insert(QStringLiteral("LC_ALL"), QStringLiteral("C.UTF-8"));
    env.insert(QStringLiteral("TERM"),   QStringLiteral("dumb"));
    m_process->setProcessEnvironment(env);

    if (!m_workingDirectory.isEmpty() && QDir(m_workingDirectory).exists()) {
        m_process->setWorkingDirectory(m_workingDirectory);
    }

    connect(m_process.data(), &QProcess::readyReadStandardOutput,
            this, &CommandExecutor::onReadyReadStandardOutput);
    connect(m_process.data(), &QProcess::readyReadStandardError,
            this, &CommandExecutor::onReadyReadStandardError);
    connect(m_process.data(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &CommandExecutor::onProcessFinished);

    // --- 全部使用参数化 start(program, args)，任何情况都不做 shell 字符串拼接 ---
    static const QString kPkexec = QStringLiteral("/usr/bin/pkexec");
    static const QString kSudo   = QStringLiteral("/usr/bin/sudo");
    static const QString kBash   = QStringLiteral("/bin/bash");

    if (needsAdmin) {
        const bool havePkexec = QFileInfo(kPkexec).isExecutable();
        if (havePkexec) {
            // pkexec /bin/bash -c <command>
            m_process->start(kPkexec, {kBash, QStringLiteral("-c"), cleanCommand});
        } else if (QFileInfo(kSudo).isExecutable()) {
            // sudo -n /bin/bash -c <command>  （无交互；失败会返回错误码）
            m_process->start(kSudo, {QStringLiteral("-n"), kBash, QStringLiteral("-c"), cleanCommand});
        } else {
            emit errorReceived(tr("[错误] 系统未安装 pkexec 或 sudo，无法以管理员权限执行命令。\n"));
            m_running = false;
            emit executionFinished(m_currentCommandId, false,
                                   tr("管理员执行失败：缺少 pkexec/sudo。"));
            return;
        }
    } else {
        m_process->start(kBash, {QStringLiteral("--noprofile"),
                                 QStringLiteral("--norc"),
                                 QStringLiteral("-c"), cleanCommand});
    }

    m_timedOut = false;
    const int t = (timeoutMs > 0) ? timeoutMs : kDefaultTimeoutMs;

    if (m_process->waitForStarted(3000)) {
        m_timeoutTimer->start(t);
    } else {
        const QString err = tr("命令启动失败：%1\n").arg(m_process->errorString());
        emit errorReceived(err);
        m_running = false;
        emit executionFinished(m_currentCommandId, false,
                               tr("启动失败：%1").arg(m_process->errorString()));
    }
}

void CommandExecutor::executeCommand(const QString& commandId, int timeoutMs)
{
    if (m_running) return;

    auto cmd = CommandMetadataManager::instance()->getCommand(commandId);
    if (cmd.id.isEmpty()) {
        emit executionFinished(commandId, false, tr("未知命令 ID：") + commandId);
        return;
    }

    // ---- 权限前置 ----
    auto permResult = PermissionManager::instance()->checkCommandPermission(commandId);
    const bool needsAdmin = cmd.needsAdmin && (permResult != PermissionCheckResult::ForbiddenAdmin);
    if (permResult == PermissionCheckResult::ForbiddenAdmin && cmd.needsAdmin) {
        qWarning() << tr("[安全拦截] 命令") << commandId << tr("被标记为禁止管理员执行，已降级为普通用户运行（若必须root则会失败）。");
    }

    // ---- 安全清洗 ----
    QString clean = sanitizeCommand(cmd.command, needsAdmin);
    if (clean.isEmpty()) {
        const QString reason = (cmd.command.length() > kMaxCommandLength)
            ? tr("命令长度超过上限（%1 字符）").arg(kMaxCommandLength)
            : tr("命令中含有被禁止的提权前缀（sudo/pkexec/doas/su），请移除后重试。");
        emit errorReceived(tr("[安全拦截] ") + reason + QStringLiteral("\n"));
        emit executionFinished(commandId, false, tr("安全拦截：%1").arg(reason));
        return;
    }

    m_currentCommandId = commandId;
    m_running = true;
    m_collectedOutput.clear();
    m_collectedError.clear();

    emit executionStarted(commandId, cmd.friendlyName);
    emit executionProgress(5, tr("正在执行：%1").arg(cmd.friendlyName));

    startProcessInternal(clean, needsAdmin, timeoutMs);
}

void CommandExecutor::executeRawCommand(const QString& command, bool needsAdmin, int timeoutMs)
{
    if (m_running) return;

    // executeRawCommand 对所有参数注入高危点做严格校验，不允许包含任何提权前缀
    QString clean = sanitizeCommand(command, needsAdmin);
    if (clean.isEmpty()) {
        const QString reason = command.trimmed().isEmpty()
            ? tr("命令为空")
            : (command.length() > kMaxCommandLength
                ? tr("命令长度超过上限（%1 字符）").arg(kMaxCommandLength)
                : tr("命令中含有被禁止的提权前缀（sudo/pkexec/doas/su），请使用功能按钮而非终端直连。"));
        emit errorReceived(tr("[安全拦截] ") + reason + QStringLiteral("\n"));
        emit executionStarted(QStringLiteral("raw"), tr("执行命令"));
        emit executionFinished(QStringLiteral("raw"), false, tr("安全拦截：%1").arg(reason));
        return;
    }

    m_running = true;
    m_currentCommandId = QStringLiteral("raw");
    m_collectedOutput.clear();
    m_collectedError.clear();

    emit executionStarted(QStringLiteral("raw"), tr("执行命令"));
    emit executionProgress(5, tr("正在执行命令..."));

    startProcessInternal(clean, needsAdmin, timeoutMs);
}

void CommandExecutor::executeBatch(const QStringList& commandIds, int timeoutPerCommandMs)
{
    if (m_running || commandIds.isEmpty()) return;

    m_batchQueue = commandIds;
    m_batchIndex = 0;
    m_batchMode = true;
    m_batchAllSuccess = true;
    m_batchTimeout = (timeoutPerCommandMs > 0) ? timeoutPerCommandMs : kDefaultTimeoutMs;

    emit batchStarted(commandIds.size());

    const QString& firstId = commandIds.first();
    auto cmd = CommandMetadataManager::instance()->getCommand(firstId);
    emit batchProgress(0, cmd.friendlyName);

    executeCommand(firstId, m_batchTimeout);
}

bool CommandExecutor::isRunning() const
{
    return m_running;
}

void CommandExecutor::terminate()
{
    m_timeoutTimer->stop();
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
    }
}

void CommandExecutor::kill()
{
    m_timeoutTimer->stop();
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
    }
}

void CommandExecutor::cancel()
{
    m_timeoutTimer->stop();
    if (!m_process) return;
    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(kKillWaitMs)) {
            m_process->kill();
            m_process->waitForFinished(1000);
        }
    }
}

void CommandExecutor::setWorkingDirectory(const QString& dir)
{
    m_workingDirectory = dir;
}

QString CommandExecutor::analyzeError(const QString& errorOutput) const
{
    const QString lower = errorOutput.toLower();

    if (lower.contains(QStringLiteral("timeout")) || lower.contains(QStringLiteral("timed out")))
        return tr("命令执行超时");
    if (lower.contains(QStringLiteral("无法获得锁")) || lower.contains(QStringLiteral("could not get lock")) ||
        lower.contains(QStringLiteral("unable to lock the administration directory")))
        return tr("包管理器被占用");
    if (lower.contains(QStringLiteral("dpkg was interrupted")) || lower.contains(QStringLiteral("dpkg 被中断")))
        return tr("dpkg 状态被中断");
    if (lower.contains(QStringLiteral("permission denied")) || lower.contains(QStringLiteral("权限不够")))
        return tr("权限不足");
    if (lower.contains(QStringLiteral("no space left on device")) || lower.contains(QStringLiteral("设备上没有空间")))
        return tr("磁盘空间不足");
    if (lower.contains(QStringLiteral("cannot resolve")) || lower.contains(QStringLiteral("temporary failure resolving")) ||
        lower.contains(QStringLiteral("无法解析")))
        return tr("网络连接失败");
    if (lower.contains(QStringLiteral("command not found")) || lower.contains(QStringLiteral("找不到命令")))
        return tr("命令不存在");
    if (lower.contains(QStringLiteral("broken packages")) || lower.contains(QStringLiteral("依赖关系不满足")) ||
        lower.contains(QStringLiteral("unmet dependencies")))
        return tr("依赖关系损坏");
    if (lower.contains(QStringLiteral("authentication failed")) || lower.contains(QStringLiteral("not authorized")))
        return tr("认证失败");
    return tr("执行出错");
}

QString CommandExecutor::getSuggestionForError(const QString& errorOutput) const
{
    const QString lower = errorOutput.toLower();

    if (lower.contains(QStringLiteral("timeout")) || lower.contains(QStringLiteral("timed out")))
        return tr("命令执行时间过长已被终止。请检查命令是否需要交互，或稍后重试。");
    if (lower.contains(QStringLiteral("无法获得锁")) || lower.contains(QStringLiteral("could not get lock")))
        return tr("建议使用「解锁包管理器」功能；确认其他更新程序已退出后再操作。");
    if (lower.contains(QStringLiteral("dpkg was interrupted")))
        return tr("建议运行「修复 dpkg 中断状态」命令。");
    if (lower.contains(QStringLiteral("permission denied")) || lower.contains(QStringLiteral("权限不够")))
        return tr("此操作需要管理员权限；请确认授权对话框已正确通过。");
    if (lower.contains(QStringLiteral("no space left on device")) || lower.contains(QStringLiteral("设备上没有空间")))
        return tr("磁盘空间不足！建议使用系统清理功能释放空间。");
    if (lower.contains(QStringLiteral("cannot resolve")) || lower.contains(QStringLiteral("temporary failure resolving")))
        return tr("网络解析失败，请检查网络或 DNS 设置。");
    if (lower.contains(QStringLiteral("command not found")) || lower.contains(QStringLiteral("找不到命令")))
        return tr("命令不存在，可能需要先安装对应软件包。");
    if (lower.contains(QStringLiteral("broken packages")) || lower.contains(QStringLiteral("unmet dependencies")))
        return tr("依赖关系损坏，建议使用「修复依赖关系」功能。");
    if (lower.contains(QStringLiteral("authentication failed")) || lower.contains(QStringLiteral("not authorized")))
        return tr("管理员认证失败；请重试并输入正确的密码。");
    return tr("请检查输出信息，或尝试重新执行命令。");
}

void CommandExecutor::onReadyReadStandardOutput()
{
    if (!m_process) return;
    const QByteArray data = m_process->readAllStandardOutput();
    const QString output = QString::fromLocal8Bit(data);
    m_collectedOutput += output;
    emit outputReceived(output);
    emit executionProgress(50, tr("正在执行中..."));
}

void CommandExecutor::onReadyReadStandardError()
{
    if (!m_process) return;
    const QByteArray data = m_process->readAllStandardError();
    const QString error = QString::fromLocal8Bit(data);
    m_collectedError += error;
    emit errorReceived(error);
}

void CommandExecutor::onTimeout()
{
    m_timedOut = true;
    emit commandTimedOut(m_currentCommandId);
    cancel();
}

void CommandExecutor::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_timeoutTimer->stop();

    if (m_process) {
        const QString remainingStdout = QString::fromLocal8Bit(m_process->readAllStandardOutput());
        const QString remainingStderr = QString::fromLocal8Bit(m_process->readAllStandardError());
        if (!remainingStdout.isEmpty()) {
            m_collectedOutput += remainingStdout;
            emit outputReceived(remainingStdout);
        }
        if (!remainingStderr.isEmpty()) {
            m_collectedError += remainingStderr;
            emit errorReceived(remainingStderr);
        }
    }

    bool success = false;
    QString message;
    auto cmd = CommandMetadataManager::instance()->getCommand(m_currentCommandId);
    const QString name = cmd.friendlyName.isEmpty() ? tr("命令") : cmd.friendlyName;

    if (m_timedOut) {
        message = tr("%1 执行超时，已被终止。").arg(name);
        emit executionProgress(100, tr("超时"));
    } else if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        success = true;
        message = tr("%1 执行成功！").arg(name);
        emit executionProgress(100, tr("完成"));
    } else {
        const QString errorAnalysis = analyzeError(m_collectedError);
        const QString suggestion = getSuggestionForError(m_collectedError);
        message = tr("%1 执行失败：%2\n%3").arg(name, errorAnalysis, suggestion);
        emit executionProgress(100, tr("失败"));
    }

    m_running = false;
    emit executionFinished(m_currentCommandId, success, message);

    // ---- 批处理队列推进 ----
    if (m_batchMode) {
        if (!success) m_batchAllSuccess = false;

        if (m_batchIndex < m_batchQueue.size() - 1) {
            m_batchIndex++;
            const QString& nextId = m_batchQueue.at(m_batchIndex);
            auto nextCmd = CommandMetadataManager::instance()->getCommand(nextId);
            emit batchProgress(m_batchIndex, nextCmd.friendlyName);
            QTimer::singleShot(500, this, [this, nextId]() {
                executeCommand(nextId, m_batchTimeout);
            });
        } else {
            m_batchMode = false;
            emit batchFinished(m_batchAllSuccess);
        }
    }
}
