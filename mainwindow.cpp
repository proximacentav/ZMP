#include "mainwindow.h"
#include "aboutdialog.h"
#include "zmpinstaller.h"
#ifndef Q_OS_WIN
#include "mpriscontroller.h"
#endif
#include "translator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QDebug>
#include <QTimer>
#include <QInputDialog>
#include <QEvent>
#include <QKeyEvent>
#include <QListWidget>
#include <QProcess>
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QImage>
#include <QPixmap>
#include <QFile>
#include <QTextEdit>
#include <QTextStream>
#include <QFileDialog>
#include <QDateTime>
#include <QDir>
#include <QRegularExpression>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkProxy>
#include <QJsonDocument>
#include <QJsonObject>
#include <unistd.h>
#include <pwd.h>
#include <crypt.h>
#include <fcntl.h>
#include <QSocketNotifier>
#include <QSplashScreen>
#include <cerrno>
#include <QPainter>

// ---------------------------------------------------------------------------
//  Liquid glass индикатор выбранной вкладки: перетекает между позициями,
//  собираясь в шар с преломлениями по краям; пульсирует под музыку.
// ---------------------------------------------------------------------------
class LiquidIndicator : public QWidget
{
public:
    explicit LiquidIndicator(QWidget *parent) : QWidget(parent)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_AlwaysStackOnTop);
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, [this]{ tick(); });
        m_timer->start(16);
    }

    void setTarget(const QRectF &r)
    {
        if (!m_anim && r == m_target) return;
        m_from = currentRect();
        m_target = r;
        m_progress = 0.0;
        m_anim = true;
    }

    void setAudioLevel(qreal lvl) { m_audioTarget = qBound(0.0, lvl, 1.0); }

    // Спектр по частотам (уровни уже умножены на чувствительность)
    void setSpectrum(const QVector<float> &s)
    {
        constexpr int K = 48;
        QVector<float> target(K, 0.0f);
        if (!s.isEmpty()) {
            for (int i = 0; i < K; ++i) {
                const int idx = qBound(0, int(qreal(i) / K * s.size()), s.size() - 1);
                target[i] = qBound(0.0f, s.at(idx), 1.0f);
            }
        }
        if (m_spec.size() != K)
            m_spec.resize(K);
        for (int i = 0; i < K; ++i)
            m_spec[i] += (target[i] - m_spec[i]) * 0.35f;
        m_hasSpec = true;
        m_specDirty = true;
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const QRectF base = currentRect();
        // пульс под музыку вдоль меньшей стороны
        const qreal pulse = 1.0 + m_level * 0.10;
        QRectF r = base;
        if (r.width() >= r.height())
            r = scaledAboutCenter(r, pulse, 1.0 / pulse);
        else
            r = scaledAboutCenter(r, 1.0 / pulse, pulse);

        const qreal radius = qMin(10.0 + m_morph * (qMin(r.width(), r.height()) / 2.0 - 8.0),
                                  qMin(r.width(), r.height()) / 2.0);

        // --- волнистый контур: ПРЯМОУГОЛЬНАЯ база + шипы-спектрограмма ---
        const int P = 80;
        QPointF pts[P];
        const qreal halfW = r.width() / 2.0, halfH = r.height() / 2.0;
        const QPointF c0 = r.center();
        // нормали сторон (наружу)
        for (int k = 0; k < P; ++k) {
            const qreal t = qreal(k) / P;                 // 0..1 по периметру
            QPointF base, normal;
            if (t < 0.25) {                               // верх: слева -> направо
                base = QPointF(c0.x() - halfW + t / 0.25 * r.width(), c0.y() - halfH);
                normal = QPointF(0, -1);
            } else if (t < 0.5) {                          // право: сверху вниз
                base = QPointF(c0.x() + halfW,
                               c0.y() - halfH + (t - 0.25) / 0.25 * r.height());
                normal = QPointF(1, 0);
            } else if (t < 0.75) {                         // низ: справа налево
                base = QPointF(c0.x() + halfW - (t - 0.5) / 0.25 * r.width(),
                               c0.y() + halfH);
                normal = QPointF(0, 1);
            } else {                                       // лево: снизу вверх
                base = QPointF(c0.x() - halfW,
                               c0.y() + halfH - (t - 0.75) / 0.25 * r.height());
                normal = QPointF(-1, 0);
            }
            qreal band = 0;
            if (m_hasSpec && !m_spec.isEmpty()) {
                band = m_spec.at(qBound(0, int(t * m_spec.size()),
                                        m_spec.size() - 1));
            }
            const qreal ampK = (1.0 - m_morph * 0.85);
            // усиление: слабые уровни вытягиваются степенной кривой
            const qreal spike = std::pow(band, 0.6) * (6.0 + 26.0 * ampK) * m_levelBoost;
            pts[k] = base + normal * spike;
        }
        QPainterPath lens;
        lens.moveTo(pts[0]);
        for (int k = 0; k < P; ++k) {
            const QPointF &cur = pts[k];
            const QPointF &nxt = pts[(k + 1) % P];
            const QPointF mid((cur.x() + nxt.x()) / 2, (cur.y() + nxt.y()) / 2);
            lens.quadTo(cur, mid);
        }
        lens.closeSubpath();

        // стеклянное тело по волнистому контуру
        QLinearGradient body(r.topLeft(), r.bottomLeft());
        body.setColorAt(0.0, QColor(255, 255, 255, 90));
        body.setColorAt(0.45, QColor(255, 255, 255, 35));
        body.setColorAt(0.55, QColor(255, 255, 255, 45));
        body.setColorAt(1.0, QColor(255, 255, 255, 75));
        p.setPen(Qt::NoPen);
        p.setBrush(body);
        p.drawPath(lens);

        // преломления по бокам: яркие узкие блики у левой и правой граней
        const qreal edgeW = 3.0 + 3.0 * m_morph + 4.0 * m_level;
        const int edgeA = 150 + int(70 * m_morph) + int(60 * m_level);

        QLinearGradient leftr(r.left(), 0, r.left() + edgeW * 3, 0);
        leftr.setColorAt(0.0, QColor(255, 255, 255, edgeA));
        leftr.setColorAt(1.0, QColor(255, 255, 255, 0));
        p.setBrush(leftr);
        p.setClipPath(lens);
        p.drawRoundedRect(QRectF(r.left() + 1.5, r.top() + radius * 0.4,
                                 edgeW * 3, r.height() - radius * 0.8),
                          edgeW, edgeW);

        QLinearGradient rightr(r.right(), 0, r.right() - edgeW * 3, 0);
        rightr.setColorAt(0.0, QColor(255, 255, 255, edgeA));
        rightr.setColorAt(1.0, QColor(255, 255, 255, 0));
        p.setBrush(rightr);
        p.drawRoundedRect(QRectF(r.right() - 1.5 - edgeW * 3, r.top() + radius * 0.4,
                                 edgeW * 3, r.height() - radius * 0.8),
                          edgeW, edgeW);
        p.setClipRect(rect());

        // блик сверху
        QLinearGradient topg(r.topLeft(), QPointF(r.left(), r.top() + r.height() * 0.35));
        topg.setColorAt(0.0, QColor(255, 255, 255, 110));
        topg.setColorAt(1.0, QColor(255, 255, 255, 0));
        p.setBrush(topg);
        p.drawRoundedRect(QRectF(r.left() + edgeW, r.top() + 1,
                                 r.width() - edgeW * 2, r.height() * 0.3),
                          radius, radius);

        // мягкое свечение вокруг при морфинге
        if (m_morph > 0.02 || m_level > 0.02) {
            const int glow = int(50 * m_morph + 80 * m_level);
            QPen pen(QColor(255, 255, 255, glow), 2.0 + 2.0 * m_morph);
            p.setBrush(Qt::NoBrush);
            p.setPen(pen);
            p.drawRoundedRect(r.adjusted(-2, -2, 2, 2), radius + 2, radius + 2);
        }
    }

