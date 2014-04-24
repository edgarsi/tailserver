# Copyright (c) 2009, 2013, Oracle and/or its affiliates. All rights reserved.
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

INCLUDE (CheckCSourceCompiles)
INCLUDE (CheckStructHasMember)
INCLUDE (CheckLibraryExists)
INCLUDE (CheckFunctionExists)
INCLUDE (CheckCCompilerFlag)
INCLUDE (CheckCSourceRuns)
INCLUDE (CheckSymbolExists)



# System type affects version_compile_os variable 
IF(NOT SYSTEM_TYPE)
  IF(PLATFORM)
    SET(SYSTEM_TYPE ${PLATFORM})
  ELSE()
    SET(SYSTEM_TYPE ${CMAKE_SYSTEM_NAME})
  ENDIF()
ENDIF()


# Debug and release versions switched manually.

IF(CMAKE_COMPILER_IS_GNUCC)
  SET(GCC_FLAGS_LIST -W -Wall -pedantic -Wno-long-long -Wno-unused
    -Wcast-qual -D_POSIX_SOURCE -D_POSIX_C_SOURCE
    -Wno-missing-braces -Wextra -Wno-missing-field-initializers -Wformat=2
    -Wswitch-default -Wswitch-enum -Wcast-align -Wpointer-arith
    -Wbad-function-cast -Wstrict-overflow=5 -Winline
    -Wnested-externs -Wcast-qual -Wshadow -Wunreachable-code
    -Wlogical-op -Wfloat-equal -Wstrict-aliasing=2 -Wredundant-decls
    -fno-omit-frame-pointer -ffloat-store -fno-common -fstrict-aliasing
    -fno-strict-aliasing -Wno-variadic-macros)
  STRING(REPLACE ";" " " GCC_FLAGS "${GCC_FLAGS_LIST}")
  
  # Debug
  #SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG} ${GCC_FLAGS}")
ENDIF()

# Release
INCLUDE(cmake/build_configurations/release.cmake)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_C_FLAGS}")


# If finds the size of a type, set SIZEOF_<type> and HAVE_<type>
FUNCTION(MY_CHECK_TYPE_SIZE type defbase)
  CHECK_TYPE_SIZE("${type}" SIZEOF_${defbase})
  IF (SIZEOF_${defbase}_KEYS)
    SET(SIZEOF_${defbase}_CODE ${SIZEOF_${defbase}_CODE} PARENT_SCOPE)
  ENDIF()
  IF(SIZEOF_${defbase})
    SET(HAVE_${defbase} 1 PARENT_SCOPE)
  ENDIF()
ENDFUNCTION()

# Same for structs, setting HAVE_STRUCT_<name> instead
FUNCTION(MY_CHECK_STRUCT_SIZE type defbase)
  CHECK_TYPE_SIZE("struct ${type}" SIZEOF_${defbase})
  IF(SIZEOF_${defbase})
    SET(HAVE_STRUCT_${defbase} 1 PARENT_SCOPE)
  ENDIF()
ENDFUNCTION()

# Searches function in libraries
# if function is found, sets output parameter result to the name of the library
# if function is found in libc, result will be empty 
FUNCTION(MY_SEARCH_LIBS func libs result)
  IF(${${result}})
    # Library is already found or was predefined
    RETURN()
  ENDIF()
  CHECK_FUNCTION_EXISTS(${func} HAVE_${func}_IN_LIBC)
  IF(HAVE_${func}_IN_LIBC)
    SET(${result} "" PARENT_SCOPE)
    RETURN()
  ENDIF()
  FOREACH(lib  ${libs})
    CHECK_LIBRARY_EXISTS(${lib} ${func} "" HAVE_${func}_IN_${lib}) 
    IF(HAVE_${func}_IN_${lib})
      SET(${result} ${lib} PARENT_SCOPE)
      SET(HAVE_${result} 1 PARENT_SCOPE)
      RETURN()
    ENDIF()
  ENDFOREACH()
ENDFUNCTION()

# Find out which libraries to use.
IF(UNIX)
  MY_SEARCH_LIBS(bind "bind;socket" LIBBIND)
  MY_SEARCH_LIBS(setsockopt socket LIBSOCKET)

  SET(CMAKE_REQUIRED_LIBRARIES 
    ${LIBBIND} ${LIBSOCKET})

  LIST(LENGTH CMAKE_REQUIRED_LIBRARIES required_libs_length)
  IF(${required_libs_length} GREATER 0)
    LIST(REMOVE_DUPLICATES CMAKE_REQUIRED_LIBRARIES)
  ENDIF()  
  LINK_LIBRARIES(${CMAKE_THREAD_LIBS_INIT})
ENDIF()

