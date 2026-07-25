#include "miniplayerbar.h"
#include "translator.h"
#include <QTime>
#include <QPixmap>
#include <QFrame>
#include <QDateTime>

MiniPlayerBar::MiniPlayerBar(AudioManager *audioManager, QWidget *parent)
    : QWidget(parent), m_audioManager(audioManager), m_isSeeking(false), m_accentColor(42,130,218), m_duration(0)
{
    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    m_downloadInfoWidget = new QWidget;
    m_downloadInfoWidget->setStyleSheet("background-color: #1e3a3a;");
    m_downloadInfoWidget->setVisible(false);
    QVBoxLayout *dlLayout = new QVBoxLayout(m_downloadInfoWidget);
    dlLayout->setContentsMargins(10, 4, 10, 4);
    dlLayout->setSpacing(1);

    m_downloadStatusLabel = new QLabel(ztr("Загрузка из Jamendo..."));
    m_downloadStatusLabel->setStyleSheet("font-size: 9pt; color: #4CAF50; font-weight: bold; background: transparent;");
    dlLayout->addWidget(m_downloadStatusLabel);

    m_downloadTrackLabel = new QLabel;
    m_downloadTrackLabel->setStyleSheet("font-size: 8pt; color: #ccc; background: transparent;");
    dlLayout->addWidget(m_downloadTrackLabel);

    QHBoxLayout *speedRow = new QHBoxLayout;
    speedRow->setSpacing(8);

    m_cancelDownloadBtn = new QPushButton(ztr("Отмена"));
    m_cancelDownloadBtn->setFixedSize(60, 20);
    m_cancelDownloadBtn->setStyleSheet(
        "QPushButton { background: #c0392b; color: white; border: none; "
        "border-radius: 3px; font-size: 8pt; font-weight: bold; }"
        "QPushButton:hover { background: #e74c3c; }");
    speedRow->addWidget(m_cancelDownloadBtn);

    m_downloadSpeedLabel = new QLabel;
    m_downloadSpeedLabel->setStyleSheet("font-size: 8pt; background: transparent;");
    speedRow->addWidget(m_downloadSpeedLabel, 1);

    dlLayout->addLayout(speedRow);

    m_downloadProgress = new QProgressBar;
    m_downloadProgress->setRange(0, 100);
    m_downloadProgress->setValue(0);
    m_downloadProgress->setFixedHeight(3);
    m_downloadProgress->setTextVisible(false);
    m_downloadProgress->setStyleSheet(
        "QProgressBar { background: transparent; border: none; }"
        "QProgressBar::chunk { background: #4CAF50; border-radius: 1px; }");
    dlLayout->addWidget(m_downloadProgress);

    outerLayout->addWidget(m_downloadInfoWidget);

    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->setContentsMargins(8, 4, 8, 4);
    mainLayout->setSpacing(8);
    outerLayout->addLayout(mainLayout);

    m_coverLabel = new QLabel;
    m_coverLabel->setFixedSize(50, 50);
    m_coverLabel->setScaledContents(true);
    m_coverLabel->setStyleSheet("background: #3a3a3a; border-radius: 4px;");
    m_coverLabel->setAlignment(Qt::AlignCenter);
    m_coverLabel->setText("♪");
    mainLayout->addWidget(m_coverLabel);

    QVBoxLayout *rightLayout = new QVBoxLayout;
    rightLayout->setSpacing(2);

    m_titleLabel = new QLabel(ztr("Нет трека"));
    m_titleLabel->setStyleSheet("font-size: 10pt; font-weight: bold; background: transparent;");
    rightLayout->addWidget(m_titleLabel);

    QHBoxLayout *sliderLayout = new QHBoxLayout;
    sliderLayout->setSpacing(6);
    m_positionSlider = new QSlider(Qt::Horizontal);
    m_positionSlider->setRange(0, 1000);
    m_timeLabel = new QLabel("00:00 / 00:00");
    m_timeLabel->setStyleSheet("font-size: 8pt; color: gray; background: transparent;");
    sliderLayout->addWidget(m_positionSlider);
    sliderLayout->addWidget(m_timeLabel);
    rightLayout->addLayout(sliderLayout);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->setSpacing(6);

    m_prevIcon = new IconButton;
    m_prevIcon->setAlignment(Qt::AlignCenter);
    m_prevIcon->setText("◀");
    m_prevIcon->setStyleSheet("font-size: 14px; font-weight: bold; color: palette(text);");
    m_prevIcon->setFixedSize(24, 24);
    btnLayout->addWidget(m_prevIcon);

    m_playIcon = new IconButton;
    m_playIcon->setAlignment(Qt::AlignCenter);
    m_playIcon->setText("▶");
    m_playIcon->setStyleSheet("font-size: 14px; font-weight: bold; color: palette(text);");
    m_playIcon->setFixedSize(24, 24);
    btnLayout->addWidget(m_playIcon);

    m_nextIcon = new IconButton;
    m_nextIcon->setAlignment(Qt::AlignCenter);
    m_nextIcon->setText("▶");
    m_nextIcon->setStyleSheet("font-size: 14px; font-weight: bold; color: palette(text);");
    m_nextIcon->setFixedSize(24, 24);
    btnLayout->addWidget(m_nextIcon);

    btnLayout->addStretch();
    rightLayout->addLayout(btnLayout);

    mainLayout->addLayout(rightLayout, 1);

    setAccentColor(m_accentColor);

    connect(m_cancelDownloadBtn, &QPushButton::clicked, this, &MiniPlayerBar::downloadCancelled);
    connect(m_prevIcon, &IconButton::clicked, this, &MiniPlayerBar::prevClicked);
    connect(m_playIcon, &IconButton::clicked, this, &MiniPlayerBar::playClicked);
    connect(m_nextIcon, &IconButton::clicked, this, &MiniPlayerBar::nextClicked);
    connect(m_positionSlider, &QSlider::sliderPressed, this, [this]() { m_isSeeking = true; });
    connect(m_positionSlider, &QSlider::sliderReleased, this, [this]() { m_isSeeking = false; });
    connect(m_positionSlider, &QSlider::sliderMoved, this, &MiniPlayerBar::onSliderMoved);
}