private:
    static QRectF scaledAboutCenter(const QRectF &r, qreal kx, qreal ky)
    {
        QPointF c = r.center();
        return QRectF(c.x() - r.width() * kx / 2, c.y() - r.height() * ky / 2,
                      r.width() * kx, r.height() * ky);
    }
    QRectF currentRect() const
    {
        const double e = easeOutCubic(qBound(0.0, m_progress, 1.0));
        QRectF r(m_from.left() + (m_target.left()-m_from.left())*e,
                 m_from.top()  + (m_target.top() -m_from.top() )*e,
                 m_from.width()  + (m_target.width() -m_from.width() )*e,
                 m_from.height() + (m_target.height()-m_from.height())*e);
        return r;
    }
    static double easeOutCubic(double t){ t=qBound(0.0,t,1.0); return 1-std::pow(1-t,3); }

    void tick()
    {
        // всегда занимаем всю панель меню (она меняет размер при layout)
        if (parentWidget() &&
            (size() != parentWidget()->size() || pos() != QPoint(0, 0))) {
            setGeometry(0, 0, parentWidget()->width(), parentWidget()->height());
            raise();
        }


        // затухание уровня звука
        m_level += (m_audioTarget - m_level) * 0.25;
        m_audioTarget *= 0.92;

        bool busy = false;
        if (m_anim) {
            m_progress += 0.09;
            const double e = easeOutCubic(qMin(1.0, m_progress));
            m_morph = std::sin(M_PI * qBound(0.0, m_progress, 1.0)); // шар на полпути
            if (m_progress >= 1.0) {
                m_progress = 1.0;
                m_morph = 0.0;
                m_anim = false;
                m_from = m_target;
            } else busy = true;
            update();
        }
        if (m_specDirty) { m_specDirty = false; update(); busy = true; }
        if (m_level > 0.005 || m_audioTarget > 0.005) { update(); busy = true; }
        else if (m_level != 0) { m_level = 0; update(); }
        if (!busy && !m_anim && m_morph == 0 && m_level == 0)
            update();
    }

    QTimer *m_timer = nullptr;
    QRectF m_from, m_target{10, 10, 180, 36};
    qreal m_progress = 1.0;
    qreal m_morph = 0.0;
    bool m_anim = false;
    qreal m_level = 0.0, m_audioTarget = 0.0;
    QVector<float> m_spec;
    bool m_hasSpec = false;
    bool m_specDirty = false;
public:
    qreal m_levelBoost = 1.0;
protected:
};

static MainWindow *g_mainWindow = nullptr;
static int readMenuSideFromConfig();
static int g_original_stderr = -1;
static QSplashScreen *g_splash = nullptr;
static QStringList g_splashLines;

void MainWindow::setSplash(QSplashScreen *splash)
{
    g_splash = splash;
}

