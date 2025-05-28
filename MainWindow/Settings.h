#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>
#include <optional>
#include "SABUtils/SABUtilsFwd.h"
//#include "SABUtils/QtUtils.h"
//#include <QPointer>
//#include <QMainWindow>
//#include <QDate>
//#include <QList>
//#include <QRunnable>
//#include <memory>
//#include <optional>
//#include <unordered_map>
//#include <unordered_set>
//
//#include "SABUtils/HashUtils.h"
//
//class CProgressDlg;
//class QStandardItem;
//class CFilterModel;
//class QStandardItemModel;
//class QFileInfo;
//class QThreadPool;
namespace Ui
{
    class CSettings;
}

class CFileFinder;

class CSettings : public QDialog
{
    Q_OBJECT
public:
    CSettings( QWidget *parent = 0 );
    ~CSettings();

    static bool showDupesOnly();
    static bool ignoreHidden();
    static std::optional< int > ignoreFilesOverMB();
    static bool caseInsensitiveNameCompare();
    static NSABUtils::TCaseInsensitiveHash getIgnoredPathNames();

    virtual void accept() override;

Q_SIGNALS:

public Q_SLOTS:
    //void slotGo();
    //void slotFinished();

    //void slotDelete();

    //void slotSelectDir();
    //void slotDirChanged();
    //void slotShowDupesOnly();
    //void slotNumFilesFinishedComputing( int numFiles );

    //void slotAddFilesFound( int numFiles );
    //void slotFileDoubleClicked( const QModelIndex &idx );
    //void slotFileContextMenu( const QPoint &pos );
    //void slotMD5FileFinished( unsigned long long threadID, const QDateTime &endTime, const QString &fileName, const QString &md5 );

    //void slotCountDirFinished( const QString &dirName );
    //void slotFindDirFinished( const QString &dirName );

    void slotAddIgnoredPathName();
    void slotDelIgnoredPathName();

    void slotIgnoreFilesOver();
    //void slotWaitForAllThreadsFinished();

private:
    //bool isFinished();

    //QList< QStandardItem * > createFileRow( const QFileInfo &fi, const QString &md5 );
    //bool hasChildFile( QStandardItem *header, const QFileInfo &fi ) const;
    //QFileInfo getFileInfo( QStandardItem *item ) const;

    //void updateResultsLabel();

    //NSABUtils::TCaseInsensitiveHash getIgnoredPathNames() const;
    void addIgnoredPathName( const QString &ignoredPathName );
    void addIgnoredPathNames( const NSABUtils::TCaseInsensitiveHash &ignoredPathNames );

    //int fileCount( int row ) const;
    //int fileCount( QStandardItem *item ) const;
    //void setFileCount( int row, int count );
    //void setFileCount( QStandardItem *item, int count );

    //bool hasDuplicates() const;
    //QStringList filesToDelete( int ii );
    //QStringList filesToDelete( QStandardItem *item );

    //QStandardItem * itemFromFilterRow( int ii );

    //bool deleteFile( QStandardItem *item ) const;
    //void determineFilesToDeleteRoot( QStandardItem *item );
    //QList< QStandardItem * > determineFilesToDelete( QStandardItem *rootFileFN );
    //void setDeleteFile( QStandardItem *item, bool deleteFile, bool handleChildren );
    //QList< QStandardItem * > getAllFiles( QStandardItem *rootFileFN ) const;
    //void showIcons();
    //void showIcons( QStandardItem *item );
    //void deleteFiles( const QStringList &filesToDelete );

    //void initModel();
    //QPointer< CProgressDlg > fProgress;
    //QStandardItemModel *fModel;
    //CFilterModel *fFilterModel;
    std::unique_ptr< Ui::CSettings > fImpl;
    //std::unordered_map< QString, std::pair< QStandardItem *, QStandardItem * > > fMap;

    //CFileFinder *fFileFinder{ nullptr };
    //std::pair< int, uint64_t > fDupesFound{ 0, 0 };   // number of dupes, size of dupes
    //int fMD5FilesComputed{ 0 };

    //int fTotalFiles{ 0 };
    //std::optional< std::pair< int, QDateTime > > fCheckForFinished;   // 5 times with over a 500 ms second delay.
    //QDateTime fStartTime;
    //QDateTime fEndTime;
};

#endif
