#include "jamendowidget.h"
#include "playlistswidget.h"
#include "translator.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QFileDialog>
#include <QNetworkProxy>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>

// ---------------------------------------------------------------------------
//  JamendoSetupDialog
// ---------------------------------------------------------------------------

JamendoSetupDialog::JamendoSetupDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(ztr("Настройка Jamendo"));
    setMinimumWidth(500);
    setModal(true);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // API Key
    mainLayout->addWidget(new QLabel(ztr("API ключ Jamendo:")));
    m_apiKeyEdit = new QLineEdit;
    m_apiKeyEdit->setPlaceholderText(ztr("Введите ваш Client ID с api.jamendo.com"));
    mainLayout->addWidget(m_apiKeyEdit);

    // Proxy type
    mainLayout->addWidget(new QLabel(ztr("Тип прокси:")));
    m_proxyTypeCombo = new QComboBox;
    m_proxyTypeCombo->addItem(ztr("Без прокси"), QString("none"));
    m_proxyTypeCombo->addItem("SOCKS5", QString("socks5"));
    m_proxyTypeCombo->addItem("HTTP", QString("http"));
    m_proxyTypeCombo->addItem("HTTPS", QString("https"));
    mainLayout->addWidget(m_proxyTypeCombo);

    // Proxy settings panel
    m_proxySettingsWidget = new QWidget;
    QVBoxLayout *proxyLayout = new QVBoxLayout(m_proxySettingsWidget);
    proxyLayout->setContentsMargins(0, 0, 0, 0);

    proxyLayout->addWidget(new QLabel(ztr("IP:Порт прокси:")));
    m_proxyHostEdit = new QLineEdit;
    m_proxyHostEdit->setPlaceholderText("127.0.0.1:1080");
    proxyLayout->addWidget(m_proxyHostEdit);

    m_authCheck = new QCheckBox(ztr("Требовать авторизацию"));
    proxyLayout->addWidget(m_authCheck);

    m_authWidget = new QWidget;
    QVBoxLayout *authLayout = new QVBoxLayout(m_authWidget);
    authLayout->setContentsMargins(0, 0, 0, 0);
    authLayout->addWidget(new QLabel(ztr("Имя пользователя:")));
    m_proxyUserEdit = new QLineEdit;
    authLayout->addWidget(m_proxyUserEdit);
    authLayout->addWidget(new QLabel(ztr("Пароль:")));
    m_proxyPassEdit = new QLineEdit;
    m_proxyPassEdit->setEchoMode(QLineEdit::Password);
    authLayout->addWidget(m_proxyPassEdit);
    m_authWidget->setVisible(false);
    proxyLayout->addWidget(m_authWidget);

    m_sslAllowCheck = new QCheckBox(ztr("Разрешить SSL сертификаты (включая самоподписанные)"));
    proxyLayout->addWidget(m_sslAllowCheck);

    m_dnsThroughCheck = new QCheckBox(ztr("DNS запросы через прокси"));
    proxyLayout->addWidget(m_dnsThroughCheck);

    m_dnsEnableCheck = new QCheckBox(ztr("Настроить DNS"));
    proxyLayout->addWidget(m_dnsEnableCheck);

    m_dnsWidget = new QWidget;
    QVBoxLayout *dnsLayout = new QVBoxLayout(m_dnsWidget);
    dnsLayout->setContentsMargins(0, 0, 0, 0);
    dnsLayout->addWidget(new QLabel("DNS1:"));
    m_dns1Edit = new QLineEdit;
    m_dns1Edit->setPlaceholderText(ztr("IP адрес DNS сервера"));
    dnsLayout->addWidget(m_dns1Edit);
    dnsLayout->addWidget(new QLabel("DNS2:"));
    m_dns2Edit = new QLineEdit;
    m_dns2Edit->setPlaceholderText(ztr("IP адрес DNS сервера"));
    dnsLayout->addWidget(m_dns2Edit);
    dnsLayout->addWidget(new QLabel("DNS3:"));
    m_dns3Edit = new QLineEdit;
    m_dns3Edit->setPlaceholderText(ztr("IP адрес DNS сервера"));
    dnsLayout->addWidget(m_dns3Edit);
    m_dnsWidget->setVisible(false);
    proxyLayout->addWidget(m_dnsWidget);

    m_proxySettingsWidget->setVisible(false);
    mainLayout->addWidget(m_proxySettingsWidget);

    mainLayout->addStretch();

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &JamendoSetupDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(m_proxyTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &JamendoSetupDialog::onProxyTypeChanged);
    connect(m_authCheck, &QCheckBox::toggled, this, &JamendoSetupDialog::onAuthToggled);
    connect(m_dnsEnableCheck, &QCheckBox::toggled, this, &JamendoSetupDialog::onDnsToggled);
}

