#ifndef PARTITIONSDIALOG_H
#define PARTITIONSDIALOG_H

#include <QDialog>
#include <QString>
#include <QList>

class QVBoxLayout;
class QLabel;

struct PartitionInfo {
    QString device;       // /dev/sda1
    QString type;         // part / disk / crypt ...
    QString fstype;
    qint64 sizeBytes = 0;
    QString mountPoint;   // пусто если не примонтирован
    QString label;
    QString model;
};

// Диалог "Разделы": диски из lsblk, монтирование/демонтирование.
class PartitionsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PartitionsDialog(QWidget *parent = nullptr);

    // Если ZMP уже в root-режиме — монтирование без повторной авторизации
    void setRootCredentials(bool rootMode, const QString &password);

signals:
    void openPathRequested(const QString &path);

private slots:
    void refresh();

private:
    QList<PartitionInfo> listPartitions() const;
    QString privPrefix() const;
    bool execPrivileged(const QStringList &args, QString *errOut);
    bool runMount(const PartitionInfo &p, const QString &dir, QString *errOut);
    bool runUnmount(const PartitionInfo &p, QString *errOut);
    QString humanSize(qint64 bytes) const;
    QWidget *makeRow(const PartitionInfo &p);

    QVBoxLayout *m_rowsLayout;
    QLabel *m_statusLabel;
    bool m_rootMode = false;
    QString m_rootPassword;
};

#endif // PARTITIONSDIALOG_H