static void qtMessageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    Q_UNUSED(ctx)
    if (g_mainWindow) {
        QString prefix;
        switch (type) {
            case QtDebugMsg: prefix = "DEBUG"; break;
            case QtWarningMsg: prefix = "WARN"; break;
            case QtCriticalMsg: prefix = "ERROR"; break;
            case QtFatalMsg: prefix = "FATAL"; break;
            case QtInfoMsg: prefix = "INFO"; break;
        }
        g_mainWindow->addLog(QString("[%1] %2").arg(prefix, msg));
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    ztrRegister(m_retrans, [this]{ setWindowTitle(ztr("Медиаплеер")); });
    resize(1200, 800);

    QWidget *central = new QWidget(this);
    m_mainLay = new QHBoxLayout(central);
    m_mainLay->setSpacing(0);
    m_mainLay->setContentsMargins(0,0,0,0);
    setCentralWidget(central);

    m_menuContainer = new QWidget(this);
    QWidget *menuContainer = m_menuContainer;
    menuContainer->setFixedWidth(200);
    menuContainer->setStyleSheet("background-color: #2b2b2b;");
    QVBoxLayout *menuContLayout = new QVBoxLayout(menuContainer);
    menuContLayout->setContentsMargins(5, 10, 5, 10);
    m_menuContLay = menuContLayout;

    m_menu = new QListWidget;
    for (int i = 0; i < 8; ++i) m_menu->addItem(QString());
    ztrRegister(m_retrans, [this]{
        static const char *items[8] = {"Устройства", "Плеер", "Плейлисты", "Jamendo", "Файлы", "Эквалайзер", "Визуализация", "Параметры"};
        for (int i = 0; i < 8 && i < m_menu->count(); ++i) m_menu->item(i)->setText(ztr(items[i]));
    });
    m_menu->setCurrentRow(0);

    m_menu->setSpacing(10);
    m_menu->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_menu->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_menu->setStyleSheet(
        "QListWidget { background: transparent; border: none; outline: none; }"
        "QListWidget::item { background: transparent; color: white; padding: 12px; border-radius: 8px; }"
        "QListWidget::item:selected { background: transparent; color: white; }"
    );
    menuContLayout->addWidget(m_menu, 1);

    // Кнопка сворачивания бокового меню ("<" спрятать, ">" показать)
    m_menuToggleBtn = new QPushButton("<", this);
    m_menuToggleBtn->setFixedSize(24, 22);
    m_menuToggleBtn->setStyleSheet(
        "QPushButton { background-color: #3a3a3a; color: white;"
        " border: 1px solid #555; border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #4a4a4a; }");
    connect(m_menuToggleBtn, &QPushButton::clicked, this, [this]() {
        m_menuCollapsed = !m_menuCollapsed;
        applyMenuGeometry();
    });


    m_userButton = new QPushButton(getCurrentUsername());
    m_userButton->setCursor(Qt::PointingHandCursor);
    updateUserButtonStyle();
    menuContLayout->addWidget(m_userButton);

    connect(m_userButton, &QPushButton::clicked, this, &MainWindow::onUserButtonClicked);

    m_logTimer = new QTimer(this);
    m_logTimer->start(10000);
    connect(m_logTimer, &QTimer::timeout, this, &MainWindow::autoSaveLogs);

    g_mainWindow = this;
    qInstallMessageHandler(qtMessageHandler);

    int pipefd[2];
    pipe(pipefd);
    g_original_stderr = dup(STDERR_FILENO);
    dup2(pipefd[1], STDERR_FILENO);
    ::close(pipefd[1]);
    // ОБА конца пайпа неблокирующие: иначе при всплеске логов (спам
    // PipeWire и т.п.) write() блокирует главный поток навсегда -> зависание
    fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
    fcntl(g_original_stderr, F_SETFL, O_NONBLOCK);

    QSocketNotifier *stderrNotifier = new QSocketNotifier(pipefd[0], QSocketNotifier::Read, this);
    connect(stderrNotifier, &QSocketNotifier::activated, this, [this](int fd) {
        char buf[4096];
        int n;
        while ((n = read(fd, buf, sizeof(buf) - 1)) > 0) {
            buf[n] = 0;
            QString msg = QString::fromUtf8(buf).trimmed();
            if (!msg.isEmpty())
                addLog(msg);
        }
    });

    addLog(ztr("Программа запущена"));

    // AudioManager должен существовать ДО connect'а спектра ниже
    m_audioManager = new AudioManager(this);

    m_liquid = new LiquidIndicator(menuContainer);
    m_liquid->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_liquid->setGeometry(0, 0, menuContainer->width(), 60);
    m_liquid->raise();

    // Пульс индикатора под музыку (спектр из AudioManager)
    connect(m_audioManager, &AudioManager::spectrumDataChanged, this,
            [this](const QVector<float> &levels, const QVector<double> &) {
        if (levels.isEmpty()) return;
        qreal sum = 0;
        for (float v : levels) sum += v;
        const qreal avg = sum / levels.size();
        m_liquid->setAudioLevel(qBound(0.0, avg * 6.0, 1.0));
        m_liquid->setSpectrum(levels);   // шипы по частотам 1Гц-20кГц
    });


    if (m_menu->item(0)) {
        updateLiquidTarget();
    }

    m_mainLay->addWidget(menuContainer);

    QWidget *rightContainer = new QWidget(this);
    m_rightContainer = rightContainer;
    QVBoxLayout *rightLayout = new QVBoxLayout(rightContainer);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    m_devicesWidget = new DevicesWidget(this);
    m_playerWidget = new PlayerWidget(m_audioManager, this);
    m_playlistsWidget = new PlaylistsWidget(this);
    m_jamendoWidget = new JamendoWidget(this);
    m_filesWidget = new FilesWidget(this);
    m_equalizerWidget = new EqualizerWidget(m_audioManager, this);
    m_visualizationWidget = new VisualizationWidget(m_audioManager, this);
    m_settingsWidget = new SettingsWidget(this);

    m_miniPlayerBar = new MiniPlayerBar(m_audioManager, this);
    rightLayout->addWidget(m_miniPlayerBar);

    // Banner above all tabs: shown while dependencies are being installed
    m_depsBanner = new QLabel(this);
    m_depsBanner->setStyleSheet(
        "background-color: #1b1b1b; color: white; padding: 5px 10px;"
        "border-bottom: 1px solid #444; font-weight: bold;");
    m_depsBanner->setTextFormat(Qt::RichText);
    m_depsBanner->setWordWrap(true);
    m_depsBanner->hide();
    rightLayout->addWidget(m_depsBanner);

    m_leftStack = new QStackedWidget;
    m_rightStack = new QStackedWidget;
    for (int i = 0; i < 8; ++i) {
        QWidget *wl = new QWidget;
        QVBoxLayout *ll = new QVBoxLayout(wl);
        ll->setContentsMargins(0,0,0,0);
        m_wrapLeft.append(wl);
        m_leftStack->addWidget(wl);

        QWidget *wr = new QWidget;
        QVBoxLayout *rl = new QVBoxLayout(wr);
        rl->setContentsMargins(0,0,0,0);
        m_wrapRight.append(wr);
        m_rightStack->addWidget(wr);
    }
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->addWidget(m_leftStack);
    m_splitter->addWidget(m_rightStack);
    m_splitter->setChildrenCollapsible(false);
    m_rightStack->setVisible(false);
    rightLayout->addWidget(m_splitter, 1);
    m_mainLay->addWidget(rightContainer, 1);

    DependencyManager *depsMgr = DependencyManager::instance();
    connect(depsMgr, &DependencyManager::installStarted, this, [this](int totalSteps) {
        updateDepsBanner(ztr("подготовка..."), 0, 0);
        m_depsBanner->show();
    });
    connect(depsMgr, &DependencyManager::installProgress,
            this, &MainWindow::updateDepsBanner);
    connect(depsMgr, &DependencyManager::installFinished, this,
            [this](bool success, const QStringList &succeeded, const QStringList &failed) {
        m_depsBanner->hide();

        // Summary is shown by DependencyCheckDialog if it's still open
        if (DependencyCheckDialog::s_active && DependencyCheckDialog::s_active->isVisible())
            return;

        QString msg;
        if (!succeeded.isEmpty())
            msg += ztr("Успешно установлено:") + "\n  " + succeeded.join("\n  ") + "\n";
        if (!failed.isEmpty()) {
            if (!msg.isEmpty()) msg += "\n";
            msg += ztr("Не удалось установить:") + "\n  " + failed.join("\n  ");
        }
        if (msg.isEmpty())
            msg = success ? ztr("Готово") : ztr("Установка завершилась с ошибками");

        QMessageBox::information(this, ztr("Установка зависимостей"), msg.trimmed());
    });

    // Автономный режим: разрываем активные сетевые подключения при включении
    connect(m_settingsWidget, &SettingsWidget::offlineModeChanged, this,
            [this](bool enabled) {
        if (enabled)
            m_filesWidget->disconnectFromServer();
    });

    createDepsWarningBanner(rightLayout);
    createInstallBanner(rightLayout);
    QTimer::singleShot(800, this, &MainWindow::checkDependenciesAtStartup);
    QTimer::singleShot(400, this, &MainWindow::checkInstallAtStartup);

    connect(m_playerWidget, &PlayerWidget::stateChanged, this, [this](bool playing) {
        if (playing) {
            const QStringList &pl = m_playerWidget->getCurrentPlaylist();
            if (!pl.isEmpty()) {
                m_playlistsWidget->onPlaylistPlaying(pl);
            }
        } else {
            m_playlistsWidget->onPlaylistStopped();
        }
    });

    connect(m_playerWidget, &PlayerWidget::featuredUpdated, this, &MainWindow::onFeaturedUpdated);

    connect(m_filesWidget, &FilesWidget::fileSelected, this, [this](const QString &path) {
        m_playerWidget->setPlaylist({path});
        m_playerWidget->setCurrentPlaylist({path});
        m_playerWidget->onPlay();
        m_menu->setCurrentRow(1);
    });

    m_pages.append(m_devicesWidget);
    m_pages.append(m_playerWidget);
    m_pages.append(m_playlistsWidget);
    m_pages.append(m_jamendoWidget);
    m_pages.append(m_filesWidget);
    m_pages.append(m_equalizerWidget);
    m_pages.append(m_visualizationWidget);
    m_pages.append(m_settingsWidget);

    // весь контент изначально в левых обёртках
    for (int i = 0; i < 8; ++i)
        m_wrapLeft[i]->layout()->addWidget(m_pages[i]);

    connect(m_menu, &QListWidget::currentRowChanged, this, [this](int row) {
        const bool shift =
            (QGuiApplication::queryKeyboardModifiers() & Qt::ShiftModifier);
        handleMenuSelection(row, shift);
    });

    connect(m_devicesWidget, &DevicesWidget::deviceChanged, this, &MainWindow::onDeviceChanged);
    connect(m_jamendoWidget, &JamendoWidget::trackSelected, this,
        [this](const QString &audioUrl, const QString &title, const QString &artist) {
        auto applyJamendoProxy = [](QNetworkAccessManager *nam) {
            QString configPath = QDir::homePath() + "/zmp_playlists/config.json";
            QFile f(configPath);
            if (!f.open(QIODevice::ReadOnly)) return;
            QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
            f.close();
            if (!doc.isObject()) return;
            QJsonObject jamendo = doc.object()["jamendo"].toObject();
            QString type = jamendo["proxy_type"].toString("none");
            if (type == "none") {
                nam->setProxy(QNetworkProxy::NoProxy);
            } else {
                QString hostPort = jamendo["proxy_host"].toString();
                QStringList parts = hostPort.split(':');
                if (parts.size() == 2) {
                    QNetworkProxy::ProxyType qtType = QNetworkProxy::NoProxy;
                    if (type == "socks5") qtType = QNetworkProxy::Socks5Proxy;
                    else if (type == "http" || type == "https") qtType = QNetworkProxy::HttpProxy;
                    QNetworkProxy proxy(qtType, parts[0], parts[1].toInt());
                    if (jamendo["proxy_auth"].toBool()) {
                        proxy.setUser(jamendo["proxy_username"].toString());
                        proxy.setPassword(jamendo["proxy_password"].toString());
                    }
                    nam->setProxy(proxy);
                }
            }
            if (jamendo["proxy_ssl_allow"].toBool()) {
                QObject::connect(nam, &QNetworkAccessManager::sslErrors,
                    [](QNetworkReply *reply, const QList<QSslError> &errors) {
                        Q_UNUSED(errors) reply->ignoreSslErrors();
                    });
            }
        };

        QStringList clusterNames;
        for (const ClusterInfo &ci : m_playlistsWidget->m_clusters)
            clusterNames << ci.name;

        JamendoPlaylistSelectDialog dlg(clusterNames,
            [this](const QString &c) { return m_playlistsWidget->getPlaylistsInCluster(c); },
            this);
        if (dlg.exec() != QDialog::Accepted) return;

        QString clusterName = dlg.selectedCluster();
        QString selectedPlaylist = dlg.selectedPlaylist();

        // Determine playlist directory
        QString playlistDir;
        if (dlg.isCustomPath()) {
            playlistDir = dlg.customPath();
        } else if (dlg.isPlayQueue()) {
            playlistDir = QDir::homePath() + "/zmp_playlists/jamendo_cache";
            QDir().mkpath(playlistDir);
        } else if (clusterName.isEmpty()) {
            playlistDir = QDir::homePath() + "/zmp_playlists/jamendo_cache";
            QDir().mkpath(playlistDir);
        } else {
            if (selectedPlaylist.isEmpty()) {
                bool ok = false;
                QString newName = QInputDialog::getText(this, ztr("Новый плейлист"),
                    ztr("Введите название плейлиста:"), QLineEdit::Normal, title, &ok);
                if (!ok || newName.trimmed().isEmpty()) return;
                newName = newName.trimmed();
                newName.replace('/', '_');
                playlistDir = PlaylistsWidget::clusterPath(clusterName) + "/" + newName;
                QDir().mkpath(playlistDir);
            } else {
                playlistDir = PlaylistsWidget::clusterPath(clusterName) + "/" + selectedPlaylist;
            }
        }

        // Determine format extension
        QString formatExt = "." + dlg.selectedFormat();

        // Determine filename
        QString customName = dlg.customFilename();
        QString safeName;
        if (!customName.isEmpty()) {
            safeName = customName;
        } else {
            safeName = title;
        }
        safeName.replace(QRegularExpression("[^a-zA-Zа-яА-Я0-9_\\-]"), "_");
        QString filePath = playlistDir + "/" + safeName + formatExt;

        // Get proxy info for display
        QJsonObject jamendoConfig = JamendoSetupDialog::loadJamendoConfig();
        QString proxyHost = jamendoConfig["proxy_host"].toString();
        bool hasProxy = jamendoConfig["proxy_type"].toString("none") != "none";

        // Cancel any previous download
        if (m_currentDownloadReply) {
            m_currentDownloadReply->abort();
            m_currentDownloadReply = nullptr;
        }

        // Start download
        QNetworkAccessManager *dlm = new QNetworkAccessManager(this);
        applyJamendoProxy(dlm);
        QNetworkReply *reply = dlm->get(QNetworkRequest(QUrl(audioUrl)));
        m_currentDownloadReply = reply;
        m_miniPlayerBar->setDownloadActive(true);
        m_miniPlayerBar->setDownloadInfo(title, 0, 0, proxyHost, hasProxy);
        m_miniPlayerBar->setDownloadProgress(0);
        connect(reply, &QNetworkReply::downloadProgress, this,
            [this, title, proxyHost, hasProxy](qint64 received, qint64 total) {
            if (total > 0) {
                m_miniPlayerBar->setDownloadProgress(100.0 * received / total);
                m_miniPlayerBar->setDownloadInfo(title, received, total, proxyHost, hasProxy);
            }
        });
        bool playAfterDownload = dlg.isPlayQueue() || dlg.isCustomPath() || clusterName.isEmpty();
        connect(reply, &QNetworkReply::finished, this,
            [this, reply, dlm, filePath, playAfterDownload]() {
            if (m_currentDownloadReply == reply)
                m_currentDownloadReply = nullptr;
            reply->deleteLater();
            dlm->deleteLater();

            const bool cancelled = (reply->error() == QNetworkReply::OperationCanceledError);
            m_miniPlayerBar->setDownloadActive(false);
            if (cancelled) {
                QFile::remove(filePath);
                return;
            }
            if (reply->error() != QNetworkReply::NoError)
                return;

            QByteArray data = reply->readAll();
            if (data.isEmpty())
                return;
            QFile file(filePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(data);
                file.close();
            }
            if (playAfterDownload) {
                m_playerWidget->setPlaylist({filePath});
                m_playerWidget->setCurrentPlaylist({filePath});
                m_playerWidget->onPlay();
                m_menu->setCurrentRow(1);
            } else {
                m_playlistsWidget->loadPlaylists();
            }
        });

        // Connect cancel button (disconnect previous, reconnect once)
        disconnect(m_miniPlayerBar, &MiniPlayerBar::downloadCancelled, nullptr, nullptr);
        connect(m_miniPlayerBar, &MiniPlayerBar::downloadCancelled, this, [this]() {
            if (m_currentDownloadReply) {
                m_currentDownloadReply->abort();
                m_currentDownloadReply = nullptr;
            }
            m_miniPlayerBar->setDownloadActive(false);
        });
    });

    connect(m_playlistsWidget, &PlaylistsWidget::playlistSelected, this, [this](const QStringList &tracks) {
        m_playerWidget->setPlaylist(tracks);
        m_playerWidget->setCurrentPlaylist(tracks);
        m_playerWidget->onPlay();
        m_menu->setCurrentRow(1);
    });
    connect(m_playerWidget, &PlayerWidget::currentPlaylistChanged, this, [this](const QStringList &tracks) {
        m_playlistsWidget->onPlaylistPlaying(tracks);
    });

    connect(m_settingsWidget, &SettingsWidget::exitRequested, this, &MainWindow::onExit);
    connect(m_settingsWidget, &SettingsWidget::metadataHeightChanged, m_playerWidget, &PlayerWidget::setMetadataHeight);
    connect(m_settingsWidget, &SettingsWidget::iconSizeChanged, m_playerWidget, &PlayerWidget::setIconSize);
    connect(m_settingsWidget, &SettingsWidget::accentColorChanged, m_playerWidget, &PlayerWidget::setAccentColor);
    connect(m_settingsWidget, &SettingsWidget::accentColorChanged, m_miniPlayerBar, &MiniPlayerBar::setAccentColor);
    connect(m_settingsWidget, &SettingsWidget::powerModeChanged, m_equalizerWidget, &EqualizerWidget::setPowerMode);
    connect(m_settingsWidget, &SettingsWidget::spectrumGainChanged, m_audioManager, &AudioManager::setSpectrumGain);
    connect(m_settingsWidget, &SettingsWidget::spectrumFpsChanged, m_audioManager, &AudioManager::setSpectrumFps);
    connect(m_settingsWidget, &SettingsWidget::spectrumBandsChanged, m_audioManager, &AudioManager::setSpectrumBands);
    connect(m_settingsWidget, &SettingsWidget::projectMPresetSelected, m_visualizationWidget, &VisualizationWidget::loadProjectMPreset);
    connect(m_settingsWidget, &SettingsWidget::maxBitrateChanged, m_audioManager, &AudioManager::setMaxBitrate);
    connect(m_settingsWidget, &SettingsWidget::jamendoReconfigureRequested, this, [this]() {
        m_jamendoWidget->reconfigure();
    });
    connect(m_audioManager, &AudioManager::spectrumDataChanged, m_playerWidget, &PlayerWidget::updateSpectrum);
    connect(m_audioManager, &AudioManager::spectrumDataChanged, m_visualizationWidget, &VisualizationWidget::updateSpectrum);

    connect(m_audioManager, &AudioManager::positionChanged, m_miniPlayerBar, &MiniPlayerBar::onPositionChanged);
    connect(m_audioManager, &AudioManager::durationChanged, m_miniPlayerBar, &MiniPlayerBar::onDurationChanged);
    connect(m_audioManager, &AudioManager::stateChanged, m_miniPlayerBar, &MiniPlayerBar::onStateChanged);
    connect(m_miniPlayerBar, &MiniPlayerBar::playClicked, m_playerWidget, &PlayerWidget::onPlayClicked);
    connect(m_miniPlayerBar, &MiniPlayerBar::prevClicked, m_playerWidget, &PlayerWidget::onPrev);
    connect(m_miniPlayerBar, &MiniPlayerBar::nextClicked, m_playerWidget, &PlayerWidget::onNext);
    connect(m_playerWidget, &PlayerWidget::trackInfoChanged, m_miniPlayerBar, &MiniPlayerBar::setTrackInfo);

    connect(m_settingsWidget, &SettingsWidget::keyBindingChanged, this, &MainWindow::onKeyBindingChanged);
    connect(m_settingsWidget, &SettingsWidget::keyBindingsSaved, this, &MainWindow::onKeyBindingsSaved);

    if (!m_devicesWidget->selectedDevice().isNull())
        m_audioManager->setActiveOutputDevice(m_devicesWidget->selectedDevice());

    qApp->installEventFilter(this);

    loadKeyBindingsFromSettings();

    connect(&Translator::instance(), &Translator::languageChanged,
            this, &MainWindow::retranslateUi);

#ifndef Q_OS_WIN
    new MprisController(m_audioManager, m_playerWidget, this, this);
#endif

    // Положение меню из конфига + применение геометрии
    m_menuSide = static_cast<MenuSide>(readMenuSideFromConfig());
    connect(m_settingsWidget, &SettingsWidget::menuSideChanged, this,
            [this](int side) {
        m_menuSide = static_cast<MenuSide>(side);
        applyMenuLayout();
    });
    applyMenuLayout();

    // Стартовое применение responsive-раскладки под текущий размер окна
    QTimer::singleShot(0, this, &MainWindow::applyResponsiveLayout);
}

