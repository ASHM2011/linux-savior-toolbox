#ifndef TRANSLATIONMANAGER_H
#define TRANSLATIONMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>

class QTranslator;

/**
 * @brief 多语言管理器（单例）
 *
 * 负责加载/保存语言偏好、安装 QTranslator，
 * 并在语言切换时发射 languageChanged 信号。
 *
 * 所有界面字符串需使用 tr() 包裹，以便 Qt 翻译系统抽取与翻译。
 */
class TranslationManager : public QObject
{
    Q_OBJECT

public:
    static TranslationManager& instance();

    /** 当前生效的语言代码（如 "zh_CN"、"en_US"） */
    QString currentLanguage() const;

    /** 支持的语言列表：语言代码 -> 显示名称 */
    QMap<QString, QString> supportedLanguages() const;

    /** 启动时调用：读取持久化语言并安装翻译器（应在 QApplication 创建后、主窗口创建前调用） */
    void initialize();

public slots:
    /** 切换到指定语言代码，持久化并发射 languageChanged */
    void switchLanguage(const QString& langCode);

signals:
    /** 语言已切换，各控件可在此时刷新文本 */
    void languageChanged(const QString& langCode);

private:
    TranslationManager();
    ~TranslationManager() override;
    TranslationManager(const TranslationManager&) = delete;
    TranslationManager& operator=(const TranslationManager&) = delete;

    bool loadTranslation(const QString& langCode);

    QTranslator* m_translator;
    QString m_currentLang;
};

#endif // TRANSLATIONMANAGER_H
