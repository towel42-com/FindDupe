
#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "FileFinder.h"

#include "ProgressDlg.h"
#include "SABUtils/MD5.h"
#include "SABUtils/ButtonEnabler.h"
#include "SABUtils/utils.h"

#include "SABUtils/FileUtils.h"
#include "SABUtils/DelayLineEdit.h"

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
#include <QTimer>
#include <QCompleter>
#include <QFileSystemModel>
#include <QDesktopServices>
#include <QInputDialog>

#include <random>
#include <unordered_set>

class CFilterModel : public QSortFilterProxyModel
{
public:
    CFilterModel( QObject *owner ) :
        QSortFilterProxyModel( owner )
    {
    }
    void setShowDupesOnly( bool showDupesOnly )
    {
        if ( showDupesOnly != fShowDupesOnly )
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
            return true;   // all children are visible if parent is

        if ( !fShowDupesOnly )
            return true;

        QModelIndex sourceIdx = sourceModel()->index( source_row, 0, source_parent );
        auto childCount = sourceModel()->rowCount( sourceIdx );
        return childCount > 1;
    }

    virtual void sort( int column, Qt::SortOrder order = Qt::AscendingOrder ) override
    {
        if ( fLoadingValues )
            return;

        if ( column == 1 )
            setSortRole( Qt::UserRole + 1 );
        else
            setSortRole( Qt::DisplayRole );
        QSortFilterProxyModel::sort( column, order );
    }

    virtual bool lessThan( const QModelIndex &source_left, const QModelIndex &source_right ) const override { return source_left.data().toInt() < source_right.data().toInt(); }

    bool fLoadingValues{ false };
    bool fShowDupesOnly{ true };
};

