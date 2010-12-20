bin_PROGRAMS = ll

ll_SOURCES = main.cpp

ll_SOURCES += NCursesConsole.cpp

ll_SOURCES += XConsole.cpp
ll_SOURCES += font.cpp

ll_SOURCES += FileView.cpp
ll_SOURCES += HexView.cpp
ll_SOURCES += TextView.cpp
ll_SOURCES += HelpView.cpp

EXTRA_ll_SOURCES = cfg.hpp
EXTRA_ll_SOURCES += IConsole.hpp

EXTRA_ll_SOURCES += NCursesConsole.hpp

EXTRA_ll_SOURCES += XConsole.hpp
EXTRA_ll_SOURCES += font.hpp

EXTRA_ll_SOURCES += IView.hpp
EXTRA_ll_SOURCES += FileView.hpp
EXTRA_ll_SOURCES += HexView.hpp
EXTRA_ll_SOURCES += TextView.hpp
EXTRA_ll_SOURCES += HelpView.hpp

# So config.h can be found
AM_CPPFLAGS = -I $(top_srcdir) $(libgamecommon_CFLAGS)

AM_LDFLAGS = $(CURSES_LIB) $(LIBICONV) $(libgamecommon_LIBS)
