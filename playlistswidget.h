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
#include <QPair>
#include <QDialog>

struct PlaylistInfo {
    QString name;
    QStringList tracks;
    QStringList trackTitles;
    QImage cover;
};
Q_DECLARE_METATYPE(PlaylistInfo)

// Forward declaration of PlaylistEditDialog
class PlaylistEditDialog;

class PlaylistTileWidget : public QWidget {
    Q_OBJECT
public:
    PlaylistTileWidget(const PlaylistInfo &info, QWidget *parent = nullptr, bool isFeatured = false) : QWidget(parent), m_info(info), m_angle(0), m_scrollX(0), m_isFeatured(isFeatured) {
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

        m_angle = (m_angle + 1) % 360;
        int flickerAlpha = 150 + (QRandomGenerator::global()->bounded(105));
        QPen pen;
        pen.setWidth(3);
        QConicalGradient grad(QPointF(90, 90), m_angle);
        
        QColor borderColor;
        QColor bottomColor;
        
        if (m_isPlaying) {
            borderColor = QColor(0, 100, 255, flickerAlpha);
            bottomColor = QColor(0, 100, 255, 255);
        } else if (m_isFeatured) {
            borderColor = QColor(255, 200, 0, flickerAlpha);
            bottomColor = QColor(255, 200, 0, 255);
        } else {
            borderColor = QColor(0, 255, 100, flickerAlpha);
            bottomColor = QColor(0, 255, 100, 255);
        }
        
        grad.setColorAt(0.0, borderColor);
        grad.setColorAt(0.15, borderColor);
        grad.setColorAt(1.0, borderColor);
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
        
        if (!m_info.cover.isNull()) {
            QColor coverColor = m_info.cover.pixel(0, 0);
            QColor bottomLine = bottomColor;
            bottomLine.setAlpha(200);
            
            QPen bottomPen(bottomLine);
            bottomPen.setWidth(4);
            p.setPen(bottomPen);
            p.drawLine(10, 230, 170, 230);
        }
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
    void setBorderColor(const QColor &color) { m_borderColor = color; update(); }

signals:
    void doubleClicked(const QStringList &tracks);

private:
    PlaylistInfo m_info;
    qreal m_rotX = 0, m_rotY = 0;
    int m_angle;
    QTimer *m_animTimer;
    bool m_isPlaying = false;
    bool m_isFeatured = false;
    QColor m_borderColor;

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
    void loadPlaylists();

private slots:
    void onAddClicked();
    void onDeleteClicked();
    void onEditClicked();
    void savePlaylistColors();
    void loadPlaylistColors();

public:
    static QString basePath();
    static QStringList supportedExts();

    QListWidget *m_listWidget;
    QList<PlaylistInfo> m_playlists;
    QMap<QString, QColor> m_playlistColors;
    
    friend class PlaylistEditDialog;
};

class PlaylistEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit PlaylistEditDialog(const QString &playlistName, QWidget *parent = nullptr);
    void loadTracks();
    void setupColorButtons();
    void saveChanges();

signals:
    void savePlaylistColors();

private slots:
    void onColorSelected(const QColor &color);
    void onAddFiles();
    void onRemoveTrack();
    void onApply();
    void onCancel();

private:
    QString m_playlistName;
    QString m_borderColorName; // not used directly
    QColor m_borderColor;
    QListWidget *m_trackList;
    QPushButton *m_applyBtn;
    QPushButton *m_cancelBtn;
    QStringList m_tracks;
};
#endif // PLAYLISTSWIDGET_H