CMainWindow::CMainWindow( QWidget *parent ) :
    QMainWindow( parent ),
    fImpl( new Ui::CMainWindow )
{
    fImpl->setupUi( this );

    fImpl->dirName->setDelay( 1000 );

    auto delayLE = new NSABUtils::CPathBasedDelayLineEdit;
    delayLE->setCheckExists( true );
    delayLE->setCheckIsDir( true );
    fImpl->dirName->setDelayLineEdit( delayLE );

    auto completer = new QCompleter( this );
    auto dirModel = new QFileSystemModel( completer );
    dirModel->setRootPath( "/" );
    completer->setModel( dirModel );
    completer->setCompletionMode( QCompleter::PopupCompletion );
    completer->setCaseSensitivity( Qt::CaseInsensitive );

    setWindowIcon( QIcon( ":/resources/finddupe.png" ) );
    setAttribute( Qt::WA_DeleteOnClose );

    fModel = new QStandardItemModel( this );
    initModel();
    fFilterModel = new CFilterModel( this );
    fFilterModel->setSourceModel( fModel );
    fImpl->files->setModel( fFilterModel );
    fImpl->files->setIconSize( QSize( 100, 100 ) );

    connect( fImpl->files, &QTreeView::doubleClicked, this, &CMainWindow::slotFileDoubleClicked );

    fImpl->go->setEnabled( false );
    fImpl->del->setEnabled( false );

    connect( fImpl->go, &QToolButton::clicked, this, &CMainWindow::slotGo );
    connect( fImpl->del, &QToolButton::clicked, this, &CMainWindow::slotDelete );
    connect( fImpl->selectDir, &QToolButton::clicked, this, &CMainWindow::slotSelectDir );

    connect( fImpl->dirName, &NSABUtils::CDelayComboBox::sigEditTextChangedAfterDelay, this, &CMainWindow::slotDirChanged );
    connect( fImpl->dirName, &NSABUtils::CDelayComboBox::editTextChanged, this, &CMainWindow::slotDirChanged );

    //connect( fImpl->dirName, &NSABUtils::CDelayLineEdit::sigTextChangedAfterDelay, this, &CMainWindow::slotDirChanged );
    //connect( fImpl->dirName, &NSABUtils::CDelayLineEdit::textChanged, this, &CMainWindow::slotDirChanged );

    connect( fImpl->showDupesOnly, &QCheckBox::clicked, this, &CMainWindow::slotShowDupesOnly );

    connect( fImpl->ignoreFilesOver, &QCheckBox::clicked, this, &CMainWindow::slotIgnoreFilesOver );
    connect( fImpl->ignoreFilesOverValue, qOverload< int >( &QSpinBox::valueChanged ), this, &CMainWindow::slotIgnoreFilesOver );

    connect( fImpl->addDir, &QToolButton::clicked, this, &CMainWindow::slotAddIgnoredDir );
    connect( fImpl->delDir, &QToolButton::clicked, this, &CMainWindow::slotDelIgnoredDir );

    connect( fImpl->addFileName, &QToolButton::clicked, this, &CMainWindow::slotAddIgnoredFileName );
    connect( fImpl->delFileName, &QToolButton::clicked, this, &CMainWindow::slotDelIgnoredFileName );

    new NSABUtils::CButtonEnabler( fImpl->ignoredDirs, fImpl->delDir );
    new NSABUtils::CButtonEnabler( fImpl->ignoredFileNames, fImpl->delFileName );

    QSettings settings;
    fImpl->dirName->clear();

    QStringList dirs;
    if ( settings.contains( "Dir" ) )
        dirs << settings.value( "Dir", QString() ).toString();
    dirs << settings.value( "Dirs" ).toStringList();
    dirs.removeDuplicates();
    dirs.removeAll( QString() );

    settings.remove( "Dir" );
    fImpl->dirName->addItems( dirs );
    fImpl->showDupesOnly->setChecked( settings.value( "ShowDupesOnly", true ).toBool() );
    fImpl->ignoreHidden->setChecked( settings.value( "IgnoreHidden", true ).toBool() );
    fImpl->ignoreFilesOver->setChecked( settings.value( "IgnoreFilesOver", true ).toBool() );
    fImpl->ignoreFilesOverValue->setValue( settings.value( "IgnoreFilesOverValue", 1000 ).toInt() );
    fImpl->caseInsensitiveNameCompare->setChecked( settings.value( "CaseInsensitiveCompare", false ).toBool() );
    addIgnoredDirs( settings.value( "IgnoredDirectories", QStringList() ).toStringList() );
    addIgnoredFileNames( settings
                             .value(
                                 "IgnoredFileNames", QStringList() << "poster.jpg"
                                                                   << "fanart.jpg"
                                                                   << R"(outtakes.*\.*)"
                                                                   << R"(Deleted Scenes\..*)"
                                                                   << R"(theatrical trailer.*\.*)"
                                                                   << R"(trailer.*\.*)"
                                                                   << R"(auditions.*\.*)"
                                                                   << R"(gag reel.*\.*)" )
                             .toStringList() );

    fImpl->files->resizeColumnToContents( 0 );
    fImpl->files->setColumnWidth( 0, 100 );

    QThreadPool::globalInstance()->setExpiryTimeout( -1 );

    fFileFinder = new CFileFinder( this );
    connect( this, &CMainWindow::sigMD5FileFinished, this, &CMainWindow::slotMD5FileFinished );

    connect( fFileFinder, &CFileFinder::sigMD5FileStarted, this, &CMainWindow::sigMD5FileStarted );
    connect( fFileFinder, &CFileFinder::sigMD5ReadPositionStatus, this, &CMainWindow::sigMD5ReadPositionStatus );
    connect( fFileFinder, &CFileFinder::sigMD5FileFinishedReading, this, &CMainWindow::sigMD5FileFinishedReading );
    connect( fFileFinder, &CFileFinder::sigMD5FileFinishedComputing, this, &CMainWindow::sigMD5FileFinishedComputing );
    connect( fFileFinder, &CFileFinder::sigMD5FileFinished, this, &CMainWindow::sigMD5FileFinished );
    connect( fFileFinder, &CFileFinder::sigDirFinished, this, &CMainWindow::slotFindDirFinished );
}

void CMainWindow::slotFindDirFinished( const QString &dirName )
{
    qDebug() << "Finished searching: " << dirName;
    fImpl->files->resizeColumnToContents( 0 );
    fImpl->files->setColumnWidth( 0, qMax( 100, fImpl->files->columnWidth( 0 ) ) );
}

