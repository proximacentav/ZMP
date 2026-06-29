#ifndef PLAYLISTSWIDGET_H
#define PLAYLISTSWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QImage>
#include <QStringList>
#include <QTimer>
#include <QPainterPath>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QMouseEvent>
#include <QRandomGenerator>
#include <QConicalGradient>
#include <QTransform>

struct PlaylistInfo {
    QString name;
    QStringList tracks;
    QStringList trackTitles;
    QImage cover;
};
Q_DECLARE_METATYPE(PlaylistInfo)

class PlaylistTileWidget : public QWidget {
    Q_OBJECT
public:
    PlaylistTileWidget(const PlaylistInfo &info, QWidget *parent = nullptr) : QWidget(parent), m_info(info), m_angle(0), m_scrollX(0) {
        setFixedSize(180, 230);
        setMouseTracking(true);
        
        m_scrollText = m_info.trackTitles.join(" • ");
        
        m_animTimer = new QTimer(this);
        connect(m_animTimer, &QTimer::timeout, this, QOverload<>::of(&PlaylistTileWidget::update));
        m_animTimer->start(33);
    }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        p.translate(width()/2.0, height()/2.0);
        QTransform t;
        t.rotate(m_rotY, Qt::YAxis);
        t.rotate(m_rotX, Qt::XAxis);
        p.setTransform(t, true);
        p.translate(-width()/2.0, -height()/2.0);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(40, 40, 40));
        p.drawRoundedRect(0, 0, width(), height(), 10, 10);

        m_angle = (m_angle + 4) % 360;
        int flickerAlpha = 150 + (QRandomGenerator::global()->bounded(105));
        QPen pen;
        pen.setWidth(3);
        QConicalGradient grad(QPointF(90, 90), m_angle);
        if (m_isPlaying) {
            grad.setColorAt(0.0, QColor(0, 100, 255, flickerAlpha));
            grad.setColorAt(0.15, QColor(0, 100, 255, 0));
            grad.setColorAt(1.0, QColor(0, 100, 255, 0));
        } else {
            grad.setColorAt(0.0, QColor(0, 255, 100, flickerAlpha));
            grad.setColorAt(0.15, QColor(0, 255, 100, 0));
            grad.setColorAt(1.0, QColor(0, 255, 100, 0));
        }
        pen.setBrush(QBrush(grad));
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(10, 10, 160, 160, 8, 8);

        QRect coverR(11, 11, 158, 158);
        QPainterPath clipPath;
        clipPath.addRoundedRect(coverR, 7, 7);
        p.setClipPath(clipPath);

        if (!m_info.cover.isNull()) {
            p.drawImage(coverR, m_info.cover.scaled(160,160,Qt::KeepAspectRatioByExpanding,Qt::SmoothTransformation));
        } else {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(80, 80, 80));
            p.drawRect(coverR);
            p.setPen(Qt::white);
            p.drawText(coverR, Qt::AlignCenter, "Нет обложки");
        }
        p.setClipping(false);

        p.setPen(Qt::white);
        p.setFont(QFont("Segoe UI", 9, QFont::Bold));
        p.drawText(QRect(10, 175, 160, 20), Qt::AlignHCenter | Qt::TextWordWrap, m_info.name);

        p.setFont(QFont("Segoe UI", 8, QFont::Normal));
        p.setPen(QColor(180, 180, 180));
        QRect scrollRect(10, 200, 160, 20);
        p.setClipRect(scrollRect);
        int textWidth = p.fontMetrics().horizontalAdvance(m_scrollText);
        if (textWidth > scrollRect.width()) {
            p.drawText(scrollRect.x() + m_scrollX, scrollRect.y(), textWidth, scrollRect.height(), Qt::AlignVCenter | Qt::TextSingleLine, m_scrollText);
            m_scrollX -= 1;
            if (m_scrollX < -textWidth) m_scrollX = scrollRect.width();
        } else {
            p.drawText(scrollRect, Qt::AlignVCenter | Qt::TextSingleLine, m_scrollText);
        }
        p.setClipping(false);
    }

    void mouseMoveEvent(QMouseEvent *e) override {
        qreal relX = (qreal)e->pos().x() / width() - 0.5;
        qreal relY = (qreal)e->pos().y() / height() - 0.5;
        m_rotY = qBound(-17.25, relX * 34.5, 17.25);
        m_rotX = qBound(-17.25, -relY * 34.5, 17.25);
        update();
    }

    void leaveEvent(QEvent*) override {
        m_rotX = 0; m_rotY = 0;
        update();
    }
    
    void mouseDoubleClickEvent(QMouseEvent*) override {
        if (!m_info.tracks.isEmpty()) emit doubleClicked(m_info.tracks);
    }

public:
    void setPlaying(bool playing) { m_isPlaying = playing; update(); }

signals:
    void doubleClicked(const QStringList &tracks);

private:
    PlaylistInfo m_info;
    qreal m_rotX = 0, m_rotY = 0;
    int m_angle;
    QTimer *m_animTimer;
    bool m_isPlaying = false;

    QString m_scrollText;
    int m_scrollX;
};

class PlaylistsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlaylistsWidget(QWidget *parent = nullptr);

signals:
    void playlistSelected(const QStringList &tracks);

public slots:
    void onPlaylistPlaying(const QStringList &tracks);
    void onPlaylistStopped();
    void onPlaylistClear();

private slots:
    void onAddClicked();
    void onDeleteClicked();

private:
    void loadPlaylists();
    QImage extractCover(const QString &filePath);
    QString extractTitle(const QString &filePath);
    QString basePath() const;
    QStringList supportedExts() const;

    QListWidget *m_listWidget;
    QList<PlaylistInfo> m_playlists;
};

#endif // PLAYLISTSWIDGET_H