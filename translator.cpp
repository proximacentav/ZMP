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

    // ------------------------------ jamendo --------------------------------
    {"Jamendo", "Jamendo", "Jamendo"},
    {"Jamendo — поиск музыки", "Jamendo — music search", "Jamendo — Musikalische Suche"},
    {"Настройка Jamendo", "Jamendo Setup", "Jamendo Setup"},
    {"API ключ Jamendo:", "Jamendo API key:", "Jamendo-API-Schlüssel:"},
    {"Введите ваш Client ID с api.jamendo.com", "Enter your Client ID from api.jamendo.com", "Geben Sie Ihre Client-ID von api.jamendo.com ein"},
    {"Тип прокси:", "Proxy type:", "Proxy-Typ:"},
    {"Без прокси", "No proxy", "Kein Proxy"},
    {"IP:Порт прокси:", "Proxy IP:Port:", "Proxy-IP:Port:"},
    {"Требовать авторизацию", "Require authentication", "Authentifizierung erforderlich"},
    {"Имя пользователя:", "Username:", "Benutzername:"},
    {"Пароль:", "Password:", "Passwort:"},
    {"Разрешить SSL сертификаты (включая самоподписанные)", "Allow SSL certificates (including self-signed)", "SSL-Zertifikate erlauben (auch selbstsignierte)"},
    {"DNS запросы через прокси", "DNS queries through proxy", "DNS-Abfragen über Proxy"},
    {"Настроить DNS", "Configure DNS", "DNS konfigurieren"},
    {"IP адрес DNS сервера", "DNS server IP address", "DNS-Server-IP-Adresse"},
    {"Введите API ключ Jamendo", "Enter Jamendo API key", "Jamendo-API-Schlüssel eingeben"},
    {"Введите IP:Порт прокси", "Enter proxy IP:Port", "Proxy-IP:Port eingeben"},
    {"Jamendo не настроен. Перезайдите во вкладку для настройки.", "Jamendo is not configured. Re-enter the tab to configure.", "Jamendo ist nicht konfiguriert. Gehen Sie erneut auf die Registerkarte, um zu konfigurieren."},
    {"Поиск...", "Search...", "Suche..."},
    {"Название", "Name", "Name"},
    {"Исполнитель", "Artist", "Interpret"},
    {"Альбом", "Album", "Album"},
    {"Все", "All", "Alle"},
    {"Поиск", "Search", "Suche"},
    {"Найдено: %1", "Found: %1", "Gefunden: %1"},
    {"Ошибка: ", "Error: ", "Fehler: "},
    {"Ошибка парсинга ответа", "Response parse error", "Antwort-Parsing-Fehler"},
    {"URL аудио недоступен", "Audio URL unavailable", "Audio-URL nicht verfügbar"},
    {"Добавить трек", "Add track", "Track hinzufügen"},
    {"Выберите кластер или очередь воспроизведения:", "Select a cluster or play queue:", "Cluster oder Wiedergabeliste auswählen:"},
    {"Очередь воспроизведения", "Play queue", "Wiedergabewarteschlange"},
    {"Выберите плейлист", "Select playlist", "Playlist auswählen"},
    {"Выберите плейлист в кластере \"%1\":", "Select a playlist in cluster \"%1\":", "Playlist im Cluster \"%1\" auswählen:"},
    {"Создать новый плейлист", "Create new playlist", "Neue Playlist erstellen"},
    {"Новый плейлист", "New playlist", "Neue Playlist"},
    {"Введите название плейлиста:", "Enter playlist name:", "Playlist-Namen eingeben:"},
    {"Очистить кэш Jamendo", "Clear Jamendo cache", "Jamendo-Cache leeren"},
    {"Очистить кэш Jamendo?", "Clear Jamendo cache?", "Jamendo-Cache leeren?"},
    {"Хотите ли вы очистить кэш", "Do you want to clear the cache", "Möchten Sie den Cache leeren"},
    {"Б", "B", "B"},
    {"КБ", "KB", "KB"},
    {"МБ", "MB", "MB"},
    {"ГБ", "GB", "GB"},
    {"ТБ", "TB", "TB"},
    {"Jamendo перенастройка", "Jamendo reconfigure", "Jamendo neu konfigurieren"},
    {"без прокси", "no proxy", "kein Proxy"},
    {"Загрузка из Jamendo...", "Downloading from Jamendo...", "Lade von Jamendo herunter..."},
    {"Кластер:", "Cluster:", "Cluster:"},
    {"Плейлист:", "Playlist:", "Playlist:"},
    {"поиск", "search", "Suche"},

    // ------------------------------ new (language selector) -------------------
    {"Язык:", "Language:", "Sprache:"},

    // ------------------------------ dependencies (depsmanager) ----------------
    {"Настройка прокси установки", "Install proxy setup", "Proxy-Einrichtung für Installation"},
    {"Библиотеки интерфейса приложения", "Application UI libraries", "Oberflächenbibliotheken der Anwendung"},
        {"Скорость и питч аудио (libSoundTouch.so)", "Audio speed and pitch (libSoundTouch.so)", "Audio-Tempo und -Pitch (libSoundTouch.so)"},
    {"Чтение тегов и обложек (libtag.so)", "Tag and cover reading (libtag.so)", "Tags und Cover lesen (libtag.so)"},
    {"Визуализация projectM (libprojectM.so)", "projectM visualization (libprojectM.so)", "projectM-Visualisierung (libprojectM.so)"},
    {"Определение битрейта и транскодирование (ffmpeg, ffprobe)", "Bitrate detection and transcoding (ffmpeg, ffprobe)", "Bitrate-Erkennung und Transkodierung (ffmpeg, ffprobe)"},
    {"Загрузка файлов по сети Windows (smbclient)", "Windows network file download (smbclient)", "Dateien über Windows-Netzwerk laden (smbclient)"},
    {"Проверка зависимостей", "Dependency check", "Abhängigkeiten prüfen"},
    {"не определён", "not detected", "nicht erkannt"},
    {"Проверка...", "Checking...", "Prüfung läuft..."},
    {"Доустановить", "Install missing", "Fehlende installieren"},
    {"найдено", "found", "gefunden"},
    {"отсутствует", "missing", "fehlt"},
    {"не удалось проверить", "could not be verified", "konnte nicht überprüft werden"},
    {"Не удалось проверить элементов:", "Could not verify items:", "Konnte nicht geprüft werden:"},
    {"Отметьте нужные для установки.", "Check the ones to install.", "Gewünschte zur Installation ankreuzen."},
    {"Отсутствующих зависимостей:", "Missing dependencies:", "Fehlende Abhängigkeiten:"},
    {"Все зависимости найдены", "All dependencies found", "Alle Abhängigkeiten gefunden"},
    {"Установка зависимостей", "Installing dependencies", "Abhängigkeiten werden installiert"},
    {"Ничего не выбрано для установки", "Nothing selected for installation", "Nichts zur Installation ausgewählt"},
    {"ожидание...", "waiting...", "warten..."},
    {"подготовка...", "preparing...", "vorbereiten..."},
    {"Успешно установлено:", "Successfully installed:", "Erfolgreich installiert:"},
    {"Не удалось установить:", "Failed to install:", "Installation fehlgeschlagen:"},
    {"Готово", "Done", "Fertig"},
    {"Установка завершилась с ошибками", "Installation finished with errors", "Installation mit Fehlern beendet"},
    {"ОС:", "OS:", "Betriebssystem:"},
    {"Пакетный менеджер:", "Package manager:", "Paketmanager:"},
    {"Не удалось запустить команду", "Failed to start command", "Befehl konnte nicht gestartet werden"},
    {"Таймаут выполнения", "Execution timeout", "Timeout bei Ausführung"},

    // ------------------------------ partitions (partitionsdialog) -------------
    {"Разделы", "Partitions", "Partitionen"},
    {"Диски и разделы (lsblk):", "Disks and partitions (lsblk):", "Festplatten und Partitionen (lsblk):"},
    {"Обновить", "Refresh", "Aktualisieren"},
    {"примонтирован в ", "mounted at ", "eingebunden in "},
    {"Перейти в точку монтирования", "Go to mount point", "Zum Einhängepunkt gehen"},
    {"ОТКЛЮЧИТЬ", "UNMOUNT", "AUSWERFEN"},
    {"не примонтирован", "not mounted", "nicht eingebunden"},
    {"Подключить", "Mount", "Einhängen"},
    {"Подключить раздел", "Mount partition", "Partition einhängen"},
    {"Точка монтирования:", "Mount point:", "Einhängepunkt:"},
    {"Нет директории", "No directory", "Verzeichnis fehlt"},
    {"Директории для монтирования нет:", "The mount directory does not exist:", "Das Einhängeverzeichnis existiert nicht:"},
    {"Создать (--mkdir)", "Create (--mkdir)", "Erstellen (--mkdir)"},
    {"Системный раздел", "System partition", "Systempartition"},
    {"Этот диск возможно системный раздел. Демонтировать?", "This may be a system partition. Unmount it?", "Dies könnte eine Systempartition sein. Aushängen?"},
    {"Отключить", "Unmount", "Aushängen"},
    {"Диск используется", "Disk is in use", "Festplatte wird verwendet"},
    {"Этот диск сейчас используется. Демонтирование опасно. Продолжить?", "This disk is currently in use. Unmounting is dangerous. Continue?", "Diese Festplatte wird derzeit verwendet. Aushängen ist gefährlich. Fortfahren?"},
    {"Продолжить", "Continue", "Fortfahren"},
    {"Демонтировано:", "Unmounted:", "Ausgehängt:"},
    {"Демонтировано (lazy):", "Unmounted (lazy):", "Ausgehängt (lazy):"},
    {"Не удалось демонтировать:", "Failed to unmount:", "Aushängen fehlgeschlagen:"},
    {"Ошибка монтирования:", "Mount error:", "Einhängefehler:"},
    {"неизвестная ошибка", "unknown error", "unbekannter Fehler"},
    {"lsblk не вернул данных", "lsblk returned no data", "lsblk lieferte keine Daten"},

    // ------------------------------ about dialog ------------------------------
    {"О ZMP etc legal", "About ZMP etc legal", "Über ZMP etc legal"},
    {"от proximacentav:", "by proximacentav:", "von proximacentav:"},
    {"Не удалось загрузить лицензию", "Failed to load license", "Lizenz konnte nicht geladen werden"},

    // ------------------------------ offline mode / startup banner -------------
    {"Автономный режим", "Offline mode", "Offline-Modus"},
    {"Не хватает зависимостей", "Missing dependencies", "Fehlende Abhängigkeiten"},
    {"Игнорировать", "Ignore", "Ignorieren"},
    {"См. далее", "See details", "Weitere Details"},

    // ------------------------------ audio modes / device manager --------------
    {"вывод в динамики", "output to speakers", "Ausgabe auf Lautsprecher"},
    {"вывод в микрофон (перехват)", "output to microphone (intercept)", "Ausgabe auf Mikrofon (Abfangen)"},
    {"виртуальный микрофон", "virtual microphone", "virtuelles Mikrofon"},
    {"Менеджер устройств", "Device manager", "Geräteverwaltung"},
    {"Микрофон для перехвата:", "Microphone to intercept:", "Abzufangendes Mikrofon:"},
    {"Название виртуального микрофона:", "Virtual microphone name:", "Name des virtuellen Mikrofons:"},
    {"Например: ZMP Music Mic", "e.g. ZMP Music Mic", "z.B. ZMP Music Mic"},
    {"Создать", "Create", "Erstellen"},
    {"Категория:", "Category:", "Kategorie:"},
    {"Ввод", "Input", "Eingabe"},
    {"Вывод", "Output", "Ausgabe"},
    {"Уничтожить", "Destroy", "Vernichten"},
    {"Раскулачить", "Manage", "Verwalten"},
    {"Выберите устройство в списке", "Select a device in the list", "Wählen Sie ein Gerät in der Liste"},
    {"Уничтожено устройство", "Destroyed device", "Gerät vernichtet"},
    {"Не удалось уничтожить (аппаратные устройства защищены системой)",
     "Failed to destroy (hardware devices are protected by the system)",
     "Vernichten fehlgeschlagen (Hardware-Geräte sind systemgeschützt)"},
    {"Отключить", "Disable", "Deaktivieren"},
    {"Включить", "Enable", "Aktivieren"},
    {"Изменить работу", "Swap role", "Funktion ändern"},
    {"Назначить стандартным", "Set as default", "Als Standard festlegen"},
    {"Освободить", "Release", "Freigeben"},
    {"Работа восстановлена (частично — модули loopback остаются до перезапуска)",
     "Role restored (partially — loopback modules remain until restart)",
     "Funktion wiederhergestellt (teilweise — Loopback-Module bleiben bis zum Neustart)"},
    {"Роль устройства изменена", "Device role changed", "Gerätefunktion geändert"},
    {"Отключено: профиль off для карты", "Disabled: off profile for card", "Deaktiviert: Off-Profil für Karte"},
    {"Не удалось отключить (виртуальные устройства не отключаются)",
     "Failed to disable (virtual devices cannot be disabled)",
     "Deaktivieren fehlgeschlagen (virtuelle Geräte können nicht deaktiviert werden)"},
    {"Включено: базовый профиль карты", "Enabled: base card profile", "Aktiviert: Basiprofil der Karte"},
    {"Не удалось включить", "Failed to enable", "Aktivieren fehlgeschlagen"},
    {"Назначено стандартным", "Set as default:", "Als Standard festgelegt:"},
    {"Не удалось назначить стандартным", "Failed to set as default", "Konnte nicht als Standard festgelegt werden"},
    {"Процессы, использующие устройство, не найдены",
     "No processes using the device were found",
     "Keine Prozesse gefunden, die das Gerät verwenden"},
    {"Завершены процессы:", "Terminated processes:", "Beendete Prozesse:"},
    {"Угроза самоуничтожения", "Self-destruct threat", "Selbstzerstörungsgefahr"},
    {"Устройство используется самим ZMP. Всё равно снять процесс?",
     "The device is used by ZMP itself. Kill the process anyway?",
     "Das Gerät wird von ZMP selbst verwendet. Prozess trotzdem beenden?"},
    {"Завершить процесс", "Kill process", "Prozess beenden"},
    {"Не удалось загрузить модуль", "Failed to load module", "Modul konnte nicht geladen werden"},
    {"[карта, возможно выключена]", "[card, possibly disabled]", "[Karte, evtl. deaktiviert]"},
    {"Карту (аппаратное устройство) уничтожить нельзя — только отключить",
     "A card (hardware device) cannot be destroyed — only disabled",
     "Karte (Hardware-Gerät) kann nicht vernichtet — nur deaktiviert werden"},
    {"Использованы библиотеки:", "Libraries used:", "Verwendete Bibliotheken:"},

    // ------------------------------ queue summary -----------------------------
    {"очередь проигрывания завершена\nбыло проиграно %1 файлов из них mp3: %2 файлов, FLAC: %3 файлов, ALAC: %4 файлов, WAV: %5 файлов, другие форматы: %6 файлов.\nсредняя продолжительность %7 минут",
     "playback queue finished\n%1 files were played, including mp3: %2 files, FLAC: %3 files, ALAC: %4 files, WAV: %5 files, other formats: %6 files.\naverage duration %7 minutes",
     "Wiedergabe-Warteschlange beendet\n%1 Dateien wurden abgespielt, davon mp3: %2 Dateien, FLAC: %3 Dateien, ALAC: %4 Dateien, WAV: %5 Dateien, andere Formate: %6 Dateien.\ndurchschnittliche Dauer %7 Minuten"},
    {"Очередь", "Queue", "Warteschlange"},

    // ------------------------------ ZMP PC install ----------------------------
    {"Установите ZMP как приложение для ПК", "Install ZMP as a desktop application",
     "Installieren Sie ZMP als Desktop-Anwendung"},
    {"Установить", "Install", "Installieren"},
    {"Установка ZMP", "ZMP installation", "ZMP-Installation"},
    {"Бинарный файл zmp будет добавлен в /usr/bin, а также в меню приложений",
     "The zmp binary will be added to /usr/bin and to the applications menu",
     "Die Binärdatei zmp wird zu /usr/bin und zum Anwendungsmenü hinzugefügt"},
    {"Сделать ZMP плеером по умолчанию для:", "Make ZMP the default player for:",
     "ZMP als Standard-Player für:"},
    {"Как назвать zmp для запуска из терминала и меню приложений:",
     "Name for running zmp from terminal and applications menu:",
     "Name für den Start von zmp im Terminal und im Anwendungsmenü:"},
    {"Выполнить", "Run", "Ausführen"},
    {"Не удалось скопировать бинарник в /usr/bin",
     "Failed to copy the binary to /usr/bin",
     "Kopieren der Binärdatei nach /usr/bin fehlgeschlagen"},
    {"Готово! Запуск:", "Done! Run:", "Fertig! Starten mit:"},
    {"из терминала или из меню приложений", "from terminal or from the applications menu",
     "im Terminal oder aus dem Anwendungsmenü"},
    {"Обновить локально", "Update locally", "Lokal aktualisieren"},
    {"Обновление ZMP", "ZMP update", "ZMP-Aktualisierung"},
    {"md5 хэш установщика (путь до zmp который запущен сейчас):",
     "md5 hash of the installer (path of the currently running zmp):",
     "md5-Hash des Installers (Pfad des aktuell laufenden zmp):"},
    {"md5 хэш установленной версии (обычно /usr/bin/zmp):",
     "md5 hash of the installed version (usually /usr/bin/zmp):",
     "md5-Hash der installierten Version (meist /usr/bin/zmp):"},
    {"файл не найден", "file not found", "Datei nicht gefunden"},
    {"Положение бокового меню:", "Side panel position:", "Position des Seitenbereichs:"},
    {"Слева", "Left", "Links"},
    {"Сверху", "Top", "Oben"},
    {"Справа", "Right", "Rechts"},
    {"Снизу", "Bottom", "Unten"},
    {"Права администратора", "Administrator rights", "Administratorrechte"},
    {"Права администратора запрашиваются автоматически (UAC)",
     "Administrator rights are requested automatically (UAC)",
     "Administratorrechte werden automatisch angefordert (UAC)"},
    {"Кажется md5 хэши совпадают. Это скорее всего значит что ничего после обновления не изменится и оно не требуется.",
     "It seems the md5 hashes match. This most likely means nothing will change after the update and it is not required.",
     "Es scheint, dass die md5-Hashes übereinstimmen. Das bedeutet höchstwahrscheinlich, dass sich nach dem Update nichts ändert und es nicht erforderlich ist."},
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
