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
    emit sigFinished();
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
    for ( auto && ii : fMD5Threads )
    {
        if ( !ii )
            continue;
        ii->stop();
    }

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

            auto md5 = new NSABUtils::CComputeMD5( curr );
            connect( md5, &NSABUtils::CComputeMD5::sigStarted, this, &CFileFinder::sigMD5FileStarted );
            connect( md5, &NSABUtils::CComputeMD5::sigReadPositionStatus, this, &CFileFinder::sigMD5ReadPositionStatus );
            connect( md5, &NSABUtils::CComputeMD5::sigFinishedReading, this, &CFileFinder::sigMD5FileFinishedReading );
            connect( md5, &NSABUtils::CComputeMD5::sigFinishedComputing, this, &CFileFinder::sigMD5FileFinishedComputing );
            connect( md5, &NSABUtils::CComputeMD5::sigFinished, this, &CFileFinder::sigMD5FileFinished );
            connect( this, &CFileFinder::sigStopped, md5, &NSABUtils::CComputeMD5::slotStop );

            auto priority = getPriority( fi );
            QThreadPool::globalInstance()->start( md5, priority );
            fMD5Threads.push_back( md5 );
        }
    }
    emit sigDirFinished( dirName );
}



