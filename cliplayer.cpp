#include "cliplayer.h"
#include "audiomanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSocketNotifier>
#include <QTimer>
#ifndef ZMP_NO_TAGLIB
#include <taglib/fileref.h>
#include <taglib/tstring.h>
#endif

#include <termios.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <poll.h>

// Ctrl+A файловый менеджер: ввод пути с живыми подсказками (ls по мере
// посимвольного ввода), затем варианты действий. Всё событийно, без
// блокирующих чтений — поэтому возврата "в главное меню" больше нет.

static termios g_oldt;
static bool g_raw = false;

CliPlayer::CliPlayer(QObject *parent)
    : QObject(parent), m_am(new AudioManager(this))
{
}

CliPlayer::~CliPlayer()
{
    if (g_raw) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_oldt);
        printf("\n");
    }
}

void CliPlayer::run(const QStringList &files, const QString &playlistLabel)
{
    m_queue = files;
    m_playlistLabel = playlistLabel;

    if (!g_raw) {
        termios t;
        tcgetattr(STDIN_FILENO, &t);
        g_oldt = t;
        g_raw = true;
        t.c_lflag &= ~(ICANON | ECHO);
        t.c_cc[VMIN] = 1;
        t.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
    }

    if (!m_stdinNotifier) {
        m_stdinNotifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read, this);
        connect(m_stdinNotifier, &QSocketNotifier::activated, this, &CliPlayer::onStdin);
    }
    connect(m_am, &AudioManager::trackEnded, this, [this]() { nextTrack(); });

    QTimer *posTimer = new QTimer(this);
    connect(posTimer, &QTimer::timeout, this, &CliPlayer::onPositionTimer);
    posTimer->start(200);

    if (!m_queue.isEmpty())
        playIndex(0);
    redraw();
}

void CliPlayer::playIndex(int idx)
{
    if (idx < 0 || idx >= m_queue.size()) return;
    m_pos = idx;
    m_selected = idx;
    m_paused = false;
    m_am->setSourceFile(m_queue.at(idx));
    m_am->play();
    redraw();
}

void CliPlayer::nextTrack()
{
    if (m_pos + 1 < m_queue.size()) playIndex(m_pos + 1); else redraw();
}

void CliPlayer::prevTrack()
{
    if (m_pos - 1 >= 0) playIndex(m_pos - 1); else redraw();
}

void CliPlayer::togglePause()
{
    if (m_pos < 0) return;
    if (m_paused) { m_am->play(); m_paused = false; }
    else          { m_am->pause(); m_paused = true; }
    redraw();
}

// ---------------------------------------------------------------------------
//  Подсказки: ls каталога-префикса / кластеры и плейлисты
// ---------------------------------------------------------------------------

QStringList CliPlayer::pathSuggestions(const QString &typed)
{
    QString dir, prefix;
    const int slash = typed.lastIndexOf('/');
    if (slash < 0)      { dir = ".";  prefix = typed; }
    else if (slash == 0){ dir = "/";  prefix = typed.mid(1); }
    else                { dir = typed.left(slash); prefix = typed.mid(slash + 1); }

    QDir d(dir);
    if (!d.exists())
        return { QStringLiteral("error: no such file or directory in ") +
                 (slash == 0 ? QStringLiteral("/") : dir) };

    QStringList result;
    const QFileInfoList entries =
        d.entryInfoList(QDir::AllEntries | QDir::Hidden, QDir::DirsFirst | QDir::Name);
    int shown = 0;
    for (const QFileInfo &fi : entries) {
        const QString name = fi.fileName();
        if (!name.startsWith(prefix)) continue;
        result << (fi.isDir() ? name + "/" : name);
        if (++shown >= 15) {
            result << QStringLiteral("... (+%1 more)").arg(entries.size() - shown);
            break;
        }
    }
    if (result.isEmpty())
        result << QStringLiteral("(nothing matches '%1' in %2)").arg(prefix, dir);
    return result;
}