void MainWindow::retranslateUi() {
    runRetrans(m_retrans);
}

void MainWindow::createDepsWarningBanner(QVBoxLayout *rightLayout)
{
    m_depsWarnBanner = new QWidget(this);
    m_depsWarnBanner->setFixedHeight(30); // примерно половина мини-плеера
    m_depsWarnBanner->setStyleSheet("background-color: #c62828;");

    QHBoxLayout *l = new QHBoxLayout(m_depsWarnBanner);
    l->setContentsMargins(8, 2, 8, 2);
    l->setSpacing(8);

    QLabel *text = new QLabel(ztr("Не хватает зависимостей"));
    text->setStyleSheet("color: white; font-weight: bold;");
    l->addWidget(text, 1);

    QPushButton *ignoreBtn = new QPushButton(ztr("Игнорировать"));
    QPushButton *moreBtn = new QPushButton(ztr("См. далее"));
    for (QPushButton *b : {ignoreBtn, moreBtn}) {
        b->setFixedHeight(22);
        b->setStyleSheet(
            "QPushButton { background-color: rgba(255,255,255,0.15); color: white;"
            " border: 1px solid rgba(255,255,255,0.4); border-radius: 3px; padding: 1px 10px; }"
            "QPushButton:hover { background-color: rgba(255,255,255,0.3); }");
        l->addWidget(b);
    }

    connect(ignoreBtn, &QPushButton::clicked, this, [this]() {
        m_depsWarnBanner->hide();
    });
    connect(moreBtn, &QPushButton::clicked, this, [this]() {
        DependencyCheckDialog *dlg = new DependencyCheckDialog(this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        // После закрытия диалога перепроверяем — если всё установлено, убираем баннер
        connect(dlg, &QDialog::finished, this, [this]() {
            QTimer::singleShot(300, this, &MainWindow::checkDependenciesAtStartup);
        });
        dlg->show();
    });

    // Скрываем красный баннер, когда началась/идёт установка (показывается жёлтый прогресс)
    connect(DependencyManager::instance(), &DependencyManager::installStarted,
            m_depsWarnBanner, &QWidget::hide);

    m_depsWarnBanner->hide();
    rightLayout->insertWidget(0, m_depsWarnBanner);
}

void MainWindow::checkDependenciesAtStartup()
{
    const QVector<DependencyManager::DepResult> results =
        DependencyManager::instance()->runChecks();

    bool missing = false;
    for (const auto &r : results) {
        if (r.status != DependencyManager::Found) {
            missing = true;
            break;
        }
    }

    if (missing && !m_depsWarnBanner->isVisible())
        m_depsWarnBanner->show();
    else if (!missing)
        m_depsWarnBanner->hide();
}

void MainWindow::createInstallBanner(QVBoxLayout *rightLayout)
{
    m_installBanner = new QWidget(this);
    m_installBanner->setFixedHeight(30);
    m_installBanner->setStyleSheet("background-color: #1565c0;");

    QHBoxLayout *l = new QHBoxLayout(m_installBanner);
    l->setContentsMargins(8, 2, 8, 2);
    l->setSpacing(8);

    QLabel *text = new QLabel(ztr("Установите ZMP как приложение для ПК"));
    text->setStyleSheet("color: white; font-weight: bold;");
    l->addWidget(text, 1);

    QPushButton *installBtn = new QPushButton(ztr("Установить"));
    QPushButton *ignoreBtn = new QPushButton(ztr("Игнорировать"));
    for (QPushButton *b : {installBtn, ignoreBtn}) {
        b->setFixedHeight(22);
        b->setStyleSheet(
            "QPushButton { background-color: rgba(255,255,255,0.15); color: white;"
            " border: 1px solid rgba(255,255,255,0.4); border-radius: 3px; padding: 1px 10px; }"
            "QPushButton:hover { background-color: rgba(255,255,255,0.3); }");
        l->addWidget(b);
    }

    connect(ignoreBtn, &QPushButton::clicked, this, [this]() {
        m_installBanner->hide();
    });
    connect(installBtn, &QPushButton::clicked, this, [this]() {
        ZmpInstallDialog dlg(ZmpInstallDialog::Mode::Install, this);
        if (dlg.exec() == QDialog::Accepted)
            m_installBanner->hide();
    });

    m_installBanner->hide();
    rightLayout->insertWidget(0, m_installBanner);
}

void MainWindow::checkInstallAtStartup()
{
    if (!zmpInstalledAsApp() && !m_installBanner->isVisible())
        m_installBanner->show();
}

void MainWindow::updateDepsBanner(const QString &pkg, int percent, qint64 speedBps)
{    // Speed coloring identical to the Jamendo download indicator
    QString color;
    double speedKBps = speedBps / 1024.0;
    if (speedKBps > 900)      color = "#4CAF50";
    else if (speedKBps > 200) color = "#FFC107";
    else if (speedKBps > 5)   color = "#F44336";
    else                      color = "#888888";

    QString speedStr;
    if (speedBps > 1000LL * 1024 * 1024)
        speedStr = QString("%1 ГБ/с").arg(speedBps / (1024.0 * 1024 * 1024), 0, 'f', 2);
    else if (speedBps > 4000 * 1024)
        speedStr = QString("%1 МБ/с").arg(speedBps / (1024.0 * 1024), 0, 'f', 2);
    else if (speedBps > 0)
        speedStr = QString("%1 КБ/с").arg(speedKBps, 0, 'f', 1);
    else
        speedStr = ztr("ожидание...");

    DependencyManager *depsMgr = DependencyManager::instance();
    QString proxyPart;
    QString proxyIp = depsMgr->proxyDisplayIp();
    if (depsMgr->hasProxy() && !proxyIp.isEmpty())
        proxyPart = QString("<span style='color:%1;'>%2</span>").arg(color, proxyIp);
    else
        proxyPart = QString("<span style='color:#4CAF50;'>%1</span>").arg(ztr("без прокси"));

    m_depsBanner->setText(QString("%1: %2 %3% | <span style='color:%4;'>%5</span> | %6")
        .arg(ztr("Установка зависимостей"), pkg)
        .arg(percent)
        .arg(color, speedStr, proxyPart));
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    positionMenuToggle();

    // Responsive-переключение с защитой от шторма событий resize
    if (!m_portraitDebounce) {
        m_portraitDebounce = new QTimer(this);
        m_portraitDebounce->setSingleShot(true);
        m_portraitDebounce->setInterval(150);
        connect(m_portraitDebounce, &QTimer::timeout,
                this, &MainWindow::applyResponsiveLayout);
    }
    m_portraitDebounce->start();
}

bool MainWindow::isPortraitLayout() const
{
    return height() > width();   // портрет/холст — «мобильный» режим
}

void MainWindow::applyResponsiveLayout()
{
    const bool portrait = isPortraitLayout();

    // Split-режим: в портрете вторая вкладка ставится снизу
    if (m_splitter)
        m_splitter->setOrientation(portrait ? Qt::Vertical : Qt::Horizontal);
    if (m_splitMode && m_splitter) {
        QList<int> sizes = {1, 1};
        m_splitter->setSizes(sizes);
    }

    // Боковое меню слева/справа в портрете автоматически скрывается
    // (раскрыть можно кнопкой ">")
    if (portrait && (m_menuSide == MenuSide::Left || m_menuSide == MenuSide::Right)
        && !m_menuCollapsed) {
        m_menuCollapsed = true;
        applyMenuGeometry();
    }

    // Вкладки
    m_playerWidget->setPortraitMode(portrait);
    m_jamendoWidget->setPortraitMode(portrait);
    m_devicesWidget->setPortraitMode(portrait);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        // Автоповтор зажатой клавиши игнорируем: иначе пауза через секунду
        // сама выключается (повторное срабатывание toggle)
        if (!keyEvent->isAutoRepeat())
            handleKeyPress(static_cast<Qt::Key>(keyEvent->key()), keyEvent->modifiers());
        return false;
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::handleKeyPress(Qt::Key key, Qt::KeyboardModifiers modifiers) {
    for (auto it = m_keyBindings.begin(); it != m_keyBindings.end(); ++it) {
        const SettingsWidget::KeyBinding &binding = it.value();
        if (binding.key == key && binding.modifiers == modifiers) {
            switch (it.key()) {
                case SettingsWidget::PrevTrack:
                    m_playerWidget->onPrev();
                    break;
                case SettingsWidget::NextTrack:
                    m_playerWidget->onNext();
                    break;
                case SettingsWidget::PlayPause:
                    m_playerWidget->onPlayClicked();
                    break;
                case SettingsWidget::EqualizerPreset:
                    showEqualizerPresetDialog();
                    break;
            }
            break;
        }
    }
}

void MainWindow::onKeyBindingChanged(SettingsWidget::KeyAction action, const SettingsWidget::KeyBinding &binding) {
    m_keyBindings[action] = binding;
}

void MainWindow::onKeyBindingsSaved() {
}

void MainWindow::showEqualizerPresetDialog() {
    EqualizerPresetDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString preset = dialog.getSelectedPreset();
        if (!preset.isEmpty()) {
            m_equalizerWidget->applyPreset(preset);
        }
    }
}

void MainWindow::loadKeyBindingsFromSettings() {
    m_keyBindings = m_settingsWidget->getKeyBindings();
}

// Целевой прямоугольник линзы для текущей вкладки (работает и сверху/снизу)
void MainWindow::updateLiquidTarget()
{
    QListWidgetItem *it = m_menu->currentItem();
    if (!it || !m_liquid) return;
    QRect r = m_menu->visualItemRect(it);
    r.translate(m_menu->viewport()->mapTo(m_menuContainer, QPoint(0, 0)));

    QRectF target;
    if (isHorizontalMenu()) {
        target = QRectF(r.x() + 2, 3, qMax<qreal>(90, r.width() - 4),
                        m_menuContainer->height() - 6);
    } else {
        target = QRectF(4, r.y() + 2, m_menuContainer->width() - 10,
                        qMax<qreal>(36, r.height() - 4));
    }
    m_liquid->setTarget(target);
}
MainWindow::~MainWindow() {}

void MainWindow::onMenuChanged(int row) { handleMenuSelection(row, false); }
void MainWindow::onDeviceChanged(const QAudioDevice &device) { m_audioManager->setActiveOutputDevice(device); }
void MainWindow::onFileSelected(const QString &path) {
    m_playerWidget->setPlaylist({path});
    m_playerWidget->onPlay();
    m_menu->setCurrentRow(1);
}
void MainWindow::onExit() { QApplication::quit(); }

QString MainWindow::getCurrentUsername()
{
    struct passwd *pw = getpwuid(getuid());
    return pw ? QString::fromUtf8(pw->pw_name) : "unknown";
}

static bool verifyShadowPassword(const QString &username, const QString &password)
{
    FILE *fp = fopen("/etc/shadow", "r");
    if (!fp) return false;

    char line[4096];
    while (fgets(line, sizeof(line), fp)) {
        QString entry = QString::fromUtf8(line).trimmed();
        int colon1 = entry.indexOf(':');
        if (colon1 <= 0) continue;
        if (entry.left(colon1) != username) continue;

        int colon2 = entry.indexOf(':', colon1 + 1);
        if (colon2 <= colon1) continue;
        QByteArray hash = entry.mid(colon1 + 1, colon2 - colon1 - 1).toUtf8();
        fclose(fp);

        if (hash == "*" || hash == "!" || hash.isEmpty()) return false;

        char *result = crypt(password.toUtf8().constData(), hash.constData());
        return result && hash == result;
    }
    fclose(fp);
    return false;
}

void MainWindow::updateUserButtonStyle()
{
    bool red = (getuid() == 0) || m_isRootMode;
    QString userName = getCurrentUsername();
    m_userButton->setText(m_isRootMode ? userName + " (root)" : userName);
    m_userButton->setStyleSheet(QString(
        "QPushButton { background: transparent; color: %1; border: 1px solid %2; "
        "border-radius: 8px; padding: 12px; font-size: 14px; }"
        "QPushButton:hover { color: %3; border-color: %4; }"
    ).arg(red ? "#FF4444" : "#aaa",
          red ? "#FF4444" : "#555",
          red ? "#FF7777" : "#ddd",
          red ? "#FF7777" : "#777"));
}

void MainWindow::addLog(const QString &message)
{
    // Подавляем подряд идущие дубли — спам не должен ни копиться, ни писать
    static QString s_lastRaw;
    if (message == s_lastRaw)
        return;
    s_lastRaw = message;

    // Шум TagLib о дублированных ID3v2 тегах не пишем в журнал вообще
    if (message.contains("Duplicate ID3v2 tags"))
        return;

    QString ts = QDateTime::currentDateTime().toString("yyyy, MM-dd-ss.zzz");
    QString line = QString("[%1] %2").arg(ts, message);
    m_logs.append(line);

    // Ограничиваем журнал: иначе при запуске без терминала (логи только в
    // память) список растёт бесконечно и через несколько минут ZMP зависает
    constexpr int kMaxLogs = 1000;
    if (m_logs.size() > kMaxLogs)
        m_logs.removeFirst();

    // Обновляем splash "ZMP is starting..." — заголовок сверху, ниже логи
    if (g_splash) {
        g_splashLines.append(message);
        while (g_splashLines.size() > 8)
            g_splashLines.removeFirst();

        QPixmap pm(520, 220);
        pm.fill(QColor("#1b1b1b"));
        QPainter p(&pm);
        p.setRenderHint(QPainter::TextAntialiasing);
        QFont f = p.font();
        f.setBold(true);
        f.setPixelSize(18);
        p.setFont(f);
        p.setPen(Qt::white);
        p.drawText(16, 34, QStringLiteral("ZMP is starting..."));

        f.setBold(false);
        f.setPixelSize(11);
        p.setFont(f);
        p.setPen(QColor("#9a9a9a"));
        int y = 70;
        for (const QString &l : g_splashLines) {
            p.drawText(16, y, l);
            y += 17;
        }
        p.end();
        g_splash->setPixmap(pm);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    if (g_original_stderr >= 0) {
        QByteArray data = (line + "\n").toUtf8();
        // Неблокирующая запись: если буфер пайпа полон (читаем через
        // QSocketNotifier в этом же потоке), сообщение просто пропускается —
        // никакого блокирующего write() из слота.
        ssize_t rc = ::write(g_original_stderr, data.constData(),
                             static_cast<size_t>(data.size()));
        if (rc < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            static int s_dropped = 0;
            if (++s_dropped <= 5)
                qDebug() << "log pipe full, dropped log line(s)";
        }
    }
}

void MainWindow::saveLogs(const QString &path)
{
    QString filePath = path.isEmpty()
        ? QDir::homePath() + "/zmp_playlists/zmp_logs.txt"
        : path + "/zmp_logs.txt";
    QDir().mkpath(QFileInfo(filePath).absolutePath());
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const QString &line : m_logs)
            out << line << "\n";
        file.close();
    }
}

