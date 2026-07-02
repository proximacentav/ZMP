#include <QApplication>  // вэлике рефакторинг
#include <QMetaType>
#include "mainwindow.h"
#include "playlistswidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    qRegisterMetaType<PlaylistInfo>();
    MainWindow w;
    w.show();
    return app.exec();
}