void CMainWindow::addIgnoredDirs( QStringList ignoredDirs )
{
    NSABUtils::TCaseInsensitiveHash beenHere = getIgnoredDirs();
    for ( auto ii = ignoredDirs.begin(); ii != ignoredDirs.end(); )
    {
        if ( beenHere.find( *ii ) == beenHere.end() )
        {
            beenHere.insert( *ii );
            ii++;
        }
        else
            ii = ignoredDirs.erase( ii );
    }

    fImpl->ignoredDirs->addItems( ignoredDirs );
}

void CMainWindow::addIgnoredFileNames( QStringList ignoredFileNames )
{
    NSABUtils::TCaseInsensitiveHash beenHere = getIgnoredFileNames();
    for ( auto ii = ignoredFileNames.begin(); ii != ignoredFileNames.end(); )
    {
        if ( beenHere.find( *ii ) == beenHere.end() )
        {
            beenHere.insert( *ii );
            ii++;
        }
        else
            ii = ignoredFileNames.erase( ii );
    }

    fImpl->ignoredFileNames->addItems( ignoredFileNames );
}

void CMainWindow::addIgnoredDir( const QString &ignoredDir )
{
    addIgnoredDirs( QStringList() << ignoredDir );
}

void CMainWindow::addIgnoredFileName( const QString &ignoredFileName )
{
    addIgnoredFileNames( QStringList() << ignoredFileName );
}

void CMainWindow::initModel()
{
    fModel->clear();
    fModel->setHorizontalHeaderLabels( QStringList() << tr( "FileName" ) << tr( "Timestamp" ) << tr( "Count" ) << tr( "Size" ) << tr( "MD5" ) );
}

CMainWindow::~CMainWindow()
{
    QSettings settings;
    settings.setValue( "Dirs", fImpl->dirName->getAllText() );
    settings.setValue( "ShowDupesOnly", fImpl->showDupesOnly->isChecked() );
    settings.setValue( "IgnoreHidden", fImpl->ignoreHidden->isChecked() );
    settings.setValue( "IgnoreFilesOver", fImpl->ignoreFilesOver->isChecked() );
    settings.setValue( "IgnoreFilesOverValue", fImpl->ignoreFilesOverValue->value() );
    settings.setValue( "CaseInsensitiveCompare", fImpl->caseInsensitiveNameCompare->isChecked() );
    auto ignoredDirs = getIgnoredDirs();
    QStringList dirs;
    for ( auto &&ii : ignoredDirs )
        dirs << ii;
    settings.setValue( "IgnoredDirectories", dirs );

    auto ignoredFileNames = getIgnoredFileNames();
    QStringList fileNames;
    for ( auto &&ii : ignoredFileNames )
        fileNames << ii;
    settings.setValue( "IgnoredFileNames", fileNames );
}

void CMainWindow::slotShowDupesOnly()
{
    fFilterModel->setShowDupesOnly( fImpl->showDupesOnly->isChecked() );
}

void CMainWindow::slotIgnoreFilesOver()
{
    fImpl->ignoreFilesOverValue->setEnabled( fImpl->ignoreFilesOver->isChecked() );
    if ( fFileFinder )
        fFileFinder->setIgnoreFilesOver( fImpl->ignoreFilesOver->isChecked(), fImpl->ignoreFilesOverValue->value() );
}

void CMainWindow::slotSelectDir()
{
    auto dir = QFileDialog::getExistingDirectory( this, "Select Directory", fImpl->dirName->currentText() );
    if ( dir.isEmpty() )
        return;

    fImpl->dirName->setCurrentText( dir );
}

void CMainWindow::slotDirChanged()
{
    initModel();
    QFileInfo fi( fImpl->dirName->currentText() );
    QString msg;
    if ( !fi.exists() )
        msg = QString( "'%1' does not exist" ).arg( fImpl->dirName->currentText() );
    else if ( !fi.isDir() )
        msg = QString( "'%1' is not a directory" ).arg( fImpl->dirName->currentText() );
    fImpl->go->setToolTip( msg );
    fImpl->go->setEnabled( fi.exists() && fi.isDir() );
}

