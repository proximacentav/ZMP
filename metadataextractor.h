#ifndef METADATAEXTRACTOR_H
#define METADATAEXTRACTOR_H

#include <QObject>
#include <QImage>
#include <QString>

struct TrackMetadata {
    QString title;
    QString artist;
    QString album;
    int year = 0;
    QImage cover;
};

class MetadataExtractor : public QObject
{
    Q_OBJECT
public:
    explicit MetadataExtractor(QObject *parent = nullptr);
    TrackMetadata extract(const QString &filePath);
};

#endif // METADATAEXTRACTOR_H