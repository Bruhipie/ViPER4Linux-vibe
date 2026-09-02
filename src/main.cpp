#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QProcess>
#include <csignal>
#include <cstdlib>
#include "ViPERState.h"
#include "EqGraphItem.h"

static void cleanupPulseModule() {
    system("for id in $(pactl list modules short 2>/dev/null | grep -iE 'ViPER4Linux' | awk '{print $1}'); do pactl unload-module $id 2>/dev/null; done");
}

static void signalHandler(int sig) {
    cleanupPulseModule();
    _exit(0);
}

int main(int argc, char *argv[])
{
    // Clean up any stale module from previous runs immediately on startup
    cleanupPulseModule();

    // Register POSIX signal handlers so closing terminal or Ctrl+C always unloads
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGHUP, signalHandler);
    std::atexit(cleanupPulseModule);

    QApplication app(argc, argv);
    app.setOrganizationName("ViPER");
    app.setOrganizationDomain("viper.audio");
    app.setApplicationName("ViPER4Linux");

    QObject::connect(&app, &QCoreApplication::aboutToQuit, &cleanupPulseModule);

    // Register our custom types
    qmlRegisterType<EqGraphItem>("ViPER4Linux", 1, 0, "EqGraphItem");

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("viperState", ViPERState::instance());

    using namespace Qt::StringLiterals;
    const QUrl url(u"qrc:/ViPER4Linux/qml/Main.qml"_s);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    
    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "CRITICAL: No root objects loaded from QML URL:" << url;
    } else {
        qDebug() << "SUCCESS: Root QML object loaded:" << engine.rootObjects().first();
        auto *win = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        if (win) {
            win->setVisible(true);
            win->show();
            win->raise();
            win->requestActivate();
            qDebug() << "Window visibility:" << win->isVisible() << "geometry:" << win->geometry();
        }
    }

    QObject::connect(&app, &QCoreApplication::aboutToQuit, []() {
        // Ensure virtual sink is restored cleanly
        QProcess::execute("sh", QStringList() << "-c" << "pactl unload-module $(pactl list modules short | grep ViPER4Linux_Sink | awk '{print $1}') 2>/dev/null");
    });

    return app.exec();
}
