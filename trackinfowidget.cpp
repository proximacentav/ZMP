#include "trackinfowidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPixmap>

TrackInfoWidget::TrackInfoWidget(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(220);  // начальная высота

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_coverLabel = new QLabel(this);
    m_coverLabel->setFixedSize(180, 180);
    m_coverLabel->setScaledContents(true);
    mainLayout->addWidget(m_coverLabel);

    QVBoxLayout *textLayout = new QVBoxLayout();
    m_titleLabel = new QLabel(this);
    m_titleLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
    m_artistLabel = new QLabel(this);
    m_albumYearLabel = new QLabel(this);
    textLayout->addWidget(m_titleLabel);
    textLayout->addWidget(m_artistLabel);
    textLayout->addWidget(m_albumYearLabel);
    textLayout->addStretch();
    mainLayout->addLayout(textLayout);
}

void TrackInfoWidget::updateInfo(const TrackMetadata &metadata)
{
    if (!metadata.cover.isNull()) {
        QPixmap pix = QPixmap::fromImage(metadata.cover);
        m_coverLabel->setPixmap(pix.scaled(180, 180, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        m_coverLabel->clear();
    }

    QString title = metadata.title.isEmpty() ? "Неизвестно" : metadata.title;
    m_titleLabel->setText(title);

    QString artist = metadata.artist.isEmpty() ? "" : metadata.artist;
    m_artistLabel->setText(artist);

    QString album = metadata.album.isEmpty() ? "" : metadata.album;
    QString year = metadata.year > 0 ? QString::number(metadata.year) : "";
    QString albumYear = album;
    if (!album.isEmpty() && !year.isEmpty()) albumYear += " • ";
    albumYear += year;
    m_albumYearLabel->setText(albumYear);
}