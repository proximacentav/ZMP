#include "playlistswidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>

class PlaylistItemDelegate : public QStyledItemDelegate {
public:
    explicit PlaylistItemDelegate(QObject *parent=nullptr) : QStyledItemDelegate(parent) {}
    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const override {
        QVariant data = idx.data(Qt::UserRole);
        if (data.canConvert<PlaylistInfo>()) {
            PlaylistInfo info = data.value<PlaylistInfo>();
            QRect r = opt.rect;
            QRect coverR(r.x(), r.y(), 150, 150);
            if (!info.cover.isNull()) {
                p->drawImage(coverR, info.cover.scaled(150,150,Qt::KeepAspectRatio,Qt::SmoothTransformation));
            } else {
                p->fillRect(coverR, Qt::lightGray);
                p->drawText(coverR, Qt::AlignCenter, "No cover");
            }
            QRect nameR(r.x(), r.y()+150, r.width(), 30);
            p->drawText(nameR, Qt::AlignCenter, info.name);
        } else {
            QStyledItemDelegate::paint(p, opt, idx);
        }
    }
    QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override {
        return QSize(160, 190);
    }
};

PlaylistsWidget::PlaylistsWidget(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout *top = new QHBoxLayout;
    QPushButton *addBtn = new QPushButton("+");
    addBtn->setFixedSize(40,40);
    QPushButton *delBtn = new QPushButton("Delete");
    delBtn->setFixedSize(70,40);
    top->addWidget(addBtn);
    top->addWidget(delBtn);
    top->addStretch();
    layout->addLayout(top);

    m_listWidget = new QListWidget;
    m_listWidget->setViewMode(QListView::IconMode);
    m_listWidget->setIconSize(QSize(150,150));
    m_listWidget->setGridSize(QSize(160,190));
    m_listWidget->setResizeMode(QListView::Adjust);
    m_listWidget->setItemDelegate(new PlaylistItemDelegate(this));
    layout->addWidget(m_listWidget);

    connect(addBtn, &QPushButton::clicked, this, &PlaylistsWidget::onAddClicked);
    connect(delBtn, &QPushButton::clicked, this, &PlaylistsWidget::onDeleteClicked);
    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, &PlaylistsWidget::onItemDoubleClicked);

    loadPlaylists();
}

QString PlaylistsWidget::basePath() const {
    QString path = QDir::homePath() + "/zmp_playlists";
    if (!QDir(path).exists()) QDir().mkpath(path);
    return path;
}

QStringList PlaylistsWidget::supportedExts() const {
    return {".mp3",".wav",".flac",".aac",".aiff"};
}

QImage PlaylistsWidget::extractCover(const QString &filePath) {
    QImage img;
    if (!filePath.endsWith(".mp3", Qt::CaseInsensitive)) return img;
    TagLib::MPEG::File file(filePath.toUtf8().data());
    if (file.ID3v2Tag()) {
        auto list = file.ID3v2Tag()->frameList("APIC");
        if (!list.isEmpty()) {
            auto *pic = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(list.front());
            if (pic && pic->picture().size()>0)
                img.loadFromData((const uchar*)pic->picture().data(), pic->picture().size());
        }
    }
    return img;
}

void PlaylistsWidget::loadPlaylists() {
    m_playlists.clear();
    m_listWidget->clear();
    QDir dir(basePath());
    for (const QString &folder : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        PlaylistInfo info;
        info.name = folder;
        QDir pd(basePath()+"/"+folder);
        for (const QString &f : pd.entryList(QDir::Files)) {
            if (supportedExts().contains(QFileInfo(f).suffix().toLower().prepend('.')))
                info.tracks.append(pd.absolutePath()+"/"+f);
        }
        if (!info.tracks.isEmpty()) info.cover = extractCover(info.tracks.first());
        m_playlists.append(info);
        QListWidgetItem *item = new QListWidgetItem;
        item->setData(Qt::UserRole, QVariant::fromValue(info));
        item->setSizeHint(QSize(160,190));
        m_listWidget->addItem(item);
    }
}

