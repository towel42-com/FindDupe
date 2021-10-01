#ifndef COMPUTE_NUM_FILES_H
#define COMPUTE_NUM_FILES_H

#include <QRunnable>
#include <QObject>

class CComputeNumFiles : public QObject, public QRunnable
{
    Q_OBJECT
public:
    CComputeNumFiles( const QString & rootDir );
    ~CComputeNumFiles();

    void run() override;
    void stop( bool stop );
public Q_SLOTS:
    void slotStop();
Q_SIGNALS:
    void sigNumFiles( int numFiles );
    void sigNumFilesSub( int numFiles );
    void sigDirFinished( const QString& dirName );
    void sigFinished();
private:
    void findNumFiles( const QString & dirName );
    QString fRootDir;
    bool fStopped{false};
    int fNumFilesFound{0};
};

#endif