void JamendoSetupDialog::onProxyTypeChanged(int index)
{
    QString type = m_proxyTypeCombo->itemData(index).toString();
    m_proxySettingsWidget->setVisible(type != "none");
}

void JamendoSetupDialog::onAuthToggled(bool checked)
{
    m_authWidget->setVisible(checked);
}

void JamendoSetupDialog::onDnsToggled(bool checked)
{
    m_dnsWidget->setVisible(checked);
}

void JamendoSetupDialog::onAccept()
{
    if (m_apiKeyEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, ztr("Ошибка"), ztr("Введите API ключ Jamendo"));
        return;
    }
    QString type = m_proxyTypeCombo->currentData().toString();
    if (type != "none" && m_proxyHostEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, ztr("Ошибка"), ztr("Введите IP:Порт прокси"));
        return;
    }
    accept();
}

QString JamendoSetupDialog::apiKey() const { return m_apiKeyEdit->text().trimmed(); }
QString JamendoSetupDialog::proxyType() const { return m_proxyTypeCombo->currentData().toString(); }
QString JamendoSetupDialog::proxyHost() const { return m_proxyHostEdit->text().trimmed(); }
bool JamendoSetupDialog::proxyAuth() const { return m_authCheck->isChecked(); }
QString JamendoSetupDialog::proxyUsername() const { return m_proxyUserEdit->text(); }
QString JamendoSetupDialog::proxyPassword() const { return m_proxyPassEdit->text(); }
bool JamendoSetupDialog::proxySslAllow() const { return m_sslAllowCheck->isChecked(); }
bool JamendoSetupDialog::proxyDnsThrough() const { return m_dnsThroughCheck->isChecked(); }
bool JamendoSetupDialog::dnsEnabled() const { return m_dnsEnableCheck->isChecked(); }
QString JamendoSetupDialog::dns1() const { return m_dns1Edit->text().trimmed(); }
QString JamendoSetupDialog::dns2() const { return m_dns2Edit->text().trimmed(); }
QString JamendoSetupDialog::dns3() const { return m_dns3Edit->text().trimmed(); }

bool JamendoSetupDialog::needsSetup()
{
    QJsonObject config = loadJamendoConfig();
    return config.isEmpty() || config["api_key"].toString().isEmpty();
}

QJsonObject JamendoSetupDialog::loadJamendoConfig()
{
    QString configPath = QDir::homePath() + "/zmp_playlists/config.json";
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly))
        return QJsonObject();

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject())
        return QJsonObject();

    QJsonObject root = doc.object();
    return root["jamendo"].toObject();
}

void JamendoSetupDialog::saveJamendoConfig(const QJsonObject &config)
{
    QString configPath = QDir::homePath() + "/zmp_playlists/config.json";
    QFile file(configPath);

    QJsonObject root;
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isObject())
            root = doc.object();
        file.close();
    }

    root["jamendo"] = config;

    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
    }
}

// ---------------------------------------------------------------------------
//  JamendoPlaylistSelectDialog
// ---------------------------------------------------------------------------