void MainWindow::autoSaveLogs()
{
    if (m_logs.isEmpty()) return;
    saveLogs();
}

void MainWindow::showLogDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle(ztr("Логи"));
    dialog.resize(700, 500);
    dialog.setStyleSheet("QDialog { background-color: #2b2b2b; color: white; }");

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setSpacing(10);
    layout->setContentsMargins(15, 15, 15, 15);

    QTextEdit *textEdit = new QTextEdit();
    textEdit->setReadOnly(true);
    textEdit->setStyleSheet("QTextEdit { background-color: #1e1e1e; color: #ddd; border: 1px solid #555; border-radius: 6px; padding: 8px; font-family: monospace; }");
    for (const QString &line : m_logs)
        textEdit->append(line);
    layout->addWidget(textEdit, 1);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *saveBtn = new QPushButton(ztr("Сохранить"));
    saveBtn->setStyleSheet(
        "QPushButton { background: transparent; color: white; border: 1px solid #555; "
        "border-radius: 6px; padding: 10px 20px; font-size: 14px; }"
        "QPushButton:hover { border-color: #aaa; color: #ddd; }");
    connect(saveBtn, &QPushButton::clicked, this, [this, &dialog]() {
        QString dir = QFileDialog::getExistingDirectory(&dialog, ztr("Выберите папку для сохранения"), QDir::homePath());
        if (!dir.isEmpty()) {
            saveLogs(dir);
            QMessageBox::information(&dialog, ztr("Успех"), ztr("Логи сохранены в ") + dir + "/zmp_logs.txt");
        }
    });
    btnLayout->addWidget(saveBtn);

    QPushButton *closeBtn = new QPushButton(ztr("Закрыть"));
    closeBtn->setStyleSheet(
        "QPushButton { background: transparent; color: white; border: 1px solid #555; "
        "border-radius: 6px; padding: 10px 20px; font-size: 14px; }"
        "QPushButton:hover { border-color: #aaa; color: #ddd; }");
    connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    btnLayout->addWidget(closeBtn);

    layout->addLayout(btnLayout);
    dialog.exec();
}

