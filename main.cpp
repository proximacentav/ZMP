#include <QApplication>  // вэлике рефакторинг
#include <QMetaType>
#include "mainwindow.h"
#include "playlistswidget.h"
#include "translator.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    qRegisterMetaType<PlaylistInfo>();

    // Resolve UI language before any widgets are built: config.json override,
    // otherwise the system locale (ru -> Russian, de -> German, else English).
    Translator::instance().initFromConfigOrLocale();

    MainWindow w;
    w.show();
    return app.exec();
}