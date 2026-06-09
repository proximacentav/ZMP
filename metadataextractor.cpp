#include "metadataextractor.h"
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>
#include <QFileInfo>
#include <QDebug>

MetadataExtractor::MetadataExtractor(QObject *parent) : QObject(parent) {}

TrackMetadata MetadataExtractor::extract(const QString &filePath) {
    TrackMetadata data;
    QFileInfo fileInfo(filePath);
    data.title = fileInfo.baseName();


    if (!fileInfo.exists() || !fileInfo.isFile()) {
        qWarning() << "File does not exist or is not a regular file:" << filePath;
        return data;
    }


    TagLib::FileName fileName(filePath.toUtf8().constData());
    TagLib::FileRef fileRef(fileName, true, TagLib::AudioProperties::Accurate);

    if (fileRef.isNull() || !fileRef.tag()) {
        qWarning() << "TagLib cannot open file or no tag found:" << filePath;
        return data;
    }

    TagLib::Tag *tag = fileRef.tag();
    if (tag) {

        if (!tag->title().isEmpty())
            data.title = tag->title().toCString(true);
        if (!tag->artist().isEmpty())
            data.artist = tag->artist().toCString(true);
        if (!tag->album().isEmpty())
            data.album = tag->album().toCString(true);
        data.year = tag->year();
    }


    TagLib::MPEG::File *mpegFile = dynamic_cast<TagLib::MPEG::File*>(fileRef.file());
    if (mpegFile && mpegFile->ID3v2Tag()) {
        TagLib::ID3v2::Tag *id3v2Tag = mpegFile->ID3v2Tag();
        if (id3v2Tag) {
            const TagLib::ID3v2::FrameListMap &frameListMap = id3v2Tag->frameListMap();
            if (frameListMap.contains("APIC")) {
                const TagLib::ID3v2::FrameList &pictures = frameListMap["APIC"];
                if (!pictures.isEmpty()) {
                    const TagLib::ID3v2::AttachedPictureFrame *picFrame =
                        dynamic_cast<const TagLib::ID3v2::AttachedPictureFrame*>(pictures.front());
                    if (picFrame && picFrame->picture().size() > 0) {
                        const TagLib::ByteVector &pictureData = picFrame->picture();
                        if (data.cover.loadFromData((const uchar*)pictureData.data(),
                                                    (int)pictureData.size())) {
                            qDebug() << "Cover image successfully loaded.";
                        } else {
                            qWarning() << "Failed to load cover image data.";
                        }
                    }
                }
            }
        }
    }

    qDebug() << "Metadata:" << data.title << data.artist << data.album;
    return data;
}