void MainWindow::showRootPasswordDialog()
{
    if (m_isRootMode) return;

    QDialog dialog(this);
    dialog.setWindowTitle(ztr("Права root"));
    dialog.setMinimumWidth(350);

    QFormLayout *form = new QFormLayout(&dialog);

    QLineEdit *passEdit = new QLineEdit();
    passEdit->setPlaceholderText(ztr("Пароль root"));
    passEdit->setEchoMode(QLineEdit::Password);
    form->addRow(ztr("Пароль:"), passEdit);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) return;

    QString password = passEdit->text();
    if (password.isEmpty()) return;

    bool ok = false;
    if (getuid() == 0) {
        ok = verifyShadowPassword("root", password);
    } else {
        QProcess proc;
        proc.start("su", {"-c", "id", "root"});
        if (proc.waitForStarted()) {
            proc.waitForReadyRead(2000);
            proc.write((password + "\n").toUtf8());
            proc.closeWriteChannel();
            if (proc.waitForFinished(5000))
                ok = (proc.exitCode() == 0);
        }
    }

    if (ok) {
        m_rootPassword = password;
        m_isRootMode = true;
        m_filesWidget->setRootPassword(password);
        updateUserButtonStyle();
        addLog(ztr("Режим root активирован"));
    } else {
        QMessageBox::warning(this, ztr("Ошибка"), ztr("Неверный пароль root"));
    }
}

