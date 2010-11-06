bin_PROGRAMS = ll

ll_SOURCES = main.cpp

# So config.h can be found
AM_CPPFLAGS = -I $(top_srcdir) $(libgamecommon_CFLAGS)

AM_LDFLAGS = $(CURSES_LIB) $(LIBICONV) $(libgamecommon_LIBS)
