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
    processDir( fRootDir );

    emit sigNumFilesFinished( fNumFilesFound );
    emit sigFinished();
}

void CFileFinder::reset()
{
    fStopped = false;
    fIgnoreHidden = false;
    fRootDir.clear();
    fIgnoredDirs.clear();
    fMD5Threads.clear();
    fNumFilesFound = 0;
}

void CFileFinder::slotStop()
{
    qDebug() << "File Finder Stopped";
    fStopped = true; 
    QThreadPool::globalInstance()->clear();
    for ( auto && ii : fMD5Threads )
    {
        if ( !ii )
            continue;
        ii->stop();
    }
    fMD5Threads.clear();

    emit sigStopped();
}

int CFileFinder::getPriority( const QString & fileName ) const
{
    QFileInfo fi( fileName );

    auto sz = fi.size();
    auto limit = 1000;
    auto priority = 10;
    while( priority > 1 )
    {
        if ( sz < limit )
            return priority;
        limit *= 10;
        priority--;

    }
    return priority;
}

void CFileFinder::processDir( const QString& dirName )
{
    QDir dir( dirName );
    if ( !dir.exists() )
        return;

    QDirIterator di( dirName, QStringList() << "*", QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Readable, QDirIterator::NoIteratorFlags );
    while ( !fStopped && di.hasNext() )
    {
        auto curr = di.next();

        QFileInfo fi( curr );
        if ( fIgnoreHidden && ( fi.isHidden() || fi.fileName().startsWith( "." ) ) )
            continue;
        if ( fi.isDir() )
        {
            if ( fIgnoredDirs.find( curr ) != fIgnoredDirs.end() )
                continue;
            processDir( curr );
            emit sigFilesFound( fNumFilesFound );
        }
        else
        {
            fNumFilesFound++;

            emit sigCurrentFindInfo( fi.absoluteFilePath() );
            emit sigFilesFound( fNumFilesFound );

            processFile( curr );
        }
    }
    emit sigDirFinished( dirName );
}

void CFileFinder::processFile( const QString & fileName )
{
    auto md5 = new NSABUtils::CComputeMD5( fileName );
    connect( md5, &NSABUtils::CComputeMD5::sigStarted, this, &CFileFinder::sigMD5FileStarted );
    connect( md5, &NSABUtils::CComputeMD5::sigReadPositionStatus, this, &CFileFinder::sigMD5ReadPositionStatus );
    connect( md5, &NSABUtils::CComputeMD5::sigFinishedReading, this, &CFileFinder::sigMD5FileFinishedReading );
    connect( md5, &NSABUtils::CComputeMD5::sigFinishedComputing, this, &CFileFinder::sigMD5FileFinishedComputing );
    connect( md5, &NSABUtils::CComputeMD5::sigFinished, this, &CFileFinder::sigMD5FileFinished );
    connect( this, &CFileFinder::sigStopped, md5, &NSABUtils::CComputeMD5::slotStop );

    auto priority = getPriority( fileName );
    QThreadPool::globalInstance()->start( md5, priority );
    fMD5Threads.emplace_back( md5 );
}


CComputeNumFiles::CComputeNumFiles( QObject * parent ) :
    CFileFinder( parent )
{

}

void CComputeNumFiles::processFile( const QString & fileName )
{
    (void)fileName;
}