#
# Tests for header files
#
INCLUDE (CheckIncludeFiles)
INCLUDE (CheckIncludeFileCXX)
CHECK_INCLUDE_FILES (fcntl.h HAVE_FCNTL_H)
CHECK_INCLUDE_FILES (memory.h HAVE_MEMORY_H)
CHECK_INCLUDE_FILES (stdio_ext.h HAVE_STDIO_EXT_H)
CHECK_INCLUDE_FILES ("stdlib.h;sys/un.h" HAVE_SYS_UN_H)
CHECK_INCLUDE_FILES (string.h HAVE_STRING_H)
CHECK_INCLUDE_FILES (strings.h HAVE_STRINGS_H)
CHECK_INCLUDE_FILES (unistd.h HAVE_UNISTD_H)

#
# Tests for functions
#
CHECK_FUNCTION_EXISTS (bzero HAVE_BZERO)
CHECK_FUNCTION_EXISTS (memcpy HAVE_MEMCPY)
CHECK_FUNCTION_EXISTS (memmove HAVE_MEMMOVE)
CHECK_FUNCTION_EXISTS (posix_fadvise HAVE_POSIX_FADVISE)
CHECK_FUNCTION_EXISTS (stpcpy HAVE_STPCPY)
CHECK_FUNCTION_EXISTS (fcntl HAVE_FCNTL)
CHECK_FUNCTION_EXISTS (__fpending HAVE_DECL___FPENDING)
CHECK_SYMBOL_EXISTS (ferror_unlocked stdio.h HAVE_DECL_FERROR_UNLOCKED)
CHECK_SYMBOL_EXISTS (fflush_unlocked stdio.h HAVE_DECL_FFLUSH_UNLOCKED)
CHECK_SYMBOL_EXISTS (putc_unlocked stdio.h HAVE_DECL_PUTC_UNLOCKED)

CHECK_FUNCTION_EXISTS (strerror HAVE_STRERROR)
CHECK_FUNCTION_EXISTS (strerror_r HAVE_STRERROR_R)

#
# Tests for type sizes (and presence)
#
INCLUDE (CheckTypeSize)
set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS}
        -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS)

SET(HAVE_VOIDP 1)
SET(HAVE_CHARP 1)
SET(HAVE_LONG 1)
SET(HAVE_SIZE_T 1)

IF(NOT APPLE)
MY_CHECK_TYPE_SIZE("void *" VOIDP)
MY_CHECK_TYPE_SIZE("char *" CHARP)
MY_CHECK_TYPE_SIZE(long LONG)
MY_CHECK_TYPE_SIZE(size_t SIZE_T)
ENDIF()

MY_CHECK_TYPE_SIZE(char CHAR)
MY_CHECK_TYPE_SIZE(short SHORT)
MY_CHECK_TYPE_SIZE(int INT)
MY_CHECK_TYPE_SIZE("long long" LONG_LONG)

SET(CMAKE_EXTRA_INCLUDE_FILES stdio.h sys/types.h)
MY_CHECK_TYPE_SIZE(off_t OFF_T)
SET(CMAKE_EXTRA_INCLUDE_FILES)

SET(CMAKE_EXTRA_INCLUDE_FILES sys/types.h)
MY_CHECK_TYPE_SIZE(bool  BOOL)
SET(CMAKE_EXTRA_INCLUDE_FILES)

IF(HAVE_SYS_SOCKET_H)
  SET(CMAKE_EXTRA_INCLUDE_FILES sys/socket.h)
ENDIF(HAVE_SYS_SOCKET_H)
MY_CHECK_TYPE_SIZE(socklen_t SOCKLEN_T)
SET(CMAKE_EXTRA_INCLUDE_FILES)

SET(CMAKE_EXTRA_INCLUDE_FILES sys/types.h unistd.h stddef.h)
MY_CHECK_TYPE_SIZE(ssize_t SSIZE_T)
SET(CMAKE_EXTRA_INCLUDE_FILES)

# CHECK_SYMBOL_EXISTS (ssize_t "sys/types.h;unistd.h;stddef.h" HAVE_SSIZE)

SET(CMAKE_EXTRA_INCLUDE_FILES sys/types.h inttypes.h)
MY_CHECK_TYPE_SIZE(intmax_t INTTYPES_H_WITH_UINTMAX)
SET(CMAKE_EXTRA_INCLUDE_FILES)
SET(CMAKE_EXTRA_INCLUDE_FILES sys/types.h stdint.h)
MY_CHECK_TYPE_SIZE(uintmax_t STDINT_H_WITH_UINTMAX)
SET(CMAKE_EXTRA_INCLUDE_FILES)

IF(HAVE_FCNTL)
  CHECK_SYMBOL_EXISTS(O_NONBLOCK "unistd.h;fcntl.h" HAVE_FCNTL_NONBLOCK)
  CHECK_SYMBOL_EXISTS(O_NDELAY "unistd.h;fcntl.h" HAVE_FCNTL_NDELAY)
ENDIF()


