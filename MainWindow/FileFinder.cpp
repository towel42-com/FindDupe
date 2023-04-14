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
    fIgnoredFileNames.clear();
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
        if ( !ii.second )
            continue;
        ii.second->stop();
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
            if ( fIgnoreFilesOver.first && ( fi.size() >= static_cast< qint64 >( fIgnoreFilesOver.second )*1024LL*1024LL ) )
                 continue;

            if ( isIgnoredFile( fi ) )
                continue;

            fNumFilesFound++;

            emit sigCurrentFindInfo( fi.absoluteFilePath() );
            emit sigFilesFound( fNumFilesFound );

            processFile( curr );
        }
    }
    emit sigDirFinished( dirName );
}

bool CFileFinder::isIgnoredFile( const QFileInfo & fi ) const
{
    bool isWC = false;
    auto fileName = fi.fileName().toLower();
    for ( auto && ii : fIgnoredFileNames )
    {
        if ( ii.first.isEmpty() )
        {
            auto match = ii.second.match( fileName );
            if ( match.hasMatch() )
                return true;
        }
        else if ( ii.first == fileName )
            return true;
    }
    return false;
}
 
void CFileFinder::processFile( const QString & fileName )
{
    if ( fCaseInsensitiveNameCompare )
    {
        auto fn = QFileInfo( fileName ).fileName().toLower();
        emit sigMD5FileStarted( 0, QDateTime::currentDateTime(), fileName );
        auto md5Results = NSABUtils::getMd5( fn, false );
        emit sigMD5FileFinished( 0, QDateTime::currentDateTime(), fileName, md5Results );
        return;
    }

    auto md5 = new NSABUtils::CComputeMD5( fileName );
    connect( md5, &NSABUtils::CComputeMD5::sigStarted, this, &CFileFinder::sigMD5FileStarted );
    connect( md5, &NSABUtils::CComputeMD5::sigReadPositionStatus, this, &CFileFinder::sigMD5ReadPositionStatus );
    connect( md5, &NSABUtils::CComputeMD5::sigFinishedReading, this, &CFileFinder::sigMD5FileFinishedReading );
    connect( md5, &NSABUtils::CComputeMD5::sigFinishedComputing, this, &CFileFinder::sigMD5FileFinishedComputing );
    connect( md5, &NSABUtils::CComputeMD5::sigFinished, this, &CFileFinder::sigMD5FileFinished );
    connect( md5, &NSABUtils::CComputeMD5::sigFinished, this, &CFileFinder::slotMD5FileFinished );
    connect( this, &CFileFinder::sigStopped, md5, &NSABUtils::CComputeMD5::slotStop );

    auto priority = getPriority( fileName );
    QThreadPool::globalInstance()->start( md5, priority );
    fMD5Threads[ fileName ] = md5;
}

void CFileFinder::slotMD5FileFinished( unsigned long long /*threadID*/, const QDateTime &/*dt*/, const QString &filename, const QString &/*md5*/ )
{
    auto pos = fMD5Threads.find( filename );
    if ( pos == fMD5Threads.end() )
        return;
    fMD5Threads.erase( pos );
}

void CFileFinder::setIgnoredFileNames( const NSABUtils::TCaseInsensitiveHash & ignoredFileNames )
{
    fIgnoredFileNames.clear();
    for ( auto && ii : ignoredFileNames )
    {
        bool isWC = false;
        if ( ii.contains( "*" ) )
            isWC = true;
        else if ( ii.contains( "?" ) )
            isWC = true;
        if ( !isWC )
            fIgnoredFileNames.push_back( { ii.toLower(), QRegularExpression() } );
        else
            fIgnoredFileNames.push_back( { QString(), QRegularExpression( "^" + ii + "$", QRegularExpression::CaseInsensitiveOption ) } );
    }
}

void CFileFinder::setIgnoreFilesOver( bool ignored, int ignoreOverMB )
{
    fIgnoreFilesOver = { ignored, ignoreOverMB };
}


CComputeNumFiles::CComputeNumFiles( QObject * parent ) :
    CFileFinder( parent )
{

}

void CComputeNumFiles::processFile( const QString & fileName )
{
    (void)fileName;
}

