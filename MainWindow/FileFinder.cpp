#include "FileFinder.h"
#include "SABUtils/MD5.h"
#include "SABUtils/utils.h"

#include <QThreadPool>
#include <QDir>
#include <QDirIterator>

CFileFinder::CFileFinder( QObject * parent ) :
    QObject( parent )
{
    setAutoDelete( false );
}

void CFileFinder::run()
{
    findFiles( fRootDir );
    for ( auto && ii : fQueuedMD5 )
    {
        auto priority = getPriority( ii.first );
        QThreadPool::globalInstance()->start( ii.second, priority );
    }

}


void CFileFinder::reset()
{
    fStopped = false;
    fIgnoreHidden = false;
    fRootDir.clear();
    fIgnoredDirs.clear();
    fFilesFound = 0;
}

void CFileFinder::slotStop()
{
    qDebug() << "File Finder Stopped";
    fStopped = true; 
    emit sigStopped();
}

int CFileFinder::getPriority( const QFileInfo & fi ) const
{
    auto sz = fi.size();
    auto limit = 1000;
    auto priority = 10;
    while( priority )
    {
        if ( sz < limit )
            return priority;
        limit *= 10;
        priority--;

    }
    return 0;
}

void CFileFinder::findFiles( const QString& dirName )
{
    QDir dir( dirName );
    if ( !dir.exists() )
        return;

    QDirIterator di( dirName, QStringList() << "*", QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Readable, QDirIterator::NoIteratorFlags );
    while ( di.hasNext() )
    {
        if ( fStopped )
            break;

        auto curr = di.next();

        QFileInfo fi( curr );
        if ( fIgnoreHidden && ( fi.isHidden() || fi.fileName().startsWith( "." ) ) )
            continue;
        if ( fi.isDir() )
        {
            if ( fIgnoredDirs.find( curr ) != fIgnoredDirs.end() )
                continue;
            findFiles( curr );
        }
        else
        {
            fFilesFound++;
            emit sigCurrentFindInfo( fi.absoluteFilePath() );
            emit sigFilesFound( fFilesFound );

            auto md5 = new NUtils::CComputeMD5( curr );
            connect( md5, &NUtils::CComputeMD5::sigStarted, this, &CFileFinder::sigMD5FileStarted );
            //connect( md5, &NUtils::CComputeMD5::sigReadPositionStatus, this, &CFileFinder::sigMD5ReadPositionStatus );
            connect( md5, &NUtils::CComputeMD5::sigFinishedReading, this, &CFileFinder::sigMD5FileFinishedReading );
            connect( md5, &NUtils::CComputeMD5::sigFinishedComputing, this, &CFileFinder::sigMD5FileFinishedComputing );
            connect( md5, &NUtils::CComputeMD5::sigFinished, this, &CFileFinder::sigMD5FileFinished );
            connect( this, &CFileFinder::sigStopped, md5, &NUtils::CComputeMD5::slotStop );

            auto priority = getPriority( fi );
            //if ( priority > 8 )
                QThreadPool::globalInstance()->start( md5, priority );
            //else
            //    fQueuedMD5.push_back( std::make_pair( fi, md5 ) );
        }
    }
    emit sigDirFinished( dirName );
}



