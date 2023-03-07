#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([unnks],[0.2.6],[unavowed@vexillium.org])
AC_CONFIG_SRCDIR([src/unnks.c])
AC_CONFIG_HEADERS([src/config.h])

AM_INIT_AUTOMAKE([foreign])

# M4 defines __unix__ to be the empty string on UNIX-like systems.  This
# causes AC_DEFINE to fail if it is used to define __unix__.  This seems to
# fix the problem.
ifdef([__unix__], define([__unix__], [[__unix__]]))

AC_CANONICAL_HOST

# Checks for programs.
AC_PROG_CC
PKG_PROG_PKG_CONFIG

AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL

AC_MSG_CHECKING([host operating system])

case "$host_os" in
  mingw32*)
    os="windows"
    ;;
  darwin*)
    os="darwin"
    ;;
  *)
    os="$host_os"
    ;;
esac

AM_CONDITIONAL([OS_WINDOWS], [test "$os" = "windows"])
AM_CONDITIONAL([OS_DARWIN], [test "$os" = "darwin"])
AC_MSG_RESULT([$os])

AC_MSG_CHECKING([whether to build nks-ls-libs])

case "$host_os" in
  darwin*|mingw32*)
    build_nks_ls_libs=yes
    ;;
  *)
    build_nks_ls_libs=no
    ;;
esac

AM_CONDITIONAL([BUILD_NKS_LS_LIBS], [test "x$build_nks_ls_libs" = "xyes"])
AC_MSG_RESULT([$build_nks_ls_libs])

AC_MSG_CHECKING([[if we need a __unix__ define]])
AC_COMPILE_IFELSE([
  #ifndef __unix__
  # if defined __APPLE__ /* add other known platforms here */
  #   error should have __unix__ but don't
  # endif
  #endif
], [AC_MSG_RESULT([no])],
[
  AC_MSG_RESULT([yes])
  AC_DEFINE([__unix__], 1,
[Define to 1 if running on a UNIX-like system where __unix__ is missing])
])


# Checks for libraries.

BACKUP_LIBS=$LIBS
AC_CHECK_LIB([gcrypt],[gcry_check_version],,[
  AC_MSG_ERROR(libgcrypt not found)
])
LIBS=$BACKUP_LIBS
GCRYPT_LIBS=-lgcrypt
AC_SUBST([GCRYPT_LIBS])

PKG_CHECK_MODULES([GLIB], [glib-2.0], ,[
  AC_MSG_ERROR(glib-2.0 not found)
])

# Checks for header files.
m4_warn([obsolete],
[The preprocessor macro `STDC_HEADERS' is obsolete.
  Except in unusual embedded environments, you can safely include all
  ISO C90 headers unconditionally.])dnl
# Autoupdate added the next two lines to ensure that your configure
# script's behavior did not change.  They are probably safe to remove.
AC_CHECK_INCLUDES_DEFAULT
AC_PROG_EGREP

AC_CHECK_HEADERS([inttypes.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_OFF_T

AC_SYS_LARGEFILE

# Checks for library functions.
AC_CHECK_FUNCS([posix_fallocate])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
