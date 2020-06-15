#include "MD5.h"

#include <QString>
#include <QCryptographicHash>
#include <QFileInfo>
namespace NUtils
{
    QByteArray formatMd5( const QByteArray & digest, bool isHex )
    {
        QByteArray md5Str = digest;
        if ( !isHex )
        {
            md5Str = md5Str.toHex();
        }
        md5Str = md5Str.toUpper();
        if ( md5Str.length() < 32 )
            md5Str = QString( "%1" ).arg( QString::fromLatin1( md5Str ), 32, QLatin1Char( '0' ) ).toLatin1();
        if ( md5Str.length() == 32 )
        {
            md5Str = md5Str.mid( 0, 8 ) + "-" +
                md5Str.mid( 8, 4 ) + "-" +
                md5Str.mid( 12, 4 ) + "-" +
                md5Str.mid( 16, 4 ) + "-" +
                md5Str.mid( 20 )
                ;
        }
        else
        {
            for( int ii = md5Str.length() - 4; ii > 0; ii -= 4 )
            {
                md5Str.insert( ii, '-' );
            }
        }
        return md5Str;
    }

    QByteArray getMd5( const QByteArray & data )
    {
        auto digest = QCryptographicHash::hash(data, QCryptographicHash::Md5);
        return formatMd5( digest, false );
    }

    QString getMd5( const QString & data, bool isFileName )
    {
        if ( isFileName )
            return getMd5( QFileInfo( data ) );

        QByteArray inData = data.toLatin1();
        return QString::fromLatin1( getMd5( inData ) );
    }

    std::string getMd5( const std::string & data, bool isFileName )
    {
        return getMd5( QString::fromStdString( data ), isFileName ).toStdString();
    }

    QString getMd5( const QFileInfo & fi )
    {
        QFile file( fi.absoluteFilePath() );
        if ( !file.open( QIODevice::ReadOnly ) )
            return QString();

        QCryptographicHash hash( QCryptographicHash::Md5 );
        if ( hash.addData( &file ) )
            return QString::fromLatin1( formatMd5( hash.result(), false ) );

        return QString();
    }
}