void MainWindow::onUserButtonClicked()
{
    struct passwd *pw = getpwuid(getuid());
    if (!pw) return;

    QString userName = QString::fromUtf8(pw->pw_name);
    QString fullName = QString::fromUtf8(pw->pw_gecos).split(',').first().trimmed();
    QString homeDir = QString::fromUtf8(pw->pw_dir);
    uid_t uid = pw->pw_uid;

    QImage avatar;
    QStringList avatarPaths = {
        QString("/var/lib/AccountsService/icons/%1").arg(userName),
        homeDir + "/.face",
        homeDir + "/.face.icon",
        homeDir + "/.local/share/accounts/service/icons/" + userName
    };
    for (const QString &path : avatarPaths) {
        if (QFile::exists(path)) {
            avatar.load(path);
            if (!avatar.isNull()) break;
        }
    }

    QDialog dialog(this);
    dialog.setWindowTitle(ztr("Пользователь"));
    dialog.setFixedSize(540, 420);
    dialog.setStyleSheet("QDialog { background-color: #2b2b2b; color: white; border-radius: 0; }");

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->setSpacing(20);

    QLabel *avatarLabel = new QLabel();
    avatarLabel->setFixedSize(180, 180);
    avatarLabel->setAlignment(Qt::AlignCenter);
    avatarLabel->setStyleSheet("background-color: #444; border-radius: 10px; color: white; font-size: 48px; font-weight: bold;");

    if (!avatar.isNull()) {
        QPixmap px = QPixmap::fromImage(
            avatar.scaled(180, 180, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        avatarLabel->setPixmap(px);
    } else {
        avatarLabel->setText(QString::number(uid));
    }
    topLayout->addWidget(avatarLabel);

    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(6);

    QLabel *displayName = new QLabel(fullName.isEmpty() ? userName : fullName);
    displayName->setStyleSheet("font-size: 20px; font-weight: bold; color: white;");
    infoLayout->addWidget(displayName);

    QLabel *userLabel = new QLabel(userName);
    userLabel->setStyleSheet("font-size: 14px; color: #aaa;");
    infoLayout->addWidget(userLabel);

    QLabel *homeLabel = new QLabel(homeDir);
    homeLabel->setStyleSheet("font-size: 14px; color: #aaa;");
    infoLayout->addWidget(homeLabel);

    QLabel *uidLabel = new QLabel(QString("UID: %1").arg(uid));
    uidLabel->setStyleSheet("font-size: 14px; color: #aaa;");
    infoLayout->addWidget(uidLabel);

    infoLayout->addStretch();
    topLayout->addLayout(infoLayout, 1);
    mainLayout->addLayout(topLayout);

    QString btnStyle = QString(
        "QPushButton { background: transparent; color: white; border: 1px solid %1; "
        "border-radius: 6px; padding: 10px; font-size: 14px; }"
        "QPushButton:hover { background: rgba(255,255,255,0.05); border-color: %2; }"
    );

#ifdef Q_OS_WIN
    QLabel *adminLabel = new QLabel(ztr("Права администратора запрашиваются автоматически (UAC)"));
    adminLabel->setAlignment(Qt::AlignCenter);
    adminLabel->setStyleSheet("color: #FF9800; font-size: 13px;");
    mainLayout->addWidget(adminLabel);
#else
    if (!m_isRootMode) {
        QPushButton *rootBtn = new QPushButton(ztr("Войти в режим root"));
        rootBtn->setStyleSheet(btnStyle.arg("#c0392b", "#e74c3c"));
        connect(rootBtn, &QPushButton::clicked, this, [this, &dialog]() {
            dialog.hide();
            showRootPasswordDialog();
            dialog.reject();
        });
        mainLayout->addWidget(rootBtn);
    } else {
        QLabel *rootActive = new QLabel(ztr("Режим root активен"));
        rootActive->setAlignment(Qt::AlignCenter);
        rootActive->setStyleSheet("color: #FF4444; font-size: 14px; font-weight: bold;");
        mainLayout->addWidget(rootActive);
    }
#endif

    QPushButton *logBtn = new QPushButton(ztr("Логи"));
    logBtn->setStyleSheet(btnStyle.arg("#555", "#aaa"));
    connect(logBtn, &QPushButton::clicked, this, [this]() { showLogDialog(); });
    mainLayout->addWidget(logBtn);

    QPushButton *exitBtn = new QPushButton(ztr("Выйти"));
    exitBtn->setStyleSheet(btnStyle.arg("#555", "#aaa"));
    connect(exitBtn, &QPushButton::clicked, qApp, &QApplication::quit);
    mainLayout->addWidget(exitBtn);

    QPushButton *legalBtn = new QPushButton(ztr("О ZMP etc legal"));
    legalBtn->setStyleSheet(btnStyle.arg("#555", "#aaa"));
    connect(legalBtn, &QPushButton::clicked, this, [&dialog]() {
        showAboutZmpDialog(&dialog);
    });
    mainLayout->addWidget(legalBtn);

    QLabel *escHint = new QLabel(ztr("нажмите ESC чтобы закрыть это меню\n для того чтобы открывать директории в root режиме\n вводите путь сверху, в виде дерева root режим работает не всегда "));
    escHint->setAlignment(Qt::AlignCenter);
    escHint->setStyleSheet("color: #666; font-size: 11px;");
    mainLayout->addWidget(escHint);

    dialog.exec();
}

void MainWindow::onFeaturedUpdated() {
    m_playlistsWidget->loadPlaylists();
}

// Файлы из файлового менеджера / -o: добавить в текущую очередь и играть
void MainWindow::openFilesInQueue(const QStringList &files)
{
    QStringList filtered;
    for (const QString &f : files) {
        if (QFileInfo::exists(f))
            filtered << f;
    }
    if (filtered.isEmpty())
        return;

    QStringList queue = m_playerWidget->getCurrentPlaylist();
    const int firstNew = queue.size();
    for (const QString &f : filtered) {
        if (!queue.contains(f))
            queue << f;
    }

    m_menu->setCurrentRow(1);
    m_playerWidget->setPlaylist(queue);
    m_playerWidget->setCurrentPlaylist(queue);
    m_playlistsWidget->onPlaylistPlaying(queue);
    m_playerWidget->playFromIndex(firstNew);
}

// Проиграть плейлист из кластера (файлы в каталоге плейлиста)
void MainWindow::playExternalPlaylist(const QString &cluster, const QString &name)
{
    QDir dir(PlaylistsWidget::clusterPath(cluster) + "/" + name);
    if (!dir.exists()) {
        QMessageBox::warning(this, ztr("Плейлист"),
                             ztr("Плейлист не найден:") + " " + cluster + "/" + name);
        return;
    }
    QStringList files;
    for (const QString &f : dir.entryList(QDir::Files, QDir::Name))
        files << dir.absoluteFilePath(f);

    m_menu->setCurrentRow(1);
    m_playerWidget->setPlaylist(files);
    m_playerWidget->setCurrentPlaylist(files);
    m_playlistsWidget->onPlaylistPlaying(files);
    m_playerWidget->playFromIndex(0);
}

EqualizerPresetDialog::EqualizerPresetDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(ztr("Выбор пресета"));
    resize(300, 250);
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    m_presetList = new QListWidget(this);
    m_presetList->addItems({"Default", "Bass", "Treble", "Pop", "Dance"});
    m_presetList->setCurrentRow(0);
    m_presetList->setFocus();
    
    layout->addWidget(m_presetList);
}

void EqualizerPresetDialog::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
        case Qt::Key_Up:
            if (m_presetList->currentRow() > 0)
                m_presetList->setCurrentRow(m_presetList->currentRow() - 1);
            break;
        case Qt::Key_Down:
            if (m_presetList->currentRow() < m_presetList->count() - 1)
                m_presetList->setCurrentRow(m_presetList->currentRow() + 1);
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            m_selectedPreset = m_presetList->currentItem()->text();
            accept();
            break;
        case Qt::Key_Escape:
            reject();
            break;
        default:
            QDialog::keyPressEvent(event);
            break;
    }
}


