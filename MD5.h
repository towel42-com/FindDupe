/*
 * (c) Copyright 2004 - 2019 Blue Pearl Software Inc.
 * All rights reserved.
 *
 * This source code belongs to Blue Pearl Software Inc.
 * It is considered trade secret and confidential, and is not to be used
 * by parties who have not received written authorization
 * from Blue Pearl Software Inc.
 *
 * Only authorized users are allowed to use, copy and modify
 * this software provided that the above copyright notice
 * remains in all copies of this software.
 *
 *
 * $Author: scott $ - $Revision: 55131 $ - $Date: 2019-09-24 19:49:25 -0700 (Tue, 24 Sep 2019) $
 * $HeadURL: http://bpsvn/svn/trunk/Shared/GenUtils/MD5.h $
 *
 *
*/
#ifndef __COMMON_MD5_H
#define __COMMON_MD5_H

class QByteArray;
class QFileInfo;
class QString;
#include <string>

namespace Verific{ class verific_stream; }
namespace NUtils
{
    QByteArray getMd5( const QByteArray & data );
    QString getMd5( const QFileInfo & fi );
    QString getMd5( const QString & data, bool isFileName=false );
    std::string getMd5( const std::string & data, bool isFileName=false );
    QByteArray formatMd5( const QByteArray & digest, bool isHex );
}

#endif
