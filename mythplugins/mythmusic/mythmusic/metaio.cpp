// Qt
#include <QFileInfo>

// Mythmusic
#include "metaio.h"
#include "metadata.h"
#include "metaioid3.h"
#include "metaiooggvorbis.h"
#include "metaioflacvorbis.h"
#include "metaiomp4.h"
#include "metaiowavpack.h"
#include "metaioavfcomment.h"

// Libmyth
#include <mythcontext.h>

/*!
 * \brief Constructor
 */
MetaIO::MetaIO()
{
    mFilenameFormat = gCoreContext->GetSetting("NonID3FileNameFormat").toUpper();
}

/*!
 * \brief Destructor
 */
MetaIO::~MetaIO()
{
}

// static
MetaIO* MetaIO::createTagger(const QString& filename)
{
    QFileInfo fi(filename);
    QString extension = fi.suffix().toLower();

    if (extension == "mp3" || extension == "mp2")
        return new MetaIOID3;
    else if (extension == "ogg" || extension == "oga")
        return new MetaIOOggVorbis;
    else if (extension == "flac")
    {
        MetaIOID3 *tagger = new MetaIOID3;
        if (tagger->TagExists(filename))
            return tagger;
        else
        {
            delete tagger;
            return new MetaIOFLACVorbis;
        }
    }
    else if (extension == "m4a")
        return new MetaIOMP4;
    else if (extension == "wv")
        return new MetaIOWavPack;
    else
        return new MetaIOAVFComment;
}

// static
Metadata* MetaIO::readMetadata(const QString &filename)
{
    Metadata *mdata = NULL;
    MetaIO *tagger = MetaIO::createTagger(filename);
    bool ignoreID3 = (gCoreContext->GetNumSetting("Ignore_ID3", 0) == 1);

    if (tagger)
    {
        if (!ignoreID3)
            mdata = tagger->read(filename);

        if (ignoreID3 || !mdata)
            mdata = tagger->readFromFilename(filename);

        delete tagger;
    }

    if (!mdata)
    {
        LOG(VB_GENERAL, LOG_ERR,
            QString("MetaIO::readMetadata(): Could not read '%1'")
                .arg(filename));
    }

    return mdata;
}

// static
Metadata* MetaIO::getMetadata(const QString &filename)
{

    Metadata *mdata = new Metadata(filename);
    if (mdata->isInDatabase())
        return mdata;

    delete mdata;

    return readMetadata(filename);
}

void MetaIO::readFromFilename(const QString &filename,
                              QString &artist, QString &album, QString &title,
                              QString &genre, int &tracknum)
{
    QString lfilename = filename;
    // Clear
    artist.clear();
    album.clear();
    title.clear();
    genre.clear();
    tracknum = 0;
    
    int part_num = 0;
    // Replace 
    lfilename.replace('_', ' ');
    lfilename.section('.', 0, -2);
    QStringList fmt_list = mFilenameFormat.split("/");
    QStringList::iterator fmt_it = fmt_list.begin();

    // go through loop once to get minimum part number
    for (; fmt_it != fmt_list.end(); ++fmt_it, --part_num) {}

    // reset to go through loop for real
    fmt_it = fmt_list.begin();
    for(; fmt_it != fmt_list.end(); ++fmt_it, ++part_num)
    {
        QString part_str = lfilename.section( "/", part_num, part_num);

        if ( *fmt_it == "GENRE" )
            genre = part_str;
        else if ( *fmt_it == "ARTIST" )
            artist = part_str;
        else if ( *fmt_it == "ALBUM" )
            album = part_str;
        else if ( *fmt_it == "TITLE" )
            title = part_str;
        else if ( *fmt_it == "TRACK_TITLE" )
        {
            QStringList tracktitle_list = part_str.split("-");
            if (tracktitle_list.size() > 1)
            {
                tracknum = tracktitle_list[0].toInt();
                title = tracktitle_list[1].simplified();
            }
            else
                title = part_str;
        }
        else if ( *fmt_it == "ARTIST_TITLE" )
        {
            QStringList artisttitle_list = part_str.split("-");
            if (artisttitle_list.size() > 1)
            {
                artist = artisttitle_list[0].simplified();
                title = artisttitle_list[1].simplified();
            }
            else
            {
                if (title.isEmpty())
                    title = part_str;
                if (artist.isEmpty())
                    title = part_str;
            }
        }
    }
}

Metadata* MetaIO::readFromFilename(const QString &filename, bool blnLength)
{
    QString artist, album, title, genre;
    int tracknum = 0, length = 0;

    readFromFilename(filename, artist, album, title, genre, tracknum);

    if (blnLength)
        length = getTrackLength(filename);

    Metadata *retdata = new Metadata(filename, artist, "", album, 
                                     title, genre, 0, tracknum, length);

    return retdata;
}

/*!
* \brief Reads Metadata based on the folder/filename.
*
* \param metadata Metadata Pointer
*/
void MetaIO::readFromFilename(Metadata* metadata)
{
    QString artist, album, title, genre;
    int tracknum = 0;

    const QString filename = metadata->Filename();

    if (filename.isEmpty())
        return;
    
    readFromFilename(filename, artist, album, title, genre, tracknum);

    if (metadata->Artist().isEmpty())
        metadata->setArtist(artist);
    
    if (metadata->Album().isEmpty())
        metadata->setAlbum(album);
    
    if (metadata->Title().isEmpty())
        metadata->setTitle(title);
    
    if (metadata->Genre().isEmpty())
        metadata->setGenre(genre);
    
    if (metadata->Track() <= 0)
        metadata->setTrack(tracknum);
}
