#ifndef CMAINWINDOW_H
#define CMAINWINDOW_H

#include "SABUtils/QtUtils.h"

#include <QMainWindow>
#include <QDate>
#include <QList>
#include <memory>
#include <unordered_map>
#include <unordered_set>

class CProgressDlg;
class QStandardItem;
class CFilterModel;
class QStandardItemModel;
namespace Ui { class CMainWindow; }


struct CCaseInsensitiveHash
{
	size_t operator()(const QString& str) const;
};

struct CCaseInsensitiveEqual
{
	size_t operator()(const QString& lhs, const QString& rhs) const;
};

using TDirSet = std::unordered_set< QString, CCaseInsensitiveHash, CCaseInsensitiveEqual > ;

class CMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    CMainWindow( QWidget *parent = 0 );
	~CMainWindow();

Q_SIGNALS:
	void sigFileComputing(const QString & currFile );
	void sigFileComputed(const QString& currFile, const QString& md5);

public Q_SLOTS:
    void slotGo();
    void slotFinished();
    void slotDelete();
    void slotSelectDir();
    void slotDirChanged();
    void slotShowDupesOnly();
    void slotNumFilesComputed( int numFiles );
    void slotAddFilesFound( int numFiles );
    void slotFileComputed(const QString& fileName, const QString& md5);
    void slotFileComputing(const QString& fileName);

    void slotAddIgnoredDir();
    void slotDelIgnoredDir();
private:
    TDirSet getIgnoredDirs() const;
	void addIgnoredDir( const QString & ignoredDir );
	void addIgnoredDirs( QStringList ignoredDirs );

	int fileCount(int row) const;
    int fileCount( QStandardItem * item ) const;
    void setFileCount( int row, int count );
    void setFileCount( QStandardItem * item, int count );

    void findFiles( const QString & dirName );

    bool hasDuplicates() const;
    QList< QStandardItem * > filesToDelete( QStandardItem * rootFileFN );
    QStringList filesToDelete( int ii );
    bool deleteFile( QStandardItem * item ) const;
    void determineFilesToDelete( QStandardItem * item );
    void setDeleteFile( QStandardItem * item, bool deleteFile, bool handleChildren, bool handleColumns );
    QList< QStandardItem * > getAllFiles( QStandardItem * rootFileFN ) const;

    void initModel();
    CProgressDlg * fProgress{ nullptr };
    QStandardItemModel * fModel;
    CFilterModel * fFilterModel;
    std::unique_ptr< Ui::CMainWindow > fImpl;
    std::unordered_map< QString, QList< QStandardItem * > > fMap;

    int fDupesFound{ 0 };
    int fFilesFound{0};
    int fMD5FilesComputed{ 0 };

    int fTotalFiles{0};
    QDateTime fStartTime;
    QDateTime fEndTime;
};

#endif // ORDERPROCESSOR_H
