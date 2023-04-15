#include "ProgressDlg.h"
#include "MainWindow.h"
#include "ui_ProgressDlg.h"
#include "SABUtils/utils.h"
#include "SABUtils/FileUtils.h"
#include "SABUtils/SystemInfo.h"
#include <QTreeWidgetItem>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QInputDialog>
#include <QLocale>
#include <QThreadPool>
#include <QCloseEvent>
#include <QPushButton>
#include <QDateTime>
#include <QTimer>
#include <QSettings>

CProgressDlg::CProgressDlg( QWidget *parent ) :
    QWidget( parent ),
    fImpl( new Ui::CProgressDlg )
{
    fImpl->setupUi( this );
    setWindowFlags( windowFlags() & ~Qt::WindowContextHelpButtonHint );
    setWindowFlags( windowFlags() & ~Qt::WindowCloseButtonHint );

    fImpl->findText->setTextFormat( Qt::TextFormat::RichText );
    fImpl->findText->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );

    fImpl->md5Text->setTextFormat( Qt::TextFormat::RichText );
    fImpl->md5Text->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );

    fImpl->computeText->setTextFormat( Qt::TextFormat::RichText );
    fImpl->computeText->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );

    fImpl->computeText->setText( tr( "Computing Number of Files..." ) );
    fImpl->findText->setText( tr( "Finding Files..." ) );
    fImpl->md5Text->setText( tr( "Computing MD5s..." ) );

    connect( fImpl->buttonBox, &QDialogButtonBox::rejected, this, &CProgressDlg::slotCanceled );

    QSettings settings;
    auto sortBy = settings.value( "SortProgressBy", 0 ).toInt();
    switch ( sortBy )
    {
        case 1:
            fImpl->sortByThreadID->setChecked( true );
            break;
        case 2:
            fImpl->sortByPercentComplete->setChecked( true );
            break;
        default:
            fImpl->sortByFileSize->setChecked( true );
            break;
    }
    setCountingFiles( true );
    setStatusLabel();
}

CProgressDlg::CProgressDlg( const QString &cancelText, QWidget *parent ) :
    CProgressDlg( parent )
{
    setCancelText( cancelText );
}

CProgressDlg::~CProgressDlg()
{
    NSABUtils::NCPUUtilization::freeQuery( fCPUUtilizationHandle );
    NSABUtils::NDiskUsage::freeQuery( fDiskIOUtilizationHandle );
    int value = 0;
    if ( fImpl->sortByThreadID->isChecked() )
        value = 1;
    else if ( fImpl->sortByPercentComplete->isChecked() )
        value = 2;
    QSettings settings;
    settings.setValue( "SortProgressBy", value );
}

void CProgressDlg::setCountingFiles( bool counting )
{
    fImpl->computeGroup->setVisible( counting );
    fImpl->findGroup->setVisible( !counting );
    fImpl->md5Group->setVisible( !counting );
    fImpl->sortGroup->setVisible( !counting );
    fImpl->statusHeader->setVisible( !counting );
    fImpl->status->setVisible( !counting );
    fImpl->statusFooter->setVisible( !counting );
}

void CProgressDlg::closeEvent( QCloseEvent *event )
{
    slotCanceled();
    event->accept();
}

void CProgressDlg::slotCanceled()
{
    fCanceled = true;
    fImpl->buttonBox->setEnabled( false );
    emit sigCanceled();
}

void CProgressDlg::slotSetMD5Remaining( int remaining )
{
    setMD5Value( md5Max() - remaining );
}

void CProgressDlg::slotFinishedComputingFileCount()
{
    setCountingFiles( false );
    fComputeNumFilesFinished = true;
    setStatusLabel();
}

void CProgressDlg::setComputeValue( int value )
{
    fImpl->computeProgress->setValue( value );
    slotUpdateStatusInfo();
}

void CProgressDlg::setComputeRange( int min, int max )
{
    fImpl->computeProgress->setRange( min, max );
}

