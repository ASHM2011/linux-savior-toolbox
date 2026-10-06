#include "TranslationManager.h"

#include <QApplication>
#include <QTranslator>
#include <QSettings>
#include <QLocale>
#include <QDir>
#include <QDebug>

TranslationManager& TranslationManager::instance()
{
    static TranslationManager inst;
    return inst;
}

TranslationManager::TranslationManager()
    : QObject(nullptr)
    , m_translator(new QTranslator(this))
    , m_currentLang(QStringLiteral("zh_CN"))
{
}

TranslationManager::~TranslationManager() = default;

QMap<QString, QString> TranslationManager::supportedLanguages() const
{
    // 语言代码 -> 显示名称（显示名称用该语言自身书写）
    return {
        { QStringLiteral("zh_CN"), QStringLiteral("简体中文") },
        { QStringLiteral("zh_TW"), QStringLiteral("繁體中文") },
        { QStringLiteral("en_US"), QStringLiteral("English") },
        { QStringLiteral("ja"), QStringLiteral("日本語") },
        { QStringLiteral("ko"), QStringLiteral("한국어") },
        { QStringLiteral("ru"), QStringLiteral("Русский") },
        { QStringLiteral("fr"), QStringLiteral("Français") },
        { QStringLiteral("de"), QStringLiteral("Deutsch") },
        { QStringLiteral("es"), QStringLiteral("Español") }
    };
}

QString TranslationManager::currentLanguage() const
{
    return m_currentLang;
}

bool TranslationManager::loadTranslation(const QString& langCode)
{
    // 资源路径：:/translations/linux-savior_zh_CN.qm
    const QString qmPath = QStringLiteral(":/translations/linux-savior_%1.qm").arg(langCode);
    if (langCode == QStringLiteral("zh_CN")) {
        // 中文为源语言，无需加载翻译文件，直接移除翻译器
        qApp->removeTranslator(m_translator);
        m_currentLang = langCode;
        return true;
    }

    if (m_translator->load(qmPath)) {
        qApp->installTranslator(m_translator);
        m_currentLang = langCode;
        return true;
    }

    qWarning() << "[TranslationManager] Failed to load translation:" << qmPath;
    return false;
}

void TranslationManager::initialize()
{
    QSettings settings(QStringLiteral("LinuxSavior"), QStringLiteral("Toolbox"));
    QString savedLang = settings.value(QStringLiteral("i18n/lang"), QString()).toString();

    if (savedLang.isEmpty()) {
        // 首次启动：根据系统语言自动选择
        QString sysLang = QLocale::system().name(); // e.g. "zh_CN", "en_US"
        if (sysLang.startsWith(QStringLiteral("zh_TW")) || sysLang.startsWith(QStringLiteral("zh_HK")) || sysLang.startsWith(QStringLiteral("zh_MO"))) {
            savedLang = QStringLiteral("zh_TW");
        } else if (sysLang.startsWith(QStringLiteral("zh"))) {
            savedLang = QStringLiteral("zh_CN");
        } else if (sysLang.startsWith(QStringLiteral("ja"))) {
            savedLang = QStringLiteral("ja");
        } else if (sysLang.startsWith(QStringLiteral("ko"))) {
            savedLang = QStringLiteral("ko");
        } else if (sysLang.startsWith(QStringLiteral("ru"))) {
            savedLang = QStringLiteral("ru");
        } else if (sysLang.startsWith(QStringLiteral("fr"))) {
            savedLang = QStringLiteral("fr");
        } else if (sysLang.startsWith(QStringLiteral("de"))) {
            savedLang = QStringLiteral("de");
        } else if (sysLang.startsWith(QStringLiteral("es"))) {
            savedLang = QStringLiteral("es");
        } else {
            savedLang = QStringLiteral("en_US");
        }
    }

    if (!supportedLanguages().contains(savedLang)) {
        savedLang = QStringLiteral("zh_CN");
    }

    loadTranslation(savedLang);
}

void TranslationManager::switchLanguage(const QString& langCode)
{
    if (!supportedLanguages().contains(langCode)) {
        qWarning() << "[TranslationManager] Unsupported language:" << langCode;
        return;
    }

    if (langCode == m_currentLang) {
        return;
    }

    if (loadTranslation(langCode)) {
        QSettings settings(QStringLiteral("LinuxSavior"), QStringLiteral("Toolbox"));
        settings.setValue(QStringLiteral("i18n/lang"), langCode);
        emit languageChanged(langCode);
    }
}
