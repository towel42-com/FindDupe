#include "MainWindow.h"
#include "ComputeNumFiles.h"
#include "SABUtils/MD5.h"
#include "ui_MainWindow.h"

#include <QFileDialog>
#include <QStandardItemModel>
#include <QSettings>
#include <QProgressDialog>
#include <QProgressBar>
#include <QDirIterator>
#include <QSortFilterProxyModel>
#include <QRegularExpression>
#include <QMessageBox>
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

    connect( fImpl->go, &QToolButton::clicked, this, &CMainWindow::slotGo );
    connect( fImpl->del, &QToolButton::clicked, this, &CMainWindow::slotDelete );
    connect( fImpl->selectDir, &QToolButton::clicked, this, &CMainWindow::slotSelectDir );
    connect( fImpl->dirName, &QLineEdit::textChanged, this, &CMainWindow::slotDirChanged );
    connect( fImpl->showDupesOnly, &QCheckBox::clicked, this, &CMainWindow::slotShowDupesOnly );

    QSettings settings;
    fImpl->dirName->setText( settings.value( "Dir", QString() ).toString() );
    fImpl->showDupesOnly->setChecked( settings.value( "ShowDupesOnly", true ).toBool() );
}

void CMainWindow::initModel()
{
    fModel->clear();
    fModel->setHorizontalHeaderLabels( QStringList() << "FileName" << "Count" << "MD5" );
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

void CMainWindow::addFile( const QString & fileName )
{
    auto md5 = NUtils::getMd5( fileName, true );
    auto pos = fMap.find( md5 );
    QStandardItem * rootFNItem = nullptr;
    QStandardItem * countItem = nullptr;
    if ( pos == fMap.end() )
    {
        rootFNItem = new QStandardItem( fileName );
        auto md5Item = new QStandardItem( md5 );
        countItem = new QStandardItem( QString() );
        setFileCount( countItem, 0 );
        QList< QStandardItem * > row = QList< QStandardItem * >() << rootFNItem << countItem << md5Item;
        fMap[ md5 ] = row;
        fModel->appendRow( row );
    }
    else
    {
        rootFNItem = (*pos).second[ 0 ];
        countItem = (*pos).second[ 1 ];
    }
    auto newCount = fileCount( countItem ) + 1;
    setFileCount( countItem, newCount );
    countItem->setText( QString::number( newCount ) );

    if ( newCount != 1 )
    {
        fDupesFound++;
        auto fileItem = new QStandardItem( fileName );
        rootFNItem->appendRow( fileItem );
        auto idx = fFilterModel->mapFromSource( fModel->indexFromItem( rootFNItem ) );
        if ( idx.isValid() )
            fImpl->files->setExpanded( idx, true );
        determineFilesToDelete( rootFNItem );
    }
}

void CMainWindow::findFiles( const QString & dirName )
{
    QDir dir( dirName );
    if ( !dir.exists() )
        return;

    QDirIterator di( dirName, QStringList() << "*", QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Readable, QDirIterator::NoIteratorFlags );
    QLocale locale;
    while ( di.hasNext() )
    {
        auto curr = di.next();
        
        fProgress->setValue( fFilesProcessed = fProgress->value() + 1 );

        QFileInfo fi( curr );
        if ( fi.isDir() )
        {
            findFiles( curr );
        }
        else
        {
            fProgress->setLabelText( tr( "Finding Files...\nCurrent Directory: '%1'\nCurrent File '%2'(%3 bytes)\nDuplicates Found: %4" ).arg( dir.dirName() ).arg( fi.fileName() ).arg( locale.toString( fi.size() ) ).arg( fDupesFound ) );
            qApp->processEvents();
            addFile( curr );
            if ( ( fProgress->value() % 500 ) == 0 )
            {
                fProgress->adjustSize();
                fImpl->files->resizeColumnToContents( 0 );
                fImpl->files->setColumnWidth( 0, qMax( 50, fImpl->files->columnWidth( 0 ) ) );
            }
        }
        if ( fProgress->wasCanceled() )
            break;
    }

    fImpl->files->resizeColumnToContents( 0 );
    fImpl->files->setColumnWidth( 0, qMax( 50, fImpl->files->columnWidth( 0 ) ) );
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
    fProgress = new QProgressDialog( tr( "Determining Files to Delete..." ), tr( "Cancel" ), 0, 0, this );
    auto bar = new QProgressBar;
    bar->setFormat( "%v of %m - %p%" );
    fProgress->setBar( bar );
    fProgress->setAutoReset( false );
    fProgress->setAutoClose( false );
    fProgress->setWindowModality( Qt::WindowModal );
    fProgress->setMinimumDuration( 1 );

    auto rowCount = fModel->rowCount();
    fProgress->setRange( 0, rowCount );
    QStringList filesToDelete;
    for ( int ii = 0; ii < rowCount; ++ii )
    {
        fProgress->setValue( ii );
        fProgress->setLabelText( tr( "Determining Files to Delete..." ) );
        qApp->processEvents();
        filesToDelete << this->filesToDelete( ii );
        qApp->processEvents();
        if ( ( fProgress->value() % 500 ) == 0 )
        {
            fProgress->adjustSize();
        }
        if ( fProgress->wasCanceled() )
            break;
    }
    delete fProgress;

    auto aok = QMessageBox::question( this, "Are you sure?", tr( "This action will delete %1 files!" ).arg( filesToDelete.count() ) );
    if ( aok != QMessageBox::StandardButton::Yes )
        return;

    fProgress = new QProgressDialog( tr( "Deleting Files..." ), tr( "Cancel" ), 0, 0, this );
    bar = new QProgressBar;
    bar->setFormat( "%v of %m - %p%" );
    fProgress->setBar( bar );
    fProgress->setAutoReset( false );
    fProgress->setAutoClose( false );
    fProgress->setWindowModality( Qt::WindowModal );
    fProgress->setMinimumDuration( 1 );

    fProgress->setRange( 0, filesToDelete.size() );
    int fileNum = 0;
    for( auto && ii : filesToDelete )
    {
        fProgress->setValue( fileNum++ );
        fProgress->setLabelText( tr( "Deleting Files..." ) );
        qApp->processEvents();
        QFile::remove( ii );
        qApp->processEvents();
        if ( ( fProgress->value() % 500 ) == 0 )
        {
            fProgress->adjustSize();
        }
        if ( fProgress->wasCanceled() )
            break;
    }
    delete fProgress;
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
    fProgress->setMaximum( numFiles );
    qApp->processEvents();
}

void CMainWindow::slotNumFilesComputed( int numFiles )
{
    if ( !fProgress )
        return;
    fTotalFiles = numFiles;
    fProgress->setMaximum( numFiles );
    qApp->processEvents();
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
    fImpl->files->resizeColumnToContents( 1 );
    fImpl->files->setSortingEnabled( false );
    fFilterModel->setLoadingValues( true );

    auto computer = new CComputeNumFiles( fImpl->dirName->text(), this );
    connect( computer, &CComputeNumFiles::finished, computer, &CComputeNumFiles::deleteLater );
    connect( computer, &CComputeNumFiles::sigNumFiles, this, &CMainWindow::slotNumFilesComputed );
    connect( computer, &CComputeNumFiles::sigNumFilesSub, this, &CMainWindow::slotAddFilesFound );

    fProgress = new QProgressDialog( tr( "Finding files..." ), tr( "Cancel" ), 0, 0, this );
    connect( fProgress, &QProgressDialog::canceled, computer, &CComputeNumFiles::slotStop );
    computer->start();

    auto bar = new QProgressBar;
    bar->setFormat( "%v of %m - %p%");
    fProgress->setBar( bar );
    fProgress->setAutoReset( false );
    fProgress->setAutoClose( false );
    fProgress->setWindowModality( Qt::WindowModal );
    fProgress->setMinimumDuration( 1 );
    fProgress->setRange( 0, 0 );
    fProgress->setValue( 0 );

    auto startTime = QDateTime::currentDateTime();
    findFiles( fImpl->dirName->text() );
    auto endTime = QDateTime::currentDateTime();

    fImpl->files->resizeColumnToContents( 0 );
    fImpl->files->setColumnWidth( 0, qMax( 50, fImpl->files->columnWidth( 0 ) ) );

    auto tmp = fProgress;
    fProgress = nullptr;
    delete tmp;
    fFilterModel->setLoadingValues( false );
    fImpl->files->setSortingEnabled( true );
    fImpl->del->setEnabled( hasDuplicates() );

    QLocale locale;
    QMessageBox::information( this, "Finished", tr( "Results: Number of Duplicates %1 of %2 files processed\n%3 total files\nElapsed Time: %4").arg( locale.toString( fDupesFound ) ).arg( locale.toString( fFilesProcessed ) ).arg( locale.toString( fTotalFiles ) ).arg( secsToString( startTime.secsTo( endTime ) ) ) );
}


