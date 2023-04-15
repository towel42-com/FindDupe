#ifndef CMAINWINDOW_H
#define CMAINWINDOW_H

#include "SABUtils/QtUtils.h"
#include <QPointer>
#include <QMainWindow>
#include <QDate>
#include <QList>
#include <QRunnable>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "SABUtils/HashUtils.h"

class CProgressDlg;
class QStandardItem;
class CFilterModel;
class QStandardItemModel;
class QFileInfo;
class QThreadPool;
namespace Ui
{
    class CMainWindow;
}

class CFileFinder;

class CMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    CMainWindow( QWidget *parent = 0 );
    ~CMainWindow();

    static QThreadPool *threadPool();
Q_SIGNALS:
    void sigMD5FileStarted( unsigned long long threadID, const QDateTime &dt, const QString &filename );
    void sigMD5ReadPositionStatus( unsigned long long threadID, const QDateTime &dt, const QString &filename, qint64 pos );
    void sigMD5FileFinishedReading( unsigned long long threadID, const QDateTime &dt, const QString &filename );
    void sigMD5FileFinishedComputing( unsigned long long threadID, const QDateTime &dt, const QString &filename );
    void sigMD5FileFinished( unsigned long long threadID, const QDateTime &dt, const QString &filename, const QString &md5 );

public Q_SLOTS:
    void slotGo();
    void slotFinished();

    void slotDelete();


    void slotSelectDir();
    void slotDirChanged();
    void slotShowDupesOnly();
    void slotNumFilesFinishedComputing( int numFiles );

    void slotAddFilesFound( int numFiles );
    void slotFileDoubleClicked( const QModelIndex &idx );
    void slotFileContextMenu( const QPoint &pos );
    void slotMD5FileFinished( unsigned long long threadID, const QDateTime &endTime, const QString &fileName, const QString &md5 );

    void slotCountDirFinished( const QString &dirName );
    void slotFindDirFinished( const QString &dirName );

    void slotAddIgnoredPathName();
    void slotDelIgnoredPathName();

    void slotIgnoreFilesOver();
    void slotWaitForAllThreadsFinished();

private:
    bool isFinished();

    QList< QStandardItem * > createFileRow( const QFileInfo &fi, const QString &md5 );
    bool hasChildFile( QStandardItem *header, const QFileInfo &fi ) const;
    QFileInfo getFileInfo( QStandardItem *item ) const;

    void updateResultsLabel();

    NSABUtils::TCaseInsensitiveHash getIgnoredPathNames() const;
    void addIgnoredPathName( const QString &ignoredPathName );
    void addIgnoredPathNames( QStringList ignoredPathNames );

    int fileCount( int row ) const;
    int fileCount( QStandardItem *item ) const;
    void setFileCount( int row, int count );
    void setFileCount( QStandardItem *item, int count );

    bool hasDuplicates() const;
    QStringList filesToDelete( int ii );
    QStringList filesToDelete( QStandardItem *item );

    QStandardItem * itemFromFilterRow( int ii );

    bool deleteFile( QStandardItem *item ) const;
    void determineFilesToDeleteRoot( QStandardItem *item );
    QList< QStandardItem * > determineFilesToDelete( QStandardItem *rootFileFN );
    void setDeleteFile( QStandardItem *item, bool deleteFile, bool handleChildren );
    QList< QStandardItem * > getAllFiles( QStandardItem *rootFileFN ) const;
    void showIcons();
    void showIcons( QStandardItem *item );
    void deleteFiles( const QStringList &filesToDelete );

    void initModel();
    QPointer< CProgressDlg > fProgress;
    QStandardItemModel *fModel;
    CFilterModel *fFilterModel;
    std::unique_ptr< Ui::CMainWindow > fImpl;
    std::unordered_map< QString, std::pair< QStandardItem *, QStandardItem * > > fMap;

    CFileFinder *fFileFinder{ nullptr };
    std::pair< int, uint64_t > fDupesFound{ 0, 0 };   // number of dupes, size of dupes
    int fMD5FilesComputed{ 0 };

    int fTotalFiles{ 0 };
    std::optional< std::pair< int, QDateTime > > fCheckForFinished;   // 5 times with over a 500 ms second delay.
    QDateTime fStartTime;
    QDateTime fEndTime;
};

#endif