JamendoPlaylistSelectDialog::JamendoPlaylistSelectDialog(
    const QStringList &clusters, PlaylistProvider provider, QWidget *parent)
    : QDialog(parent), m_provider(provider), m_selectedCluster(), m_selectedPlaylist()
{
    setWindowTitle(ztr("Выберите куда скачать"));
    setMinimumWidth(380);
    setMinimumHeight(400);
    setModal(true);

    QVBoxLayout *layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel(ztr("Имя файла:")));
    m_filenameEdit = new QLineEdit;
    m_filenameEdit->setPlaceholderText(ztr("Название (без расширения)"));
    layout->addWidget(m_filenameEdit);

    layout->addWidget(new QLabel(ztr("Формат:")));
    m_formatCombo = new QComboBox;
    m_formatCombo->addItem("MP3", "mp3");
    m_formatCombo->addItem("FLAC", "flac");
    m_formatCombo->addItem("OGG", "ogg");
    m_formatCombo->addItem("WAV", "wav");
    layout->addWidget(m_formatCombo);

    m_headerLabel = new QLabel(ztr("Выберите кластер:"));
    m_headerLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    layout->addWidget(m_headerLabel);

    m_stack = new QStackedWidget;

    m_mainList = new QListWidget;
    m_mainList->setStyleSheet(
        "QListWidget { background: transparent; border: 1px solid #444; border-radius: 6px; }"
        "QListWidget::item { padding: 12px; border-bottom: 1px solid #333; font-size: 13px; }"
        "QListWidget::item:hover { background: rgba(255,255,255,0.05); }"
        "QListWidget::item:selected { background: rgba(42,130,218,0.3); }");
    QListWidgetItem *pathItem = new QListWidgetItem(ztr("Выбрать путь"));
    pathItem->setData(Qt::UserRole, QString("__path__"));
    m_mainList->addItem(pathItem);
    QListWidgetItem *queueItem = new QListWidgetItem(ztr("В очередь воспроизведения"));
    queueItem->setData(Qt::UserRole, QString("__queue__"));
    m_mainList->addItem(queueItem);
    for (const QString &c : clusters) {
        QListWidgetItem *item = new QListWidgetItem(c);
        item->setData(Qt::UserRole, QString("cluster:" + c));
        m_mainList->addItem(item);
    }
    m_stack->addWidget(m_mainList);

    QWidget *playlistPage = new QWidget;
    QVBoxLayout *plLayout = new QVBoxLayout(playlistPage);
    plLayout->setContentsMargins(0, 0, 0, 0);

    m_backBtn = new QPushButton(ztr("< Назад к кластерам"));
    m_backBtn->setStyleSheet(
        "QPushButton { background: transparent; color: palette(highlight); border: none; "
        "font-size: 13px; padding: 6px; text-align: left; }"
        "QPushButton:hover { color: white; }");
    plLayout->addWidget(m_backBtn);

    m_playlistList = new QListWidget;
    m_playlistList->setStyleSheet(
        "QListWidget { background: transparent; border: 1px solid #444; border-radius: 6px; }"
        "QListWidget::item { padding: 12px; border-bottom: 1px solid #333; font-size: 13px; }"
        "QListWidget::item:hover { background: rgba(255,255,255,0.05); }"
        "QListWidget::item:selected { background: rgba(42,130,218,0.3); }");
    plLayout->addWidget(m_playlistList, 1);

    m_stack->addWidget(playlistPage);

    layout->addWidget(m_stack, 1);

    QPushButton *cancelBtn = new QPushButton(ztr("Отмена"));
    cancelBtn->setStyleSheet(
        "QPushButton { background: transparent; color: white; border: 1px solid #555; "
        "border-radius: 6px; padding: 8px 20px; font-size: 14px; }"
        "QPushButton:hover { border-color: #aaa; }");
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);

    connect(m_mainList, &QListWidget::itemClicked, this, &JamendoPlaylistSelectDialog::onMainItemClicked);
    connect(m_playlistList, &QListWidget::itemClicked, this, &JamendoPlaylistSelectDialog::onPlaylistItemClicked);
    connect(m_backBtn, &QPushButton::clicked, this, &JamendoPlaylistSelectDialog::onBackClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void JamendoPlaylistSelectDialog::setPlaylists(const QStringList &playlists)
{
    m_playlistList->clear();
    QListWidgetItem *newItem = new QListWidgetItem(ztr("Создать новый плейлист"));
    newItem->setData(Qt::UserRole, QString("__new__"));
    m_playlistList->addItem(newItem);
    for (const QString &p : playlists) {
        QListWidgetItem *item = new QListWidgetItem(p);
        item->setData(Qt::UserRole, QString("pl:" + p));
        m_playlistList->addItem(item);
    }
}

void JamendoPlaylistSelectDialog::onMainItemClicked(QListWidgetItem *item)
{
    QString data = item->data(Qt::UserRole).toString();
    if (data == "__path__") {
        QString path = QFileDialog::getExistingDirectory(this, ztr("Выберите папку для сохранения"));
        if (!path.isEmpty()) {
            m_customPath = path;
            m_isCustomPath = true;
            m_isPlayQueue = false;
            m_selectedCluster.clear();
            m_selectedPlaylist.clear();
            accept();
        }
        return;
    }
    if (data == "__queue__") {
        m_isPlayQueue = true;
        m_isCustomPath = false;
        m_selectedCluster.clear();
        m_selectedPlaylist.clear();
        accept();
        return;
    }
    if (data.startsWith("cluster:")) {
        m_selectedCluster = data.mid(8);
        m_isPlayQueue = false;
        m_isCustomPath = false;
        m_headerLabel->setText(ztr("Выберите плейлист в кластере \"%1\":").
            arg(m_selectedCluster));
        if (m_provider) {
            setPlaylists(m_provider(m_selectedCluster));
        }
        m_stack->setCurrentIndex(1);
    }
}

void JamendoPlaylistSelectDialog::onPlaylistItemClicked(QListWidgetItem *item)
{
    QString data = item->data(Qt::UserRole).toString();
    m_isPlayQueue = false;
    m_isCustomPath = false;
    if (data == "__new__") {
        m_selectedPlaylist.clear();
    } else if (data.startsWith("pl:")) {
        m_selectedPlaylist = data.mid(3);
    }
    accept();
}

void JamendoPlaylistSelectDialog::onBackClicked()
{
    m_headerLabel->setText(ztr("Выберите кластер:"));
    m_stack->setCurrentIndex(0);
}

QString JamendoPlaylistSelectDialog::selectedCluster() const
{
    return m_selectedCluster;
}

QString JamendoPlaylistSelectDialog::selectedPlaylist() const
{
    return m_selectedPlaylist;
}

bool JamendoPlaylistSelectDialog::isCustomPath() const
{
    return m_isCustomPath;
}

QString JamendoPlaylistSelectDialog::customPath() const
{
    return m_customPath;
}

bool JamendoPlaylistSelectDialog::isPlayQueue() const
{
    return m_isPlayQueue;
}

QString JamendoPlaylistSelectDialog::customFilename() const
{
    QString name = m_filenameEdit->text().trimmed();
    if (name.isEmpty()) return QString();
    return name;
}

QString JamendoPlaylistSelectDialog::selectedFormat() const
{
    return m_formatCombo->currentData().toString();
}

// ---------------------------------------------------------------------------
//  JamendoSearchWidget
// ---------------------------------------------------------------------------

JamendoSearchWidget::JamendoSearchWidget(QWidget *parent)
    : QWidget(parent), m_nam(new QNetworkAccessManager(this))
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(15, 15, 15, 15);

    QLabel *titleLabel = new QLabel("api.jamendo.com — " + ztr("поиск"));
    titleLabel->setStyleSheet("font-size: 11px; color: #888;");
    layout->addWidget(titleLabel);

    // Search bar
    QHBoxLayout *searchLayout = new QHBoxLayout;

    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText(ztr("Поиск..."));
    searchLayout->addWidget(m_searchEdit, 1);

    m_searchTypeCombo = new QComboBox;
    m_searchTypeCombo->addItem(ztr("Название"), "name");
    m_searchTypeCombo->addItem(ztr("Исполнитель"), "artist");
    m_searchTypeCombo->addItem(ztr("Альбом"), "album");
    m_searchTypeCombo->addItem(ztr("Все"), "all");
    searchLayout->addWidget(m_searchTypeCombo);

    m_searchBtn = new QPushButton(ztr("Поиск"));
    m_searchBtn->setStyleSheet(
        "QPushButton { background: transparent; color: white; border: 1px solid palette(highlight); "
        "border-radius: 6px; padding: 8px 20px; font-size: 14px; }"
        "QPushButton:hover { background: rgba(42,130,218,0.2); }");
    searchLayout->addWidget(m_searchBtn);

    layout->addLayout(searchLayout);

    m_statusLabel = new QLabel;
    m_statusLabel->setStyleSheet("color: #888; font-size: 12px;");
    layout->addWidget(m_statusLabel);

    m_resultsList = new QListWidget;
    m_resultsList->setStyleSheet(
        "QListWidget { background: transparent; border: 1px solid #444; border-radius: 6px; }"
        "QListWidget::item { padding: 10px; border-bottom: 1px solid #333; }"
        "QListWidget::item:hover { background: rgba(255,255,255,0.05); }"
        "QListWidget::item:selected { background: rgba(42,130,218,0.3); }");
    layout->addWidget(m_resultsList, 1);

    connect(m_searchBtn, &QPushButton::clicked, this, &JamendoSearchWidget::onSearch);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &JamendoSearchWidget::onSearch);
    connect(m_resultsList, &QListWidget::itemClicked, this, &JamendoSearchWidget::onTrackClicked);

    loadConfigAndApplyProxy();
}

