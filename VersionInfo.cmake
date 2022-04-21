# The MIT License (MIT)
#
# Copyright (c) 2022 Scott Aron Bloom
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# Thg above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

find_package(Git REQUIRED)
find_package(CreateVersion REQUIRED)
GetGitInfo(${CMAKE_SOURCE_DIR} GIT_VERSION_INFO)
STRING(TIMESTAMP BUILD_DATE "%m/%d/%Y")
SET( MAJOR_VERSION 1 )
SET( MINOR_VERSION 3 )
SET( APP_NAME "Find Dupe" )
SET( VENDOR   "Scott Aron Bloom" )
SET( HOMEPAGE "www.towel42.com" )
SET( PRODUCT_HOMEPAGE "github.com/towel42-com/FindDupe" )
SET( EMAIL            "support@towel42.com" )

CreateVersion( ${CMAKE_SOURCE_DIR} 
	MAJOR ${MAJOR_VERSION} 
    MINOR ${MINOR_VERSION} 
    PATCH ${GIT_VERSION_INFO_REV}
    DIFF  ${GIT_VERSION_INFO_DIFF}
    APP_NAME ${APP_NAME} 
    VENDOR ${VENDOR} 
    HOMEPAGE ${HOMEPAGE} 
    PRODUCT_HOMEPAGE ${PRODUCT_HOMEPAGE} 
    EMAIL ${EMAIL} 
    BUILD_DATE ${BUILD_DATE})