void MiniPlayerBar::setTrackInfo(const TrackMetadata &meta) {
    m_titleLabel->setText(meta.title.isEmpty() ? ztr("Неизвестно") : meta.title);
    if (!meta.cover.isNull()) {
        QPixmap pix = QPixmap::fromImage(meta.cover);
        m_coverLabel->setPixmap(pix.scaled(50, 50, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        m_coverLabel->clear();
        m_coverLabel->setText("♪");
    }
}

void MiniPlayerBar::onPositionChanged(qint64 pos) {
    if (!m_isSeeking) {
        qint64 dur = m_duration > 0 ? m_duration : m_audioManager->duration();
        if (dur > 0) {
            m_positionSlider->blockSignals(true);
            m_positionSlider->setValue(static_cast<int>(pos * 1000 / dur));
            m_positionSlider->blockSignals(false);
        }
        QTime t(0,0), cur = t.addMSecs(pos), tot = t.addMSecs(dur);
        m_timeLabel->setText(cur.toString("mm:ss") + " / " + tot.toString("mm:ss"));
    }
}

void MiniPlayerBar::onDurationChanged(qint64 dur) {
    m_duration = dur;
    onPositionChanged(m_audioManager->position());
}

void MiniPlayerBar::onStateChanged(bool playing) {
    updatePlayButtonIcon(playing);
}

void MiniPlayerBar::onSliderMoved(int value) {
    qint64 dur = m_audioManager->duration();
    if (dur > 0) {
        m_audioManager->setPosition(value * dur / 1000);
    }
}

void MiniPlayerBar::setAccentColor(const QColor &color) {
    m_accentColor = color;
    QString sliderStyle = QString(
        "QSlider::groove:horizontal { height:3px; background:palette(mid); border-radius:1px; }"
        "QSlider::sub-page:horizontal { background:%1; border-radius:1px; }"
        "QSlider::handle:horizontal { background:%1; width:12px; height:12px; margin:-5px 0; border-radius:6px; }"
    ).arg(color.name());
    m_positionSlider->setStyleSheet(sliderStyle);
}

void MiniPlayerBar::setDownloadActive(bool active)
{
    m_downloadInfoWidget->setVisible(active);
    if (!active) {
        m_downloadProgress->setValue(0);
    }
}

void MiniPlayerBar::setDownloadProgress(double percent) {
    m_downloadProgress->setValue(static_cast<int>(percent));
}

void MiniPlayerBar::setDownloadInfo(const QString &trackName, qint64 bytesReceived, qint64 bytesTotal,
                                     const QString &proxyHost, bool hasProxy)
{
    m_downloadTrackLabel->setText(trackName);

    // Speed calculation
    qint64 speedBps = 0;
    static qint64 prevBytes = 0;
    static qint64 prevTime = 0;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (prevTime > 0 && now > prevTime) {
        speedBps = (bytesReceived - prevBytes) * 1000 / (now - prevTime);
    }
    prevBytes = bytesReceived;
    prevTime = now;

    // Format speed
    QString speedStr;
    double speedKBps = speedBps / 1024.0;
    if (speedBps > 1000LL * 1024 * 1024) {
        speedStr = QString("%1 ГБ/с").arg(speedBps / (1024.0 * 1024 * 1024), 0, 'f', 2);
    } else if (speedBps > 4000 * 1024) {
        speedStr = QString("%1 МБ/с").arg(speedBps / (1024.0 * 1024), 0, 'f', 2);
    } else {
        speedStr = QString("%1 КБ/с").arg(speedKBps, 0, 'f', 1);
    }

    // Proxy info with color
    QString proxyText;
    QString proxyColor;
    if (hasProxy) {
        proxyText = proxyHost;
        if (speedKBps > 900)
            proxyColor = "#4CAF50";
        else if (speedKBps > 200)
            proxyColor = "#FFC107";
        else if (speedKBps > 5)
            proxyColor = "#F44336";
        else
            proxyColor = "#000000";
    } else {
        proxyText = ztr("без прокси");
        proxyColor = "#4CAF50";
    }

    m_downloadSpeedLabel->setText(QString("%1 | <span style='color:%2;'>%3</span>").arg(speedStr, proxyColor, proxyText));
    m_downloadSpeedLabel->setTextFormat(Qt::RichText);
}

void MiniPlayerBar::updatePlayButtonIcon(bool playing) {
    m_playIcon->setText(playing ? "⏹" : "▶");
}
