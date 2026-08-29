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
#include <QApplication>
#include <QConicalGradient>
#include <QTransform>
#include <QPair>
#include <QDialog>
#include <QMap>
#include <QPushButton>
#include <QStackedWidget>
#include <QComboBox>
#include <QLabel>
#include <QFrame>
#include <QPointer>
#include <QVariantAnimation>
#include "translator.h"

struct PlaylistInfo {
    QString name;
    QStringList tracks;
    QStringList trackTitles;
    QImage cover;
};
Q_DECLARE_METATYPE(PlaylistInfo)

class PlaylistEditDialog;
class PlaylistsWidget;

class PlaylistTileWidget : public QWidget {
    Q_OBJECT
public:
    PlaylistTileWidget(const PlaylistInfo &info, QWidget *parent = nullptr, bool isFeatured = false) : QWidget(parent), m_info(info), m_angle(0), m_scrollX(0), m_isFeatured(isFeatured) {
        setFixedSize(180, 230);
        setMouseTracking(true);

        m_scrollText = m_info.trackTitles.join(" • ");

        m_animTimer = new QTimer(this);
        connect(m_animTimer, &QTimer::timeout, this, [this]() {
            if (m_hovered) m_rayPhase += 0.07;
            update();   // всегда: бегущий текст и мерцание рамки
        });
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
        } else if (m_borderColor.isValid()) {
            borderColor = QColor(m_borderColor.red(), m_borderColor.green(), m_borderColor.blue(), flickerAlpha);
            bottomColor = m_borderColor;
            bottomColor.setAlpha(255);
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
            p.drawText(coverR, Qt::AlignCenter, ztr("Нет обложки"));
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
        // --- hover: лучи вверх в цвете плейлиста + свечение снизу ---
        if (m_hovered) {
            const QColor col = m_borderColor.isValid() ? m_borderColor
                                                       : QColor(0, 200, 255);
            QLinearGradient glow(0, height() - 90, 0, height());
            QColor g0 = col; g0.setAlpha(0);
            QColor g1 = col; g1.setAlpha(70);
            glow.setColorAt(0.0, g0);
            glow.setColorAt(1.0, g1);
            p.setPen(Qt::NoPen);
            p.setBrush(glow);
            p.drawRoundedRect(QRectF(4, height() - 90, width() - 8, 86), 8, 8);

            constexpr int N = 9;
            for (int k = 0; k < N; ++k) {
                const qreal t = (k + 0.5) / N;
                const qreal x = 10 + t * (width() - 20);
                const qreal ph = std::sin(m_rayPhase * 2.2 + k * 1.7) * 0.5 + 0.5;
                const qreal len = 28 + 62 * ph;
                const qreal w2 = 3.0 + 5.0 * ph;

                QLinearGradient lg(x, height(), x, height() - len);
                QColor c1 = col; c1.setAlpha(160 + int(80 * ph));
                QColor c0 = col; c0.setAlpha(0);
                lg.setColorAt(0.0, c1);
                lg.setColorAt(1.0, c0);

                QPainterPath beam;
                beam.moveTo(x - w2 / 2, height());
                beam.lineTo(x, height() - len);
                beam.lineTo(x + w2 / 2, height());
                beam.closeSubpath();
                p.setPen(Qt::NoPen);
                p.setBrush(lg);
                p.drawPath(beam);
            }
        }

    }

    void mouseMoveEvent(QMouseEvent *e) override {
        if (m_pressed && !m_dragging
            && (e->pos() - m_pressPos).manhattanLength() >= QApplication::startDragDistance()) {
            startDrag();
        }
        if (m_dragging && m_ghost) {
            QWidget *top = m_ghost->parentWidget();
            if (top) m_ghost->move(top->mapFromGlobal(e->globalPosition().toPoint())
                                   - QPoint(m_ghost->width()/2, 20));
            return;
        }
        qreal relX = (qreal)e->pos().x() / width() - 0.5;
        qreal relY = (qreal)e->pos().y() / height() - 0.5;
        m_rotY = qBound(-17.25, relX * 34.5, 17.25);
        m_rotX = qBound(-17.25, -relY * 34.5, 17.25);
        update();
    }

