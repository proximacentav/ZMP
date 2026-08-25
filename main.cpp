#include <QApplication>  // вэлике рефакторинг
#include <QMetaType>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QSplashScreen>
#include <QPainter>
#include <QPixmap>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include "mainwindow.h"
#include "cliplayer.h"
#include "playlistswidget.h"
#include "singleinstance.h"
#include "depsmanager.h"
#include "translator.h"

#include <cstdio>

// ---------------------------------------------------------------------------
//  Аргументы командной строки:
//    zmp [файлы]                     открыть файлы в очереди воспроизведения
//    zmp -o /путь/до/файла           то же (открыть с этим файлом)
//    zmp --playlist-edit кластер/плейлист add /файл [--force] [--mkplaylist]
//    zmp --playlist play кластер/плейлист   играть плейлист
//    zmp --cli [файлы | --playlist play к/п]   консольный режим без GUI
// ---------------------------------------------------------------------------

struct Args {
    QStringList openFiles;
    // playlist-edit
    bool editMode = false;
    QString editCluster, editPlaylist, editFile;
    bool force = false, mkPlaylist = false;
    // playlist play
    bool playPlaylist = false;
    QString plCluster, plName;
    bool cli = false;
};

static void parseArgs(const QStringList &a, Args &out)
{
    for (int i = 1; i < a.size(); ++i) {
        const QString arg = a.at(i);
        if (arg == "-o" || arg == "--open") {
            if (++i < a.size()) out.openFiles << a.at(i);
        } else if (arg == "--playlist-edit") {
            if (++i >= a.size()) break;
            const QString spec = a.at(i);                 // кластер/плейлист
            const int slash = spec.indexOf('/');
            out.editCluster = slash > 0 ? spec.left(slash) : QStringLiteral("default");
            out.editPlaylist = slash > 0 ? spec.mid(slash + 1) : spec;
            if (++i < a.size() && a.at(i) == "add") {
                if (++i < a.size()) out.editFile = a.at(i);
            }
            out.editMode = true;
        } else if (arg == "--force") {
            out.force = true;
        } else if (arg == "--mkplaylist") {
            out.mkPlaylist = true;
        } else if (arg == "--playlist") {
            if (++i >= a.size()) break;
            const QString verb = a.at(i);                 // "play"
            if (++i >= a.size()) break;
            const QString spec = a.at(i);                 // кластер/плейлист
            const int slash = spec.indexOf('/');
            out.playPlaylist = (verb == "play");
            out.plCluster = slash > 0 ? spec.left(slash) : QStringLiteral("default");
            out.plName = slash > 0 ? spec.mid(slash + 1) : spec;
        } else if (arg == "--cli") {
            out.cli = true;
        } else if (!arg.startsWith('-')) {
            out.openFiles << arg;
        }
    }
}

static bool isAudioFile(const QString &path)
{
    static const QStringList exts = {"mp3", "wav", "flac", "ogg", "m4a",
                                     "aac", "aiff", "alac", "opus", "wma"};
    return exts.contains(QFileInfo(path).suffix().toLower());
}

// zmp --playlist-edit кластер/плейлист add файл [--force] [--mkplaylist]
static int doPlaylistEdit(const Args &args)
{
    if (args.editFile.isEmpty()) {
        fprintf(stderr, "zmp: --playlist-edit: укажите файл: add /путь/до/файла\n");
        return 1;
    }
    const QFileInfo fi(args.editFile);
    if (!fi.exists()) {
        fprintf(stderr, "zmp: файл не найден: %s\n", args.editFile.toUtf8().constData());
        return 1;
    }
    if (!args.force && !isAudioFile(args.editFile)) {
        fprintf(stderr, "zmp: %s не аудио файл (используйте --force чтобы добавить принудительно)\n",
                args.editFile.toUtf8().constData());
        return 1;
    }

    const QString plDir = PlaylistsWidget::clusterPath(args.editCluster)
                          + "/" + args.editPlaylist;
    if (!QDir(plDir).exists()) {
        if (!args.mkPlaylist) {
            fprintf(stderr, "zmp: плейлист не существует: %s/%s (добавьте --mkplaylist для создания)\n",
                    args.editCluster.toUtf8().constData(),
                    args.editPlaylist.toUtf8().constData());
            return 1;
        }
        QDir().mkpath(plDir);
    }

    const QString dst = plDir + "/" + fi.fileName();
    if (!QFile::copy(args.editFile, dst)) {
        fprintf(stderr, "zmp: не удалось добавить файл (возможно, уже есть): %s\n",
                dst.toUtf8().constData());
        return 1;
    }
    printf("zmp: добавлено в %s/%s: %s\n",
           args.editCluster.toUtf8().constData(),
           args.editPlaylist.toUtf8().constData(),
           args.editFile.toUtf8().constData());
    return 0;
}

