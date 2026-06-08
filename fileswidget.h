#ifndef FILESWIDGET_H
#define FILESWIDGET_H


#include <QtGlobal>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#error "install qt6"
#endif

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>

class FilesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FilesWidget(QWidget *parent = nullptr);
    QString currentSelectedFile() const;

signals:
    void fileSelected(const QString &path);

private slots:
    void onDoubleClicked(const QModelIndex &index);

private:
    QTreeView *m_treeView;
    QFileSystemModel *m_model;
};

#endif // FILESWIDGET_H