    void enterEvent(QEnterEvent*) override {
        m_hovered = true;
        emit hoverStarted(this);
        update();
    }

    void leaveEvent(QEvent*) override {
        m_rotX = 0; m_rotY = 0;
        if (m_hovered) { m_hovered = false; emit hoverEnded(this); }
        update();
    }
    
    void mouseDoubleClickEvent(QMouseEvent*) override {
        if (!m_info.tracks.isEmpty()) emit doubleClicked(m_info.tracks);
    }

    void mousePressEvent(QMouseEvent *e) override {
        if (e->button() == Qt::LeftButton) {
            m_pressed = true;
            m_pressPos = e->pos();
        }
        QWidget::mousePressEvent(e);
    }

    void mouseReleaseEvent(QMouseEvent *e) override {
        bool wasDragging = m_dragging;
        m_pressed = false;
        if (m_dragging) {
            m_dragging = false;
            unsetCursor();
            if (m_ghost) { m_ghost->deleteLater(); m_ghost = nullptr; }
            const QPoint gp = e->globalPosition().toPoint();
            if (wasDragging) { emit dropRequested(this, gp); return; }   // не трактовать как клик
        }
        QWidget::mouseReleaseEvent(e);
    }

private:
    void startDrag() {
        m_dragging = true;
        QPixmap pm = grab();
        QWidget *top = window();
        m_ghost = new QLabel(top);
        m_ghost->setPixmap(pm);
        m_ghost->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_ghost->setWindowOpacity(0.85);
        m_ghost->resize(pm.size());
        m_ghost->raise();
        m_ghost->show();
        m_ghost->move(top->mapFromGlobal(QCursor::pos()) - QPoint(pm.width()/2, 20));
        setCursor(Qt::ClosedHandCursor);
    }

public:
    void setPlaying(bool playing) { m_isPlaying = playing; update(); }
    void setBorderColor(const QColor &color) { m_borderColor = color; update(); }
    QColor borderColor() const { return m_borderColor; }
    QString playlistName() const { return m_info.name; }
    void refreshFromInfo() {
        m_scrollText = m_info.trackTitles.join(" • ");
        m_scrollX = 0;
        update();
    }

signals:
    void doubleClicked(const QStringList &tracks);
    void hoverStarted(PlaylistTileWidget *tile);
    void hoverEnded(PlaylistTileWidget *tile);
    void dropRequested(PlaylistTileWidget *tile, const QPoint &globalPos);

private:
    PlaylistInfo m_info;
public:
    bool m_hovered = false;
protected:
    qreal m_rayPhase = 0.0;
private:
    qreal m_rotX = 0, m_rotY = 0;
    int m_angle;
    QTimer *m_animTimer;
    bool m_isPlaying = false;
    bool m_isFeatured = false;
    QColor m_borderColor;

    QString m_scrollText;
    int m_scrollX;

    QPoint m_pressPos;
    bool m_pressed = false;
    bool m_dragging = false;
    QLabel *m_ghost = nullptr;

    friend class PlaylistsWidget;
};

struct ClusterInfo {
    QString name;
    QColor color;
};

class CreateClusterDialog;
class EditClusterDialog;
class ArtistScanDialog;

class ClustersPanel : public QWidget {
    Q_OBJECT
public:
    explicit ClustersPanel(QWidget *parent = nullptr);
    void loadClusters();
    void setPlaylistsWidget(class PlaylistsWidget *pw) { m_playlistsWidget = pw; }

signals:
    void clusterSelected(const QString &clusterName);

private:
    void onToggleVisibility();
    void onAddCluster();
    void onEditCluster();

    QListWidget *m_clusterList;
    class PlaylistsWidget *m_playlistsWidget = nullptr;
    friend class PlaylistsWidget;
};

