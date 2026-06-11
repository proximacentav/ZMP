#include "deletedialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>

DeleteDialog::DeleteDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Удаление плейлиста");
    setModal(true);
    resize(300, 150);
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Введите название плейлиста для удаления:"));
    m_nameEdit = new QLineEdit;
    layout->addWidget(m_nameEdit);
    
    QHBoxLayout *btnLayout = new QHBoxLayout;
    QPushButton *okBtn = new QPushButton("Удалить");
    QPushButton *cancelBtn = new QPushButton("Отмена");
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);
    
    connect(okBtn, &QPushButton::clicked, this, &DeleteDialog::onConfirm);
    connect(cancelBtn, &QPushButton::clicked, this, &DeleteDialog::reject);
}

void DeleteDialog::onConfirm()
{
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите название.");
        return;
    }
    accept();
}

QString DeleteDialog::playlistName() const
{
    return m_nameEdit->text().trimmed();
}