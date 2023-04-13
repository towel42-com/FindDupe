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

    appl.setApplicationName( NVersion::APP_NAME );
    appl.setApplicationVersion( NVersion::getVersionString( true ) );
    appl.setOrganizationName( NVersion::VENDOR );
    appl.setOrganizationDomain( NVersion::HOMEPAGE );

    appl.setWindowIcon( QPixmap( ":/resources/finddupe.png" ) );

    CMainWindow * wnd = new CMainWindow;
    wnd->show();
    wnd->setWindowTitle( QString( "%1 v%2 - http://%3" ).arg( NVersion::APP_NAME ).arg( NVersion::getVersionString( true ) ).arg( NVersion::HOMEPAGE ) );
    return appl.exec();
}