// zmp --help | -h
static void printHelp()
{
    printf("ZMP - music player\n\n"
           "Usage:\n"
           "  zmp [files...]                open files in the play queue\n"
           "  zmp -o /path/to/file          open ZMP with this file in the queue\n"
           "  zmp --playlist-edit CLUSTER/PLAYLIST add FILE [--force] [--mkplaylist]\n"
           "                                add FILE to playlist (--force: even if not audio,\n"
           "                                --mkplaylist: create playlist/cluster if missing)\n"
           "  zmp --playlist play CLUSTER/PLAYLIST\n"
           "                                open ZMP playing this playlist\n"
           "  zmp --cli [files...]          console player (no GUI)\n"
           "  zmp --cli --playlist play CLUSTER/PLAYLIST\n"
           "  zmp --help | -h               show this help\n\n"
           "CLI keys: Ctrl+A file manager, Ctrl+B pause, n next, p prev,\n"
           "          arrows+Enter select track, Ctrl+C exit\n");
}

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        const QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == "--help" || arg == "-h") {
            printHelp();
            return 0;
        }
    }

    QApplication app(argc, argv);
    zmpInstallCrashHandler();
    qRegisterMetaType<PlaylistInfo>();

    Args args;
    parseArgs(app.arguments(), args);

    // Режим редактирования плейлиста — без GUI и без единого экземпляра
    if (args.editMode) {
        Translator::instance().initFromConfigOrLocale();
        return doPlaylistEdit(args);
    }

    // Если экземпляр ZMP уже запущен — передаём ему задачу и выходим
    {
        QJsonObject msg;
        if (args.playPlaylist) {
            msg["cmd"] = "playlist";
            msg["cluster"] = args.plCluster;
            msg["name"] = args.plName;
        } else if (!args.openFiles.isEmpty()) {
            msg["cmd"] = "open";
            QJsonArray arr;
            for (const QString &f : args.openFiles)
                arr.append(f);
            msg["files"] = arr;
        }
        if (!msg.isEmpty() &&
            SingleInstance::sendToRunning(
                QJsonDocument(msg).toJson(QJsonDocument::Compact)))
            return 0;
    }

    // Первый запуск: ~/zmp_playlists не существует — диалог выбора языка
    // (всегда на английском, не переводится)
    if (!QDir(QDir::homePath() + "/zmp_playlists").exists()) {
        QDialog dlg;
        dlg.setWindowTitle(QStringLiteral("Welcome to ZMP"));
        QVBoxLayout *l = new QVBoxLayout(&dlg);
        QLabel *title = new QLabel(QStringLiteral("welcome to ZMP"));
        QFont tf = title->font();
        tf.setBold(true);
        tf.setPointSize(tf.pointSize() + 2);
        title->setFont(tf);
        l->addWidget(title);
        l->addWidget(new QLabel(QStringLiteral("select language :")));
        QComboBox *langCombo = new QComboBox;
        langCombo->addItem(Translator::nativeName(Translator::Russian), int(Translator::Russian));
        langCombo->addItem(Translator::nativeName(Translator::English), int(Translator::English));
        langCombo->addItem(Translator::nativeName(Translator::German),  int(Translator::German));
        l->addWidget(langCombo);
        QHBoxLayout *btns = new QHBoxLayout;
        QPushButton *okBtn = new QPushButton(QStringLiteral("OK"));
        QPushButton *exitBtn = new QPushButton(QStringLiteral("EXIT"));
        btns->addWidget(okBtn);
        btns->addWidget(exitBtn);
        l->addLayout(btns);

        bool accepted = false;
        QObject::connect(okBtn, &QPushButton::clicked, &dlg, [&]() {
            Translator::instance().setLanguage(
                static_cast<Translator::Language>(langCombo->currentData().toInt()));
            accepted = true;
            dlg.accept();
        });
        QObject::connect(exitBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

        dlg.exec();
        if (!accepted) return 0;   // EXIT — закрыть ZMP без сохранения языка
    }

    // Resolve UI language before any widgets are built
    Translator::instance().initFromConfigOrLocale();

    // Консольный режим без GUI (пустая очередь допустима — треки можно
    // добавить через Ctrl+A файловый менеджер)
    if (args.cli) {
        QStringList files = args.openFiles;
        QString label;
        if (files.isEmpty() && args.playPlaylist) {
            QDir dir(PlaylistsWidget::clusterPath(args.plCluster) + "/" + args.plName);
            label = args.plCluster + "/" + args.plName;
            for (const QString &f : dir.entryList(QDir::Files, QDir::Name))
                files << dir.absoluteFilePath(f);
        }
        CliPlayer player;
        player.run(files, label);
        return app.exec();
    }

    // Splash "ZMP is starting..." с последними строками логов — показывается
    // сразу и закрывается, когда основное окно открыто
    QSplashScreen *splash = new QSplashScreen;
    splash->setFixedSize(520, 220);
    QPixmap pm(520, 220);
    pm.fill(QColor("#1b1b1b"));
    splash->setPixmap(pm);
    splash->showMessage(QStringLiteral("ZMP is starting..."),
                        Qt::AlignLeft | Qt::AlignTop, Qt::white);
    splash->show();
    app.processEvents();
    MainWindow::setSplash(splash);

    MainWindow w;
    w.show();

    // Основное окно открыто — splash больше не нужен
    MainWindow::setSplash(nullptr);
    splash->finish(&w);
    splash->deleteLater();

    // Локальный IPC-сервер: принимаем файлы/плейлисты от вторых экземпляров
    SingleInstance ipc;
    ipc.listen([&w](const QByteArray &payload) {
        const QJsonObject msg = QJsonDocument::fromJson(payload).object();
        if (msg.value("cmd").toString() == "open") {
            QStringList files;
            for (const QJsonValue &v : msg.value("files").toArray())
                files << v.toString();
            if (!files.isEmpty())
                w.openFilesInQueue(files);
        } else if (msg.value("cmd").toString() == "playlist") {
            w.playExternalPlaylist(msg.value("cluster").toString(),
                                   msg.value("name").toString());
        }
    });

    // Файлы из аргументов запуска (файловый менеджер, -o, --playlist play)
    if (!args.openFiles.isEmpty()) {
        QTimer::singleShot(200, &w, [&w, files = args.openFiles]() mutable {
            w.openFilesInQueue(files);
        });
    } else if (args.playPlaylist) {
        QTimer::singleShot(200, &w, [&w, c = args.plCluster, n = args.plName]() {
            w.playExternalPlaylist(c, n);
        });
    }

    return app.exec();
}
