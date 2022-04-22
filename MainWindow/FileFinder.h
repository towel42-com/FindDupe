#ifndef FILEFINDER_H
#define FILEFINDER_H

#include "SABUtils/QtUtils.h"
#include <QRegularExpression>
#include <QRunnable>
#include <QObject>
#include <unordered_set>
#include <QString>
#include <QPointer>
namespace NSABUtils
{
    class CComputeMD5;
}

class QFileInfo;
class CFileFinder : public QObject, public QRunnable
{
    Q_OBJECT;
public:
    CFileFinder( QObject * parent );
    virtual ~CFileFinder() override = default;

    void setRootDir( const QString& rootDir ) { fRootDir = rootDir;  }
    void setIgnoredDirs( const NSABUtils::TCaseInsensitiveHash & ignoredDirs ) { fIgnoredDirs = ignoredDirs;  }
    void setIgnoredFileNames( const NSABUtils::TCaseInsensitiveHash & ignoredFileNames );
    void setIgnoreHidden( bool ignoreHidden ) { fIgnoreHidden = ignoreHidden;  }
    void setIgnoreFilesOver( bool ignore, int ignoreOverMB );
    void setCaseInsensitiveNameCompare( bool caseInsensitiveNameCompare ) { fCaseInsensitiveNameCompare = caseInsensitiveNameCompare; }

    void run() override;

    int numFilesFound() const { return fNumFilesFound; }
    void reset();
public Q_SLOTS:
    void slotStop();
Q_SIGNALS:
    void sigStopped();
    void sigFinished();
    void sigCurrentFindInfo( const QString& fileName );

    void sigNumFilesFinished( int numFiles ); // when the thread is finished finding all files
    void sigFilesFound( int numFileFound );

    void sigMD5FileStarted( unsigned long long threadID, const QDateTime& dt, const QString& filename );
    void sigMD5ReadPositionStatus( unsigned long long threadID, const QDateTime &dt, const QString &filename, qint64 pos );
    void sigMD5FileFinishedReading( unsigned long long threadID, const QDateTime& dt, const QString& filename );
    void sigMD5FileFinishedComputing( unsigned long long threadID, const QDateTime& dt, const QString& filename );
    void sigMD5FileFinished( unsigned long long threadID, const QDateTime& dt, const QString& filename, const QString& md5 );
    void sigDirFinished( const QString& dirName );

protected:
    int getPriority( const QString & fileName ) const;
    virtual void processDir( const QString &dirName );

    bool isIgnoredFile( const QFileInfo & fi ) const;

    virtual void processFile( const QString & fileName );

    bool fStopped{ false };
    bool fIgnoreHidden{ false };
    QString fRootDir;
    NSABUtils::TCaseInsensitiveHash fIgnoredDirs;
    std::list< std::pair< QString, QRegularExpression > > fIgnoredFileNames;
    int fNumFilesFound{ 0 };
    std::list< QPointer< NSABUtils::CComputeMD5 > > fMD5Threads;
    std::pair< bool, int > fIgnoreFilesOver{ false, 0 };
    bool fCaseInsensitiveNameCompare{ false };
};

class CComputeNumFiles : public CFileFinder
{
public:
    CComputeNumFiles( QObject * parent );

    virtual void processFile( const QString & fileName ) override;
};

#endif 
