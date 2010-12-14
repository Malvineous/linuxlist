bin_PROGRAMS = ll

ll_SOURCES = main.cpp
ll_SOURCES += NCursesConsole.cpp
ll_SOURCES += HexView.cpp

EXTRA_ll_SOURCES = cfg.hpp
EXTRA_ll_SOURCES += IConsole.hpp
EXTRA_ll_SOURCES += NCursesConsole.hpp
EXTRA_ll_SOURCES += IView.hpp
EXTRA_ll_SOURCES += HexView.hpp

# So config.h can be found
AM_CPPFLAGS = -I $(top_srcdir) $(libgamecommon_CFLAGS)

AM_LDFLAGS = $(CURSES_LIB) $(LIBICONV) $(libgamecommon_LIBS)