void CProgressDlg::setCurrentComputeInfo( const QFileInfo &fileInfo )
{
    setCurrentInfo( fileInfo, fImpl->computeText );
}

void CProgressDlg::slotCurrentComputeInfo( const QString &fileName )
{
    setCurrentComputeInfo( QFileInfo( fileName ) );
}

void CProgressDlg::slotSetFindRemaining( int remaining )
{
    setFindValue( findMax() - remaining );
}

void CProgressDlg::setFindValue( int value )
{
    fImpl->findProgress->setValue( value );
    slotUpdateStatusInfo();
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

void CProgressDlg::setFindFormat( const QString &format )
{
    fImpl->findProgress->setFormat( format );
}

void CProgressDlg::setComputeFormat( const QString &format )
{
    fImpl->computeProgress->setFormat( format );
}

QString CProgressDlg::findFormat() const
{
    return fImpl->findProgress->format();
}

void CProgressDlg::setCurrentFindInfo( const QFileInfo &fileInfo )
{
    setCurrentInfo( fileInfo, fImpl->findText );
}

void CProgressDlg::setCurrentInfo( const QFileInfo &fileInfo, QLabel *label )
{
    auto fileDirStr = fRelToDir.relativeFilePath( fileInfo.absoluteFilePath() );
    auto fileSizeStr = NSABUtils::NFileUtils::byteSizeString( fileInfo );

    label->setText( tr( "Current File '%2' (%3)" ).arg( fileDirStr ).arg( fileSizeStr ) );
}

void CProgressDlg::setMD5Value( int value )
{
    fImpl->md5Progress->setValue( value );
    slotUpdateStatusInfo();
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

void CProgressDlg::setMD5Format( const QString &format )
{
    fImpl->md5Progress->setFormat( format );
}

QString CProgressDlg::md5Format() const
{
    return fImpl->md5Progress->format();
}

void CProgressDlg::setCurrentMD5Info( const QFileInfo &fileInfo )
{
    QString fileDirStr = fRelToDir.relativeFilePath( fileInfo.absoluteFilePath() );

    fImpl->md5Text->setText( tr( "Current File '%2' (%3)" ).arg( fileDirStr ).arg( NSABUtils::NFileUtils::byteSizeString( fileInfo ) ) );
}

void CProgressDlg::setNumDuplicates( const std::pair< int, size_t > &numDuplicates )
{
    fNumDuplicates = numDuplicates;
}

void CProgressDlg::slotMD5FileStarted( unsigned long long threadID, const QDateTime &startTime, const QString &fileName )
{
    fMap[ threadID ] = std::make_shared< SThreadInfo >( threadID, startTime, fileName );
}

void CProgressDlg::slotMD5ReadPositionStatus( unsigned long long threadID, const QDateTime & /*startTime*/, const QString &fileName, qint64 filePos )
{
    auto threadInfo = getThreadInfo( threadID, fileName );
    if ( !threadInfo )
        return;

    threadInfo->fPos = filePos;
    slotUpdateStatusInfo();
}

void CProgressDlg::slotMD5FileFinishedReading( unsigned long long threadID, const QDateTime &dt, const QString &fileName )
{
    auto threadInfo = getThreadInfo( threadID, fileName );
    if ( !threadInfo )
        return;

    threadInfo->fState = SThreadInfo::EState::eComputing;
    threadInfo->fReadingEndTime = dt;
}

void CProgressDlg::slotMD5FileFinishedComputing( unsigned long long threadID, const QDateTime &dt, const QString &fileName )
{
    auto threadInfo = getThreadInfo( threadID, fileName );
    if ( !threadInfo )
        return;

    threadInfo->fState = SThreadInfo::EState::eFormating;
    threadInfo->fComputingEndTime = dt;
}

void CProgressDlg::slotMD5FileFinished( unsigned long long threadID, const QDateTime &endTime, const QString &fileName, const QString &md5 )
{
    auto threadInfo = getThreadInfo( threadID, fileName );
    if ( !threadInfo )
        return;

    threadInfo->fEndTime = endTime;
    threadInfo->fMD5 = md5;
    threadInfo->fState = SThreadInfo::EState::eFinished;
}

void CProgressDlg::setCancelText( const QString &label )
{
    fImpl->buttonBox->button( QDialogButtonBox::StandardButton::Cancel )->setText( label );
}

QString CProgressDlg::cancelText() const
{
    return fImpl->buttonBox->button( QDialogButtonBox::StandardButton::Cancel )->text();
}

void CProgressDlg::slotFindFinished()
{
    fFindFinished = true;
    fImpl->findGroup->setVisible( false );

    setStatusLabel();
}

void CProgressDlg::slotCurrentFindInfo( const QString &fileName )
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
    if ( !fComputeNumFilesFinished )
        text << "Computing Number of Files";
    if ( !fFindFinished )
        text << tr( "Finding Files" );
    if ( !fMD5Finished )
        text << tr( "Computing MD5s" );

    if ( text.isEmpty() )
        fImpl->mainLabel->setText( tr( "Finished" ) );
    else
    {
        auto msg = text[ 0 ];
        for ( int ii = 1; ii < text.length(); ++ii )
        {
            msg += ", ";
            if ( ii == ( text.length() - 1 ) )
                msg += "and ";
            msg += text[ ii ];
        }
        msg += "...";
        fImpl->mainLabel->setText( msg );
    }
}

void CProgressDlg::setRelToDir( const QDir &relToDir )
{
    fRelToDir = relToDir;
}

void CProgressDlg::setMD5Finished()
{
    fMD5Finished = true;
    fImpl->md5Group->setVisible( false );

    setStatusLabel();
}

CProgressDlg::SThreadInfo::SThreadInfo( unsigned long long threadID, const QDateTime &start, const QString &fileName ) :
    fStartTime( start ),
    fFileInfo( fileName ),
    fFileName( fileName ),
    fThreadID( threadID )
{
    fSize = QFileInfo( fileName ).size();
}

QString CProgressDlg::SThreadInfo::msg() const
{
    auto retVal = QString( "%1: Filename: %2 (%3) (%4) - Current Runtime: %5 - Overall Runtime: %6" ).arg( fThreadID, 5, 10, QChar( '0' ) ).arg( fFileInfo.fileName() ).arg( getState() ).arg( NSABUtils::NFileUtils::byteSizeString( fFileInfo ) ).arg( getCurrentRuntimeString() ).arg( getRuntimeString() );
    if ( fState == EState::eReading )
        retVal += QString( " - File Position: %1 of %2 (%3%)" ).arg( NSABUtils::NFileUtils::byteSizeString( fPos ) ).arg( NSABUtils::NFileUtils::byteSizeString( fSize ) ).arg( getPercentage(), 2 );
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
    NSABUtils::CTimeString ts( getCurrentRuntime() );
    return ts.toString();
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
    NSABUtils::CTimeString ts( getRuntime() );
    return ts.toString();
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

constexpr double sMaxPercentage = 25.0;

void CProgressDlg::slotUpdateStatusInfo()
{
    auto currentTime = QDateTime::currentDateTime();
    if ( ( ( fImpl->findProgress->value() + fImpl->md5Progress->value() + fImpl->computeProgress->value() ) % 500 ) == 0 )
        fAdjustDelayed = true;

    if ( fLastUpdate.isValid() && ( fLastUpdate.msecsTo( currentTime ) < 500 ) )
        return;

    fLastUpdate = currentTime;

    auto threadPool = CMainWindow::threadPool();
    auto numActive = threadPool->activeThreadCount();

    auto cpuUtilization = getCPUUtilization();

    //if ( !fHitMaxPercentage && ( cpuUtilization.second < sMaxPercentage ) ) //&& ( threadPool->maxThreadCount() < 3 * QThread::idealThreadCount() ) )
    //    threadPool->setMaxThreadCount( threadPool->maxThreadCount() + 1 );
    //else
    //    fHitMaxPercentage = true;

    auto diskUtilization = getDiskUtilization();

    QString txt = tr( "<dl>" ) + tr( "<dt>Number of Active Threads: %1 (MD5 Processing is behind by: %2) CPU Utilization: %3 Disk Read %4 Disk Write %5</dt></dl" ).arg( numActive ).arg( fImpl->findProgress->value() - fImpl->md5Progress->value() ).arg( cpuUtilization.first ).arg( diskUtilization.first ).arg( diskUtilization.second );
    fImpl->statusHeader->setText( txt );

    std::map< qint64, std::shared_ptr< SThreadInfo > > runtimeMap;

    for ( auto &&ii = fMap.begin(); ii != fMap.end(); )
    {
        if ( ( *ii ).second->expired() )
            ii = fMap.erase( ii );
        else
        {
            auto key = ( *ii ).second->fSize;

            if ( fImpl->sortByThreadID->isChecked() )
                key = ( *ii ).first;
            else if ( fImpl->sortByPercentComplete->isChecked() )
                key = static_cast< quint64 >( ( *ii ).second->getPercentageD() * 1000 );

            runtimeMap[ key ] = ( *ii ).second;
            ++ii;
        }
    }

    txt = "<dl>";
    for ( auto &&ii = runtimeMap.rbegin(); ii != runtimeMap.rend(); ++ii )
    {
        txt += "<dd>" + ( *ii ).second->msg() + "</dd>";
    }
    txt += "<dl>";
    fImpl->status->setHtml( txt );

    txt = "<dl>";
    txt += "<dt>" + tr( "Number of Duplicates Found: %1" ).arg( fNumDuplicates.first ) + "</dt>";
    txt += "<dt>" + tr( "Total Size of Duplicates: %1" ).arg( NSABUtils::NFileUtils::byteSizeString( fNumDuplicates.second ) ) + "</dt>";
    txt += "</dl>";

    fImpl->statusFooter->setText( txt );

    //if ( fAdjustDelayed )
    //    adjustSize();
    fAdjustDelayed = false;
}

std::pair< QString, double > CProgressDlg::getCPUUtilization()
{
    if ( !fCPUUtilizationHandle.first )
    {
        auto tmp = NSABUtils::NCPUUtilization::initQuery();
        if ( tmp.has_value() )
            fCPUUtilizationHandle = tmp.value();
    }

    auto cpuUtils = NSABUtils::NCPUUtilization::getCPUCoreUtilizations( fCPUUtilizationHandle );
    double total = 0.0;
    double max = 0;
    for ( auto &&ii : cpuUtils )
    {
        if ( ii.first == -1 )
            continue;
        total += ii.second.total();
        max += 100.0;
    }

    auto percentage = total * 100/ max;

    return std::make_pair( QString( "%1%" ).arg( percentage, 5, 'f', 2 ), percentage );
}

std::pair< QString, QString > CProgressDlg::getDiskUtilization()
{
    if ( !fDiskIOUtilizationHandle.first )
    {
        auto tmp = NSABUtils::NDiskUsage::initQuery();
        if ( tmp.has_value() )
            fDiskIOUtilizationHandle = tmp.value();
    }

    auto cpuUtils = NSABUtils::NDiskUsage::getDiskUtilizations( fDiskIOUtilizationHandle );
    auto pos = cpuUtils.find( std::wstring() );
    std::pair< QString, QString > retVal;
    if ( pos != cpuUtils.end() )
    {
        retVal.first = QString( "%1/s" ).arg( NSABUtils::NFileUtils::byteSizeString( ( *pos ).second.fRead ) );
        retVal.second = QString( "%1/s" ).arg( NSABUtils::NFileUtils::byteSizeString( ( *pos ).second.fWrite ) );
    }
    return retVal;
}