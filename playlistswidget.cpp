#include "playlistswidget.h"
#include "createdialog.h"
#include "deletedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QIcon>
#include <QPainter>
#include <QMessageBox>
#include <QDebug>
#include <QStyledItemDelegate>

class PlaylistItemDelegate : public QStyledItemDelegate
{
public:
    explicit PlaylistItemDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}
    
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QVariant data = index.data(Qt::UserRole);
        if (data.canConvert<PlaylistInfo>()) {
            PlaylistInfo info = data.value<PlaylistInfo>();
            QRect rect = option.rect;
            QRect coverRect(rect.x(), rect.y(), 150, 150);
            if (!info.cover.isNull()) {
                painter->drawImage(coverRect, info.cover.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            } else {
                painter->fillRect(coverRect, QBrush(Qt::lightGray));
                painter->drawText(coverRect, Qt::AlignCenter, "No cover");
            }
            QRect nameRect(rect.x(), rect.y() + 150, rect.width(), 30);
            painter->drawText(nameRect, Qt::AlignCenter, info.name);
        } else {
            QStyledItemDelegate::paint(painter, option, index);
        }
    }
    
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        Q_UNUSED(option); Q_UNUSED(index);
        return QSize(160, 190);
    }
};

PlaylistsWidget::PlaylistsWidget(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    QHBoxLayout *topLayout = new QHBoxLayout;
    QPushButton *addBtn = new QPushButton("+");
    addBtn->setFixedSize(40, 40);
    QPushButton *deleteBtn = new QPushButton("Delete"); 
    deleteBtn->setFixedSize(60, 40);
    topLayout->addWidget(addBtn);
    topLayout->addWidget(deleteBtn);
    topLayout->addStretch();
    layout->addLayout(topLayout);
    
    m_listWidget = new QListWidget;
    m_listWidget->setViewMode(QListView::IconMode);
    m_listWidget->setIconSize(QSize(150, 150));
    m_listWidget->setGridSize(QSize(160, 190));
    m_listWidget->setResizeMode(QListView::Adjust);
    m_listWidget->setItemDelegate(new PlaylistItemDelegate(this));
    layout->addWidget(m_listWidget);
    
    connect(addBtn, &QPushButton::clicked, this, &PlaylistsWidget::onAddClicked);
    connect(deleteBtn, &QPushButton::clicked, this, &PlaylistsWidget::onDeleteClicked);
    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, &PlaylistsWidget::onItemDoubleClicked);
    
    qDebug() << "ZMP LOADED playlists";
    loadPlaylists();
}

void PlaylistsWidget::loadPlaylists()
{
    m_listWidget->clear();
    QList<PlaylistInfo> playlists = m_manager.getAllPlaylists();
    for (const PlaylistInfo &info : playlists) {
        QListWidgetItem *item = new QListWidgetItem;
        item->setData(Qt::UserRole, QVariant::fromValue(info));
        item->setSizeHint(QSize(160, 190));
        m_listWidget->addItem(item);
    }
    qDebug() << "Loaded" << playlists.size() << "playlists";
}

void PlaylistsWidget::onAddClicked()
{
    CreateDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        QString name = dlg.playlistName();
        QStringList files = dlg.selectedFiles();
        if (!name.isEmpty() && !files.isEmpty()) {
            if (m_manager.createPlaylist(name, files)) {
                loadPlaylists();
            } else {
                QMessageBox::warning(this, "Ошибка", "Не удалось создать плейлист");
            }
        }
    }
}

void PlaylistsWidget::onDeleteClicked()
{
    DeleteDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        QString name = dlg.playlistName();
        if (!name.isEmpty()) {
            if (m_manager.deletePlaylist(name)) {
                loadPlaylists();
            } else {
                QMessageBox::warning(this, "Ошибка", "Плейлист не найден или не удалён");
            }
        }
    }
}

void PlaylistsWidget::onItemDoubleClicked(QListWidgetItem *item)
{
    PlaylistInfo info = item->data(Qt::UserRole).value<PlaylistInfo>();
    if (!info.tracks.isEmpty()) {
        emit playlistSelected(info.tracks);
    }
}

void PlaylistsWidget::refreshPlaylists()
{
    loadPlaylists();
}