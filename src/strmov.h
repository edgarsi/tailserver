/*
   Copyright (c) 2000, 2012, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */


/*TODO: Why do my headers have no wrappers? */

/*TODO: Move through all functions I use - some might be defined by mysql in weird ways */

/*TODO: Go through m_string.h - maybe I'm missing something useful */



#include "config.h"

#include "my_compiler.h"


#if defined(HAVE_STPCPY) && MY_GNUC_PREREQ(3, 4) && !defined(__INTEL_COMPILER)
#define strmov(A,B) __builtin_stpcpy((A),(B))
#elif defined(HAVE_STPCPY)
#define strmov(A,B) stpcpy((A),(B))
#ifndef stpcpy
extern char *stpcpy(char *, const char *);	/* For AIX with gcc 2.95.3 */
#endif
#endif

#ifndef strmov
#define strmov_overlapp(A,B) strmov(A,B)
#define strmake_overlapp(A,B,C) strmake(A,B,C)
#endif

#ifndef strmov
extern	char *strmov(char *dst,const char *src);
#else
extern	char *strmov_overlapp(char *dst,const char *src);
#endif

