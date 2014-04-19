/* Prefer faster, non-thread-safe stdio functions if available.

   Copyright (C) 2001-2004, 2009-2013 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Jim Meyering.  */

#ifndef UNLOCKED_IO_H
# define UNLOCKED_IO_H 1

/* These are wrappers for functions/macros from the GNU C library, and
   from other C libraries supporting POSIX's optional thread-safe functions.

   The standard I/O functions are thread-safe.  These *_unlocked ones are
   more efficient but not thread-safe.  That they're not thread-safe is
   fine since all of the applications in this package are single threaded.

   Also, some code that is shared with the GNU C library may invoke
   the *_unlocked functions directly.  On hosts that lack those
   functions, invoke the non-thread-safe versions instead.  */

# include <stdio.h>

# if HAVE_DECL_FERROR_UNLOCKED
#  undef ferror
#  define ferror(x) ferror_unlocked (x)
# else
#  define ferror_unlocked(x) ferror (x)
# endif

# if HAVE_DECL_FFLUSH_UNLOCKED
#  undef fflush
#  define fflush(x) fflush_unlocked (x)
# else
#  define fflush_unlocked(x) fflush (x)
# endif

# if HAVE_DECL_PUTC_UNLOCKED
#  undef putc
#  define putc(x,y) putc_unlocked (x,y)
# else
#  define putc_unlocked(x,y) putc (x,y)
# endif

#endif /* UNLOCKED_IO_H */
