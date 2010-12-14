/**
 * @file   IConsole.hpp
 * @brief  Console interface class.
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

#ifndef ICONSOLE_HPP_
#define ICONSOLE_HPP_

#include <string>

/// Which status bar to use.
enum SB_Y {
	SB_TOP = 0,
	SB_BOTTOM = 1
};

/// Position of text on the status bars.
enum SB_X {
	SB_LEFT,
	SB_CENTRE,
	SB_RIGHT
};

class IConsole
{
	public:
		virtual ~IConsole()
			throw ()
		{
		}

		// Update the content area (not status bars) after changes have been made
		virtual void update(void)
			throw () = 0;

		/*// Same as update but only affects the status bars
		virtual void updateStatusBars(void)
			throw () = 0;*/

		// Blank out the text on the specified status bar.  Not shown until update()
		virtual void clearStatusBar(SB_Y eY)
			throw () = 0;

		// Set the content of the top status bar.  Not shown until next update().
		virtual void setStatusBar(SB_Y eY, SB_X eX, const std::string& strMessage)
			throw () = 0;

		//
		// Display routines
		//
		virtual void gotoxy(int x, int y) throw () = 0;
		virtual void putstr(const std::string& strContent) throw () = 0;
		virtual void getContentDims(int *iWidth, int *iHeight) throw () = 0;
		virtual void scrollContent(int iX, int iY) throw () = 0;
		virtual void eraseToEOL(void) throw () = 0; // erase from cursor pos to end of line

};

#endif // ICONSOLE_HPP_
