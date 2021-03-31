#include "MainWindow.h"
#include "ComputeNumFiles.h"
#include "SABUtils/MD5.h"
#include "ProgressDlg.h"
#include "ui_MainWindow.h"

#include <QFileDialog>
#include <QStandardItemModel>
#include <QSettings>
#include <QProgressBar>
#include <QDirIterator>
#include <QSortFilterProxyModel>
#include <QRegularExpression>
#include <QProgressDialog>
#include <QMessageBox>
#include <QThreadPool>
#include <QFontDatabase>
#include <random>

class CFilterModel : public QSortFilterProxyModel
{
public:
    CFilterModel( QObject * owner ) : 
        QSortFilterProxyModel( owner )
    {
    }
    void setShowDupesOnly( bool showDupesOnly )
    {
        if( showDupesOnly != fShowDupesOnly )
        {
            fShowDupesOnly = showDupesOnly;
            invalidate();
        }
    }

    void setLoadingValues( bool loading )
    {
        if ( loading != fLoadingValues )
        {
            fLoadingValues = loading;
            invalidate();
        }
    }

    virtual bool filterAcceptsRow( int source_row, const QModelIndex &source_parent ) const override
    {
        if ( source_parent.isValid() )
            return true; // all children are visible if parent is

        if ( !fShowDupesOnly )
            return true;

        QModelIndex sourceIdx = sourceModel()->index( source_row, 1, source_parent );
        auto cnt = sourceIdx.data().toString();
        if ( cnt.toInt() > 1 )
            return true;
        return false;
    }

    virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override
    {
        if ( fLoadingValues )
            return;

        if ( column == 1 )
            setSortRole( Qt::UserRole + 1 );
        else
            setSortRole( Qt::DisplayRole );
        QSortFilterProxyModel::sort( column, order );
    }

    virtual bool lessThan( const QModelIndex &source_left, const QModelIndex &source_right ) const override
    {
        return source_left.data().toInt() < source_right.data().toInt();
    }

    bool fLoadingValues{ false };
    bool fShowDupesOnly{ true };
};

CMainWindow::CMainWindow( QWidget *parent )
    : QMainWindow( parent ),
    fImpl( new Ui::CMainWindow )
{
    fImpl->setupUi( this );
    setWindowIcon( QIcon( ":/resources/finddupe.png" ) );
    setAttribute( Qt::WA_DeleteOnClose );

    fModel = new QStandardItemModel( this );
    initModel();
    fFilterModel = new CFilterModel( this );
    fFilterModel->setSourceModel( fModel );
    fImpl->files->setModel( fFilterModel );

    fImpl->go->setEnabled( false );
    fImpl->del->setEnabled( false );

    connect( fImpl->go,            &QToolButton::clicked,   this, &CMainWindow::slotGo );
    connect( fImpl->del,           &QToolButton::clicked,   this, &CMainWindow::slotDelete );
    connect( fImpl->selectDir,     &QToolButton::clicked,   this, &CMainWindow::slotSelectDir );
    connect( fImpl->dirName,       &QLineEdit::textChanged, this, &CMainWindow::slotDirChanged );
    connect( fImpl->showDupesOnly, &QCheckBox::clicked,     this, &CMainWindow::slotShowDupesOnly );

    QSettings settings;
    fImpl->dirName->setText( settings.value( "Dir", QString() ).toString() );
    fImpl->showDupesOnly->setChecked( settings.value( "ShowDupesOnly", true ).toBool() );

	fImpl->files->resizeColumnToContents(0);
	fImpl->files->setColumnWidth(0, 100);

    QThreadPool::globalInstance()->setExpiryTimeout(-1);
	connect(this, &CMainWindow::sigFileComputing, this, &CMainWindow::slotFileComputing);
	connect(this, &CMainWindow::sigFileComputed, this, &CMainWindow::slotFileComputed);
}

void CMainWindow::initModel()
{
    fModel->clear();
    fModel->setHorizontalHeaderLabels( QStringList() << "FileName" << "Count" << "Size" << "MD5" );
}

CMainWindow::~CMainWindow()
{
    QSettings settings;
    settings.setValue( "Dir", fImpl->dirName->text() );
    settings.setValue( "ShowDupesOnly", fImpl->showDupesOnly->isChecked() );
}

