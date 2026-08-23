#ifndef CLIPLAYER_H
#define CLIPLAYER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QSocketNotifier>

class AudioManager;

// Консольный плеер (zmp --cli): воспроизведение очереди без GUI,
// управление с клавиатуры и вывод статуса в терминал.
class CliPlayer : public QObject
{
    Q_OBJECT
public:
    explicit CliPlayer(QObject *parent = nullptr);
    ~CliPlayer() override;

    // files — очередь; playlistLabel — "кластер/плейлист" для заголовка (опц.)
    void run(const QStringList &files, const QString &playlistLabel);

private slots:
    void onStdin();
    void onPositionTimer();

private:
    enum class FmState {
        None,
        PathEntry,
        VariantSelect,
        QueueNumber,
        ForceConfirm,
        PlaylistEntry,
        CreatePlaylistConfirm,
        FeaturedConflict,
        SudoPassword
    };

    void playIndex(int idx);
    void nextTrack();
    void prevTrack();
    void togglePause();
    void redraw();
    QString statusLine() const;

    // Файловый менеджер (Ctrl+A), событийная машина состояний
    QStringList pathSuggestions(const QString &typed);
    QStringList clusterSuggestions(const QString &typed);
    void printHints(const QStringList &hints);
    void hintMove(int delta);
    void hintAcceptTab();
    void beginInput(FmState state, const QString &prompt);
    void inputAppendChar(const QByteArray &utf8char);
    void inputBackspace();
    void redrawInputLine();
    void finishInput(const char *message);
    void startFileManager();
    void onPathEnter();
    void variantAddToQueue();
    void onQueueNumberEnter();
    void variantAddToPlaylist();
    void onPlaylistEnter();
    void variantAddToFeatured();
    void variantDeleteSudo();
    void onSudoPasswordEnter();

    AudioManager *m_am;
    QStringList m_queue;
    int m_pos = -1;
    int m_selected = 0;
    bool m_paused = false;
    QString m_playlistLabel;
    QSocketNotifier *m_stdinNotifier = nullptr;
    QByteArray m_inputBuf;

    // состояние файлового менеджера
    FmState m_state = FmState::None;
    QString m_prompt;
    int m_promptLen = 0;
    int m_hintLines = 0;
    QString m_inputLine;
    QStringList m_currentHints;   // текущие подсказки (для стрелок и TAB)
    int m_hintIndex = -1;         // выбранная подсказка (синяя)
    QString m_pendingFile;
    QString m_pendingCluster;
    QString m_sudoPassword;
    int m_pendingNum = -1;
};

#endif // CLIPLAYER_H