class HoverGlowBar;

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
    void animateItemHeight(QListWidgetItem *item, int targetH);
    void stopItemAnimations();
    QListWidgetItem *findItemOfTile(QWidget *tile);
    void connectTileHover(PlaylistTileWidget *tile, QListWidgetItem *item);
    void filterByCluster(const QString &clusterName);

private slots:
    void onAddClicked();
    void onDeleteClicked();
    void onEditClicked();
    void savePlaylistColors();
    void loadPlaylistColors();

public:
    static QString basePath();
    static QStringList supportedExts();
    static QString clusterPath(const QString &clusterName);
    static QString clusterFolderName(const QString &clusterName);

    QStringList getPlaylistsInCluster(const QString &clusterName) const;
    QStringList getUnclusteredPlaylists() const;
    QStringList allPlaylistFolders() const;
    void addPlaylistToCluster(const QString &playlistName, const QString &clusterName);

    void saveClusters();
    void loadClusters();
    bool isFirstRun() const;
    void setupDefaultClusters();

    void handleTileDrop(PlaylistTileWidget *tile, const QPoint &globalPos);
    QString playlistPath(const QString &playlistName) const;
    void savePlaylistOrder();
    QStringList savedPlaylistOrder() const;

    QListWidget *m_listWidget;
    HoverGlowBar *m_glowBar = nullptr;
    QFrame *m_tilesFrame;
    QPushButton *m_delBtn = nullptr;
    QVector<QPointer<QVariantAnimation>> m_itemAnims;   // живые анимации плиток
    bool m_tilesUpdating = false;   // список пересобирается — hover/анимации запрещены
    QList<PlaylistInfo> m_playlists;
    QMap<QString, QColor> m_playlistColors;
    QList<ClusterInfo> m_clusters;
    ClustersPanel *m_clustersPanel = nullptr;
    QString m_currentClusterFilter;
    static const QString UnclusteredFilter;

    RetransList m_retrans;
    void retranslateUi();

    friend class PlaylistEditDialog;
    friend class CreateClusterDialog;
    friend class EditClusterDialog;
    friend class ArtistScanDialog;
    friend class ClustersPanel;
};

class PlaylistEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit PlaylistEditDialog(const QString &playlistName, QWidget *parent = nullptr);
    void loadTracks();
    void setupColorButtons();
    void saveChanges();
    void setupClusterCombo();

signals:
    void playlistColorChanged(const QString &name, const QColor &color);

private slots:
    void onColorSelected(const QColor &color);
    void onAddFiles();
    void onRemoveTrack();
    void onApply();
    void onCancel();

private:
    QString m_playlistName;
    QColor m_borderColor;
    QListWidget *m_trackList;
    QPushButton *m_applyBtn;
    QPushButton *m_cancelBtn;
    QStringList m_tracks;
    QComboBox *m_clusterCombo = nullptr;
};

class CreateClusterDialog : public QDialog {
    Q_OBJECT
public:
    explicit CreateClusterDialog(QWidget *parent = nullptr);
    QString clusterName() const;
    QColor clusterColor() const;
    QStringList selectedPlaylists() const;

private slots:
    void onColorSelected(const QColor &color);
    void onCreate();
    void onAddPlaylist();

private:
    QLineEdit *m_nameEdit;
    QColor m_color;
    QListWidget *m_playlistCheckList;
};

class EditClusterDialog : public QDialog {
    Q_OBJECT
public:
    explicit EditClusterDialog(const QString &clusterName, QWidget *parent = nullptr);
    QColor clusterColor() const;
    QStringList selectedPlaylists() const;

private slots:
    void onColorSelected(const QColor &color);
    void onSave();
    void onAddPlaylist();

private:
    QString m_clusterName;
    QColor m_color;
    QListWidget *m_playlistCheckList;
};

class ArtistScanDialog : public QDialog {
    Q_OBJECT
public:
    explicit ArtistScanDialog(QWidget *parent = nullptr);
    void scanFolders();

private slots:
    void onAddFolder();
    void onStartScan();

private:
    QListWidget *m_folderList;
    QPushButton *m_startBtn;
    QLabel *m_statusLabel;
};

#endif // PLAYLISTSWIDGET_H