void CMainWindow::slotShowDupesOnly()
{
    fFilterModel->setShowDupesOnly( fImpl->showDupesOnly->isChecked() );
}

void CMainWindow::slotSelectDir()
{
    auto dir = QFileDialog::getExistingDirectory( this, "Select Directory", fImpl->dirName->text() );
    if ( dir.isEmpty() )
        return;

    fImpl->dirName->setText( dir );
}

void CMainWindow::slotDirChanged()
{
    initModel();
    QFileInfo fi( fImpl->dirName->text() );
    fImpl->go->setEnabled( fi.exists() && fi.isDir() );
}

void CMainWindow::slotFileComputing(const QString& fileName)
{
    fProgress->setCurrentMD5Info(QDir(fImpl->dirName->text()), QFileInfo(fileName) );
    fProgress->setNumActiveMD5(QThreadPool::globalInstance()->activeThreadCount());
}

void CMainWindow::slotFileComputed(const QString& fileName, const QString& md5)
{
    fMD5FilesComputed++;
	fProgress->setMD5Value(fMD5FilesComputed);
    auto fi = QFileInfo(fileName);
	fProgress->setCurrentMD5Info(QDir(fImpl->dirName->text()), fi);
	fProgress->setNumActiveMD5(QThreadPool::globalInstance()->activeThreadCount());

    auto pos = fMap.find(md5);
    QStandardItem* rootFNItem = nullptr;
    QStandardItem* countItem = nullptr;
    QStandardItem* sizeItem = nullptr;
    if (pos == fMap.end())
    {
        rootFNItem = new QStandardItem(fileName);
        auto md5Item = new QStandardItem(md5);
        md5Item->setTextAlignment(Qt::AlignmentFlag::AlignRight |Qt::AlignmentFlag::AlignVCenter);
        md5Item->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        countItem = new QStandardItem(QString());
		QLocale locale;
        sizeItem = new QStandardItem(locale.toString(fi.size()));
		sizeItem->setTextAlignment(Qt::AlignmentFlag::AlignRight | Qt::AlignmentFlag::AlignVCenter);
        setFileCount(countItem, 0);
        QList< QStandardItem* > row = QList< QStandardItem* >() << rootFNItem << countItem << sizeItem << md5Item;
        if (fi.size() != 0)
        {
            fMap[md5] = row;
            fModel->appendRow(row);
        }
    }
    else
    {
        rootFNItem = (*pos).second[0];
        countItem = (*pos).second[1];
    }
    auto newCount = fileCount(countItem) + 1;
    setFileCount(countItem, newCount);
    countItem->setText(QString::number(newCount));

    if (newCount != 1)
    {
        fDupesFound++;
        auto fileItem = new QStandardItem(fileName);
        rootFNItem->appendRow(fileItem);
        auto idx = fFilterModel->mapFromSource(fModel->indexFromItem(rootFNItem));
        if (idx.isValid())
            fImpl->files->setExpanded(idx, true);
        determineFilesToDelete(rootFNItem);
    }
    fProgress->setNumDuplicaes(fDupesFound);
}

void CMainWindow::findFiles( const QString & dirName )
{
    QDir dir( dirName );
    if ( !dir.exists() )
        return;

    QDirIterator di( dirName, QStringList() << "*", QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Readable, QDirIterator::NoIteratorFlags );
    while ( di.hasNext() )
    {
        auto curr = di.next();
        
        QFileInfo fi( curr );
        if ( fi.isDir() )
        {
            findFiles( curr );
        }
        else
        {
            fFilesFound++;
			fProgress->setCurrentFindInfo(QDir(fImpl->dirName->text()), fi);
			fProgress->setFindValue( fFilesFound );
            qApp->processEvents();

            QThreadPool::globalInstance()->start(
                [this, curr]()
                {
					emit sigFileComputing(curr);
					auto md5 = NUtils::getMd5(curr, true);
					emit sigFileComputed(curr, md5);
                });

            if ( ( fProgress->findValue() % 50 ) == 0 )
            {
                fProgress->adjustSize();
                fImpl->files->resizeColumnToContents( 0 );
                fImpl->files->setColumnWidth( 0, qMax( 100, fImpl->files->columnWidth( 0 ) ) );
				qApp->processEvents();
            }
        }
        if ( fProgress->wasCanceled() )
            break;
    }

    fImpl->files->resizeColumnToContents( 0 );
    fImpl->files->setColumnWidth( 0, qMax( 100, fImpl->files->columnWidth( 0 ) ) );
}