// ---------------------------------------------------------------------------
//  Split-режим: Shift+клик по вкладке открывает её справа, текущая остаётся
//  слева; между ними перетаскиваемый QSplitter. Обычный клик возвращает
//  одинарный режим. Контент вкладок перемещается между постоянными обёртками,
//  поэтому ничего не теряется при любых переключениях.
// ---------------------------------------------------------------------------

void MainWindow::placeContent(int row, bool rightSide)
{
    QWidget *c = m_pages.at(row);
    QWidget *target = rightSide ? static_cast<QWidget*>(m_wrapRight.at(row))
                                : static_cast<QWidget*>(m_wrapLeft.at(row));
    if (c->parentWidget() == target)
        return;
    target->layout()->addWidget(c);   // reparent из любой другой обёртки
}

void MainWindow::showSingleTab(int row)
{
    placeContent(row, false);
    m_rightStack->setVisible(false);
    m_leftStack->setCurrentWidget(m_wrapLeft.at(row));
    m_leftRow = row;

    m_splitMode = false;
    m_splitLeft = m_splitRight = -1;
}

void MainWindow::handleMenuSelection(int row, bool shift)
{
    Q_UNUSED(row)
    updateLiquidTarget();
    QListWidgetItem *item = m_menu->item(row);
    Q_UNUSED(item)
    m_miniPlayerBar->setVisible(row != 1);

    if (!shift) {
        showSingleTab(row);
        return;
    }

    // Shift + клик по уже открытой слева вкладке — ничего не делаем
    const QWidget *cur = m_leftStack->currentWidget();
    if (!m_splitMode && cur == m_wrapLeft.at(row))
        return;
    if (m_splitMode && row == m_splitRight)
        return;

    if (!m_splitMode) {
        enterSplitMode(m_leftRow, row);
    } else {
        setRightPane(row);
    }
}

void MainWindow::enterSplitMode(int leftRow, int rightRow)
{
    // правый контент мог остаться слева с прошлого раза — убираем дубликат
    placeContent(rightRow, true);

    m_rightStack->setVisible(true);
    QList<int> sizes = {1, 1};
    m_splitter->setSizes(sizes);

    m_leftStack->setCurrentWidget(m_wrapLeft.at(leftRow));
    m_rightStack->setCurrentWidget(m_wrapRight.at(rightRow));
    m_leftRow = leftRow;

    m_splitMode = true;
    m_splitLeft = leftRow;
    m_splitRight = rightRow;
}

void MainWindow::setRightPane(int row)
{
    if (!m_splitMode || row == m_splitRight)
        return;

    // прежний правый контент возвращается на левую половину
    placeContent(m_splitRight, false);
    placeContent(row, true);

    m_rightStack->setCurrentWidget(m_wrapRight.at(row));
    m_splitRight = row;
}

void MainWindow::exitSplitMode(int gotoRow)
{
    if (m_splitRight >= 0)
        placeContent(m_splitRight, false);
    m_rightStack->setVisible(false);
    m_splitMode = false;
    m_splitLeft = m_splitRight = -1;

    if (gotoRow >= 0)
        showSingleTab(gotoRow);
}

// ---------------------------------------------------------------------------
//  Положение бокового меню (слева/сверху/справа/снизу) и сворачивание
// ---------------------------------------------------------------------------

static int readMenuSideFromConfig()
{
    QFile f(QDir::homePath() + "/zmp_playlists/config.json");
    if (f.open(QIODevice::ReadOnly)) {
        const int v = QJsonDocument::fromJson(f.readAll())
                          .object().value("menu_side").toInt(0);
        f.close();
        if (v >= 0 && v <= 3) return v;
    }
    return 0;
}

bool MainWindow::isHorizontalMenu() const
{
    return m_menuSide == MenuSide::Top || m_menuSide == MenuSide::Bottom;
}

void MainWindow::applyMenuLayout()
{
    const bool horiz = isHorizontalMenu();
    m_mainLay->setDirection(horiz ? QBoxLayout::TopToBottom
                                  : QBoxLayout::LeftToRight);

    // Порядок: меню сверху/слева — первым, снизу/справа — последним
    m_mainLay->removeWidget(m_menuContainer);
    m_mainLay->removeWidget(m_rightContainer);
    const bool menuFirst = (m_menuSide == MenuSide::Left ||
                            m_menuSide == MenuSide::Top);
    if (menuFirst) {
        m_mainLay->addWidget(m_menuContainer);
        m_mainLay->addWidget(m_rightContainer, 1);
    } else {
        m_mainLay->addWidget(m_rightContainer, 1);
        m_mainLay->addWidget(m_menuContainer);
    }

    // Ориентация списка вкладок
    m_menu->setFlow(horiz ? QListView::LeftToRight : QListView::TopToBottom);
    m_menu->setWrapping(false);

    // Внутренний layout контейнера: горизонтальное меню — кнопки в строку
    // (пункты слева, кнопка пользователя справа), вертикальное — колонкой
    if (m_menuContLay) {
        m_menuContLay->setDirection(horiz ? QBoxLayout::LeftToRight
                                          : QBoxLayout::TopToBottom);
        m_menuContLay->setSpacing(horiz ? 10 : 5);
    }

    // Линза работает во всех ориентациях
    m_liquid->setGeometry(0, 0,
                          m_menuContainer->width(), m_menuContainer->height());
    QTimer::singleShot(0, this, &MainWindow::updateLiquidTarget);

    applyMenuGeometry();
    QTimer::singleShot(0, this, &MainWindow::positionMenuToggle);
}

void MainWindow::applyMenuGeometry()
{
    const bool horiz = isHorizontalMenu();

    if (!m_menuCollapsed) {
        if (horiz) {
            m_menuContainer->setFixedHeight(76);
            m_menuContainer->setFixedWidth(QWIDGETSIZE_MAX);


        } else {
            m_menuContainer->setFixedWidth(200);
            m_menuContainer->setFixedHeight(QWIDGETSIZE_MAX);

        }
    } else {
        if (horiz)
            m_menuContainer->setFixedHeight(26);
        else
            m_menuContainer->setFixedWidth(30);
    }

    m_menu->setVisible(!m_menuCollapsed);
    m_userButton->setVisible(!m_menuCollapsed);

    // Текст кнопки: "<" прячет меню, ">" показывает
    m_menuToggleBtn->setText(m_menuCollapsed ? ">" : "<");
    positionMenuToggle();
}

void MainWindow::positionMenuToggle()
{
    if (!m_menuToggleBtn || !m_menuContainer)
        return;
    const QSize s = m_menuToggleBtn->sizeHint();
    m_menuToggleBtn->resize(s);

    const QRect g = m_menuContainer->geometry();
    QPoint pos;
    switch (m_menuSide) {
        case MenuSide::Left:
            pos = QPoint(m_menuCollapsed ? 2 : g.width() - s.width() - 3,
                         g.top() + 3);
            break;
        case MenuSide::Right:
            pos = QPoint(g.x() + (m_menuCollapsed ? g.width() - s.width() - 2 : 3),
                         g.top() + 3);
            break;
        case MenuSide::Top:
            pos = QPoint(g.x() + 3,
                         m_menuCollapsed ? 2 : g.height() - s.height() - 3);
            break;
        case MenuSide::Bottom:
            pos = QPoint(g.x() + 3,
                         m_menuCollapsed ? g.height() - s.height() - 2 : 3);
            break;
    }
    m_menuToggleBtn->move(pos);
    m_menuToggleBtn->raise();
}
