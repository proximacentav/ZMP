#include "translator.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>

// ---------------------------------------------------------------------------
//  Translation table. Key = exact Russian source string. Every user-facing
//  Russian literal wrapped with ztr(...) in the code must have a row here.
//  Identifiers, regexes and data tokens are intentionally NOT translated and
//  therefore not listed (see project notes).
// ---------------------------------------------------------------------------
namespace {
struct Tr { const char *ru; const char *en; const char *de; };

const Tr kTable[] = {
    // ------------------------------ audiomanager ------------------------------
    {"Не удалось загрузить файл", "Failed to load file", "Datei konnte nicht geladen werden"},
    {"Файл не выбран", "No file selected", "Keine Datei ausgewählt"},
    {"Больше файлов в очереди", "This is the last file in the queue", "Dies ist die letzte Datei in der Warteschlange"},
    {"Это первый файл в очереди", "This is the first file in the queue", "Dies ist die erste Datei in der Warteschlange"},
    {"По умолчанию", "Default", "Standard"},

    // ------------------------------ deviceswidget -----------------------------
    {"Устройство вывода звука:", "Audio output device:", "Audioausgabegerät:"},

    // ------------------------------ equalizerwidget ---------------------------
    {"Сбросить EQ", "Reset EQ", "EQ zurücksetzen"},
    {"Эхо", "Echo", "Echo"},

    // ------------------------------ fileswidget -------------------------------
    {"Загрузка...", "Loading...", "Wird geladen..."},
    {"=Меню", "=Menu", "=Menü"},
    {"Поиск по названию и расширению...", "Search by name and extension...", "Nach Name und Erweiterung suchen..."},
    {"Ошибка", "Error", "Fehler"},
    {"Нет доступа к ", "No access to ", "Kein Zugriff auf "},
    {"< Назад", "< Back", "< Zurück"},
    {"Отключиться от FTP/FTPS/SMB", "Disconnect from FTP/FTPS/SMB", "Von FTP/FTPS/SMB trennen"},
    {"Скрыть поиск", "Hide search", "Suche ausblenden"},
    {"Поиск", "Search", "Suche"},
    {"Подключение к FTPS", "Connect to FTPS", "Mit FTPS verbinden"},
    {"Подключение к SMB", "Connect to SMB", "Mit SMB verbinden"},
    {"Подключение к FTP", "Connect to FTP", "Mit FTP verbinden"},
    {"example.com или IP", "example.com or IP", "example.com oder IP"},
    {"Хост:", "Host:", "Host:"},
    {"Порт:", "Port:", "Port:"},
    {"Пользователь:", "User:", "Benutzer:"},
    {"Пароль:", "Password:", "Passwort:"},
    {"Ресурс (share):", "Share:", "Freigabe (Share):"},
    {"Хост не может быть пустым", "Host cannot be empty", "Host darf nicht leer sein"},
    {"Подключение к %1:%2...", "Connecting to %1:%2...", "Verbindung zu %1:%2..."},
    {"Загрузка списка файлов...", "Loading file list...", "Dateiliste wird geladen..."},
    {"Ошибка: ", "Error: ", "Fehler: "},
    {"Ошибка подключения", "Connection error", "Verbindungsfehler"},
    {"Отключено", "Disconnected", "Getrennt"},
    {"(пусто)", "(empty)", "(leer)"},
    {"Сохранение файла", "Save file", "Datei speichern"},
    {"Куда сохранить файл \"", "Where to save file \"", "Wohin die Datei speichern \""},
    {"Плейлист", "Playlist", "Playlist"},
    {"Директория", "Directory", "Verzeichnis"},
    {"Отмена", "Cancel", "Abbrechen"},
    {"Скачивание %1: %2...", "Downloading %1: %2...", "Herunterladen %1: %2..."},
    {"Выберите плейлист", "Select playlist", "Playlist auswählen"},
    {"Выберите плейлист для сохранения:", "Select a playlist to save to:", "Playlist zum Speichern auswählen:"},
    {"Выберите папку для сохранения", "Select a folder to save to", "Ordner zum Speichern auswählen"},
    {"Ошибка SSL сертификата", "SSL certificate error", "SSL-Zertifikatsfehler"},
    {"Сертификат сервера не действителен:", "Server certificate is not valid:", "Serverzertifikat ist ungültig:"},
    {"\n  Издатель: ", "\n  Issuer: ", "\n  Aussteller: "},
    {"  Владелец: ", "  Owner: ", "  Inhaber: "},
    {"  Срок действия: ", "  Valid until: ", "  Gültig bis: "},
    {"Разрешить", "Allow", "Zulassen"},
    {"Запретить", "Deny", "Ablehnen"},
    {"Ошибка: sudo не доступен", "Error: sudo is not available", "Fehler: sudo ist nicht verfügbar"},
    {"Ошибка: таймаут", "Error: timeout", "Fehler: Zeitüberschreitung"},
    {"Не удалось скопировать файл", "Failed to copy file", "Datei konnte nicht kopiert werden"},
    {"Неверный IP-адрес", "Invalid IP address", "Ungültige IP-Adresse"},
    {"IP-адрес не действителен. Продолжить попытку подключения?", "The IP address is not valid. Continue the connection attempt?", "Die IP-Adresse ist ungültig. Verbindungsversuch fortsetzen?"},
    {"Продолжить", "Continue", "Fortfahren"},
    {"Сервер не доступен", "Server is not available", "Server ist nicht erreichbar"},
    {"Скачивание", "Download", "Download"},
    {"Файл сохранен в %1", "File saved to %1", "Datei gespeichert unter %1"},
    {"Не удалось сохранить файл", "Failed to save file", "Datei konnte nicht gespeichert werden"},
    {"Ошибка скачивания", "Download error", "Download-Fehler"},

    // ------------------------------ mainwindow --------------------------------
    {"Медиаплеер", "Media Player", "Medienplayer"},
    {"Устройства", "Devices", "Geräte"},
    {"Плеер", "Player", "Player"},
    {"Плейлисты", "Playlists", "Playlists"},
    {"Файлы", "Files", "Dateien"},
    {"Эквалайзер", "Equalizer", "Equalizer"},
    {"Визуализация", "Visualization", "Visualisierung"},
    {"Параметры", "Settings", "Einstellungen"},
    {"Программа запущена", "Application started", "Anwendung gestartet"},
    {"Логи", "Logs", "Protokolle"},
    {"Сохранить", "Save", "Speichern"},
    {"Успех", "Success", "Erfolg"},
    {"Логи сохранены в ", "Logs saved to ", "Protokolle gespeichert unter "},
    {"Закрыть", "Close", "Schließen"},
    {"Права root", "Root privileges", "Root-Rechte"},
    {"Пароль root", "Root password", "Root-Passwort"},
    {"Режим root активирован", "Root mode activated", "Root-Modus aktiviert"},
    {"Неверный пароль root", "Incorrect root password", "Falsches Root-Passwort"},
    {"Пользователь", "User", "Benutzer"},
    {"Войти в режим root", "Enter root mode", "In den Root-Modus wechseln"},
    {"Режим root активен", "Root mode is active", "Root-Modus ist aktiv"},
    {"Выйти", "Exit", "Beenden"},
    {"нажмите ESC чтобы закрыть это меню\n для того чтобы открывать директории в root режиме\n вводите путь сверху, в виде дерева root режим работает не всегда ",
     "press ESC to close this menu\n to open directories in root mode\n enter the path at the top; in the tree view root mode does not always work ",
     "ESC drücken, um dieses Menü zu schließen\n um Verzeichnisse im Root-Modus zu öffnen,\n geben Sie den Pfad oben ein; in der Baumansicht funktioniert der Root-Modus nicht immer "},
    {"Выбор пресета", "Select preset", "Preset auswählen"},

    // ------------------------------ miniplayerbar -----------------------------
    {"Нет трека", "No track", "Kein Titel"},
    {"Неизвестно", "Unknown", "Unbekannt"},

    // -------------------------- playbackcontrolwidget -------------------------
    {"Очередь воспроизведения:", "Playback queue:", "Wiedergabewarteschlange:"},
    {"Внимание", "Warning", "Achtung"},
    {"Нет активного трека", "No active track", "Kein aktiver Titel"},
    {"Папка плейлистов не найдена", "Playlists folder not found", "Playlist-Ordner nicht gefunden"},
    {"Добавить в плейлист", "Add to playlist", "Zur Playlist hinzufügen"},
    {"Выберите категорию:", "Select a category:", "Kategorie auswählen:"},
    {"Без кластера", "No cluster", "Kein Cluster"},
    {"Далее", "Next", "Weiter"},
    {"Выберите плейлист:", "Select a playlist:", "Playlist auswählen:"},
    {"ОК", "OK", "OK"},
    {"Плейлисты в кластере \"%1\":", "Playlists in cluster \"%1\":", "Playlists im Cluster \"%1\":"},
    {"Файл перезаписан", "File overwritten", "Datei überschrieben"},
    {"Не удалось удалить старый файл", "Failed to delete the old file", "Alte Datei konnte nicht gelöscht werden"},
    {"Файл добавлен в плейлист", "File added to playlist", "Datei zur Playlist hinzugefügt"},
    {"Не удалось создать папку featured", "Failed to create featured folder", "Ordner \"featured\" konnte nicht erstellt werden"},
    {"Подтверждение удаления", "Confirm deletion", "Löschen bestätigen"},
    {"Вы уверены, что хотите удалить:\n", "Are you sure you want to delete:\n", "Möchten Sie wirklich löschen:\n"},
    {"Не удалось удалить файл", "Failed to delete file", "Datei konnte nicht gelöscht werden"},
    {"Подтверждение добавления", "Confirm adding", "Hinzufügen bestätigen"},
    {"Вы уверены, что хотите добавить:\n", "Are you sure you want to add:\n", "Möchten Sie wirklich hinzufügen:\n"},
    {"Файл добавлен в featured", "File added to featured", "Datei zu \"featured\" hinzugefügt"},
    {"Не удалось добавить файл", "Failed to add file", "Datei konnte nicht hinzugefügt werden"},

    // ------------------------------ playlistswidget ---------------------------
    {"(без кластера)", "(no cluster)", "(kein Cluster)"},
    {"Редактировать", "Edit", "Bearbeiten"},
    {"Кластеры", "Clusters", "Cluster"},
    {"+ кластер", "+ cluster", "+ Cluster"},
    {"ред. кластер", "edit cluster", "Cluster bearb."},
    {"по исполнителю", "By artist", "Nach Interpret"},
    {"Создать плейлист", "Create playlist", "Playlist erstellen"},
    {"Название:", "Name:", "Name:"},
    {"Файлы:", "Files:", "Dateien:"},
    {"Добавить файлы", "Add files", "Dateien hinzufügen"},
    {"Удалить выбранный", "Remove selected", "Ausgewählte entfernen"},
    {"Импорт", "Import", "Import"},
    {"Добавить в кластер:", "Add to cluster:", "Zum Cluster hinzufügen:"},
    {"Создать", "Create", "Erstellen"},
    {"Выберите аудиофайлы", "Select audio files", "Audiodateien auswählen"},
    {"Аудио (*.mp3 *.wav *.flac *.aac *.aiff)", "Audio (*.mp3 *.wav *.flac *.aac *.aiff)", "Audio (*.mp3 *.wav *.flac *.aac *.aiff)"},
    {"Выберите папку с аудиофайлами", "Select a folder with audio files", "Ordner mit Audiodateien auswählen"},
    {"Введите название", "Enter a name", "Namen eingeben"},
    {"Добавьте файлы", "Add files", "Fügen Sie Dateien hinzu"},
    {"Плейлист уже существует", "Playlist already exists", "Playlist existiert bereits"},
    {"Не удалось создать папку", "Failed to create folder", "Ordner konnte nicht erstellt werden"},
    {"Удалить плейлист", "Delete playlist", "Playlist löschen"},
    {"Введите название плейлиста для удаления:", "Enter the name of the playlist to delete:", "Namen der zu löschenden Playlist eingeben:"},
    {"Удалить", "Delete", "Löschen"},
    {"Плейлист не найден", "Playlist not found", "Playlist nicht gefunden"},
    {"Не удалось удалить", "Failed to delete", "Löschen fehlgeschlagen"},
    {"Редактировать плейлист", "Edit playlist", "Playlist bearbeiten"},
    {"Открыть", "Open", "Öffnen"},
    {"Редактирование плейлиста: ", "Editing playlist: ", "Playlist bearbeiten: "},
    {"Цвет полосы:", "Bar color:", "Balkenfarbe:"},
    {"Применить", "Apply", "Anwenden"},
    {"Встроенные цвета:", "Built-in colors:", "Integrierte Farben:"},
    {"Зеленый", "Green", "Grün"},
    {"Фиолетовый", "Purple", "Violett"},
    {"Красный", "Red", "Rot"},
    {"Синий", "Blue", "Blau"},
    {"Оранжевый", "Orange", "Orange"},
    {"Желтый", "Yellow", "Gelb"},
    {"Выбрать", "Choose", "Auswählen"},
    {"Кластер:", "Cluster:", "Cluster:"},
    {"Создать кластер", "Create cluster", "Cluster erstellen"},
    {"Название кластера:", "Cluster name:", "Cluster-Name:"},
    {"Введите название...", "Enter a name...", "Namen eingeben..."},
    {"Цвет:", "Color:", "Farbe:"},
    {"Добавить плейлисты:", "Add playlists:", "Playlists hinzufügen:"},
    {"+ добавить плейлист", "+ add playlist", "+ Playlist hinzufügen"},
    {"Введите название кластера", "Enter a cluster name", "Cluster-Namen eingeben"},
    {"Информация", "Information", "Information"},
    {"Нет доступных плейлистов для добавления", "No playlists available to add", "Keine Playlists zum Hinzufügen verfügbar"},
    {"Добавить плейлист", "Add playlist", "Playlist hinzufügen"},
    {"Добавить", "Add", "Hinzufügen"},
    {"Редактировать кластер: ", "Editing cluster: ", "Cluster bearbeiten: "},
    {"Цвет кластера:", "Cluster color:", "Cluster-Farbe:"},
    {"Плейлисты в кластере:", "Playlists in cluster:", "Playlists im Cluster:"},
    {"Настройка кластера \"по исполнителю\"", "\"By artist\" cluster setup", "Cluster \"Nach Interpret\" einrichten"},
    {"Выберите папки для сканирования аудиофайлов.\nБудут найдены все mp3, flac, wav, aac, aiff файлы,\nпрочитаны их теги исполнителей и созданы плейлисты\nв кластере \"по исполнителю\".",
     "Select folders to scan for audio files.\nAll mp3, flac, wav, aac and aiff files will be found,\ntheir artist tags read and playlists created\nin the \"By artist\" cluster.",
     "Wählen Sie Ordner zum Scannen nach Audiodateien.\nAlle mp3-, flac-, wav-, aac- und aiff-Dateien werden gefunden,\nihre Interpreten-Tags gelesen und Playlists erstellt\nim Cluster \"Nach Interpret\"."},
    {"Добавить папку", "Add folder", "Ordner hinzufügen"},
    {"Начать сканирование", "Start scan", "Scan starten"},
    {"Пропустить", "Skip", "Überspringen"},
    {"Добавьте хотя бы одну папку для сканирования", "Add at least one folder to scan", "Fügen Sie mindestens einen Ordner zum Scannen hinzu"},
    {"Сканирование...", "Scanning...", "Wird gescannt..."},
    {"Готово: найдено %1 файлов, создано %2 плейлистов", "Done: found %1 files, created %2 playlists", "Fertig: %1 Dateien gefunden, %2 Playlists erstellt"},
    {"Нет обложки", "No cover", "Kein Cover"},

    // ------------------------------ settingswidget ----------------------------
    {"Светлая тема", "Light theme", "Helles Design"},
    {"Тёмная тема", "Dark theme", "Dunkles Design"},
    {"О программе", "About", "Über"},
    {"version 1.4.0 (cluster&keybinds)\nby proximacentav..\nhttps://github.com/proximacentav/ZMP\nMIT license\nRELEASE\nтакже был использован projectM",
     "version 1.4.0 (cluster&keybinds)\nby proximacentav..\nhttps://github.com/proximacentav/ZMP\nMIT license\nRELEASE\nprojectM was also used",
     "version 1.4.0 (cluster&keybinds)\nby proximacentav..\nhttps://github.com/proximacentav/ZMP\nMIT license\nRELEASE\nprojectM wurde ebenfalls verwendet"},
    {"Максимальный битрейт (kbps):", "Maximum bitrate (kbps):", "Maximale Bitrate (kbps):"},
    {"0 или значение >1000 означает 'не ограничено'", "0 or a value >1000 means 'unlimited'", "0 oder ein Wert >1000 bedeutet 'unbegrenzt'"},
    {"Высота области метаданных (px):", "Metadata area height (px):", "Höhe des Metadatenbereichs (px):"},
    {"Размер иконок (px):", "Icon size (px):", "Symbolgröße (px):"},
    {"чувствительность спектрограммы:", "spectrogram sensitivity:", "Spektrogramm-Empfindlichkeit:"},
    {"частота обновления спектраграммы (FPS):", "spectrogram refresh rate (FPS):", "Spektrogramm-Bildrate (FPS):"},
    {"количество частот спектрограммы:", "number of spectrogram bands:", "Anzahl der Spektrogrammbänder:"},
    {"projectM пресет:", "projectM preset:", "projectM-Preset:"},
    {"Выбрать .milk файл", "Choose .milk file", ".milk-Datei auswählen"},
    {"Выберите .milk пресет", "Select a .milk preset", ".milk-Preset auswählen"},
    {"Зелёный", "Green", "Grün"},
    {"Коричневый", "Brown", "Braun"},
    {"Выйти из программы", "Exit the application", "Anwendung beenden"},
    {"Клавиши", "Keys", "Tasten"},
    {"Назад", "Back", "Zurück"},
    {"Назначение клавиш", "Key bindings", "Tastenbelegung"},
    {"Предыдущий трек", "Previous track", "Vorheriger Titel"},
    {"Следующий трек", "Next track", "Nächster Titel"},
    {"Снять с паузы/Запустить трек", "Resume/Play track", "Fortsetzen/Titel abspielen"},
    {"Пресет эквалайзера", "Equalizer preset", "Equalizer-Preset"},
    {"КЛАВИША", "KEY", "TASTE"},
    {"Нажмите клавишу...", "Press a key...", "Taste drücken..."},
    {"не назначено", "not assigned", "nicht zugewiesen"},

    // ------------------------------ visualizationwidget -----------------------
    {"Ничего", "None", "Keine"},
    {"Спектрограмма", "Spectrogram", "Spektrogramm"},

    // ------------------------------ new (language selector) -------------------
    {"Язык:", "Language:", "Sprache:"},
};
} // namespace

