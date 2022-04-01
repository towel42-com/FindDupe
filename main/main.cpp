#include "MainWindow/MainWindow.h"

#include <QApplication>
#include <QMessageBox>

#include "Version.h"

int main( int argc, char ** argv )
{
    QApplication::setAttribute( Qt::AA_EnableHighDpiScaling );
    QApplication::setAttribute( Qt::AA_UseHighDpiPixmaps );

    QApplication appl( argc, argv );
    Q_INIT_RESOURCE( application );

    appl.setApplicationName( QString::fromStdString( NVersion::APP_NAME ) );
    appl.setApplicationVersion( QString::fromStdString( NVersion::getVersionString( true ) ) );
    appl.setOrganizationName( QString::fromStdString( NVersion::VENDOR ) );
    appl.setOrganizationDomain( QString::fromStdString( NVersion::HOMEPAGE ) );

    appl.setWindowIcon( QPixmap( ":/resources/finddupe.png" ) );

    CMainWindow * wnd = new CMainWindow;
    wnd->show();
    wnd->setWindowTitle( QString( "%1 v%2 - http://%3" ).arg( QString::fromStdString( NVersion::APP_NAME ) ).arg( QString::fromStdString( NVersion::getVersionString( true ) ) ).arg( QString::fromStdString( NVersion::HOMEPAGE ) ) );
    return appl.exec();
}

