#ifndef MY_COMPILER_INCLUDED
#define MY_COMPILER_INCLUDED

/* Copyright (c) 2010, 2011, Oracle and/or its affiliates. All rights reserved.

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

/**
  Header for compiler-dependent features.

  Intended to contain a set of reusable wrappers for preprocessor
  macros, attributes, pragmas, and any other features that are
  specific to a target compiler.
*/


/**
  Compiler-dependent internal convenience macros.
*/

/* GNU C/C++ */
#if defined __GNUC__
/* Convenience macro to test the minimum required GCC version. */
# define MY_GNUC_PREREQ(maj, min) \
    ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#endif

/**
  Generic (compiler-independent) features.
*/

#ifndef MY_GNUC_PREREQ
# define MY_GNUC_PREREQ(maj, min) (0)
#endif



#endif /* MY_COMPILER_INCLUDED */
