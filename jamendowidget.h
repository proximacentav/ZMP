#ifndef JAMENDOWIDGET_H
#define JAMENDOWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QStackedWidget>
#include <QShowEvent>
#include "translator.h"

struct JamendoTrack {
    QString id;
    QString name;
    QString artistName;
    QString albumName;
    int duration = 0;
    QString audioUrl;
    QString imageUrl;
};

class JamendoSetupDialog : public QDialog {
    Q_OBJECT
public:
    explicit JamendoSetupDialog(QWidget *parent = nullptr);
    QString apiKey() const;
    QString proxyType() const;
    QString proxyHost() const;
    bool proxyAuth() const;
    QString proxyUsername() const;
    QString proxyPassword() const;
    bool proxySslAllow() const;
    bool proxyDnsThrough() const;
    bool dnsEnabled() const;
    QString dns1() const;
    QString dns2() const;
    QString dns3() const;

    static bool needsSetup();
    static QJsonObject loadJamendoConfig();
    static void saveJamendoConfig(const QJsonObject &config);

private slots:
    void onProxyTypeChanged(int index);
    void onAuthToggled(bool checked);
    void onDnsToggled(bool checked);
    void onAccept();

private:
    QLineEdit *m_apiKeyEdit;
    QComboBox *m_proxyTypeCombo;
    QWidget *m_proxySettingsWidget;
    QLineEdit *m_proxyHostEdit;
    QCheckBox *m_authCheck;
    QWidget *m_authWidget;
    QLineEdit *m_proxyUserEdit;
    QLineEdit *m_proxyPassEdit;
    QCheckBox *m_sslAllowCheck;
    QCheckBox *m_dnsThroughCheck;
    QCheckBox *m_dnsEnableCheck;
    QWidget *m_dnsWidget;
    QLineEdit *m_dns1Edit;
    QLineEdit *m_dns2Edit;
    QLineEdit *m_dns3Edit;
};

class JamendoPlaylistSelectDialog : public QDialog {
    Q_OBJECT
public:
    using PlaylistProvider = std::function<QStringList(const QString &clusterName)>;

    explicit JamendoPlaylistSelectDialog(const QStringList &clusters,
                                          PlaylistProvider provider,
                                          QWidget *parent = nullptr);
    void setPlaylists(const QStringList &playlists);
    QString selectedCluster() const;
    QString selectedPlaylist() const;
    bool isCustomPath() const;
    QString customPath() const;
    bool isPlayQueue() const;
    QString customFilename() const;
    QString selectedFormat() const;

private slots:
    void onMainItemClicked(QListWidgetItem *item);
    void onPlaylistItemClicked(QListWidgetItem *item);
    void onBackClicked();

private:
    QStackedWidget *m_stack;
    QListWidget *m_mainList;
    QListWidget *m_playlistList;
    QPushButton *m_backBtn;
    QLabel *m_headerLabel;
    PlaylistProvider m_provider;
    QLineEdit *m_filenameEdit;
    QComboBox *m_formatCombo;
    QString m_selectedCluster;
    QString m_selectedPlaylist;
    QString m_customPath;
    bool m_isPlayQueue = false;
    bool m_isCustomPath = false;
};

class JamendoSearchWidget : public QWidget {
    Q_OBJECT
public:
    explicit JamendoSearchWidget(QWidget *parent = nullptr);
    void loadConfigAndApplyProxy();

signals:
    void trackSelected(const QString &audioUrl, const QString &title, const QString &artist);

private slots:
    void onSearch();
    void onSearchReply(QNetworkReply *reply);
    void onTrackClicked(QListWidgetItem *item);

private:
    QLineEdit *m_searchEdit;
    QComboBox *m_searchTypeCombo;
    QPushButton *m_searchBtn;
    QListWidget *m_resultsList;
    QLabel *m_statusLabel;
    QNetworkAccessManager *m_nam;
    QList<JamendoTrack> m_tracks;
    QString m_apiKey;

    void applyProxySettings(const QJsonObject &config);
};

class JamendoWidget : public QWidget {
    Q_OBJECT
public:
    explicit JamendoWidget(QWidget *parent = nullptr);
    void reconfigure();

signals:
    void trackSelected(const QString &audioUrl, const QString &title, const QString &artist);

protected:
    void showEvent(QShowEvent *event) override;

private:
    QStackedWidget *m_stack;
    JamendoSearchWidget *m_searchWidget;
    QLabel *m_placeholderLabel;
    void checkAndSetup();
};

#endif
