#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([linuxlist], [0.2], [malvineous@shikadi.net])
AM_INIT_AUTOMAKE([linuxlist], [0.2])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_LANG(C++)

AC_PROG_CXX
AC_PROG_LIBTOOL

AC_PATH_X
AM_CONDITIONAL([HAVE_X], [test \! "x$no_x" = "xyes"])
AM_COND_IF([HAVE_X], [
	AC_DEFINE([USE_X11], [1], [Define to use X11 hotkeys])
	AC_SUBST([X_LIBS], ["$x_libraries -lX11"])
	status_x="enabled"
], [
	status_x="disabled"
])
#AC_CHECK_LIB([X11], [XOpenDisplay])

AX_BOOST_BASE([1.46], [], [AC_MSG_ERROR([Could not find Boost])])
PKG_CHECK_MODULES([libgamecommon], [libgamecommon < 2.0])

AX_WITH_CURSES
AM_CONDITIONAL([HAVE_NCURSES], [test "x$ax_cv_ncurses" = "xyes"])

AM_COND_IF([HAVE_NCURSES], [
	if test "x$ax_cv_ncursesw" = "xno";
	then
		AC_MSG_WARN([ncurses does not support wide-characters.])
		AC_MSG_WARN([This means that UTF-8 cannot be used to see ASCII control characters.])
		status_ncurses="enabled (non-wide)"
	else
		status_ncurses="enabled"
	fi
], [
	status_ncurses="disabled"
])

AC_SUBST([CURSES_LIB])

AM_ICONV

AM_SILENT_RULES([yes])

AC_OUTPUT(Makefile src/Makefile doc/Makefile)

echo
echo "Platform availability summary:"
echo "  X-Windows:   $status_x"
echo "  ncurses:     $status_ncurses"
