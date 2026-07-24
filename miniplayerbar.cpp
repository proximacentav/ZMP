#include "miniplayerbar.h"
#include "translator.h"
#include <QTime>
#include <QPixmap>

MiniPlayerBar::MiniPlayerBar(AudioManager *audioManager, QWidget *parent)
    : QWidget(parent), m_audioManager(audioManager), m_isSeeking(false), m_accentColor(42,130,218), m_duration(0)
{
    setFixedHeight(80);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(8, 4, 8, 4);
    mainLayout->setSpacing(8);

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

void MiniPlayerBar::updatePlayButtonIcon(bool playing) {
    m_playIcon->setText(playing ? "⏹" : "▶");
}