int CMainWindow::fileCount( int row ) const
{
    auto item = fModel->item( row, 1 );
    return fileCount( item );
}

int CMainWindow::fileCount( QStandardItem * item ) const
{
    if ( !item )
        return 0;
    return item->data( Qt::UserRole + 1 ).toInt();
}

void CMainWindow::setFileCount( int row, int cnt )
{
    auto item = fModel->item( row, 1 );
    return setFileCount( item, cnt );
}

void CMainWindow::setFileCount( QStandardItem * item, int cnt )
{
    if ( !item )
        return;
    return item->setData( cnt, Qt::UserRole + 1 );
}

void CMainWindow::slotDelete()
{
    auto progress = new QProgressDialog( tr( "Determining Files to Delete..." ), tr( "Cancel" ), 0, 0, this );
    auto bar = new QProgressBar;
    bar->setFormat( "%v of %m - %p%" );
    progress->setBar( bar );
    progress->setAutoReset( false );
    progress->setAutoClose( false );
    progress->setWindowModality( Qt::WindowModal );
    progress->setMinimumDuration( 1 );

    auto rowCount = fModel->rowCount();
    progress->setRange( 0, rowCount );
    QStringList filesToDelete;
    for ( int ii = 0; ii < rowCount; ++ii )
    {
        progress->setValue( ii );
        progress->setLabelText( tr( "Determining Files to Delete..." ) );
        filesToDelete << this->filesToDelete( ii );
        if ( ( progress->value() % 50 ) == 0 )
        {
            progress->adjustSize();
        }
        if ( progress->wasCanceled() )
            break;
    }
    delete progress;

    auto aok = QMessageBox::question( this, "Are you sure?", tr( "This action will delete %1 files!" ).arg( filesToDelete.count() ) );
    if ( aok != QMessageBox::StandardButton::Yes )
        return;

    progress = new QProgressDialog( tr( "Deleting Files..." ), tr( "Cancel" ), 0, 0, this );
    bar = new QProgressBar;
    bar->setFormat( "%v of %m - %p%" );
    progress->setBar( bar );
    progress->setAutoReset( false );
    progress->setAutoClose( false );
    progress->setWindowModality( Qt::WindowModal );
    progress->setMinimumDuration( 1 );

    progress->setRange( 0, filesToDelete.size() );
    int fileNum = 0;
    for( auto && ii : filesToDelete )
    {
        progress->setValue( fileNum++ );
        progress->setLabelText( tr( "Deleting Files..." ) );
        QFile::remove( ii );
        if ( ( progress->value() % 50 ) == 0 )
        {
            progress->adjustSize();
        }
        if ( progress->wasCanceled() )
            break;
    }
    delete progress;
    fImpl->del->setEnabled( false );
}

QList< QStandardItem * > findFilesToDelete( QStandardItem * item )
{
    QList< QStandardItem * > retVal;
    std::random_device rd;
    std::mt19937 gen( rd() );
    std::uniform_int_distribution<> dis( 0, 1 + item->rowCount() );
    int val = dis( gen ); // the one NOT to delete
    for ( int ii = 0; ii < item->rowCount() + 1; ++ii )
    {
        if ( ii == val )
            continue;

        QStandardItem * currItem = nullptr;
        if ( ii == 0 )
            currItem = item;
        else
            currItem = item->child( ii - 1, 0 );

        retVal << currItem;
    }
    return retVal;
}

QList< QStandardItem * > CMainWindow::getAllFiles( QStandardItem * rootFileFN ) const
{
    if ( !rootFileFN )
        return {};

    QList< QStandardItem * > retVal;
    retVal << rootFileFN;

    for( auto ii = 0; ii < rootFileFN->rowCount(); ++ii )
    {
        auto child = rootFileFN->child( ii, 0 );
        if ( !child )
            continue;
        retVal << child;
    }

    return retVal;
}


// returns true if lhs file is older than rhs file 
bool isOlder( QStandardItem * lhs, QStandardItem * rhs )
{
    if ( !lhs )
        return false;
    if ( !rhs )
        return true;

    auto lhsFile = QFileInfo( lhs->text() );
    auto rhsFile = QFileInfo( rhs->text() );

    return ( lhsFile.metadataChangeTime() < rhsFile.metadataChangeTime() );
}

