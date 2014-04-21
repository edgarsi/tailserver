/* Copyright (c) 2009, 2011, Oracle and/or its affiliates. All rights reserved.
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2 of the License.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#ifndef MY_CONFIG_H
#define MY_CONFIG_H

/* Headers we may want to use. */
#cmakedefine HAVE_FCNTL_H 1
#cmakedefine HAVE_MEMORY_H 1
#cmakedefine HAVE_STDIO_EXT_H 1
#cmakedefine HAVE_STRING_H 1
#cmakedefine HAVE_STRINGS_H 1
#cmakedefine HAVE_SYS_UN_H 1
#cmakedefine HAVE_UNISTD_H 1

/* Functions we may want to use. */
#cmakedefine HAVE_BZERO 1
#cmakedefine HAVE_MEMCPY 1
#cmakedefine HAVE_MEMMOVE 1
#cmakedefine HAVE_POSIX_FADVISE 1
#cmakedefine HAVE_STPCPY 1
#cmakedefine HAVE_FCNTL 1
#cmakedefine HAVE_DECL___FPENDING 1
#cmakedefine HAVE_DECL_FERROR_UNLOCKED 1
#cmakedefine HAVE_DECL_FFLUSH_UNLOCKED 1
#cmakedefine HAVE_DECL_PUTC_UNLOCKED 1

#define USE_UNLOCKED_IO 1

/* Types we may use */
#ifdef __APPLE__
  /*
    Special handling required for OSX to support universal binaries that 
    mix 32 and 64 bit architectures.
  */
  #if(__LP64__)
    #define SIZEOF_LONG 8
  #else
    #define SIZEOF_LONG 4
  #endif
  #define SIZEOF_VOIDP   SIZEOF_LONG
  #define SIZEOF_CHARP   SIZEOF_LONG
  #define SIZEOF_SIZE_T  SIZEOF_LONG
#else
/* No indentation, to fetch the lines from verification scripts */
#cmakedefine SIZEOF_LONG   @SIZEOF_LONG@
#cmakedefine SIZEOF_VOIDP  @SIZEOF_VOIDP@
#cmakedefine SIZEOF_CHARP  @SIZEOF_CHARP@
#cmakedefine SIZEOF_SIZE_T @SIZEOF_CHARP@
#endif

#cmakedefine SIZEOF_CHAR @SIZEOF_CHAR@
#define HAVE_CHAR 1
#define HAVE_LONG 1
#define HAVE_CHARP 1
#cmakedefine SIZEOF_SHORT @SIZEOF_SHORT@
#define HAVE_SHORT 1
#cmakedefine SIZEOF_INT @SIZEOF_INT@
#define HAVE_INT 1
#cmakedefine SIZEOF_LONG_LONG @SIZEOF_LONG_LONG@
#cmakedefine HAVE_LONG_LONG 1
#ifdef HAVE_LONG_LONG
#define HAVE_LONG_LONG_INT 1
#endif
#cmakedefine SIZEOF_OFF_T @SIZEOF_OFF_T@
#cmakedefine HAVE_OFF_T 1
#cmakedefine SIZEOF_SIGSET_T @SIZEOF_SIGSET_T@
#cmakedefine HAVE_SIGSET_T 1
#cmakedefine HAVE_SIZE_T 1
#cmakedefine SIZEOF_BOOL @SIZEOF_BOOL@
#cmakedefine HAVE_BOOL 1

#cmakedefine HAVE_INTTYPES_H_WITH_UINTMAX 1
#cmakedefine HAVE_STDINT_H_WITH_UINTMAX 1

#cmakedefine HAVE_FCNTL_NONBLOCK 1
#cmakedefine HAVE_FCNTL_NDELAY 1

#cmakedefine HAVE_SSIZE_T 1
#if !defined(HAVE_SSIZE_T) && !defined(ssize_t)
#define ssize_t int
#define HAVE_SSIZE_T 1
#define SSIZE_T_SIZE INT_SIZE
#endif

#cmakedefine SOCKET_SIZE_TYPE @SOCKET_SIZE_TYPE@

/* Define to `__inline__' or `__inline' if that's what the C compiler calls
   it, or to nothing if 'inline' is not supported under any name.  */
#cmakedefine C_HAS_inline 1
#if !(C_HAS_inline)
#ifndef __cplusplus
# define inline @C_INLINE@
#endif
#endif


/* Not using threads allows me not to have strerror_r */
#cmakedefine HAVE_STRERROR_R 1
#if !defined(HAVE_STRERROR_R) && !defined(strerror_r) && defined(HAVE_STRERROR)
#define strerror_r strerror
#define HAVE_STRERROR_R 1
#endif
#if defined(HAVE_STRERROR_R)
#define HAVE_DECL_STRERROR_R 1
#endif

/* I'm not interested in any translations. Only using the _() wrappers for practice of style. */
#define _(msgid) (msgid)

#define PENDING_OUTPUT_N_BYTES "@PENDING_OUTPUT_N_BYTES"

/* I don't really need safe reopen as long as long as only stdin is ever reopened.
TODO: Remove safe_reopen completely in future. I'm not doing it now because reinserting it, if needed, will take more
time than leaving it in.
#ifdef HAVE_FCNTL_H
#define GNULIB_FREOPEN_SAFER 1
#endif
*/

#define SYSTEM_TYPE "@SYSTEM_TYPE@"
#define MACHINE_TYPE "@CMAKE_SYSTEM_PROCESSOR@"
#cmakedefine HAVE_DTRACE 1


/* This should mean case insensitive file system */
#cmakedefine FN_NO_CASE_SENSE 1


#define MAJOR_VERSION @MINOR_VERSION@
#define MINOR_VERSION @MAJOR_VERSION@

#define PACKAGE "tailserver"
#define PACKAGE_BUGREPORT "<https://github.com/edgarsi/tailserver/issues>"
#define PACKAGE_URL "https://github.com/edgarsi/tailserver"
#define PACKAGE_NAME "tailserver"
#define PACKAGE_STRING "tailserver"
#define PACKAGE_TARNAME "tailserver"
#define PACKAGE_VERSION "@VERSION@"
#define VERSION "@VERSION@"


#endif