void JamendoSearchWidget::loadConfigAndApplyProxy()
{
    QJsonObject config = JamendoSetupDialog::loadJamendoConfig();
    m_apiKey = config["api_key"].toString();
    applyProxySettings(config);
}

void JamendoSearchWidget::applyProxySettings(const QJsonObject &config)
{
    QString type = config["proxy_type"].toString("none");

    if (type == "none") {
        m_nam->setProxy(QNetworkProxy::NoProxy);
    } else {
        QString hostPort = config["proxy_host"].toString();
        QStringList parts = hostPort.split(':');
        if (parts.size() == 2) {
            QString host = parts[0];
            int port = parts[1].toInt();

            QNetworkProxy::ProxyType qtType = QNetworkProxy::NoProxy;
            if (type == "socks5") qtType = QNetworkProxy::Socks5Proxy;
            else if (type == "http") qtType = QNetworkProxy::HttpProxy;
            else if (type == "https") qtType = QNetworkProxy::HttpProxy;

            QNetworkProxy proxy(qtType, host, port);

            if (config["proxy_auth"].toBool()) {
                proxy.setUser(config["proxy_username"].toString());
                proxy.setPassword(config["proxy_password"].toString());
            }

            m_nam->setProxy(proxy);
        }
    }

    // SSL certificate handling
    if (config["proxy_ssl_allow"].toBool()) {
        connect(m_nam, &QNetworkAccessManager::sslErrors,
                this, [](QNetworkReply *reply, const QList<QSslError> &errors) {
            Q_UNUSED(errors)
            reply->ignoreSslErrors();
        });
    }
}

