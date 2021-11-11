#include "ProgressDlg.h"
#include "ui_ProgressDlg.h"
#include "SABUtils/utils.h"

#include <QTreeWidgetItem>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QInputDialog>
#include <QLocale>
#include <QThreadPool>
#include <QCloseEvent>
#include <QDateTime>
#include <QTimer>

CProgressDlg::CProgressDlg( QWidget* parent )
    : QDialog( parent ),
    fImpl( new Ui::CProgressDlg )
{
    fImpl->setupUi( this );
    setWindowFlags( windowFlags() & ~Qt::WindowContextHelpButtonHint );
    setWindowFlags( windowFlags() & ~Qt::WindowCloseButtonHint );

    fImpl->findText->setTextFormat( Qt::TextFormat::RichText );
    fImpl->findText->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );

    fImpl->md5Text->setTextFormat( Qt::TextFormat::RichText );
    fImpl->md5Text->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );

    connect( fImpl->cancelButton, &QPushButton::clicked, this, &CProgressDlg::slotCanceled );
    setStatusLabel();
    fTimer = new QTimer( this );
    fTimer->setSingleShot( false );
    fTimer->setInterval( 500 );
    connect( fTimer, &QTimer::timeout, this, &CProgressDlg::slotUpdateThreadInfo );
    fTimer->start();
}

CProgressDlg::CProgressDlg( const QString& cancelText, QWidget* parent ) :
    CProgressDlg( parent )
{
    fImpl->findText->setText( tr( "Finding Files..." ) );
    fImpl->md5Text->setText( tr( "Computing MD5s..." ) );
    setCancelText( cancelText );
}

CProgressDlg::~CProgressDlg()
{
}

void CProgressDlg::closeEvent( QCloseEvent* event )
{
    slotCanceled();
    event->accept();
}

void CProgressDlg::slotCanceled()
{
    fCanceled = true;
    fImpl->cancelButton->setEnabled( false );
    emit sigCanceled();
}


void CProgressDlg::slotSetMD5Remaining( int remaining )
{
    setMD5Value( md5Max() - remaining );
}

void CProgressDlg::slotFinishedComputingFileCount()
{
    fImpl->processingFileCountLabel->setVisible( false );
}

void CProgressDlg::slotSetFindRemaining( int remaining )
{
    setFindValue( findMax() - remaining );
}

void CProgressDlg::setFindValue( int value )
{
    fImpl->findProgress->setValue( value );
    if ( ( value % 50 ) == 0 )
        adjustSize();
}

int CProgressDlg::findValue() const
{
    return fImpl->findProgress->value();
}

void CProgressDlg::setFindRange( int min, int max )
{
    fImpl->findProgress->setRange( min, max );
}

int CProgressDlg::findMin() const
{
    return fImpl->findProgress->minimum();
}

int CProgressDlg::findMax() const
{
    return fImpl->findProgress->maximum();
}

void CProgressDlg::setFindFormat( const QString& format )
{
    fImpl->findProgress->setFormat( format );
}


QString CProgressDlg::findFormat() const
{
    return fImpl->findProgress->format();
}


void CProgressDlg::setCurrentFindInfo( const QFileInfo& fileInfo )
{
    QLocale locale;
    QString fileDirStr = fRelToDir.relativeFilePath( fileInfo.absoluteFilePath() );

    fImpl->findText->setText( tr( "Current File '%2' (%3 bytes)" ).arg( fileDirStr ).arg( locale.toString( fileInfo.size() ) ) );
}

void CProgressDlg::setMD5Value( int value )
{
    fImpl->md5Progress->setValue( value );
}

int CProgressDlg::md5Value() const
{
    return fImpl->md5Progress->value();
}

void CProgressDlg::setMD5Range( int min, int max )
{
    fImpl->md5Progress->setRange( min, max );
}

int CProgressDlg::md5Min() const
{
    return fImpl->md5Progress->minimum();
}

int CProgressDlg::md5Max() const
{
    return fImpl->md5Progress->maximum();
}

void CProgressDlg::setMD5Format( const QString& format )
{
    fImpl->md5Progress->setFormat( format );
}

QString CProgressDlg::md5Format() const
{
    return fImpl->md5Progress->format();
}


void CProgressDlg::setCurrentMD5Info( const QFileInfo& fileInfo )
{
    QLocale locale;
    QString fileDirStr = fRelToDir.relativeFilePath( fileInfo.absoluteFilePath() );

    fImpl->md5Text->setText( tr( "Current File '%2' (%3 bytes)" ).arg( fileDirStr ).arg( locale.toString( fileInfo.size() ) ) );
}

void CProgressDlg::setNumDuplicaes( int numDuplicates )
{
    fImpl->numDuplicates->setText( tr( "Num Duplicates Found: %1" ).arg( numDuplicates ) );
}

void CProgressDlg::slotUpdateThreadInfo()
{
    auto threadPool = QThreadPool::globalInstance();
    auto numActive = threadPool->activeThreadCount();
    QString txt = tr( "<dl>" )
        + tr( "<dd>Num Active Threads: %1 (MD5 Processing is behind by: %2)</dd>" ).arg( numActive ).arg( fImpl->findProgress->value() - fImpl->md5Progress->value() );

    std::map < qint64, std::shared_ptr< SThreadInfo > > runtimeMap;

    for ( auto &&ii : fMap )
    {
        runtimeMap[ii.second->getRuntime() ] = ii.second;
    }

    for( auto && ii = runtimeMap.rbegin(); ii != runtimeMap.rend(); ++ii )
    {
        txt += "<dt>" + (*ii).second->msg() + "</dt>";
    }
    txt += "</dl>";
    fImpl->threadsLabel->setText( txt );
}

