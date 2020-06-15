#
# (c) Copyright 2004 - 2015 Blue Pearl Software Inc.
# All rights reserved.
#
# This source code belongs to Blue Pearl Software Inc.
# It is considered trade secret and confidential, and is not to be used
# by parties who have not received written authorization
# from Blue Pearl Software Inc.
#
# Only authorized users are allowed to use, copy and modify
# this software provided that the above copyright notice
# remains in all copies of this software.
#
#
#
MACRO(CreateUserProj)
	IF(WIN32)
		SET(BPS_DEBUG_ENV "" CACHE STRING "Added environmental variables during debug separate with '&#x0a;'" )
                SET(BPS_DEBUG_CLI "-check -nobpsvdb -output results -verbosity 3" CACHE STRING "Added command line arguments for the CLI tool in debug" )
		SET(BPS_DEBUG_TEST "--gtest_catch_exceptions=0" CACHE STRING "Added command line arguments for the Unit Tests" )

		IF ( MSVC12 )
			SET(PREFIX vcxproj)
			SET(VCPROJ ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.vcxproj.user )
            SET(BPS_VCX_ARCH x64)
            IF(CMAKE_SIZEOF_VOID_P EQUAL 4) 
                SET(BPS_VCX_ARCH Win32)
            ELSE()
                SET(BPS_VCX_ARCH x64)
            ENDIF()
		ELSE()
			SET(PREFIX vcproj)
			SET(VCPROJ ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.vcproj.$ENV{USERDOMAIN}.$ENV{USERNAME}.user )
		ENDIF()

		IF ( NOT EXISTS ${VCPROJ} )
			IF( ${PROJECT_NAME} STREQUAL "BluePearlCLI" )
		                SET( FILE_TEMPLATE ${CMAKE_SOURCE_DIR}/FileTemplates/${PREFIX}.cli.user.in)
			ELSEIF( ${PROJECT_NAME} STREQUAL "BluePearlVVE" )
				SET( FILE_TEMPLATE ${CMAKE_SOURCE_DIR}/FileTemplates/${PREFIX}.vve.user.in)
			ELSEIF( ${PROJECT_NAME} MATCHES "^Test_.*$" )
				SET( FILE_TEMPLATE ${CMAKE_SOURCE_DIR}/FileTemplates/${PREFIX}.unittest.user.in)
			ELSE()
				SET( FILE_TEMPLATE ${CMAKE_SOURCE_DIR}/FileTemplates/${PREFIX}.user.in)
			ENDIF()
			Message(STATUS "===Generating User VCProj File: Project:${PROJECT_NAME} File:${VCPROJ} SOURCE=${FILE_TEMPLATE} USER=$ENV{USERDOMAIN}.$ENV{USERNAME} DebugDir:${CMAKE_INSTALL_PREFIX}")
			configure_file(
				"${FILE_TEMPLATE}"
				"${VCPROJ}"
			)
		ENDIF()
	ENDIF()
ENDMACRO(CreateUserProj)

