#include "metadataextractor.h"
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>
#include <QFileInfo>
#include <QDebug>

MetadataExtractor::MetadataExtractor(QObject *parent)
    : QObject(parent)
{
}

TrackMetadata MetadataExtractor::extract(const QString &filePath)
{
    TrackMetadata data;
    QFileInfo fileInfo(filePath);
    data.title = fileInfo.baseName();

    TagLib::FileRef f(filePath.toLocal8Bit().data());
    if (!f.isNull() && f.tag()) {
        TagLib::Tag *tag = f.tag();
        if (!tag->title().isEmpty())
            data.title = tag->title().toCString(true);
        if (!tag->artist().isEmpty())
            data.artist = tag->artist().toCString(true);
        if (!tag->album().isEmpty())
            data.album = tag->album().toCString(true);
        data.year = tag->year();
    }

    if (filePath.endsWith(".mp3", Qt::CaseInsensitive)) {
        TagLib::MPEG::File file(filePath.toLocal8Bit().data());
        if (file.ID3v2Tag()) {
            auto frameList = file.ID3v2Tag()->frameList("APIC");
            if (!frameList.isEmpty()) {
                auto *frame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(frameList.front());
                if (frame && frame->picture().size() > 0) {
                    data.cover.loadFromData((const uchar*)frame->picture().data(), frame->picture().size());
                }
            }
        }
    }

    return data;
}