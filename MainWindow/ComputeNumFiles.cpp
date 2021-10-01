#include "ComputeNumFiles.h"
#include <QDir>
#include <QDirIterator>
#include <QDebug>
CComputeNumFiles::CComputeNumFiles( const QString& rootDir ) 
{
    fRootDir = QFileInfo( rootDir ).absoluteFilePath();
}

CComputeNumFiles::~CComputeNumFiles()
{
}

void CComputeNumFiles::run()
{
    fNumFilesFound = 0;
    findNumFiles( fRootDir );

    emit sigNumFiles( fNumFilesFound );
    emit sigFinished();
}

void CComputeNumFiles::findNumFiles( const QString& dirName )
{
    QDir dir( dirName );
    if ( !dir.exists() )
        return;

    fNumFilesFound += dir.count() - 2;
    QDirIterator di( dirName, QStringList() << "*", QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Readable, QDirIterator::NoIteratorFlags );
    while ( !fStopped && di.hasNext() )
    {
        auto curr = di.next();
        QFileInfo fi( curr );
        if ( fi.isDir() )
        {
            findNumFiles( curr );
            emit sigNumFilesSub( fNumFilesFound );
        }
    }
    emit sigDirFinished( dirName );
}

void CComputeNumFiles::stop( bool stopped )
{
    qDebug() << "File Counter Stopped";
    fStopped = stopped;
}

void CComputeNumFiles::slotStop()
{
    stop( true );
}
