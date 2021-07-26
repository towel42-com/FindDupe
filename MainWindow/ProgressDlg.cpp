#include "ProgressDlg.h"
#include "ui_ProgressDlg.h"

#include <QTreeWidgetItem>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QInputDialog>
#include <QLocale>
#include <QThreadPool>
#include <QCloseEvent>

CProgressDlg::CProgressDlg(QWidget* parent)
	: QDialog(parent),
	fImpl(new Ui::CProgressDlg)
{
	fImpl->setupUi(this);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);

	fImpl->findText->setTextFormat(Qt::TextFormat::RichText);
	fImpl->findText->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

	fImpl->md5Text->setTextFormat(Qt::TextFormat::RichText);
	fImpl->md5Text->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

	connect(fImpl->cancelButton, &QPushButton::clicked, this, &CProgressDlg::slotCanceled);
	setStatusLabel();
}

CProgressDlg::CProgressDlg(const QString& cancelText, QWidget* parent) :
	CProgressDlg(parent)
{
	fImpl->findText->setText(tr("Finding Files..."));
	fImpl->md5Text->setText(tr("Computing MD5s..."));
	setCancelText(cancelText);
}

CProgressDlg::~CProgressDlg()
{
}

void CProgressDlg::closeEvent(QCloseEvent* event)
{
	slotCanceled();
	event->accept();
}

void CProgressDlg::slotCanceled()
{
	fCanceled = true;
	emit sigCanceled();
}


void CProgressDlg::slotSetMD5Remaining(int remaining)
{
	setMD5Value(md5Max() - remaining);
}

void CProgressDlg::slotFinishedComputingFileCount()
{
	fImpl->processingFileCountLabel->setVisible(false);
}

void CProgressDlg::slotSetFindRemaining(int remaining)
{
	setFindValue(findMax() - remaining);
}
void CProgressDlg::setFindValue(int value)
{
	fImpl->findProgress->setValue(value);
}

int CProgressDlg::findValue() const
{
	return fImpl->findProgress->value();
}

void CProgressDlg::setFindRange(int min, int max)
{
	fImpl->findProgress->setRange(min, max);
}

int CProgressDlg::findMin() const
{
	return fImpl->findProgress->minimum();
}

int CProgressDlg::findMax() const
{
	return fImpl->findProgress->maximum();
}

void CProgressDlg::setFindFormat(const QString& format)
{
	fImpl->findProgress->setFormat(format);
}


QString CProgressDlg::findFormat() const
{
	return fImpl->findProgress->format();
}


void CProgressDlg::setCurrentFindInfo(const QFileInfo& fileInfo)
{
	QLocale locale;
	QString fileDirStr = fRelToDir.relativeFilePath(fileInfo.absoluteFilePath());

	fImpl->findText->setText(tr("Current File '%2' (%3 bytes)").arg(fileDirStr).arg(locale.toString(fileInfo.size())));
}

void CProgressDlg::setMD5Value(int value)
{
	fImpl->md5Progress->setValue(value);
}

int CProgressDlg::md5Value() const
{
	return fImpl->md5Progress->value();
}

void CProgressDlg::setMD5Range(int min, int max)
{
	fImpl->md5Progress->setRange(min, max);
}

int CProgressDlg::md5Min() const
{
	return fImpl->md5Progress->minimum();
}

int CProgressDlg::md5Max() const
{
	return fImpl->md5Progress->maximum();
}

void CProgressDlg::setMD5Format(const QString& format)
{
	fImpl->md5Progress->setFormat(format);
}

QString CProgressDlg::md5Format() const
{
	return fImpl->md5Progress->format();
}


void CProgressDlg::setCurrentMD5Info(const QFileInfo& fileInfo)
{
	QLocale locale;
	QString fileDirStr = fRelToDir.relativeFilePath(fileInfo.absoluteFilePath());

	fImpl->md5Text->setText(tr("Current File '%2' (%3 bytes)").arg(fileDirStr).arg(locale.toString(fileInfo.size())));
}

void CProgressDlg::setNumDuplicaes(int numDuplicates)
{
	fImpl->numDuplicates->setText(tr("Num Duplicates Found: %1").arg(numDuplicates));
}

void CProgressDlg::updateActiveThreads()
{
	auto threadPool = QThreadPool::globalInstance();
	auto numActive = threadPool->activeThreadCount();
	fImpl->numThreads->setText(tr("Num Active Threads: %1 (MD5 Processing is behind by: %2)").arg(numActive).arg( fImpl->findProgress->value() - fImpl->md5Progress->value() ) );
}

void CProgressDlg::setCancelText(const QString& label)
{
	fImpl->cancelButton->setText(label);
}

QString CProgressDlg::cancelText() const
{
	return fImpl->cancelButton->text();
}

void CProgressDlg::setFindFinished()
{
	fFindFinished = true;
	fImpl->findGroup->setVisible(false);

	setStatusLabel();
}


void CProgressDlg::setStatusLabel()
{
	QStringList text;
	if (!fFindFinished)
		text << tr("Finding Files");
	if (!fMD5Finished)
		text << tr("Computing MD5s");

	if ( text.isEmpty() )
		fImpl->mainLabel->setText( tr( "Finished" ) );
	else
		fImpl->mainLabel->setText(text.join(" and ") + "...");
}

void CProgressDlg::setRelToDir(const QDir& relToDir)
{
	fRelToDir = relToDir;
}

void CProgressDlg::setMD5Finished()
{
	fMD5Finished = true;
	fImpl->md5Group->setVisible(false);

	setStatusLabel();
}
