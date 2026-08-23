#include "aboutdialog.h"
#include "translator.h"

#include <QDialog>
#include <QFile>
#include <QLabel>
#include <QObject>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

void showAboutZmpDialog(QWidget *parent)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(ztr("О ZMP etc legal"));
    dlg.resize(720, 620);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QPlainTextEdit *licenseText = new QPlainTextEdit;
    licenseText->setReadOnly(true);
    licenseText->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    QFile f(":/license/LICENSE.txt");
    if (f.open(QIODevice::ReadOnly))
        licenseText->setPlainText(QString::fromUtf8(f.readAll()));
    else
        licenseText->setPlainText(ztr("Не удалось загрузить лицензию"));
    layout->addWidget(licenseText, 1);

    QLabel *creditsLabel = new QLabel(
        ztr("Использованы библиотеки:") + " Qt6, projectM, miniaudio, SoundTouch, KissFFT, FFmpeg, TagLib");
    creditsLabel->setWordWrap(true);
    creditsLabel->setStyleSheet("color: #888; font-size: 11px;");
    layout->addWidget(creditsLabel);

    QLabel *byLabel = new QLabel(
        ztr("от proximacentav:") +
        "<br><a href=\"https://github.com/proximacentav/ZMP\">https://github.com/proximacentav/ZMP</a>"
        "<br><a href=\"https://gitverse.ru/proximacentav/ZMP\">https://gitverse.ru/proximacentav/ZMP</a>");
    byLabel->setTextFormat(Qt::RichText);
    byLabel->setOpenExternalLinks(true);
    layout->addWidget(byLabel);

    QPushButton *okBtn = new QPushButton(ztr("ОК"));
    QObject::connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    layout->addWidget(okBtn);

    dlg.exec();
}
