#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([linuxlist], [0.1], [malvineous@shikadi.net])
AM_INIT_AUTOMAKE([linuxlist], [0.1])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_LANG(C++)

# Overarching version of all libraries (used for installation dir)
AC_SUBST(linuxlist_release, linuxlist-1.0)

# Checks for programs.
AC_PROG_CXX

# Checks for libraries.
AC_PROG_LIBTOOL

#AC_DEFINE([HAVE_CURSES_H], [1], [Is ncurses available])

AX_WITH_CURSES
case "$ax_cv_ncurses" in "no")
	AC_ERROR([ncurses not found])
esac
case "$ax_cv_ncursesw" in "no")
	AC_MSG_WARN([ncurses does not support wide-characters.])
	AC_MSG_WARN([This means that UTF-8 cannot be used to see ASCII control characters.])
esac
AC_SUBST(CURSES_LIB)

AM_ICONV

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_OUTPUT(Makefile src/Makefile doc/Makefile)