QList< QStandardItem * > CMainWindow::createFileRow( const QFileInfo &fi, const QString &md5 )
{
    if ( fi.size() == 0 )
        return {};

    if ( md5.isEmpty() )
        qDebug() << "Creating file row for file" << fi << " MD5 already existed";
    else
        qDebug() << "Creating MD5 Header for file" << fi << "MD5: " << md5;
    bool header = !md5.isEmpty();

    auto relToDir = QDir( fImpl->dirName->currentText() );

    auto rootFNItem = new QStandardItem( relToDir.relativeFilePath( fi.absoluteFilePath() ) );
    QStandardItem *tsItem = nullptr;
    QStandardItem *md5Item = nullptr;
    QStandardItem *countItem = nullptr;
    QStandardItem *sizeItem = nullptr;

    if ( header )
    {
        countItem = new QStandardItem( QString() );

        sizeItem = new QStandardItem( NSABUtils::NFileUtils::byteSizeString( fi ) );
        sizeItem->setTextAlignment( Qt::AlignmentFlag::AlignRight | Qt::AlignmentFlag::AlignVCenter );
        setFileCount( countItem, 0 );

        md5Item = new QStandardItem( md5 );
        md5Item->setTextAlignment( Qt::AlignmentFlag::AlignRight | Qt::AlignmentFlag::AlignVCenter );
        md5Item->setFont( QFontDatabase::systemFont( QFontDatabase::FixedFont ) );
    }
    else
    {
        auto timeStamp = fi.lastModified().toString();
        tsItem = new QStandardItem( timeStamp );
        tsItem->setTextAlignment( Qt::AlignmentFlag::AlignRight | Qt::AlignmentFlag::AlignVCenter );
    }

    auto row = QList< QStandardItem * >() << rootFNItem << tsItem << countItem << sizeItem << md5Item;
    return row;
}

void CMainWindow::slotMD5FileFinished( unsigned long long /*threadID*/, const QDateTime & /*endTime*/, const QString &fileName, const QString &md5 )
{
    qDebug() << "Finished MD5 Computation: " << fileName << "MD5=" << md5;
    fMD5FilesComputed++;
    if ( fProgress )
        fProgress->setMD5Value( fMD5FilesComputed );

    if ( md5.isEmpty() )
        return;

    auto fi = QFileInfo( fileName );
    if ( fi.size() == 0 )
        return;

    auto pos = fMap.find( md5 );
    QStandardItem *rootItem = nullptr;
    QStandardItem *countItem = nullptr;
    if ( pos == fMap.end() )
    {
        auto row = createFileRow( fi, md5 );
        if ( row.empty() )
            return;

        rootItem = row[ 0 ];
        countItem = row[ 2 ];
        fMap[ md5 ] = { rootItem, countItem };
        fModel->appendRow( row );
    }
    else
    {
        std::tie( rootItem, countItem ) = ( *pos ).second;
    }

    auto newCount = fileCount( rootItem ) + 1;
    setFileCount( countItem, newCount );
    countItem->setText( QString::number( newCount ) );

    if ( !hasChildFile( rootItem, fi ) )
    {
        auto childRow = createFileRow( fi, QString() );
        rootItem->appendRow( childRow );
    }

    if ( newCount != 1 )
    {
        fDupesFound.first++;
        fDupesFound.second += QFileInfo( fileName ).size();
        showIcons( rootItem );
        updateResultsLabel();
        determineFilesToDelete( rootItem );
    }

    auto srcIdx = fModel->indexFromItem( rootItem );
    if ( srcIdx.isValid() )
    {
        auto idx = fFilterModel->mapFromSource( srcIdx );
        if ( idx.isValid() )
            fImpl->files->setExpanded( idx, true );
    }
    if ( fProgress )
        fProgress->setNumDuplicates( fDupesFound );
}

