#ifndef __PROGRESSDLG_H
#define __PROGRESSDLG_H

#include <QDialog>
#include <memory>
#include <QDir>
#include <QDateTime>
#include <map>
class QTreeWidgetItem;
class QDir;
class QFileInfo;
namespace Ui { class CProgressDlg; };

class CProgressDlg : public QDialog
{
    Q_OBJECT
public:
    CProgressDlg( QWidget* parent );
    CProgressDlg( const QString& cancelText, QWidget* parent );
    ~CProgressDlg();
    virtual void closeEvent( QCloseEvent* event ) override;

    void setFindValue( int value );
    int findValue() const;
    void setFindRange( int min, int max );
    int findMin() const;
    int findMax() const;
    void setFindFormat( const QString& format );
    QString findFormat() const;
    void setCurrentFindInfo( const QFileInfo& fileInfo );

    void setStatusLabel();

    void setRelToDir( const QDir& relToDir );

    void setMD5Value( int value );
    int md5Value() const;
    void setMD5Range( int min, int max );
    int md5Min() const;
    int md5Max() const;
    void setMD5Format( const QString& format );
    QString md5Format() const;
    void setCurrentMD5Info( const QFileInfo& fileInfo );
    void setMD5Finished();

    void setNumDuplicaes( int numDuplicates );

    void setCancelText( const QString& cancelText );
    QString cancelText() const;

    bool wasCanceled() const { return fCanceled; }
public Q_SLOTS:
    void slotFindFinished();
    void slotCurrentFindInfo( const QString& fileName );
    void slotUpdateFilesFound( int numFilesFound );

    void slotCanceled();
    void slotSetFindRemaining( int remaining );
    void slotSetMD5Remaining( int remaining );
    void slotFinishedComputingFileCount();

    void slotMD5FileStarted( unsigned long long threadID, const QDateTime& startTime, const QString& fileName );
    void slotMD5ReadPositionStatus( unsigned long long threadID, const QDateTime &startTime, const QString &fileName, qint64 pos );

    void slotMD5FileFinishedReading( unsigned long long threadID, const QDateTime &dt, const QString &filename );
    void slotMD5FileFinishedComputing( unsigned long long threadID, const QDateTime& dt, const QString& filename );
    void slotMD5FileFinished( unsigned long long threadID, const QDateTime& endTime, const QString& fileName, const QString& md5 );

    void slotUpdateThreadInfo();
Q_SIGNALS:
    void sigCanceled();
private:
    bool fCanceled{ false };
    bool fFindFinished{ false };
    bool fMD5Finished{ false };

    QDir fRelToDir;
    struct SThreadInfo
    {
        enum class EState
        {
            eReading,
            eComputing,
            eFormating,
            eFinished
        };
        SThreadInfo() {}
        SThreadInfo( unsigned long long threadID, const QDateTime& start, const QString& fileName );

        QString msg() const;
        QString getState() const
        {
            if ( fState == EState::eReading )
                return "Reading";
            else if ( fState == EState::eComputing )
                return "Computing";
            else if ( fState == EState::eFormating )
                return "Formating";
            else
                return "Finished";
        }
        qint64 getRuntime() const;
        QString getRuntimeString() const;

        qint64 getCurrentRuntime() const;
        QString getCurrentRuntimeString() const;

        EState fState{ EState::eReading };
        QDateTime fStartTime;
        QDateTime fReadingEndTime;
        QDateTime fComputingEndTime;
        QDateTime fEndTime;
        QFileInfo fFileInfo;
        QString fFileName;
        QString fMD5;
        qint64 fPos{ 0 };
        unsigned long long fThreadID{ 0 };
    };
    std::shared_ptr< SThreadInfo > getThreadInfo( unsigned long long threadID, const QString &fileName ) const;
    std::map< unsigned long long, std::shared_ptr< SThreadInfo > > fMap;
    std::unique_ptr< Ui::CProgressDlg > fImpl;
    QTimer* fTimer{ nullptr };
    QDateTime fLastUpdate;
    bool fAdjustDelayed{ false };
};
#endif 