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
#cmakedefine HAVE_SYS_UN_H 1

/* Libraries */
#cmakedefine HAVE_LIBSOCKET 1

/* Functions we may want to use. */
#cmakedefine HAVE_MEMCPY 1
#cmakedefine HAVE_MEMMOVE 1
#cmakedefine HAVE_POSIX_FADVISE 1

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
#cmakedefine SIZEOF_OFF_T @SIZEOF_OFF_T@
#cmakedefine HAVE_OFF_T 1
#cmakedefine SIZEOF_SIGSET_T @SIZEOF_SIGSET_T@
#cmakedefine HAVE_SIGSET_T 1
#cmakedefine HAVE_SIZE_T 1
#cmakedefine SIZEOF_UCHAR @SIZEOF_UCHAR@
#cmakedefine HAVE_UCHAR 1
#cmakedefine SIZEOF_UINT @SIZEOF_UINT@
#cmakedefine HAVE_UINT 1
#cmakedefine SIZEOF_ULONG @SIZEOF_ULONG@
#cmakedefine HAVE_ULONG 1
#cmakedefine SIZEOF_INT8 @SIZEOF_INT8@
#cmakedefine HAVE_INT8 1
#cmakedefine SIZEOF_UINT8 @SIZEOF_UINT8@
#cmakedefine HAVE_UINT8 1
#cmakedefine SIZEOF_INT16 @SIZEOF_INT16@
#cmakedefine HAVE_INT16 1
#cmakedefine SIZEOF_UINT16 @SIZEOF_UINT16@
#cmakedefine HAVE_UINT16 1
#cmakedefine SIZEOF_INT32 @SIZEOF_INT32@
#cmakedefine HAVE_INT32 1
#cmakedefine SIZEOF_UINT32 @SIZEOF_UINT32@
#cmakedefine HAVE_UINT32 1
#cmakedefine SIZEOF_U_INT32_T @SIZEOF_U_INT32_T@
#cmakedefine HAVE_U_INT32_T 1
#cmakedefine SIZEOF_INT64 @SIZEOF_INT64@
#cmakedefine HAVE_INT64 1
#cmakedefine SIZEOF_UINT64 @SIZEOF_UINT64@
#cmakedefine HAVE_UINT64 1
#cmakedefine SIZEOF_BOOL @SIZEOF_BOOL@
#cmakedefine HAVE_BOOL 1

#cmakedefine SOCKET_SIZE_TYPE @SOCKET_SIZE_TYPE@

/* Define to `__inline__' or `__inline' if that's what the C compiler calls
   it, or to nothing if 'inline' is not supported under any name.  */
#cmakedefine C_HAS_inline 1
#if !(C_HAS_inline)
#ifndef __cplusplus
# define inline @C_INLINE@
#endif
#endif


#cmakedefine HAVE_FCNTL_NONBLOCK 1

/* If a system has neither O_NONBLOCK nor NDELAY, upgrade is recommended :P */
/* Not having fcntl() is too archaic as well. */
#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif

#cmakedefine HAVE_SSIZE_T 1
#if !defined(HAVE_SSIZE_T) && !defined(ssize_t)
#define ssize_t int
#define HAVE_SSIZE_T 1
#define SSIZE_T_SIZE INT_SIZE
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
