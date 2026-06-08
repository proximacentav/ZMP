#ifndef TRACKINFOWIDGET_H
#define TRACKINFOWIDGET_H

#include <QWidget>
#include <QLabel>
#include "metadataextractor.h"

class TrackInfoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TrackInfoWidget(QWidget *parent = nullptr);
    void updateInfo(const TrackMetadata &metadata);

private:
    QLabel *m_coverLabel;
    QLabel *m_titleLabel;
    QLabel *m_artistLabel;
    QLabel *m_albumYearLabel;
};

#endif // TRACKINFOWIDGET_H