# The MIT License (MIT)
#
# Copyright (c) 2020 Scott Aron Bloom
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

project(FindDupe) 

include( include.cmake )
IncludeProjectCMake()

add_executable( FindDupe WIN32
                ${_PROJECT_DEPENDENCIES} 
                application.rc
          )
set_target_properties( ${PROJECT_NAME} PROPERTIES FOLDER ${folderName} )
          
get_filename_component( QTDIR ${DEPLOYQT_EXECUTABLE} DIRECTORY )
include_directories( ${CMAKE_BINARY_DIR} )
set ( DEBUG_PATH 
        "%PATH%"
        "$<TARGET_FILE_DIR:SABUtils>"
        "${QTDIR}"
        )

set_target_properties( ${PROJECT_NAME} PROPERTIES FOLDER Apps 
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:FindDupe>" 
        VS_DEBUGGER_COMMAND "$<TARGET_FILE:FindDupe>" 
        VS_DEBUGGER_ENVIRONMENT "PATH=${DEBUG_PATH}" 
        )

target_link_libraries( FindDupe
      PUBLIC
          ${project_pub_DEPS}
      PRIVATE
          ${project_pri_DEPS}
)

DeployQt( FindDupe . INSTALL_ONLY 1 )
DeployQt( SABUtils . INSTALL_ONLY 1 )
DeploySystem( FindDupe . INSTALL_ONLY 1 )

INSTALL( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin )
INSTALL( FILES ${CMAKE_CURRENT_BINARY_DIR}/Debug/${PROJECT_NAME}.pdb DESTINATION bin CONFIGURATIONS Debug)
INSTALL( FILES ${CMAKE_CURRENT_BINARY_DIR}/Debug/${PROJECT_NAME}.pdb DESTINATION pdbs CONFIGURATIONS RelWithDebInfo )