// ---------------------------------------------------------------------------

Translator::Translator(QObject *parent) : QObject(parent)
{
    buildTables();
}

Translator &Translator::instance()
{
    static Translator inst;
    return inst;
}

void Translator::buildTables()
{
    const int n = int(sizeof(kTable) / sizeof(kTable[0]));
    m_en.reserve(n);
    m_de.reserve(n);
    for (int i = 0; i < n; ++i) {
        const QString ru = QString::fromUtf8(kTable[i].ru);
        m_en.insert(ru, QString::fromUtf8(kTable[i].en));
        m_de.insert(ru, QString::fromUtf8(kTable[i].de));
    }
}

QString Translator::t(const QString &ru) const
{
    switch (m_language) {
    case English: {
        auto it = m_en.constFind(ru);
        return it != m_en.constEnd() ? it.value() : ru;
    }
    case German: {
        auto it = m_de.constFind(ru);
        if (it != m_de.constEnd())
            return it.value();
        auto e = m_en.constFind(ru); // fall back to English, then to the key
        return e != m_en.constEnd() ? e.value() : ru;
    }
    case Russian:
    default:
        return ru;
    }
}

QString Translator::codeFor(Language lang)
{
    switch (lang) {
    case Russian: return QStringLiteral("ru");
    case German:  return QStringLiteral("de");
    case English:
    default:      return QStringLiteral("en");
    }
}

