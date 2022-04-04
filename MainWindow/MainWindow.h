#ifndef CMAINWINDOW_H
#define CMAINWINDOW_H

#include "SABUtils/QtUtils.h"
#include <QPointer>
#include <QMainWindow>
#include <QDate>
#include <QList>
#include <QRunnable>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "SABUtils/HashUtils.h"

class CProgressDlg;
class QStandardItem;
class CFilterModel;
class QStandardItemModel;
class QFileInfo;
namespace Ui { class CMainWindow; }

class CFileFinder;

class CMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    CMainWindow( QWidget* parent = 0 );
    ~CMainWindow();

Q_SIGNALS:
    void sigMD5FileStarted( unsigned long long threadID, const QDateTime& dt, const QString& filename );
    void sigMD5ReadPositionStatus( unsigned long long threadID, const QDateTime &dt, const QString &filename, qint64 pos );
    void sigMD5FileFinishedReading( unsigned long long threadID, const QDateTime& dt, const QString& filename );
    void sigMD5FileFinishedComputing( unsigned long long threadID, const QDateTime& dt, const QString& filename );
    void sigMD5FileFinished( unsigned long long threadID, const QDateTime& dt, const QString& filename, const QString& md5 );

public Q_SLOTS:
    void slotGo();
    void slotFinished();
    void slotDelete();
    void slotSelectDir();
    void slotDirChanged();
    void slotShowDupesOnly();
    void slotNumFilesFinishedComputing( int numFiles );
    void slotAddFilesFound( int numFiles );

    void slotMD5FileFinished( unsigned long long threadID, const QDateTime& endTime, const QString& fileName, const QString& md5 );

    void slotCountDirFinished( const QString& dirName );
    void slotFindDirFinished( const QString& dirName );
    void slotAddIgnoredDir();
    void slotDelIgnoredDir();
private:
    QList< QStandardItem* > getFileRow( const QFileInfo & fi, const QString& md5 );
   
    NSABUtils::TCaseInsensitiveHash getIgnoredDirs() const;
    void addIgnoredDir( const QString& ignoredDir );
    void addIgnoredDirs( QStringList ignoredDirs );

    int fileCount( int row ) const;
    int fileCount( QStandardItem* item ) const;
    void setFileCount( int row, int count );
    void setFileCount( QStandardItem* item, int count );

    bool hasDuplicates() const;
    QList< QStandardItem* > filesToDelete( QStandardItem* rootFileFN );
    QStringList filesToDelete( int ii );
    bool deleteFile( QStandardItem* item ) const;
    void determineFilesToDelete( QStandardItem* item );
    void setDeleteFile( QStandardItem* item, bool deleteFile, bool handleChildren, bool handleColumns );
    QList< QStandardItem* > getAllFiles( QStandardItem* rootFileFN ) const;

    void initModel();
    QPointer< CProgressDlg > fProgress;
    QStandardItemModel* fModel;
    CFilterModel* fFilterModel;
    std::unique_ptr< Ui::CMainWindow > fImpl;
    std::unordered_map< QString, QList< QStandardItem* > > fMap;

    CFileFinder* fFileFinder{ nullptr };
    int fDupesFound{ 0 };
    int fMD5FilesComputed{ 0 };

    int fTotalFiles{ 0 };
    QDateTime fStartTime;
    QDateTime fEndTime;
};

#endif 
