#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([SCRAP: Security Camera Recording Application], 1.0.0, fabio.ornellas@gmail.com, scrap)
AM_INIT_AUTOMAKE
AC_COPYRIGHT([(C)2005 Fabio Pugliese Ornellas])
AC_REVISION(1.0.0)
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
CFLAGS="-O2"
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_CHECK_PROG(DOXYGEN, doxygen, yes, no, [], [])
AM_CONDITIONAL(DOXYGENBUILD, test x$DOXYGEN = xyes)
if test x$DOXYGEN = xno
then
  echo "*** Not making doxygen documentation!"
fi
AC_CHECK_PROG(TXT2TAGS, txt2tags, yes, no, [], [])
if test x$TXT2TAGS = xno
then
  echo "*** Not building documentation!"
else
  echo -n "checking txt2tags version >= 2.0... "
  TXT2TAGSV="`( txt2tags -V | awk '{printf $3;}' ) || echo Unknown`"
  if test "$TXT2TAGSV" "<" "2.0"
  then
    echo "$TXT2TAGSV"
    echo "*** Obsolete/unknown version found, not building documentation!"
    TXT2TAGS=no
  else
    echo "$TXT2TAGSV, ok"
  fi
fi
AM_CONDITIONAL(TXT2TAGSBUILD, test x$TXT2TAGS = xyes)

# IJG JPEG library
#AC_CHECK_HEADER([jpeglib.h], [], [echo "*** IJG JPEG library include not found!" ; exit 1])
#AC_CHECK_LIB([jpeg], [jpeg_stdio_dest], [], [echo "*** IJG JPEG library not found!" ; exit 1])
# POSIX threads
AC_CHECK_HEADER([pthread.h], [], [echo "*** POSIX thread include not found!" ; exit 1])
AC_CHECK_LIB([pthread], [pthread_create], [], [echo "*** Missing POSIX threads library!" ; exit 1])
# XviD presence / API check
AC_CHECK_HEADER([xvid.h], [], [echo "*** Missing XviD include!" ; exit 1])
AC_CHECK_LIB([xvidcore], [xvid_global], [], [echo "*** Missing XviD library!" ; exit 1])
echo -n "checking XviD's API version... "
AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[#include <xvid.h>
#if XVID_API_MAJOR(XVID_API) != 4
#error Incompatible XviD API version!
#endif]], []), [], [echo "*** Incompatible XviD API! Please compile using a XviD with API version 4 (eg. XviD 1.0.3)." ; exit 1])
echo "ok"

# Checks for header files.
AC_CHECK_HEADERS([ctype.h dirent.h errno.h error.h fcntl.h fnmatch.h glob.h limits.h linux/types.h linux/videodev.h malloc.h signal.h stdarg.h stdio.h stdlib.h string.h sys/ioctl.h sys/mman.h sys/stat.h sys/time.h sys/types.h time.h unistd.h], [], [echo "*** Missing required header!" ; exit 1])

AC_CONFIG_FILES([Makefile
                 avilib/Makefile
                 doc/Makefile
                 src/Makefile])
AC_OUTPUT