void CMainWindow::showIcons( QStandardItem *item )
{
    auto relToDir = QDir( fImpl->dirName->currentText() );
    auto path = relToDir.absoluteFilePath( item->text() );

    auto icon = QIcon( path );
    item->setIcon( icon );
    for ( int ii = 0; ii < item->rowCount(); ++ii )
    {
        auto child = item->child( ii, 0 );
        showIcons( child );
    }
}

bool CMainWindow::hasChildFile( QStandardItem *header, const QFileInfo &fi ) const
{
    for ( int ii = 0; ii < header->rowCount(); ++ii )
    {
        auto child = header->child( ii, 0 );
        if ( getFileInfo( child ).absoluteFilePath() == fi.absoluteFilePath() )
            return true;
    }
    return false;
}

QFileInfo CMainWindow::getFileInfo( QStandardItem *item ) const
{
    if ( item->column() != 0 )
    {
        auto idx = fModel->indexFromItem( item );
        idx = fModel->index( idx.row(), 0, idx.parent() );
        item = fModel->itemFromIndex( idx );
    }

    auto fn = item->text();
    auto rootDir = QDir( fImpl->dirName->currentText() );
    QFileInfo fi( rootDir.absoluteFilePath( fn ) );
    return fi;
}

NSABUtils::TCaseInsensitiveHash CMainWindow::getIgnoredDirs() const
{
    NSABUtils::TCaseInsensitiveHash ignoredDirs;
    for ( int ii = 0; ii < fImpl->ignoredDirs->count(); ++ii )
    {
        ignoredDirs.insert( fImpl->ignoredDirs->item( ii )->text() );
    }
    return ignoredDirs;
}

NSABUtils::TCaseInsensitiveHash CMainWindow::getIgnoredFileNames() const
{
    NSABUtils::TCaseInsensitiveHash ignoredFileNames;
    for ( int ii = 0; ii < fImpl->ignoredFileNames->count(); ++ii )
    {
        ignoredFileNames.insert( fImpl->ignoredFileNames->item( ii )->text() );
    }
    return ignoredFileNames;
}

int CMainWindow::fileCount( int row ) const
{
    auto item = fModel->item( row, 0 );
    return fileCount( item );
}

int CMainWindow::fileCount( QStandardItem *item ) const
{
    if ( !item )
        return 0;
    return item->rowCount();
}

void CMainWindow::setFileCount( int row, int cnt )
{
    auto item = fModel->item( row, 1 );
    return setFileCount( item, cnt );
}

void CMainWindow::setFileCount( QStandardItem *item, int cnt )
{
    if ( !item )
        return;
    return item->setData( cnt, Qt::UserRole + 1 );
}

