#include <QApplication> // ыыыыыыыы ы ыыы ыы ы  ы ыыы ы ы ы ы
#include <QIcon>
#include <QFile>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QFile styleFile(":/style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(styleFile.readAll());
        app.setStyleSheet(styleSheet);
    }

    MainWindow w;
    w.show();
    return app.exec();
}