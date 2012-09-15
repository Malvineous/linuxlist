/**
 * @file   XConsole.hpp
 * @brief  X11 implementation of IConsole.
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

#ifndef XCONSOLE_HPP_
#define XCONSOLE_HPP_

#include <config.h>

#include <stdint.h>

#ifndef USE_X11
#error This file should not be compiled without X-Windows!
#endif

#include <X11/Xlib.h>
#include "IConsole.hpp"

/// Console interface to an X-Windows window.
class XConsole: virtual public IConsole
{
	private:
		IViewPtr pView;     ///< Current view in use
		IViewPtr nextView;  ///< If non-NULL, next view to replace pView

		Display *display;   ///< X11 display
		Window win;         ///< Main window
		GC gc;              ///< GC for drawing on window
		Pixmap font;        ///< Font glyphs (one line, 256 chars wide)

		int fontWidth;      ///< Width of font char in pixels
		int fontHeight;     ///< Height of font char in pixels
		int cursorX;        ///< Current X position of text cursor
		int cursorY;        ///< Current Y position of text cursor
		bool cursorVisible; ///< Draw the text cursor on the display?
		uint8_t *text;      ///< Screen content
		uint8_t *changed;   ///< Screen content
		int screenWidth;    ///< Screen width in text cells, used for sizeof(text)
		int screenHeight;   ///< Screen height in text cells, used for sizeof(text)

#define PX_DOC_FG 0
#define PX_DOC_BG 1
#define PX_SB_FG  2
#define PX_SB_BG  3
#define PX_HL_FG  4
#define PX_HL_BG  5
#define PX_TOTAL  6
		unsigned long pixels[PX_TOTAL]; ///< X11 pixel values to use for colours

	public:
		XConsole(Display *display);
		~XConsole();

		void setView(IViewPtr pView);
		void mainLoop();
		void update(void);
		void clearStatusBar(SB_Y eY);
		void setStatusBar(SB_Y eY, SB_X eX, const std::string& strMessage);
		void gotoxy(int x, int y);
		void putstr(const std::string& strContent);
		void getContentDims(int *iWidth, int *iHeight);
		void scrollContent(int iX, int iY);
		void eraseToEOL(void);
		void cursor(bool visible);
		std::string getString(const std::string& strPrompt, int maxLen);
		void setColoursFromConfig();

	protected:
		/// Redraw the characters at the given text coordinates.
		/**
		 * After each cell is drawn, the entry for that cell in this->changed is
		 * set to zero.
		 *
		 * @param startX
		 *   X-coordinate of first cell to draw.
		 *
		 * @param startY
		 *   Y-coordinate of first cell to draw.
		 *
		 * @param endX
		 *   Draw up until (but not including) this X-coordinate.
		 *
		 * @param endY
		 *   Draw up until (but not including) this Y-coordinate.
		 *
		 * @param changedOnly
		 *   If true, only draw those cells in the area which have changed since
		 *   the last redraw (where this->changed != 0 for this cell's entry.)  If
		 *   false, all cells are redrawn regardless.
		 */
		void redrawCells(int startX, int startY, int endX, int endY, bool changedOnly);

		/// Write text at the given location.
		/**
		 * This writes anywhere on the screen, including on the status bars.
		 *
		 * @pre The text can overlap onto the next line, but it must not run
		 *   off the edge of the screen.
		 *
		 * @param x
		 *   X-coordinate where the first character will appear.
		 *
		 * @param y
		 *   Y-coordinate where the first character will appear.
		 *
		 * @param strContent
		 *   The text to draw.
		 */
		void writeText(int x, int y, const std::string& strContent);
};

#endif // XCONSOLE_HPP_