void CMainWindow::slotFileDoubleClicked( const QModelIndex &idx )
{
    auto srcIdx = fFilterModel->mapToSource( idx );
    auto item = fModel->itemFromIndex( srcIdx );
    if ( !item )
        return;

    auto fi = getFileInfo( item );
    auto dir = fi.absoluteDir();
    auto path = dir.absolutePath();
    auto url = QUrl::fromLocalFile( dir.absolutePath() );
    QDesktopServices::openUrl( url );
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
    for ( auto &&ii : filesToDelete )
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

QList< QStandardItem * > findFilesToDelete( QStandardItem *item )
{
    QList< QStandardItem * > retVal;
    std::random_device rd;
    std::mt19937 gen( rd() );
    std::uniform_int_distribution<> dis( 0, 1 + item->rowCount() );
    int val = dis( gen );   // the one NOT to delete
    for ( int ii = 0; ii < item->rowCount() + 1; ++ii )
    {
        if ( ii == val )
            continue;

        QStandardItem *currItem = nullptr;
        if ( ii == 0 )
            currItem = item;
        else
            currItem = item->child( ii - 1, 0 );

        retVal << currItem;
    }
    return retVal;
}

QList< QStandardItem * > CMainWindow::getAllFiles( QStandardItem *rootFileFN ) const
{
    if ( !rootFileFN )
        return {};

    QList< QStandardItem * > retVal;
    retVal << rootFileFN;

    for ( auto ii = 0; ii < rootFileFN->rowCount(); ++ii )
    {
        auto child = rootFileFN->child( ii, 0 );
        if ( !child )
            continue;
        retVal << child;
    }

    return retVal;
}

// returns true if lhs file is older than rhs file
bool isOlder( QStandardItem *lhs, QStandardItem *rhs )
{
    if ( !lhs )
        return false;
    if ( !rhs )
        return true;

    auto lhsFile = QFileInfo( lhs->text() );
    auto rhsFile = QFileInfo( rhs->text() );

    return ( lhsFile.metadataChangeTime() < rhsFile.metadataChangeTime() );
}

QList< QStandardItem * > CMainWindow::filesToDelete( QStandardItem *rootFileFN )
{
    QRegularExpression regExp( "^(?<basename>.*)\\s+\\(\\s*\\d+\\s*\\)$" );
    Q_ASSERT( regExp.isValid() );
    QRegularExpression regExp2( "^(?<basename>.*)\\s+\\-\\s+Copy$" );
    Q_ASSERT( regExp2.isValid() );
    QRegularExpression regExp3( "^Copy of (?<basename>.*)$" );
    Q_ASSERT( regExp.isValid() );

    std::unordered_map< QString, QStandardItem * > baseFiles;   // Files without the (N) in the name
    QList< std::pair< QStandardItem *, QString > > copies;   // Files without the (N) in the name

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
    for ( auto &&ii : copies )
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
    if ( baseFiles.size() > 1 )   // more than one baseFile, pick the oldest one to keep.
    {
        QStandardItem *oldest = nullptr;
        for ( auto &&curr : baseFiles )
        {
            if ( isOlder( curr.second, oldest ) )
            {
                oldest = curr.second;
            }
        }

        for ( auto &&curr : baseFiles )
        {
            if ( curr.second == oldest )
                continue;
            retVal << curr.second;
        }
    }
    return retVal;
}

void CMainWindow::determineFilesToDelete( QStandardItem *item )
{
    if ( !item )
        return;

    if ( item->parent() )
        return;   // only done on the root filename

    setDeleteFile( item, false, true, false );

    auto filesToDelete = this->filesToDelete( item );
    for ( auto &&ii : filesToDelete )
    {
        setDeleteFile( ii, true, false, false );
    }
}

void CMainWindow::setDeleteFile( QStandardItem *item, bool deleteFile, bool handleChildren, bool /*handleColumns*/ )
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
    for ( int ii = 0; ii < childCount; ++ii )
    {
        auto child = item->child( ii );
        if ( !child )
            continue;

        setDeleteFile( child, deleteFile, false, false );
    }
}

bool CMainWindow::deleteFile( QStandardItem *item ) const
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
    for ( int ii = 0; ii < childCount; ++ii )
    {
        auto childItem = item->child( ii, 0 );
        if ( deleteFile( childItem ) )
            retVal << getFileInfo( childItem ).absoluteFilePath();
    }
    return retVal;
}

bool CMainWindow::hasDuplicates() const
{
    auto rowCount = fModel->rowCount();
    for ( int ii = 0; ii < rowCount; ++ii )
    {
        auto cnt = fileCount( ii );
        if ( cnt > 1 )
            return true;
    }
    return false;
}

void CMainWindow::slotCountDirFinished( const QString &dirName )
{
    qDebug() << "Finished counting dir: " << dirName;
}

void CMainWindow::slotAddFilesFound( int numFiles )
{
    if ( !fProgress )
        return;
    fProgress->setComputeRange( 0, numFiles );
    fProgress->setFindRange( 0, numFiles );
    fProgress->setMD5Range( 0, numFiles );
}

