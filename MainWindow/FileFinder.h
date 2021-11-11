#ifndef FILEFINDER_H
#define FILEFINDER_H

#include "SABUtils/QtUtils.h"

#include <QRunnable>
#include <QObject>
#include <unordered_set>
#include <QString>

class QFileInfo;
class CFileFinder : public QObject, public QRunnable
{
    Q_OBJECT;
public:
    CFileFinder( QObject * parent );
    void setRootDir( const QString& rootDir ) { fRootDir = rootDir;  }
    void setIgnoredDirs( const NQtUtils::TCaseInsensitiveHash & ignoredDirs ) { fIgnoredDirs = ignoredDirs;  }
    void setIgnoreHidden( bool ignoreHidden ) { fIgnoreHidden = ignoreHidden;  }
    void run() override;

    int numFilesFound() const { return fFilesFound; }
    void reset();
public Q_SLOTS:
    void slotStop();
Q_SIGNALS:
    void sigStopped();
    void sigFinished();
    void sigCurrentFindInfo( const QString& fileName );
    void sigFilesFound( int numFileFound );

    void sigMD5FileStarted( unsigned long long threadID, const QDateTime& dt, const QString& filename );
    void sigMD5ReadPositionStatus( unsigned long long threadID, const QDateTime &dt, const QString &filename, qint64 pos );
    void sigMD5FileFinishedReading( unsigned long long threadID, const QDateTime& dt, const QString& filename );
    void sigMD5FileFinishedComputing( unsigned long long threadID, const QDateTime& dt, const QString& filename );
    void sigMD5FileFinished( unsigned long long threadID, const QDateTime& dt, const QString& filename, const QString& md5 );
    void sigDirFinished( const QString& dirName );

private:
    int getPriority( const QFileInfo &fi ) const;
    void findFiles( const QString &dirName );
    bool fStopped{ false };
    bool fIgnoreHidden{ false };
    QString fRootDir;
    NQtUtils::TCaseInsensitiveHash fIgnoredDirs;
    int fFilesFound{ 0 };
    std::list< std::pair< QFileInfo, QRunnable * > > fQueuedMD5;
};

#endif // ORDERPROCESSOR_H
