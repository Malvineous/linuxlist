/**
 * @file   NCursesConsole.hpp
 * @brief  ncurses implementation of IConsole.
 *
 * Copyright (C) 2009-2016 Adam Nielsen <malvineous@shikadi.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NCURSESCONSOLE_HPP_
#define NCURSESCONSOLE_HPP_

#include <config.h>

#ifdef HAVE_NCURSESW
# ifdef HAVE_NCURSESW_H
#  include <ncursesw/ncurses.h>
# else
#  include <ncurses.h>
# endif
#else
# error This file should not be compiled without ncurses!
#endif

#ifndef HAVE_ICONV
#error This file should not be compiled without iconv!
#endif

#include <iconv.h>
#include "BaseConsole.hpp"

/// Console interface to a standard terminal using ncurses for control codes.
class NCursesConsole: virtual public BaseConsole
{
	private:
		// The status bars need to be separate windows, otherwise updating them
		// will overwrite the content window!
		WINDOW *winStatus[2]; ///< Status bars (0 == top, 1 == bottom)
		WINDOW *winContent;   ///< Main display area (between the status bars)
		bool cursorInWindow;  ///< false if the cursor is in the bottom status bar

		int maxLineLen;  ///< Maximum length of a single line (will expand as necessary)

		iconv_t cd;      ///< iconv handle

// Colour pairs
#define CLR_STATUSBAR 1
#define CLR_CONTENT 2
		attr_t iAttribute[3]; ///< Attributes (bold etc.) for matching colour pairs

	public:
		NCursesConsole(void);
		virtual ~NCursesConsole();

		void mainLoop();
		void update(void);
		void clearStatusBar(SB_Y eY);
		void setStatusBar(SB_Y eY, SB_X eX, const std::string& strMessage,
			int cursor);
		void gotoxy(int x, int y);
		void putstr(const std::string& strContent);
		void getContentDims(int *iWidth, int *iHeight);
		void scrollContent(int iX, int iY);
		void eraseToEOL(void);
		void cursor(bool visible);
		void setColoursFromConfig();
};

#endif // NCURSESCONSOLE_HPP_
