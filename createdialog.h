#ifndef CREATEDIALOG_H
#define CREATEDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>

class CreateDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CreateDialog(QWidget *parent = nullptr);
    QString playlistName() const;
    QStringList selectedFiles() const;

private slots:
    void onAddFiles();
    void onRemoveFile();

private:
    QLineEdit *m_nameEdit;
    QListWidget *m_filesList;
};

#endif // CREATEDIALOG_H