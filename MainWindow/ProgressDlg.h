#ifndef __PROGRESSDLG_H
#define __PROGRESSDLG_H

#include <QDialog>
#include <memory>

class QTreeWidgetItem;
class QDir;
class QFileInfo;
namespace Ui {class CProgressDlg;};

class CProgressDlg : public QDialog
{
    Q_OBJECT
public:
    CProgressDlg( QWidget * parent );
    CProgressDlg( const QString & cancelText, QWidget * parent );
    ~CProgressDlg();

    void setFindValue( int value );
    int findValue() const;
    void setFindRange( int min, int max );
    int findMin() const;
    int findMax() const;
    void setFindFormat( const QString & format );
    QString findFormat() const;
    void setCurrentFindInfo( const QDir & relToDir, const QFileInfo& fileInfo);
    void setFindFinished();

    void setStatusLabel();

    void setMD5Value( int value );
    int md5Value() const;
    void setMD5Range( int min, int max );
    int md5Min() const;
    int md5Max() const;
    void setMD5Format( const QString & format );
    QString md5Format() const;
    void setCurrentMD5Info( const QDir & relToDir, const QFileInfo& fileInfo);
    void setMD5Finished();
    void setNumActiveMD5(int numActive);

    void setNumDuplicaes(int numDuplicates);

    void setCancelText( const QString & cancelText );
    QString cancelText() const;

    bool wasCanceled() const{ return fCanceled; }
public Q_SLOTS:
    void slotCanceled();
	void slotSetFindRemaining(int remaining);
    void slotSetMD5Remaining(int remaining);

Q_SIGNALS:
    void sigCanceled();
private:
    bool fCanceled{ false };
    bool fFindFinished{ false };
    bool fMD5Finished{ false };

    std::unique_ptr< Ui::CProgressDlg > fImpl;
};
#endif 