QList< QStandardItem * > CMainWindow::filesToDelete( QStandardItem * rootFileFN )
{
    QRegularExpression regExp( "^(?<basename>.*)\\s+\\(\\s*\\d+\\s*\\)$" );
    Q_ASSERT( regExp.isValid() );
    QRegularExpression regExp2( "^(?<basename>.*)\\s+\\-\\s+Copy$" );
    Q_ASSERT( regExp2.isValid() );
    QRegularExpression regExp3( "^Copy of (?<basename>.*)$" );
    Q_ASSERT( regExp.isValid() );

    std::unordered_map< QString, QStandardItem * > baseFiles; // Files without the (N) in the name
    QList< std::pair< QStandardItem *, QString > > copies; // Files without the (N) in the name

    auto files = getAllFiles( rootFileFN );
    for ( int ii = 0; ii < files.count(); ++ii )
    {
        auto item = files[ ii ];
        if ( !item )
            continue;

        auto fullName = item->text();
        QFileInfo fi( item->text() );
        auto baseName = fi.completeBaseName();

        auto match = regExp.match( baseName, 0 );
        auto match2 = regExp2.match( baseName, 0 );
        auto match3 = regExp3.match( baseName, 0 );
        if ( match.hasMatch() )
        {
            auto computedBaseName = fi.absolutePath() + "/" + match.captured( "basename" ) + "." + fi.suffix();
            copies << std::make_pair( files[ ii ], computedBaseName );
        }
        else if ( match2.hasMatch() )
        {
            auto computedBaseName = fi.absolutePath() + "/" + match2.captured( "basename" ) + "." + fi.suffix();
            copies << std::make_pair( files[ ii ], computedBaseName );
        }
        else if ( match3.hasMatch() )
        {
            auto computedBaseName = fi.absolutePath() + "/" + match3.captured( "basename" ) + "." + fi.suffix();
            copies << std::make_pair( files[ ii ], computedBaseName );
        }
        else
            baseFiles[ fullName ] = files[ ii ];
    }

    // for each "copy" see if the basefile is in the list
    // if it is not move the copy to the base file list
    QList< QStandardItem * > retVal;
    for( auto && ii : copies )
    {
        auto pos = baseFiles.find( ii.second );
        if ( pos != baseFiles.end() )
        {
            // a basefile exsts.. definately delete this one
            retVal << ii.first;
        }
        else
            baseFiles[ ii.second ] = ii.first;
    }
    if ( baseFiles.size() > 1 ) // more than one baseFile, pick the oldest one to keep.
    {
        QStandardItem * oldest = nullptr;
        for( auto && curr : baseFiles )
        {
            if ( isOlder( curr.second, oldest ) )
            {
                oldest = curr.second;
            }
        }

        for ( auto && curr : baseFiles )
        {
            if ( curr.second == oldest )
                continue;
            retVal << curr.second;
        }

    }
    return retVal;
}

void CMainWindow::determineFilesToDelete( QStandardItem * item )
{
    if ( !item )
        return;

    if ( item->parent() )
        return; // only done on the root filename

    setDeleteFile( item, false, true, false );

    auto filesToDelete = this->filesToDelete( item );
    for( auto && ii : filesToDelete )
    {
        setDeleteFile( ii, true, false, false );
    }
}

void CMainWindow::setDeleteFile( QStandardItem * item, bool deleteFile, bool handleChildren, bool /*handleColumns*/ )
{
    if ( !item )
        return;

    item->setData( deleteFile, Qt::UserRole + 2 );
    item->setBackground( deleteFile ? Qt::red : Qt::white );

    //auto row = item->row();
    //for ( auto ii = 1; handleColumns && ii < fModel->columnCount(); ++ii )
    //{
    //    auto colItem = fModel->item( row, ii );
    //    setDeleteFile( item, deleteFile, false, false );
    //}

    if ( !handleChildren )
        return;

    auto childCount = item->rowCount();
    for( int ii = 0; ii < childCount; ++ii )
    {
        auto child = item->child( ii );
        if ( !child )
            continue;

        setDeleteFile( child, deleteFile, false, false );
    }
}

bool CMainWindow::deleteFile( QStandardItem * item ) const
{
    if ( !item )
        return false;
    return item->data( Qt::UserRole + 2 ).toBool();
}

