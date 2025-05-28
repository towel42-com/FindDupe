
#include "Settings.h"
#include "ui_Settings.h"
//#include "FileFinder.h"
//
//#include "ProgressDlg.h"
//#include "SABUtils/MD5.h"
#include "SABUtils/ButtonEnabler.h"
//#include "SABUtils/utils.h"
//
//#include "SABUtils/FileUtils.h"
//#include "SABUtils/DelayLineEdit.h"
//
//#include <QFileDialog>
//#include <QStandardItemModel>
#include <QSettings>
//#include <QProgressBar>
//#include <QDirIterator>
//#include <QSortFilterProxyModel>
//#include <QRegularExpression>
//#include <QProgressDialog>
//#include <QMessageBox>
//#include <QThreadPool>
//#include <QFontDatabase>
//#include <QTimer>
//#include <QCompleter>
//#include <QFileSystemModel>
//#include <QDesktopServices>
#include <QInputDialog>
//#include <QMenu>
//
//#include <random>
//#include <unordered_set>

CSettings::CSettings( QWidget *parent ) :
    QDialog( parent ),
    fImpl( new Ui::CSettings )
{
    fImpl->setupUi( this );

    setWindowIcon( QIcon( ":/resources/finddupe.png" ) );

    connect( fImpl->ignoreFilesOver, &QCheckBox::clicked, this, &CSettings::slotIgnoreFilesOver );
    connect( fImpl->ignoreFilesOverValue, &QSpinBox::valueChanged, this, &CSettings::slotIgnoreFilesOver );

    connect( fImpl->addPathName, &QToolButton::clicked, this, &CSettings::slotAddIgnoredPathName );
    connect( fImpl->delPathName, &QToolButton::clicked, this, &CSettings::slotDelIgnoredPathName );

    new NSABUtils::CButtonEnabler( fImpl->ignoredPathNames, fImpl->delPathName );

    QSettings settings;
    fImpl->showDupesOnly->setChecked( showDupesOnly() );
    fImpl->ignoreHidden->setChecked( ignoreHidden() );
    fImpl->ignoreFilesOver->setChecked( ignoreFilesOverMB().has_value() );
    if ( ignoreFilesOverMB().has_value() )
        fImpl->ignoreFilesOverValue->setValue( ignoreFilesOverMB().value() );
    fImpl->caseInsensitiveNameCompare->setChecked( caseInsensitiveNameCompare() );
    addIgnoredPathNames( getIgnoredPathNames() );
}

void CSettings::addIgnoredPathNames( const NSABUtils::TCaseInsensitiveHash &ignoredPathNames )
{
    fImpl->ignoredPathNames->addItems( QStringList( { ignoredPathNames.begin(), ignoredPathNames.end() } ) );
}

void CSettings::addIgnoredPathName( const QString &ignoredFileName )
{
    addIgnoredPathNames( { ignoredFileName } );
}

CSettings::~CSettings()
{
}

void CSettings::accept()
{
    QSettings settings;
    settings.setValue( "IgnoreHidden", fImpl->ignoreHidden->isChecked() );
    settings.setValue( "ShowDupesOnly", fImpl->showDupesOnly->isChecked() );
    settings.setValue( "IgnoreFilesOver", fImpl->ignoreFilesOver->isChecked() );
    settings.setValue( "IgnoreFilesOverValue", fImpl->ignoreFilesOverValue->value() );
    settings.setValue( "CaseInsensitiveCompare", fImpl->caseInsensitiveNameCompare->isChecked() );

    auto ignoredPathNames = getIgnoredPathNames();
    QStringList fileNames;
    for ( auto &&ii : ignoredPathNames )
        fileNames << ii;
    settings.setValue( "IgnoredPathNames", fileNames );
    QDialog::accept();
}

bool CSettings::showDupesOnly()
    {
    QSettings settings;
    return settings.value( "ShowDupesOnly", true ).toBool();
}

bool CSettings::ignoreHidden()
{
    QSettings settings;
    return settings.value( "IgnoreHidden", true ).toBool();
}

std::optional< int > CSettings::ignoreFilesOverMB()
{
    std::optional< int > retVal;

    QSettings settings;
    if ( !settings.contains( "IgnoreFilesOver" ) && !settings.contains( "IgnoreFilesOverValue" ) )
        return 1000;

    if ( settings.value( "IgnoreFilesOver", true ).toBool() )
        return {};

    return settings.value( "IgnoreFilesOverValue", 1000 ).toInt();
}

bool CSettings::caseInsensitiveNameCompare()
{
    QSettings settings;
    return settings.value( "CaseInsensitiveCompare", false ).toBool();
}

void CSettings::slotIgnoreFilesOver()
{
    fImpl->ignoreFilesOverValue->setEnabled( fImpl->ignoreFilesOver->isChecked() );
}

NSABUtils::TCaseInsensitiveHash CSettings::getIgnoredPathNames()
{
    QSettings settings;
    auto ignoredPathNames = settings
                                .value(
                                    "IgnoredPathNames", QStringList() << "poster.jpg"
                                                                      << "fanart.jpg"
                                                                      << R"(outtakes.*\.*)"
                                                                      << R"(Deleted Scenes\..*)"
                                                                      << R"(theatrical trailer.*\.*)"
                                                                      << R"(trailer.*\.*)"
                                                                      << R"(auditions.*\.*)"
                                                                      << R"(gag reel.*\.*)"
                                                                      << R"(.*slideshow.*)" )
                                .toStringList();

    NSABUtils::TCaseInsensitiveHash retVal = { ignoredPathNames.begin(), ignoredPathNames.end() };
    return retVal;
}

void CSettings::slotAddIgnoredPathName()
{
    auto fn = QInputDialog::getText( this, tr( "Pathname to Ignore" ), tr( "Path Name (Regular Expression):" ) );
    if ( fn.isEmpty() )
        return;

    addIgnoredPathName( fn );
}

void CSettings::slotDelIgnoredPathName()
{
    auto curr = fImpl->ignoredPathNames->currentItem();
    if ( !curr )
        return;

    delete curr;
}