CHECK_CXX_SOURCE_COMPILES("
#include <sys/socket.h>
int main(int argc, char **argv)
{
  getsockname(0,0,(socklen_t *) 0);
  return 0; 
}"
HAVE_SOCKET_SIZE_T_AS_socklen_t)

IF(HAVE_SOCKET_SIZE_T_AS_socklen_t)
  SET(SOCKET_SIZE_TYPE socklen_t)
ELSE()
  CHECK_CXX_SOURCE_COMPILES("
  #include <sys/socket.h>
  int main(int argc, char **argv)
  {
    getsockname(0,0,(int *) 0);
    return 0; 
  }"
  HAVE_SOCKET_SIZE_T_AS_int)
  IF(HAVE_SOCKET_SIZE_T_AS_int)
    SET(SOCKET_SIZE_TYPE int)
  ELSE()
    CHECK_CXX_SOURCE_COMPILES("
    #include <sys/socket.h>
    int main(int argc, char **argv)
    {
      getsockname(0,0,(size_t *) 0);
      return 0; 
    }"
    HAVE_SOCKET_SIZE_T_AS_size_t)
    IF(HAVE_SOCKET_SIZE_T_AS_size_t)
      SET(SOCKET_SIZE_TYPE size_t)
    ELSE()
      SET(SOCKET_SIZE_TYPE int)
    ENDIF()
  ENDIF()
ENDIF()

#
# Test for how the C compiler does inline, if at all
#
CHECK_C_SOURCE_COMPILES("
static inline int foo(){return 0;}
int main(int argc, char *argv[]){return 0;}"
                            C_HAS_inline)
IF(NOT C_HAS_inline)
  CHECK_C_SOURCE_COMPILES("
  static __inline int foo(){return 0;}
  int main(int argc, char *argv[]){return 0;}"
                            C_HAS___inline)
  SET(C_INLINE __inline)
ENDIF()
  
#
# Check type of signal routines (posix, 4.2bsd, 4.1bsd or v7)
#
CHECK_C_SOURCE_COMPILES("
  #include <signal.h>
  int main(int ac, char **av)
  {
    sigset_t ss;
    struct sigaction sa;
    sigemptyset(&ss); sigsuspend(&ss);
    sigaction(SIGINT, &sa, (struct sigaction *) 0);
    sigprocmask(SIG_BLOCK, &ss, (sigset_t *) 0);
  }"
  HAVE_POSIX_SIGNALS)

IF(NOT HAVE_POSIX_SIGNALS)
 CHECK_C_SOURCE_COMPILES("
  #include <signal.h>
  int main(int ac, char **av)
  {
    int mask = sigmask(SIGINT);
    sigsetmask(mask); sigblock(mask); sigpause(mask);
  }"
  HAVE_BSD_SIGNALS)
  IF (NOT HAVE_BSD_SIGNALS)
    CHECK_C_SOURCE_COMPILES("
    #include <signal.h>
    void foo() { }
    int main(int ac, char **av)
    {
      int mask = sigmask(SIGINT);
      sigset(SIGINT, foo); sigrelse(SIGINT);
      sighold(SIGINT); sigpause(SIGINT);
    }"
   HAVE_SVR3_SIGNALS)  
   IF (NOT HAVE_SVR3_SIGNALS)
    SET(HAVE_V7_SIGNALS 1)
   ENDIF(NOT HAVE_SVR3_SIGNALS)
 ENDIF(NOT HAVE_BSD_SIGNALS)
ENDIF(NOT HAVE_POSIX_SIGNALS)

#
# Check how to determine the number of pending output bytes on a stream
# (taken from fpending.m4 of coreutils)
#
FOREACH(testcode 
  # glibc2
    "fp->_IO_write_ptr - fp->_IO_write_base"
  # traditional Unix
    "fp->_ptr - fp->_base"
  # BSD
    "fp->_p - fp->_bf._base"
  # SCO, Unixware
    "(fp->__ptr ? fp->__ptr - fp->__base : 0)"
  # QNX
    "(fp->_Mode & 0x2000 /*_MWRITE*/ ? fp->_Next - fp->_Buf : 0)"
  # old glibc?
    "fp->__bufp - fp->__buffer"
  # old glibc iostream?
    "fp->_pptr - fp->_pbase"
  # emx+gcc
    "fp->_ptr - fp->_buffer"
  # Minix
    "fp->_ptr - fp->_buf"
  # Plan9
    "fp->wp - fp->buf"
  # VMS
    "(*fp)->_ptr - (*fp)->_base"
  # e.g., DGUX R4.11; the info is not available
    1
  )
  CHECK_C_SOURCE_COMPILES("
    #include <stdio.h>
    int main(int ac, char **av)
    {
     FILE *fp = stdin; (void) (${testcode});
    }"
    HAVE_FPENDING)
  IF(HAVE_FPENDING)
    SET(PENDING_OUTPUT_N_BYTES ${testcode})
    BREAK()
  ENDIF()
  UNSET(HAVE_FPENDING CACHE)
ENDFOREACH()