QStringList CMainWindow::filesToDelete( int ii )
{
    auto item = fModel->item( ii, 0 );
    if ( !item )
        return {};

    QStringList retVal;
    if ( deleteFile( item ) )
        retVal << item->text();
    auto childCount = item->rowCount();
    for( int ii = 0; ii < childCount; ++ii )
    {
        auto childItem = item->child( ii, 0 );
        if ( deleteFile( childItem ) )
            retVal << childItem->text();
    }
    return retVal;
}

bool CMainWindow::hasDuplicates() const
{
    auto rowCount = fModel->rowCount();
    for( int ii = 0; ii < rowCount; ++ii )
    {
        auto cnt = fileCount( ii );
        if ( cnt > 1 )
            return true;
    }
    return false;
}

void CMainWindow::slotAddFilesFound( int numFiles )
{
    if ( !fProgress )
        return;
    fProgress->setFindRange( 0, numFiles );
    fProgress->setMD5Range(0, numFiles);
}

void CMainWindow::slotNumFilesComputed( int numFiles )
{
    if ( !fProgress )
        return;
    fTotalFiles = numFiles;
	fProgress->setFindRange(0, numFiles);
	fProgress->setMD5Range(0, numFiles);
}

QString secsToString( qint64 seconds )
{
    const qint64 DAY = 86400;
    qint64 days = seconds / DAY;
    QTime t = QTime( 0, 0 ).addSecs( seconds % DAY );
    return QString( "%1 days, %2 hours, %3 minutes, %4 seconds" ).arg( days ).arg( t.hour() ).arg( t.minute() ).arg( t.second() );
}

void CMainWindow::slotGo()
{
    initModel();
    fMap.clear();
    fDupesFound = 0;
    fImpl->files->resizeColumnToContents(1);
    fImpl->files->setSortingEnabled(false);
    fFilterModel->setLoadingValues(true);

    auto computer = new CComputeNumFiles(fImpl->dirName->text(), this);
    connect(computer, &CComputeNumFiles::finished, computer, &CComputeNumFiles::deleteLater);
    connect(computer, &CComputeNumFiles::sigNumFiles, this, &CMainWindow::slotNumFilesComputed);
    connect(computer, &CComputeNumFiles::sigNumFilesSub, this, &CMainWindow::slotAddFilesFound);

    fProgress = new CProgressDlg(tr("Cancel"), this);
    connect(fProgress, &CProgressDlg::sigCanceled, computer, &CComputeNumFiles::slotStop);

    computer->start();
    fFilesFound = 0;
    fMD5FilesComputed = 0;
    fDupesFound = 0;
    fTotalFiles = 0;

    fProgress->setFindFormat("%v of %m - %p%");
	fProgress->setMD5Format("%v of %m - %p%");
    //fProgress->setWindowModality(Qt::WindowModal);
    fProgress->setFindRange(0, 0);
    fProgress->setFindValue(0);
	fProgress->setMD5Range(0, 0);
	fProgress->setMD5Value(0);
    fProgress->show();
	fProgress->adjustSize();
    fStartTime = QDateTime::currentDateTime();
    findFiles(fImpl->dirName->text());
	fProgress->setFindFinished();

    // finding is finished, now wait for the MD5 calculations to finish
    while (QThreadPool::globalInstance()->activeThreadCount())
    {
        QThreadPool::globalInstance()->waitForDone(100);
		qApp->processEvents();
    }
    fEndTime = QDateTime::currentDateTime();
	slotFinished();
}

void CMainWindow::slotFinished()
{
    fImpl->files->resizeColumnToContents( 0 );
    fImpl->files->setColumnWidth( 0, qMax( 100, fImpl->files->columnWidth( 0 ) ) );
	fProgress->setMD5Finished();

    fProgress->hide();
    fProgress->deleteLater();

    fFilterModel->setLoadingValues( false );
    fImpl->files->setSortingEnabled( true );
    fImpl->del->setEnabled( hasDuplicates() );

    QLocale locale;
    QMessageBox::information( this, "Finished", 
        tr( 
            "<ul><li>Results: Number of Duplicates %1 of %2 files processed</li>"
            "<li>Elapsed Time: %4</li>"
        ).arg( locale.toString( fDupesFound ) ).arg( locale.toString( fFilesFound ) ).arg( secsToString( fStartTime.secsTo( fEndTime ) ) ) );
}