QStringList CliPlayer::clusterSuggestions(const QString &typed)
{
    QDir base(QDir::homePath() + "/zmp_playlists");
    const int slash = typed.indexOf('/');
    if (slash < 0) {
        QStringList out;
        for (const QString &d : base.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            if (!d.startsWith("cls_")) continue;
            const QString name = d.mid(4);
            if (name.startsWith(typed)) out << name + "/";
        }
        if (out.isEmpty())
            out << QStringLiteral("error: no such cluster in ~/zmp_playlists");
        return out;
    }

    const QString cluster = typed.left(slash);
    const QString plPrefix = typed.mid(slash + 1);
    QDir cdir(base.absoluteFilePath("cls_" + cluster));
    if (!cdir.exists())
        return { QStringLiteral("error: no such cluster in ~/zmp_playlists") };

    QStringList out;
    for (const QString &p : cdir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        if (p.startsWith(plPrefix)) out << p;   // хвост — для TAB-дополнения
    if (out.isEmpty())
        out << QStringLiteral("(nothing matches '%1')").arg(plPrefix);
    return out;
}

void CliPlayer::printHints(const QStringList &hints)
{
    m_currentHints = hints;
    if (m_hintIndex >= hints.size()) m_hintIndex = hints.size() - 1;

    printf("\033[J");                       // очистить всё ниже курсора
    int n = 0;
    for (int i = 0; i < hints.size(); ++i) {
        if (i == m_hintIndex)
            printf("\n  \033[34m%s\033[0m", hints.at(i).toUtf8().constData());
        else
            printf("\n  %s", hints.at(i).toUtf8().constData());
        ++n;
    }
    m_hintLines = n;
    if (n > 0)
        printf("\033[%dA\r\033[%dC", n, m_promptLen + m_inputLine.toUtf8().size());
    fflush(stdout);
}

// Стрелка вверх/вниз по подсказкам — перерисовать блок с синей подсветкой
void CliPlayer::hintMove(int delta)
{
    if (m_currentHints.isEmpty())
        return;
    const int count = m_currentHints.size();
    m_hintIndex = (m_hintIndex < 0) ? ((delta > 0) ? 0 : count - 1)
                                    : (m_hintIndex + delta + count) % count;

    // перерисовать только блок подсказок
    if (m_hintLines > 0)
        printf("\033[%dA", m_hintLines);
    printf("\r");
    for (int i = 0; i < count; ++i) {
        if (i == m_hintIndex)
            printf("\033[2K  \033[34m%s\033[0m\n",
                   m_currentHints.at(i).toUtf8().constData());
        else
            printf("\033[2K  %s\n", m_currentHints.at(i).toUtf8().constData());
    }
    printf("\033[%dA\r\033[%dC", count, m_promptLen + m_inputLine.toUtf8().size());
    fflush(stdout);
}

// TAB: впечатать выбранную подсказку и показать подсказки следующего уровня
void CliPlayer::hintAcceptTab()
{
    if (m_currentHints.isEmpty())
        return;
    const QString pick = m_currentHints.at(m_hintIndex < 0 ? 0 : m_hintIndex);
    if (pick.startsWith("error:") || pick.startsWith("("))
        return;   // служебные строки не впечатываем

    // head — всё до последнего '/', хвост заменяется подсказкой
    QString head;
    const int slash = m_inputLine.lastIndexOf('/');
    if (slash >= 0) head = m_inputLine.left(slash + 1);

    m_inputLine = head + pick;
    m_hintIndex = -1;
    redrawInputLine();

    // подсказки нового уровня
    if (m_state == FmState::PathEntry)
        printHints(pathSuggestions(m_inputLine));
    else if (m_state == FmState::PlaylistEntry)
        printHints(clusterSuggestions(m_inputLine));
}

void CliPlayer::beginInput(FmState state, const QString &prompt)
{
    m_state = state;
    m_inputLine.clear();
    m_hintLines = 0;
    m_hintIndex = -1;
    m_currentHints.clear();
    m_prompt = prompt;
    m_promptLen = prompt.toUtf8().size() + 2;   // + "> "
    printf("%s> ", prompt.toUtf8().constData());
    fflush(stdout);
}

void CliPlayer::inputAppendChar(const QByteArray &utf8char)
{
    m_inputLine += QString::fromUtf8(utf8char);
    fputs(utf8char.constData(), stdout);
    fflush(stdout);
}

void CliPlayer::inputBackspace()
{
    if (m_inputLine.isEmpty()) return;
    QByteArray b = m_inputLine.toUtf8();
    while (!b.isEmpty() && (b.at(b.size() - 1) & 0xC0) == 0x80) b.chop(1);
    if (!b.isEmpty()) b.chop(1);
    m_inputLine = QString::fromUtf8(b);
    printf("\b \b");
    fflush(stdout);
}

// Перезаписать строку ввода целиком (после стирания подсказок)
void CliPlayer::redrawInputLine()
{
    printf("\r\033[K%s> %s", m_prompt.toUtf8().constData(),
           m_inputLine.toUtf8().constData());
    fflush(stdout);
}

void CliPlayer::finishInput(const char *message)
{
    if (message && message[0])
        printf("%s\n", message);
    m_state = FmState::None;
    redraw();
}

// ---------------------------------------------------------------------------
//  Файловый менеджер (Ctrl+A) — состояния
// ---------------------------------------------------------------------------

void CliPlayer::startFileManager()
{
    beginInput(FmState::PathEntry, QStringLiteral("path to file"));
    printHints(pathSuggestions(QString()));
}

void CliPlayer::onPathEnter()
{
    const QString path = QDir::cleanPath(m_inputLine);
    if (!QFileInfo(path).exists()) {
        finishInput("file not found");
        return;
    }
    m_pendingFile = path;
    m_state = FmState::VariantSelect;
    printf("\nselect variant\n"
           "  1: add to queue\n"
           "  2: add to playlist\n"
           "  3: add to featured\n"
           "  4: delete\n"
           "  5: delete(sudo)\n");
    fflush(stdout);
}

void CliPlayer::variantAddToQueue()
{
    beginInput(FmState::QueueNumber,
               QStringLiteral("number of track in queue>>0-") +
                   QString::number(qMax(0, m_queue.size())));
}

void CliPlayer::onQueueNumberEnter()
{
    bool ok = false;
    const int num = m_inputLine.toInt(&ok);
    if (!ok || num < 0 || num > 2147483646) {
        finishInput("error: number out of range 0-2147483646");
        return;
    }

    if (num < m_queue.size()) {
        // Номер занят
        m_pendingNum = num;
        m_state = FmState::ForceConfirm;
        printf("\nerror: this number is using by %s\n"
               "  1: force-add\n  2: cancel\n",
               m_queue.at(num).toUtf8().constData());
        fflush(stdout);
        return;
    }

    m_queue.append(m_pendingFile);
    finishInput(nullptr);
}

void CliPlayer::variantAddToPlaylist()
{
    beginInput(FmState::PlaylistEntry,
               QStringLiteral("enter cluster/playlist to add file"));
    printHints(clusterSuggestions(QString()));
}

void CliPlayer::onPlaylistEnter()
{
    QString input = m_inputLine;
    if (input.startsWith("cls_"))
        input.remove(0, 4);

    QString cluster, playlist;
    const int slash = input.indexOf('/');
    cluster = (slash > 0) ? input.left(slash) : input;
    playlist = (slash > 0) ? input.mid(slash + 1) : QString();

    if (playlist.isEmpty()) {
        finishInput("failed: no such playlist");
        return;
    }

    QDir plDir(QDir::homePath() + "/zmp_playlists/cls_" + cluster + "/" + playlist);
    if (!plDir.exists()) {
        m_pendingCluster = input;   // кластер/плейлист для создания
        m_state = FmState::CreatePlaylistConfirm;
        printf("\nfailed: no such playlist\n  1: cancel\n  2: create playlist and cluster\n");
        fflush(stdout);
        return;
    }

    const QString dst = plDir.absoluteFilePath(QFileInfo(m_pendingFile).fileName());
    if (QFile::copy(m_pendingFile, dst))
        finishInput((QStringLiteral("added to ") + cluster + "/" + playlist).toUtf8());
    else
        finishInput("failed: copy error (file may already exist)");
}

void CliPlayer::variantAddToFeatured()
{
    QDir featDir(QDir::homePath() + "/zmp_playlists/featured");
    QDir().mkpath(featDir.absolutePath());

    const QString fileName = QFileInfo(m_pendingFile).fileName();
    const QString dst = featDir.absoluteFilePath(fileName);

    if (!QFileInfo(dst).exists()) {
        if (QFile::copy(m_pendingFile, dst))
            finishInput(("added to featured: " + dst).toUtf8().constData());
        else
            finishInput("failed: copy error");
        return;
    }

    m_state = FmState::FeaturedConflict;
    printf("\nfile with same name is founded\n  1: rewrite\n  2: add _1 to name of this file\n");
    fflush(stdout);
}

void CliPlayer::variantDeleteSudo()
{
    const QString p = QDir::cleanPath(m_pendingFile);
    if (p == "/" || p == "/*") {
        printf("critical error: ZMP tryed to remove your filesystem exiting(error: 19)...\n");
        fflush(stdout);
        QCoreApplication::exit(19);
        return;
    }
    beginInput(FmState::SudoPassword, QStringLiteral("sudo password"));
}

void CliPlayer::onSudoPasswordEnter()
{
    QProcess proc;
    proc.start("sudo", {"-S", "rm", "-frv", m_pendingFile});
    proc.waitForStarted(5000);
    proc.write(m_sudoPassword.toUtf8() + "\n");
    proc.closeWriteChannel();
    proc.waitForFinished(15000);

    // Пароль сразу затирается из памяти
    std::memset(m_sudoPassword.data(), 0, static_cast<size_t>(m_sudoPassword.toUtf8().size()));
    m_sudoPassword.clear();

    if (proc.exitCode() == 0)
        finishInput("deleted (sudo)");
    else
        finishInput(("failed: " +
                     QString::fromUtf8(proc.readAllStandardError()).trimmed()).toUtf8());
}

// ---------------------------------------------------------------------------
//  Основной ввод — диспетчер по состояниям
// ---------------------------------------------------------------------------

static bool readByteTimeout(char &out, int ms)
{
    pollfd p{STDIN_FILENO, POLLIN, 0};
    if (poll(&p, 1, ms) != 1)
        return false;
    return ::read(STDIN_FILENO, &out, 1) == 1;
}

void CliPlayer::onStdin()
{
    char c;
    while (::read(STDIN_FILENO, &c, 1) == 1) {

        // Escape-последовательности (стрелки) и одиночный ESC
        if (static_cast<unsigned char>(c) == 0x1b) {
            char b = 0, code = 0;
            if (readByteTimeout(b, 120) && b == '[' && readByteTimeout(code, 120)) {
                if (code == 'A') {          // вверх
                    if (m_state == FmState::None) {
                        if (m_selected > 0) { --m_selected; redraw(); }
                    } else if (m_state == FmState::PathEntry ||
                               m_state == FmState::PlaylistEntry) {
                        hintMove(-1);
                    }
                } else if (code == 'B') {   // вниз
                    if (m_state == FmState::None) {
                        if (m_selected + 1 < m_queue.size()) { ++m_selected; redraw(); }
                    } else if (m_state == FmState::PathEntry ||
                               m_state == FmState::PlaylistEntry) {
                        hintMove(+1);
                    }
                }
                continue;
            }
            // одиночный ESC — отмена ввода
            if (m_state != FmState::None)
                finishInput(nullptr);
            continue;
        }

        if (m_state == FmState::None) {
            switch (static_cast<unsigned char>(c)) {
                case '\r': case '\n': playIndex(m_selected); break;
                case 0x02: togglePause(); break;
                case 0x01: startFileManager(); break;
                case 'n': nextTrack(); break;
                case 'p': prevTrack(); break;
                case 0x11: QCoreApplication::quit(); break;
                default: break;
            }
            continue;
        }

        // TAB — впечатать выбранную подсказку
        if (c == '\t' && (m_state == FmState::PathEntry ||
                          m_state == FmState::PlaylistEntry)) {
            hintAcceptTab();
            continue;
        }

        // Состояния с построчным вводом
        if (m_state == FmState::PathEntry || m_state == FmState::PlaylistEntry ||
            m_state == FmState::QueueNumber || m_state == FmState::SudoPassword) {

            if (c == '\r' || c == '\n') {
                printf("\n");
                fflush(stdout);
                if (m_state == FmState::PathEntry) onPathEnter();
                else if (m_state == FmState::PlaylistEntry) onPlaylistEnter();
                else if (m_state == FmState::QueueNumber) onQueueNumberEnter();
                else onSudoPasswordEnter();
                continue;
            }
            if (c == 0x7f || c == '\b') {
                inputBackspace();
            } else if (c >= 32 && c < 127) {
                inputAppendChar(QByteArray(&c, 1));
            } else if ((static_cast<unsigned char>(c) & 0xC0) == 0xC0 &&
                       m_state != FmState::SudoPassword) {
                // UTF-8 символ (не для пароля)
                int extra = (static_cast<unsigned char>(c) & 0xE0) == 0xE0 ? 2 :
                            (static_cast<unsigned char>(c) & 0xF8) == 0xF0 ? 3 : 1;
                QByteArray seq(&c, 1);
                for (int i = 0; i < extra; ++i) {
                    char cc;
                    if (::read(STDIN_FILENO, &cc, 1) == 1) seq.append(cc);
                }
                inputAppendChar(seq);
            } else if (m_state == FmState::SudoPassword && c >= 32) {
                m_sudoPassword.append(c);
            }

            // Живые подсказки (сброс выбора при изменении ввода)
            m_hintIndex = -1;
            if (m_state == FmState::PathEntry)
                printHints(m_inputLine.isEmpty() ? QStringList{"(type absolute or relative path)"}
                                                 : pathSuggestions(m_inputLine));
            else if (m_state == FmState::PlaylistEntry)
                printHints(clusterSuggestions(m_inputLine));
            else if (m_state == FmState::QueueNumber) {
                bool ok = false;
                const int num = m_inputLine.toInt(&ok);
                QStringList hint;
                if (!ok || num < 0)
                    hint << QStringLiteral("(position 0-%1)").arg(qMax(0, m_queue.size()));
                else if (num < m_queue.size())
                    hint << QStringLiteral("busy by: %1").arg(m_queue.at(num));
                else
                    hint << QStringLiteral("free position (will append)");
                printHints(hint);
            } else if (m_state == FmState::SudoPassword) {
                // пароль не эхом
            }
            continue;
        }

        // Одиночный выбор варианта
        if (m_state == FmState::VariantSelect) {
            switch (c) {
                case '1': variantAddToQueue(); break;
                case '2': variantAddToPlaylist(); break;
                case '3': variantAddToFeatured(); break;
                case '4':
                    QFile::remove(m_pendingFile);
                    finishInput("deleted");
                    break;
                case '5': variantDeleteSudo(); break;
                default:
                    finishInput(nullptr);
                    break;
            }
            continue;
        }

        if (m_state == FmState::ForceConfirm) {
            if (c == '1') {
                m_queue[m_pendingNum] = m_pendingFile;
                finishInput("track replaced");
            } else {
                finishInput("cancelled");
            }
            continue;
        }

        if (m_state == FmState::CreatePlaylistConfirm) {
            if (c == '2') {
                const QString input = m_pendingCluster;
                const int slash = input.indexOf('/');
                const QString cluster = (slash > 0) ? input.left(slash) : input;
                const QString playlist = (slash > 0) ? input.mid(slash + 1) : QStringLiteral("new");
                QDir().mkpath(QDir::homePath() + "/zmp_playlists/cls_" + cluster);
                QDir plDir(QDir::homePath() + "/zmp_playlists/cls_" + cluster + "/" + playlist);
                QDir().mkpath(plDir.absolutePath());
                const QString dst = plDir.absoluteFilePath(QFileInfo(m_pendingFile).fileName());
                if (QFile::copy(m_pendingFile, dst))
                    finishInput(("created and added to " + cluster + "/" + playlist).toUtf8());
                else
                    finishInput("failed: copy error");
            } else {
                finishInput("cancelled");
            }
            continue;
        }

        if (m_state == FmState::FeaturedConflict) {
            QDir featDir(QDir::homePath() + "/zmp_playlists/featured");
            const QFileInfo fi(m_pendingFile);
            if (c == '1') {
                QFile::remove(featDir.absoluteFilePath(fi.fileName()));
                QFile::copy(m_pendingFile, featDir.absoluteFilePath(fi.fileName()));
                finishInput("rewritten in featured");
            } else if (c == '2') {
                const QString base = fi.completeBaseName();
                const QString ext = fi.suffix().isEmpty() ? QString() : "." + fi.suffix();
                QString candidate = base + "_1" + ext;
                int n = 1;
                while (featDir.exists(candidate)) { ++n; candidate = base + "_" + QString::number(n) + ext; }
                QFile::copy(m_pendingFile, featDir.absoluteFilePath(candidate));
                finishInput(("added to featured: " + candidate).toUtf8());
            } else {
                finishInput(nullptr);
            }
            continue;
        }
    }
}

QString CliPlayer::statusLine() const
{
    if (m_pos < 0 || m_pos >= m_queue.size())
        return QStringLiteral("ZMP CLI VERSION");

    const QString path = m_queue.at(m_pos);
    QString meta;
#ifndef ZMP_NO_TAGLIB
    TagLib::FileRef fr(path.toUtf8().constData());
    if (!fr.isNull() && fr.tag()) {
        const QString artist = TStringToQString(fr.tag()->artist());
        const QString title = TStringToQString(fr.tag()->title());
        meta = QString(" (%1 - %2 - %3)").arg(artist, title).arg(fr.tag()->year());
    }
#endif

    qint64 percent = 0;
    const qint64 dur = m_am->duration();
    if (dur > 0)
        percent = qMin<qint64>(100, m_am->position() * 100 / dur);

    if (!m_playlistLabel.isEmpty())
        return QString("cls_%1/%2%3 %4%")
            .arg(m_playlistLabel, QFileInfo(path).fileName(), meta).arg(percent);
    return QString("ZMP CLI VERSION playing %1 %2%").arg(path).arg(percent);
}

void CliPlayer::redraw()
{
    printf("\033[H\033[2J");
    printf("%s\n", statusLine().toUtf8().constData());
    printf("--- queue ---\n");
    for (int i = 0; i < m_queue.size(); ++i) {
        qint64 secs = 0;
#ifndef ZMP_NO_TAGLIB
        TagLib::FileRef fr(m_queue.at(i).toUtf8().constData());
        if (!fr.isNull() && fr.audioProperties())
            secs = fr.audioProperties()->lengthInSeconds();
#endif
        char line[1024];
        snprintf(line, sizeof(line), "%d %s %.3fmin", i,
                 QFileInfo(m_queue.at(i)).fileName().toUtf8().constData(),
                 secs / 60.0);
        if (i == m_selected)      printf("\033[34m> %s\033[0m\n", line);
        else if (i == m_pos && !m_paused) printf("* %s\n", line);
        else                      printf("  %s\n", line);
    }
    printf("[Ctrl+A file manager | Ctrl+B pause | n next | p prev | arrows+Enter select | Ctrl+C exit]\n");
    fflush(stdout);
}

void CliPlayer::onPositionTimer()
{
    if (m_state == FmState::None)
        redraw();   // во время ввода не мигаем перерисовкой
}