void CProgressDlg::slotMD5FileStarted( unsigned long long threadID, const QDateTime& startTime, const QString& fileName )
{
    fMap[threadID] = std::make_shared< SThreadInfo >( threadID, startTime, fileName );
}


std::shared_ptr< CProgressDlg::CProgressDlg::SThreadInfo > CProgressDlg::getThreadInfo( unsigned long long threadID, const QString &fileName ) const
{
    auto pos = fMap.find( threadID );
    if ( pos == fMap.end() )
        return {};

    if ( ( *pos ).second->fFileInfo != QFileInfo( fileName ) )
        return {};
    return ( *pos ).second;
}

void CProgressDlg::slotMD5ReadPositionStatus( unsigned long long threadID, const QDateTime & /*startTime*/, const QString & fileName, qint64 filePos )
{
    auto threadInfo = getThreadInfo( threadID, fileName );
    if ( !threadInfo )
        return;

    threadInfo->fPos = filePos;
}

void CProgressDlg::slotMD5FileFinishedReading( unsigned long long threadID, const QDateTime& dt, const QString& fileName )
{
    auto threadInfo = getThreadInfo( threadID, fileName );
    if ( !threadInfo )
        return;

    threadInfo->fState = SThreadInfo::EState::eComputing;
    threadInfo->fReadingEndTime = dt;
}

void CProgressDlg::slotMD5FileFinishedComputing( unsigned long long threadID, const QDateTime& dt, const QString& fileName )
{
    auto threadInfo = getThreadInfo( threadID, fileName );
    if ( !threadInfo )
        return;

    threadInfo->fState = SThreadInfo::EState::eFormating;
    threadInfo->fComputingEndTime = dt;
}

void CProgressDlg::slotMD5FileFinished( unsigned long long threadID, const QDateTime& endTime, const QString& fileName, const QString& md5 )
{
    auto threadInfo = getThreadInfo( threadID, fileName );
    if ( !threadInfo )
        return;

    threadInfo->fEndTime = endTime;
    threadInfo->fMD5 = md5;
    threadInfo->fState = SThreadInfo::EState::eFinished;
}

void CProgressDlg::setCancelText( const QString& label )
{
    fImpl->cancelButton->setText( label );
}

QString CProgressDlg::cancelText() const
{
    return fImpl->cancelButton->text();
}

void CProgressDlg::slotFindFinished()
{
    fFindFinished = true;
    fImpl->findGroup->setVisible( false );

    setStatusLabel();
}

void CProgressDlg::slotCurrentFindInfo( const QString& fileName )
{
    setCurrentFindInfo( QFileInfo( fileName ) );
}

void CProgressDlg::slotUpdateFilesFound( int numFilesFound )
{
    setFindValue( numFilesFound );
}

void CProgressDlg::setStatusLabel()
{
    QStringList text;
    if ( !fFindFinished )
        text << tr( "Finding Files" );
    if ( !fMD5Finished )
        text << tr( "Computing MD5s" );

    if ( text.isEmpty() )
        fImpl->mainLabel->setText( tr( "Finished" ) );
    else
        fImpl->mainLabel->setText( text.join( " and " ) + "..." );
}

void CProgressDlg::setRelToDir( const QDir& relToDir )
{
    fRelToDir = relToDir;
}

void CProgressDlg::setMD5Finished()
{
    fMD5Finished = true;
    fImpl->md5Group->setVisible( false );

    setStatusLabel();
}

CProgressDlg::SThreadInfo::SThreadInfo( unsigned long long threadID, const QDateTime& start, const QString& fileName ) :
    fStartTime( start ),
    fFileInfo( fileName ),
    fFileName( fileName ),
    fThreadID( threadID )
{
}

QString CProgressDlg::SThreadInfo::msg() const
{
    QLocale locale;

    auto retVal = QString( "%1: Filename: %2 (%3) (%4 bytes) - Current Runtime: %5 - Overall Runtime: %6" ).arg( fThreadID, 5, 10, QChar( '0' ) ).arg( fFileInfo.fileName() ).arg( getState() ).arg( locale.toString( fFileInfo.size() ) ).arg( getCurrentRuntimeString() ).arg( getRuntimeString() );
    if ( fState == EState::eReading )
        retVal += QString( " - File Position: %1" ).arg( locale.toString( fPos ) );
    if ( fState == EState::eFinished )
        retVal += QString( " - MD5: %1" ).arg( fMD5 );
    return retVal;
}


qint64 CProgressDlg::SThreadInfo::getCurrentRuntime() const
{
    QDateTime startTime;
    QDateTime endTime = QDateTime::currentDateTime();
    switch ( fState )
    {
        case EState::eReading:
            startTime = fStartTime;
            break;
        case EState::eComputing:
            startTime = fReadingEndTime;
            break;
        case EState::eFormating:
            startTime = fComputingEndTime;
            break;
        case EState::eFinished:
            endTime = startTime = fEndTime;
            break;
    }
    return startTime.msecsTo( endTime );
}

QString CProgressDlg::SThreadInfo::getCurrentRuntimeString() const
{
    return NUtils::getTimeString( getCurrentRuntime() );
}

qint64 CProgressDlg::SThreadInfo::getRuntime() const
{
    if ( fState == EState::eFinished )
        return fStartTime.msecsTo( fEndTime );
    else
        return fStartTime.msecsTo( QDateTime::currentDateTime() );
}

QString CProgressDlg::SThreadInfo::getRuntimeString() const
{
    return NUtils::getTimeString( getRuntime() );
}
