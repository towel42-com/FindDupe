#include "ComputeNumFiles.h"
#include <QDir>
#include <QDirIterator>

CComputeNumFiles::CComputeNumFiles( const QString & rootDir, QObject * parent ) :
    QThread( parent ),
    fRootDir( rootDir )
{

}

void CComputeNumFiles::run()
{
    int numFiles = findNumFiles( fRootDir );

    emit sigNumFiles( numFiles );
}

int CComputeNumFiles::findNumFiles( const QString & dirName )
{
    QDir dir( dirName );
    if ( !dir.exists() )
        return 0;

    int retVal = dir.count() - 2;
    QDirIterator di( dirName, QStringList() << "*", QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Readable, QDirIterator::NoIteratorFlags );
    while ( di.hasNext() )
    {
        auto curr = di.next();
        QFileInfo fi( curr );
        if ( fi.isDir() )
        {
            retVal += findNumFiles( curr );
        }
    }

    return retVal;
}