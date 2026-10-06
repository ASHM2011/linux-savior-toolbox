#include <QApplication>
#include <QStyleFactory>
#include <QFile>
#include <QTextStream>
#include <QSettings>
#include <QFont>
#include <QScreen>
#include <QGuiApplication>
#include <QFontDatabase>
#include <QDebug>
#include <QProcessEnvironment>
#include "MainWindow.h"
#include "core/TranslationManager.h"

int main(int argc, char *argv[])
{
    // Set environment variables for native GTK dialog support
    qputenv("QT_QPA_PLATFORMTHEME", QByteArray("gtk2"));

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    app.setApplicationName("Linux Savior Toolbox");
    app.setApplicationVersion("5.2.0");
    app.setOrganizationName("LinuxSavior");

    // 在创建主窗口前加载语言，确保首次渲染即为目标语言
    TranslationManager::instance().initialize();

    app.setStyle(QStyleFactory::create("Fusion"));

    QSettings settings("LinuxSavior", "Toolbox");
    int zoomPercent = settings.value("display/zoom", 100).toInt();
    qreal zoomFactor = zoomPercent / 100.0;

    QFont defaultFont;
    defaultFont.setStyleStrategy(QFont::PreferAntialias);
    defaultFont.setHintingPreference(QFont::PreferFullHinting);

    QStringList fontFamilies = {
        "Noto Sans CJK SC",
        "Source Han Sans CN",
        "WenQuanYi Micro Hei",
        "WenQuanYi Zen Hei",
        "PingFang SC",
        "Microsoft YaHei",
        "Segoe UI",
        "DejaVu Sans",
        "Ubuntu",
        "sans-serif"
    };

    QString foundFont;
    for (const QString& family : fontFamilies) {
        if (QFontDatabase().families().contains(family)) {
            foundFont = family;
            break;
        }
    }
    if (!foundFont.isEmpty()) {
        defaultFont.setFamily(foundFont);
    }
    defaultFont.setPointSizeF(10.0 * zoomFactor);
    app.setFont(defaultFont);

    QFile styleFile(":/styles/main.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&styleFile);
        app.setStyleSheet(stream.readAll());
        styleFile.close();
    }

    MainWindow w;
    w.show();

    return app.exec();
}
