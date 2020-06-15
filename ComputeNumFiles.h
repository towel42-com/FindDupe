#ifndef COMPUTE_NUM_FILES_H
#define COMPUTE_NUM_FILES_H

#include <QThread>

class CComputeNumFiles : public QThread
{
    Q_OBJECT
public:
    CComputeNumFiles( const QString & rootDir, QObject * parent );

    void run() override;

signals:
    void sigNumFiles( int numFiles );
private:
    int findNumFiles( const QString & dirName );
    QString fRootDir;
};

#endif