Translator::Language Translator::fromCode(const QString &code)
{
    const QString c = code.trimmed().toLower();
    if (c.startsWith("ru")) return Russian;
    if (c.startsWith("de")) return German;
    return English;
}

QString Translator::nativeName(Language lang)
{
    switch (lang) {
    case Russian: return QString::fromUtf8("Русский");
    case German:  return QString::fromUtf8("Deutsch");
    case English:
    default:      return QStringLiteral("English");
    }
}

QString Translator::configFilePath()
{
    return QDir::homePath() + "/zmp_playlists/config.json";
}

void Translator::initFromConfigOrLocale()
{
    // 1) Explicit override saved in config.json
    QFile f(configFilePath());
    if (f.open(QIODevice::ReadOnly)) {
        const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
        f.close();
        if (root.contains("language")) {
            const QString code = root.value("language").toString();
            if (!code.isEmpty()) {
                m_language = fromCode(code);
                return;
            }
        }
    }

    // 2) Fall back to the system locale
    switch (QLocale::system().language()) {
    case QLocale::Russian: m_language = Russian; break;
    case QLocale::German:  m_language = German;  break;
    default:               m_language = English; break;
    }
}

void Translator::setLanguage(Language lang)
{
    const bool changed = (m_language != lang);
    m_language = lang;
    persist();
    if (changed)
        emit languageChanged();
}

