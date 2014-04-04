
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
# Avoid warnings in higher versions
IF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" GREATER 2.6)
  CMAKE_POLICY(VERSION 2.8)
  SET(CMAKE_LEGACY_CYGWIN_WIN32 0)
ENDIF()

MESSAGE(STATUS "Running cmake version ${CMAKE_VERSION}")

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

PROJECT("tailserver")

set(CMAKE_VERBOSE_MAKEFILE on)

# Include the platform-specific file. To allow exceptions, this code
# looks for files in order of how specific they are. If there is, for
# example, a generic Linux.cmake and a version-specific
# Linux-2.6.28-11-generic, it will pick Linux-2.6.28-11-generic and
# include it. It is then up to the file writer to include the generic
# version if necessary.
FOREACH(_base
    ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_VERSION}-${CMAKE_SYSTEM_PROCESSOR}
    ${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_VERSION}
    ${CMAKE_SYSTEM_NAME})
  SET(_file ${CMAKE_SOURCE_DIR}/cmake/os/${_base}.cmake)
  IF(EXISTS ${_file})
    INCLUDE(${_file})
    BREAK()
  ENDIF()
ENDFOREACH()



# Following autotools tradition, add preprocessor definitions
# specified in environment variable CPPFLAGS
IF(DEFINED ENV{CPPFLAGS})
  ADD_DEFINITIONS($ENV{CPPFLAGS})
ENDIF()


# Add macros
INCLUDE(version)
INCLUDE(install_macros)
INCLUDE(install_layout)

# Libraries
INCLUDE(find_libev)

# Handle options
# - none -

include(CheckCSourceCompiles)
include(CheckCXXSourceCompiles)
# We need some extra FAIL_REGEX patterns
# Note that CHECK_C_SOURCE_COMPILES is a misnomer, it will also link.
MACRO (MY_CHECK_C_COMPILER_FLAG FLAG RESULT)
  SET(SAVE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  SET(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${FLAG}")
  CHECK_C_SOURCE_COMPILES("int main(void) { return 0; }" ${RESULT}
    FAIL_REGEX "argument unused during compilation"
    FAIL_REGEX "unsupported .*option"
    FAIL_REGEX "unknown .*option"
    FAIL_REGEX "unrecognized .*option"
    FAIL_REGEX "ignoring unknown option"
    FAIL_REGEX "[Ww]arning: [Oo]ption"
    )
  SET(CMAKE_REQUIRED_FLAGS "${SAVE_CMAKE_REQUIRED_FLAGS}")
ENDMACRO()

MACRO (MY_CHECK_CXX_COMPILER_FLAG FLAG RESULT)
  SET(SAVE_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  SET(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${FLAG}")
  CHECK_CXX_SOURCE_COMPILES("int main(void) { return 0; }" ${RESULT}
    FAIL_REGEX "argument unused during compilation"
    FAIL_REGEX "unsupported .*option"
    FAIL_REGEX "unknown .*option"
    FAIL_REGEX "unrecognized .*option"
    FAIL_REGEX "ignoring unknown option"
    FAIL_REGEX "[Ww]arning: [Oo]ption"
    )
  SET(CMAKE_REQUIRED_FLAGS "${SAVE_CMAKE_REQUIRED_FLAGS}")
ENDMACRO()

OPTION(WITH_ASAN "Enable address sanitizer" OFF)
IF (WITH_ASAN)
  # gcc 4.8.1 and new versions of clang
  MY_CHECK_C_COMPILER_FLAG("-fsanitize=address" HAVE_C_FSANITIZE)
  MY_CHECK_CXX_COMPILER_FLAG("-fsanitize=address" HAVE_CXX_FSANITIZE)

  IF(HAVE_C_FSANITIZE AND HAVE_CXX_FSANITIZE)
    # We switch on basic optimization also for debug builds.
    # With optimization we may get some warnings, so we switch off -Werror
    SET(CMAKE_C_FLAGS_DEBUG
      "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address -O1 -Wno-error -fPIC")
    SET(CMAKE_C_FLAGS_RELWITHDEBINFO
      "${CMAKE_C_FLAGS_RELWITHDEBINFO} -fsanitize=address -fPIC")
    SET(CMAKE_CXX_FLAGS_DEBUG
      "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -O1 -Wno-error -fPIC")
    SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO
      "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -fsanitize=address -fPIC")
    SET(WITH_ASAN_OK 1)
  ELSE()
    # older versions of clang
    MY_CHECK_C_COMPILER_FLAG("-faddress-sanitizer" HAVE_C_FADDRESS)
    MY_CHECK_CXX_COMPILER_FLAG("-faddress-sanitizer" HAVE_CXX_FFADDRESS)

    IF(HAVE_C_FADDRESS AND HAVE_CXX_FFADDRESS)
      # We switch on basic optimization also for debug builds.
      SET(CMAKE_C_FLAGS_DEBUG
        "${CMAKE_C_FLAGS_DEBUG} -faddress-sanitizer -O1 -fPIC")
      SET(CMAKE_C_FLAGS_RELWITHDEBINFO
        "${CMAKE_C_FLAGS_RELWITHDEBINFO} -faddress-sanitizer -fPIC")
      SET(CMAKE_CXX_FLAGS_DEBUG
        "${CMAKE_CXX_FLAGS_DEBUG} -faddress-sanitizer -O1 -fPIC")
      SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO
        "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -faddress-sanitizer -fPIC")
      SET(WITH_ASAN_OK 1)
    ENDIF()
  ENDIF()

  IF(NOT WITH_ASAN_OK)
    MESSAGE(FATAL_ERROR "Do not know how to enable address sanitizer")
  ENDIF()
ENDIF()


# Run platform tests
INCLUDE(configure.cmake)

# Common defines and includes
ADD_DEFINITIONS(-DHAVE_CONFIG_H)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/include)



ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(docs)



CONFIGURE_FILE(config.h.cmake   ${CMAKE_BINARY_DIR}/include/config.h)

# Packaging
SET(CPACK_GENERATOR "TGZ")

# Create a single package with "make package"
# (see http://public.kitware.com/Bug/view.php?id=11452)
SET(CPACK_MONOLITHIC_INSTALL 1 CACHE INTERNAL "")

INCLUDE(CPack)


# Proxy Makefile in source root directory. We want builds in build/ directory while allowing user not to care.
# We also need a way to get rid of cmake cache and files. So, shooting two birds with one Makefile.
IF (NOT EXISTS ${CMAKE_SOURCE_DIR}/Makefile) 
  FILE(WRITE ${CMAKE_SOURCE_DIR}/Makefile "
.DEFAULT_GOAL := all
clean:
	[ -f build/CMakeFiles/Makefile2 ] && cd build/ && make clean || true
	/bin/rm -rf build/CMakeCache.txt build/CMakeFiles/
%:
	cd build/ && make $@
  ")
ENDIF()