void JamendoSearchWidget::onSearch()
{
    QString query = m_searchEdit->text().trimmed();
    if (query.isEmpty() || m_apiKey.isEmpty()) return;

    m_statusLabel->setText(ztr("Поиск..."));
    m_resultsList->clear();
    m_tracks.clear();

    QString searchType = m_searchTypeCombo->currentData().toString();

    QUrl url("https://api.jamendo.com/v3.0/tracks/");
    QUrlQuery params;
    params.addQueryItem("client_id", m_apiKey);
    params.addQueryItem("format", "json");
    params.addQueryItem("limit", "50");

    if (searchType == "name" || searchType == "all")
        params.addQueryItem("name", query);
    if (searchType == "artist" || searchType == "all")
        params.addQueryItem("artistname", query);
    if (searchType == "album" || searchType == "all")
        params.addQueryItem("albumname", query);

    url.setQuery(params);

    QNetworkRequest req(url);
    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onSearchReply(reply);
    });
}

void JamendoSearchWidget::onSearchReply(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        m_statusLabel->setText(ztr("Ошибка: ") + reply->errorString());
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        m_statusLabel->setText(ztr("Ошибка парсинга ответа"));
        return;
    }

    QJsonObject root = doc.object();
    QJsonArray results = root["results"].toArray();

    m_statusLabel->setText(ztr("Найдено: %1").arg(results.size()));

    for (const QJsonValue &val : results) {
        QJsonObject obj = val.toObject();
        JamendoTrack track;
        track.id = obj["id"].toString();
        track.name = obj["name"].toString();
        track.artistName = obj["artist_name"].toString();
        track.albumName = obj["album_name"].toString();
        track.duration = obj["duration"].toInt();
        track.audioUrl = obj["audio"].toString();
        track.imageUrl = obj["image"].toString();

        m_tracks.append(track);

        int mins = track.duration / 60;
        int secs = track.duration % 60;
        QString timeStr = QString("%1:%2").arg(mins).arg(secs, 2, 10, QChar('0'));

        QString display = QString("%1 — %2 [%3]")
            .arg(track.artistName, track.name, timeStr);
        if (!track.albumName.isEmpty())
            display += QString(" (%1)").arg(track.albumName);

        QListWidgetItem *item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, m_tracks.size() - 1);
        m_resultsList->addItem(item);
    }
}

