#ifndef DELETEDIALOG_H
#define DELETEDIALOG_H

#include <QDialog>
#include <QLineEdit>

class DeleteDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DeleteDialog(QWidget *parent = nullptr);
    QString playlistName() const;

private slots:
    void onConfirm();

private:
    QLineEdit *m_nameEdit;
};

#endif // DELETEDIALOG_H