void Translator::persist() const
{
    const QString dir = QDir::homePath() + "/zmp_playlists";
    QDir().mkpath(dir);
    const QString path = dir + "/config.json";

    // Read-modify-write so we never clobber keys owned by other features
    // (keyBindings, clusters, playlist colors, ...).
    QJsonObject root;
    QFile f(path);
    if (f.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        if (doc.isObject())
            root = doc.object();
        f.close();
    }

    root["language"] = codeFor(m_language);

    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        f.close();
    }
}

// --------------------------- retranslation helpers -------------------------

static void applyAndRegister(RetransList &reg, QWidget *w, const char *prop, const char *ru)
{
    auto fn = [w, prop, ru]() { w->setProperty(prop, ztr(ru)); };
    fn();
    reg.append(fn);
}

QLabel *ztrLabel(RetransList &reg, const char *ru, QWidget *parent)
{
    QLabel *l = new QLabel(parent);
    applyAndRegister(reg, l, "text", ru);
    return l;
}

QPushButton *ztrButton(RetransList &reg, const char *ru, QWidget *parent)
{
    QPushButton *b = new QPushButton(parent);
    applyAndRegister(reg, b, "text", ru);
    return b;
}

void ztrSetText(RetransList &reg, QWidget *w, const char *ru)
{
    applyAndRegister(reg, w, "text", ru);
}

void ztrSetTitle(RetransList &reg, QWidget *w, const char *ru)
{
    applyAndRegister(reg, w, "windowTitle", ru);
}

void ztrSetPlaceholder(RetransList &reg, QLineEdit *e, const char *ru)
{
    applyAndRegister(reg, e, "placeholderText", ru);
}

void ztrSetTooltip(RetransList &reg, QWidget *w, const char *ru)
{
    applyAndRegister(reg, w, "toolTip", ru);
}

void ztrRegister(RetransList &reg, std::function<void()> fn)
{
    fn();
    reg.append(fn);
}