void PlaylistsWidget::onAddClicked() {
    QDialog dlg(this);
    dlg.setWindowTitle("Создать плейлист");
    dlg.setModal(true);
    dlg.resize(450, 400);
    QVBoxLayout *l = new QVBoxLayout(&dlg);
    l->addWidget(new QLabel("Название (без пробелов, спецсимволов, кроме _ - ( )):"));
    QLineEdit *nameEdit = new QLineEdit;
    l->addWidget(nameEdit);
    l->addWidget(new QLabel("Файлы:"));
    QListWidget *fileList = new QListWidget;
    l->addWidget(fileList);
    QHBoxLayout *btnL = new QHBoxLayout;
    QPushButton *addFileBtn = new QPushButton("Добавить файлы");
    QPushButton *removeFileBtn = new QPushButton("Удалить выбранный");
    btnL->addWidget(addFileBtn);
    btnL->addWidget(removeFileBtn);
    l->addLayout(btnL);
    QHBoxLayout *dialogBtns = new QHBoxLayout;
    QPushButton *okBtn = new QPushButton("Создать");
    QPushButton *cancelBtn = new QPushButton("Отмена");
    dialogBtns->addWidget(okBtn);
    dialogBtns->addWidget(cancelBtn);
    l->addLayout(dialogBtns);

    connect(addFileBtn, &QPushButton::clicked, [&](){
        QStringList files = QFileDialog::getOpenFileNames(&dlg, "Выберите аудиофайлы", QDir::homePath(),
            "Аудио (*.mp3 *.wav *.flac *.aac *.aiff)");
        for (const QString &f : files) fileList->addItem(f);
    });
    connect(removeFileBtn, &QPushButton::clicked, [&](){
        delete fileList->currentItem();
    });
    connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        QString name = nameEdit->text().trimmed();
        if (name.isEmpty()) { QMessageBox::warning(this, "Ошибка", "Введите название"); return; }
        QStringList files;
        for (int i=0; i<fileList->count(); ++i) files << fileList->item(i)->text();
        if (files.isEmpty()) { QMessageBox::warning(this, "Ошибка", "Добавьте файлы"); return; }
        QString playlistDir = basePath() + "/" + name;
        QDir dir;
        if (dir.exists(playlistDir)) { QMessageBox::warning(this, "Ошибка", "Плейлист уже существует"); return; }
        if (!dir.mkpath(playlistDir)) { QMessageBox::warning(this, "Ошибка", "Не удалось создать папку"); return; }
        for (const QString &src : files) {
            QFileInfo fi(src);
            if (supportedExts().contains(fi.suffix().toLower().prepend('.'))) {
                QString dst = playlistDir + "/" + fi.fileName();
                QFile::copy(src, dst);
            }
        }
        loadPlaylists();
    }
}

void PlaylistsWidget::onDeleteClicked() {
    QDialog dlg(this);
    dlg.setWindowTitle("Удалить плейлист");
    dlg.setModal(true);
    dlg.resize(300,150);
    QVBoxLayout *l = new QVBoxLayout(&dlg);
    l->addWidget(new QLabel("Введите название плейлиста для удаления:"));
    QLineEdit *nameEdit = new QLineEdit;
    l->addWidget(nameEdit);
    QHBoxLayout *b = new QHBoxLayout;
    QPushButton *ok = new QPushButton("Удалить");
    QPushButton *cancel = new QPushButton("Отмена");
    b->addWidget(ok);
    b->addWidget(cancel);
    l->addLayout(b);
    connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(cancel, &QPushButton::clicked, &dlg, &QDialog::reject);
    if (dlg.exec() == QDialog::Accepted) {
        QString name = nameEdit->text().trimmed();
        if (name.isEmpty()) { QMessageBox::warning(this, "Ошибка", "Введите название"); return; }
        QString path = basePath() + "/" + name;
        QDir dir(path);
        if (!dir.exists()) { QMessageBox::warning(this, "Ошибка", "Плейлист не найден"); return; }
        if (dir.removeRecursively()) loadPlaylists();
        else QMessageBox::warning(this, "Ошибка", "Не удалось удалить");
    }
}

void PlaylistsWidget::onItemDoubleClicked(QListWidgetItem *item) {
    PlaylistInfo info = item->data(Qt::UserRole).value<PlaylistInfo>();
    if (!info.tracks.isEmpty()) emit playlistSelected(info.tracks);
}