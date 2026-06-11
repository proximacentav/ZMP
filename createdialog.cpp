#include "createdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>

CreateDialog::CreateDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Создание плейлиста");
    setModal(true);
    resize(500, 400);
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    layout->addWidget(new QLabel("Название плейлиста (без пробелов и спецсимволов:"));
    m_nameEdit = new QLineEdit;
    layout->addWidget(m_nameEdit);
    
    layout->addWidget(new QLabel("Файлы:"));
    m_filesList = new QListWidget;
    layout->addWidget(m_filesList);
    
    QHBoxLayout *btnLayout = new QHBoxLayout;
    QPushButton *addFilesBtn = new QPushButton("Добавить файлы");
    QPushButton *removeFileBtn = new QPushButton("Удалить выбранный");
    btnLayout->addWidget(addFilesBtn);
    btnLayout->addWidget(removeFileBtn);
    layout->addLayout(btnLayout);
    
    QHBoxLayout *dialogBtns = new QHBoxLayout;
    QPushButton *okBtn = new QPushButton("Создать");
    QPushButton *cancelBtn = new QPushButton("Отмена");
    dialogBtns->addWidget(okBtn);
    dialogBtns->addWidget(cancelBtn);
    layout->addLayout(dialogBtns);
    
    connect(addFilesBtn, &QPushButton::clicked, this, &CreateDialog::onAddFiles);
    connect(removeFileBtn, &QPushButton::clicked, this, &CreateDialog::onRemoveFile);
    connect(okBtn, &QPushButton::clicked, this, &CreateDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &CreateDialog::reject);
}

void CreateDialog::onAddFiles()
{
    QStringList files = QFileDialog::getOpenFileNames(this, "Выберите аудиофайлы", QDir::homePath(),
        "Аудио файлы (*.mp3 *.wav *.flac *.aac *.aiff);;Все файлы (*.*)");
    for (const QString &file : files) {
        m_filesList->addItem(file);
    }
}

void CreateDialog::onRemoveFile()
{
    QListWidgetItem *item = m_filesList->currentItem();
    if (item) delete item;
}

QString CreateDialog::playlistName() const
{
    return m_nameEdit->text().trimmed();
}

QStringList CreateDialog::selectedFiles() const
{
    QStringList files;
    for (int i = 0; i < m_filesList->count(); ++i) {
        files.append(m_filesList->item(i)->text());
    }
    return files;
}