void CMainWindow::slotNumFilesFinishedComputing( int numFiles )
{
    if ( !fProgress )
        return;
    fTotalFiles = numFiles;
    slotAddFilesFound( numFiles );

    fFileFinder->reset();
    fFileFinder->setRootDir( fImpl->dirName->currentText() );
    fFileFinder->setIgnoredDirs( getIgnoredDirs() );
    fFileFinder->setIgnoredFileNames( getIgnoredFileNames() );
    fFileFinder->setIgnoreHidden( fImpl->ignoreHidden->isChecked() );
    fFileFinder->setIgnoreFilesOver( fImpl->ignoreFilesOver->isChecked(), fImpl->ignoreFilesOverValue->value() );
    fFileFinder->setCaseInsensitiveNameCompare( fImpl->caseInsensitiveNameCompare->isChecked() );

    QThreadPool::globalInstance()->start( fFileFinder );
    QTimer::singleShot( 0, this, &CMainWindow::slotWaitForAllThreadsFinished );
}

void CMainWindow::slotWaitForAllThreadsFinished()
{
    if ( !isFinished() )
    {
        QTimer::singleShot( 100, this, &CMainWindow::slotWaitForAllThreadsFinished );
        return;
    }
    fEndTime = QDateTime::currentDateTime();
    slotFinished();
}

bool CMainWindow::isFinished()
{
    if ( fCheckForFinished.has_value() && ( ( fCheckForFinished.value().second.msecsTo( QDateTime::currentDateTime() ) > 1000 ) && ( fCheckForFinished.value().first >= 5 ) ) )
        return true;

    if ( !fCheckForFinished.has_value() )
        fCheckForFinished = std::make_pair( 0, QDateTime::currentDateTime() );

    // finding is finished, now wait for the MD5 calculations to finish
    int cnt = 0;
    if ( QThreadPool::globalInstance()->activeThreadCount() )
    {
        QThreadPool::globalInstance()->waitForDone( 100 );
        return false;
    }
    fCheckForFinished.value().first++;
    return false;
}

void CMainWindow::slotGo()
{
    initModel();
    fMap.clear();
    fDupesFound = { 0, 0 };
    fImpl->files->resizeColumnToContents( 0 );
    fImpl->files->setSortingEnabled( false );
    fFilterModel->setLoadingValues( true );

    auto computer = new CComputeNumFiles( this );
    fProgress = new CProgressDlg( tr( "Cancel" ), nullptr );

    connect( computer, &CComputeNumFiles::sigFilesFound, this, &CMainWindow::slotAddFilesFound );
    connect( computer, &CComputeNumFiles::sigNumFilesFinished, this, &CMainWindow::slotNumFilesFinishedComputing );
    connect( computer, &CComputeNumFiles::sigFinished, fProgress, &CProgressDlg::slotFinishedComputingFileCount );
    connect( computer, &CFileFinder::sigCurrentFindInfo, fProgress, &CProgressDlg::slotCurrentComputeInfo );

    connect( fProgress, &CProgressDlg::sigCanceled, computer, &CComputeNumFiles::slotStop );
    connect( fProgress, &CProgressDlg::sigCanceled, fFileFinder, &CFileFinder::slotStop );

    connect( this, &CMainWindow::sigMD5FileStarted, fProgress, &CProgressDlg::slotMD5FileStarted );
    connect( this, &CMainWindow::sigMD5ReadPositionStatus, fProgress, &CProgressDlg::slotMD5ReadPositionStatus );
    connect( this, &CMainWindow::sigMD5FileFinishedReading, fProgress, &CProgressDlg::slotMD5FileFinishedReading );
    connect( this, &CMainWindow::sigMD5FileFinishedComputing, fProgress, &CProgressDlg::slotMD5FileFinishedComputing );
    connect( this, &CMainWindow::sigMD5FileFinished, fProgress, &CProgressDlg::slotMD5FileFinished );

    connect( fFileFinder, &CFileFinder::sigDirFinished, this, &CMainWindow::slotCountDirFinished );
    connect( fFileFinder, &CFileFinder::sigFinished, fProgress, &CProgressDlg::slotFindFinished );
    connect( fFileFinder, &CFileFinder::sigCurrentFindInfo, fProgress, &CProgressDlg::slotCurrentFindInfo );
    connect( fFileFinder, &CFileFinder::sigFilesFound, fProgress, &CProgressDlg::slotUpdateFilesFound );

    //QThreadPool::globalInstance()->setMaxThreadCount( 32 );

    computer->reset();
    computer->setRootDir( fImpl->dirName->currentText() );
    computer->setIgnoredDirs( getIgnoredDirs() );
    computer->setIgnoredFileNames( getIgnoredFileNames() );
    computer->setIgnoreHidden( fImpl->ignoreHidden->isChecked() );
    computer->setIgnoreFilesOver( fImpl->ignoreFilesOver->isChecked(), fImpl->ignoreFilesOverValue->value() );
    computer->setCaseInsensitiveNameCompare( fImpl->caseInsensitiveNameCompare->isChecked() );

    fProgress->setComputeRange( 0, 0 );
    fProgress->setComputeValue( 0 );

    QThreadPool::globalInstance()->start( computer );
    fCheckForFinished = {};
    fMD5FilesComputed = 0;
    fDupesFound = { 0, 0 };
    fTotalFiles = 0;

    fProgress->setFindFormat( "%v of %m - %p%" );
    fProgress->setMD5Format( "%v of %m - %p%" );
    fProgress->setComputeFormat( "%v of %m - %p%" );
    //fProgress->setWindowModality(Qt::WindowModal);
    fProgress->setFindRange( 0, 0 );
    fProgress->setFindValue( 0 );
    fProgress->setMD5Range( 0, 0 );
    fProgress->setMD5Value( 0 );
    fProgress->show();
    fProgress->adjustSize();
    fStartTime = QDateTime::currentDateTime();
    fProgress->setRelToDir( fImpl->dirName->currentText() );
}

