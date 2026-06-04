#include "fileswidget.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QStyleFactory>
#include <QStyleOptionViewItem>
#include <QPainter>
#include <QDir>
#include <QFileInfo>
#include <QStyledItemDelegate>
#include <QDebug>


class MusicHighlightDelegate : public QStyledItemDelegate
{
public:
    explicit MusicHighlightDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}
    
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt = option;
        QString filePath = index.data(QFileSystemModel::FilePathRole).toString();
        if (QFileInfo(filePath).isFile()) {
            QString suffix = QFileInfo(filePath).suffix().toLower();
            if (suffix == "mp3" || suffix == "wav" || suffix == "flac" || suffix == "ogg" || suffix == "m4a") {
                opt.palette.setColor(QPalette::Text, Qt::darkGreen);
                opt.palette.setColor(QPalette::Highlight, QColor(144, 238, 144));
            }
        }
        QStyledItemDelegate::paint(painter, opt, index);
    }
};

FilesWidget::FilesWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    m_treeView = new QTreeView(this);
    m_model = new QFileSystemModel(this);
    m_model->setRootPath(QDir::rootPath());
    m_treeView->setModel(m_model);
    m_treeView->setRootIndex(m_model->index(QDir::homePath()));
    m_treeView->setHeaderHidden(false);
    m_treeView->setAnimated(true);
    m_treeView->setIndentation(20);
    m_treeView->setSortingEnabled(true);
    m_treeView->setItemDelegate(new MusicHighlightDelegate(this));

    layout->addWidget(m_treeView);

    connect(m_treeView, &QTreeView::doubleClicked, this, &FilesWidget::onDoubleClicked);
}

QString FilesWidget::currentSelectedFile() const
{
    QModelIndex idx = m_treeView->currentIndex();
    if (idx.isValid() && m_model->isDir(idx) == false)
        return m_model->filePath(idx);
    return QString();
}

void FilesWidget::onDoubleClicked(const QModelIndex &index)
{
    if (!m_model->isDir(index)) {
        QString path = m_model->filePath(index);
        emit fileSelected(path);
    }
}