/**
 * @file   NCursesConsole.hpp
 * @brief  ncurses implementation of IConsole.
 *
 * Copyright (C) 2009-2010 Adam Nielsen <malvineous@shikadi.net>
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

#ifndef HAVE_NCURSESW_H
#error This file should not be compiled without ncurses!
#endif
#ifndef HAVE_ICONV
#error This file should not be compiled without iconv!
#endif

#include <ncursesw/ncurses.h>
#include <iconv.h>
#include "IConsole.hpp"

/// Console interface to a standard terminal using ncurses for control codes.
class NCursesConsole: virtual public IConsole
{
	private:
		// The status bars need to be separate windows, otherwise updating them
		// will overwrite the content window!
		WINDOW *winStatus[2]; ///< Status bars (0 == top, 1 == bottom)
		WINDOW *winContent;   ///< Main display area (between the status bars)

		int maxLineLen;  ///< Maximum length of a single line (will expand as necessary)

		iconv_t cd;      ///< iconv handle

// Colour pairs
#define CLR_STATUSBAR 1
#define CLR_CONTENT 2
		attr_t iAttribute[3]; ///< Attributes (bold etc.) for matching colour pairs

	public:
		NCursesConsole(void)
			throw ();

		~NCursesConsole()
			throw ();

		void mainLoop(IView *pView)
			throw ();

		void update(void)
			throw ();

		void clearStatusBar(SB_Y eY)
			throw ();

		void setStatusBar(SB_Y eY, SB_X eX, const std::string& strMessage)
			throw ();

		void gotoxy(int x, int y)
			throw ();

		void putstr(const std::string& strContent)
			throw ();

		void getContentDims(int *iWidth, int *iHeight)
			throw ();

		void scrollContent(int iX, int iY)
			throw ();

		void eraseToEOL(void)
			throw ();

		void cursor(bool visible)
			throw ();

		std::string getString(const std::string& strPrompt, int maxLen)
			throw ();

};

#endif // NCURSESCONSOLE_HPP_