void CMainWindow::slotFinished()
{
    fImpl->files->resizeColumnToContents( 0 );
    fImpl->files->setColumnWidth( 0, qMax( 100, fImpl->files->columnWidth( 0 ) ) );

    if ( fProgress )
    {
        fProgress->setMD5Finished();
        fProgress->hide();
        fProgress->deleteLater();
    }

    fFilterModel->setLoadingValues( false );
    fImpl->files->setSortingEnabled( true );
    fImpl->del->setEnabled( hasDuplicates() );

    QLocale locale;
    QMessageBox::information(
        this, "Finished",
        tr( "<ul><li>Results: Number of Duplicates %1 of %2 files processed</li>"
            "<ul><li>Total size of Duplicates: %3</li>"
            "<li>Elapsed Time: %4</li>" )
            .arg( locale.toString( fDupesFound.first ) )
            .arg( locale.toString( fFileFinder->numFilesFound() ) )
            .arg( NSABUtils::NFileUtils::byteSizeString( fDupesFound.second ) )
            .arg( NSABUtils::secsToString( fStartTime.secsTo( fEndTime ) ) ) );
    updateResultsLabel();
}

void CMainWindow::updateResultsLabel()
{
    QLocale locale;

    QString text;
    if ( fDupesFound.first == 0 )
        text = tr( "Results:" );
    else
        text = tr( "Results: Number of Duplicates %1 of %2 files processed, Total size of Duplicates: %3" ).arg( locale.toString( fDupesFound.first ) ).arg( locale.toString( fFileFinder->numFilesFound() ) ).arg( NSABUtils::NFileUtils::byteSizeString( fDupesFound.second ) );

    fImpl->resultsLabel->setText( text );
}

void CMainWindow::slotAddIgnoredDir()
{
    auto dir = QFileDialog::getExistingDirectory( this, "Select Directory", fImpl->dirName->currentText() );
    if ( dir.isEmpty() )
        return;

    addIgnoredDir( dir );
}

void CMainWindow::slotDelIgnoredDir()
{
    auto curr = fImpl->ignoredDirs->currentItem();
    if ( !curr )
        return;

    delete curr;
}

void CMainWindow::slotAddIgnoredFileName()
{
    auto fn = QInputDialog::getText( this, tr( "Filename to Ignore" ), tr( "FileName:" ) );
    if ( fn.isEmpty() )
        return;

    addIgnoredFileName( fn );
}

void CMainWindow::slotDelIgnoredFileName()
{
    auto curr = fImpl->ignoredFileNames->currentItem();
    if ( !curr )
        return;

    delete curr;
}
