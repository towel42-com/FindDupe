#include "ComputeNumFiles.h"
#include <QDir>
#include <QDirIterator>

CComputeNumFiles::CComputeNumFiles( const QString & rootDir, QObject * parent ) :
    QThread( parent ),
    fRootDir( rootDir )
{

}

CComputeNumFiles::~CComputeNumFiles()
{
}

void CComputeNumFiles::run()
{
    fNumFilesFound = 0;
    findNumFiles( fRootDir );

    emit sigNumFiles( fNumFilesFound );
}

void CComputeNumFiles::findNumFiles( const QString & dirName )
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
}

void CComputeNumFiles::stop( bool stopped )
{
    fStopped = stopped;
}

void CComputeNumFiles::slotStop()
{
    stop( true );
}
