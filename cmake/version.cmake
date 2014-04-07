# Copyright (c) 2009, 2012, Oracle and/or its affiliates. All rights reserved.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA 

#
# Global constants, only to be changed between major releases.
#

# Generate "something" to trigger cmake rerun when VERSION changes
CONFIGURE_FILE(
  ${CMAKE_SOURCE_DIR}/src/VERSION
  ${CMAKE_BINARY_DIR}/VERSION.dep
)

# Read value for a variable from VERSION.

MACRO(GET_CONFIG_VALUE keyword var)
 IF(NOT ${var})
   FILE (STRINGS ${CMAKE_SOURCE_DIR}/src/VERSION str REGEX "^[ ]*${keyword}=")
   IF(str)
     STRING(REPLACE "${keyword}=" "" str ${str})
     STRING(REGEX REPLACE  "[ ].*" ""  str "${str}")
     SET(${var} ${str})
   ENDIF()
 ENDIF()
ENDMACRO()


# Read version for configure script

MACRO(GET_VERSION)
  GET_CONFIG_VALUE("VERSION_MAJOR" MAJOR_VERSION)
  GET_CONFIG_VALUE("VERSION_MINOR" MINOR_VERSION)

  IF(NOT DEFINED MAJOR_VERSION OR NOT DEFINED MINOR_VERSION)
    MESSAGE(FATAL_ERROR "VERSION file cannot be parsed.")
  ENDIF()

  SET(VERSION "${MAJOR_VERSION}.${MINOR_VERSION}${EXTRA_VERSION}")
  MESSAGE(STATUS "tailserver ${VERSION}")
  MATH(EXPR VERSION_ID "10000*${MAJOR_VERSION} + 100*${MINOR_VERSION}")
  MARK_AS_ADVANCED(VERSION VERSION_ID "${MAJOR_VERSION}.${MINOR_VERSION}")
  SET(CPACK_PACKAGE_VERSION_MAJOR ${MAJOR_VERSION})
  SET(CPACK_PACKAGE_VERSION_MINOR ${MINOR_VERSION})
ENDMACRO()

# Get version and other interesting variables
GET_VERSION()

IF(COMPILATION_COMMENT_VERSION_SOURCE)
  SET(COMPILATION_COMMENT "${COMPILATION_COMMENT}; ${VERSION}")
  GET_INFO_SRC_VALUE("abbrev-commit" COMMIT_ID)
  IF(COMMIT_ID)
    SET(COMPILATION_COMMENT "${COMPILATION_COMMENT}; ${COMMIT_ID}")
  ELSE()
    SET(COMPILATION_COMMENT "${COMPILATION_COMMENT}; Working tree")
  ENDIF()
ENDIF()

MESSAGE(STATUS "${COMPILATION_COMMENT}")

INCLUDE(package_name)
IF(NOT CPACK_PACKAGE_FILE_NAME)
  GET_PACKAGE_FILE_NAME(CPACK_PACKAGE_FILE_NAME)
ENDIF()

IF(NOT CPACK_SOURCE_PACKAGE_FILE_NAME)
  SET(CPACK_SOURCE_PACKAGE_FILE_NAME "tailserver-${VERSION}")
ENDIF()
SET(CPACK_PACKAGE_CONTACT "Edgars Irmejs <edgars.irmejs@gmail.com>")
SET(CPACK_PACKAGE_VENDOR "")
SET(CPACK_SOURCE_GENERATOR "TGZ")
INCLUDE(cpack_source_ignore_files)