void JamendoSearchWidget::onTrackClicked(QListWidgetItem *item)
{
    int idx = item->data(Qt::UserRole).toInt();
    if (idx < 0 || idx >= m_tracks.size()) return;

    const JamendoTrack &track = m_tracks[idx];

    if (track.audioUrl.isEmpty()) {
        QMessageBox::warning(this, ztr("Ошибка"), ztr("URL аудио недоступен"));
        return;
    }

    emit trackSelected(track.audioUrl, track.name, track.artistName);
}

// ---------------------------------------------------------------------------
//  JamendoWidget
// ---------------------------------------------------------------------------

JamendoWidget::JamendoWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_stack = new QStackedWidget;
    layout->addWidget(m_stack);

    m_placeholderLabel = new QLabel(ztr("Jamendo не настроен. Перезайдите во вкладку для настройки."));
    m_placeholderLabel->setAlignment(Qt::AlignCenter);
    m_placeholderLabel->setStyleSheet("color: #888; font-size: 16px;");
    m_stack->addWidget(m_placeholderLabel);

    m_searchWidget = new JamendoSearchWidget;
    m_stack->addWidget(m_searchWidget);

    connect(m_searchWidget, &JamendoSearchWidget::trackSelected,
            this, &JamendoWidget::trackSelected);

    m_stack->setCurrentWidget(m_placeholderLabel);
}

void JamendoWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    checkAndSetup();
}

void JamendoWidget::checkAndSetup()
{
    if (!JamendoSetupDialog::needsSetup()) {
        m_searchWidget->loadConfigAndApplyProxy();
        m_stack->setCurrentWidget(m_searchWidget);
        return;
    }

    JamendoSetupDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QJsonObject config;
        config["api_key"] = dialog.apiKey();
        config["proxy_type"] = dialog.proxyType();
        config["proxy_host"] = dialog.proxyHost();
        config["proxy_auth"] = dialog.proxyAuth();
        config["proxy_username"] = dialog.proxyUsername();
        config["proxy_password"] = dialog.proxyPassword();
        config["proxy_ssl_allow"] = dialog.proxySslAllow();
        config["proxy_dns_through"] = dialog.proxyDnsThrough();
        config["dns_enabled"] = dialog.dnsEnabled();
        config["dns1"] = dialog.dns1();
        config["dns2"] = dialog.dns2();
        config["dns3"] = dialog.dns3();
        JamendoSetupDialog::saveJamendoConfig(config);
        m_searchWidget->loadConfigAndApplyProxy();
        m_stack->setCurrentWidget(m_searchWidget);
    } else {
        m_stack->setCurrentWidget(m_placeholderLabel);
    }
}

void JamendoWidget::reconfigure()
{
    JamendoSetupDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QJsonObject config;
        config["api_key"] = dialog.apiKey();
        config["proxy_type"] = dialog.proxyType();
        config["proxy_host"] = dialog.proxyHost();
        config["proxy_auth"] = dialog.proxyAuth();
        config["proxy_username"] = dialog.proxyUsername();
        config["proxy_password"] = dialog.proxyPassword();
        config["proxy_ssl_allow"] = dialog.proxySslAllow();
        config["proxy_dns_through"] = dialog.proxyDnsThrough();
        config["dns_enabled"] = dialog.dnsEnabled();
        config["dns1"] = dialog.dns1();
        config["dns2"] = dialog.dns2();
        config["dns3"] = dialog.dns3();
        JamendoSetupDialog::saveJamendoConfig(config);
        m_searchWidget->loadConfigAndApplyProxy();
        m_stack->setCurrentWidget(m_searchWidget);
    }
}
