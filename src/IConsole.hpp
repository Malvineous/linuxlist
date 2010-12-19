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
#include "IView.hpp"

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
		/// Virtual destructor.
		virtual ~IConsole()
			throw ()
		{
		}

		/// Set the view that will be shown in this console.
		/**
		 * @param pView
		 *   IView to pass keypress events to.
		 */
		virtual void setView(IView *pView)
			throw () = 0;

		/// Main loop for reading keystrokes and passing actions to the view.
		/**
		 * @return On return the application will terminate.
		 */
		virtual void mainLoop()
			throw () = 0;

		/// Update the content area (not status bars) after changes have been made.
		virtual void update(void)
			throw () = 0;

		/// Blank out the text on the specified status bar.
		/**
		 * @note The change is not shown until the next call to update().
		 */
		virtual void clearStatusBar(SB_Y eY)
			throw () = 0;

		/// Set the content of the given status bar.
		/**
		 * @note The change is not shown until the next call to update().
		 *
		 * @param eY
		 *   Status bar to change (top/bottom)
		 *
		 * @param eX
		 *   Area of the status bar to change (left/mid/right)
		 *
		 * @param strMessage
		 *   Text to show.
		 */
		virtual void setStatusBar(SB_Y eY, SB_X eX, const std::string& strMessage)
			throw () = 0;

		//
		// Display routines
		//

		/// Move the cursor to the given location.
		/**
		 * @param x
		 *   New X-coordinate, 0 is left-most.
		 *
		 * @param y
		 *   New Y-coordinate, 0 is top-most.
		 */
		virtual void gotoxy(int x, int y)
			throw () = 0;

		/// Write a string at the current location.
		/**
		 * @param strContent
		 *   String to write.
		 */
		virtual void putstr(const std::string& strContent)
			throw () = 0;

		/// Get the screen size.
		/**
		 * The values are in text cells, e.g. 80x25.
		 *
		 * @param iWidth
		 *   Pointer to where the width value will be written.
		 *
		 * @param iHeight
		 *   Pointer to where the height value will be written.
		 */
		virtual void getContentDims(int *iWidth, int *iHeight)
			throw () = 0;

		/// Scroll the content area (between the status bars)
		/**
		 * @param iX
		 *   Number of cells to scroll horizontally.
		 *
		 * @param iY
		 *   Number of cells to scroll vertically.
		 */
		virtual void scrollContent(int iX, int iY)
			throw () = 0;

		/// Erase from the current location to the end of the line.
		virtual void eraseToEOL()
			throw () = 0;

		/// Show or hide the text cursor.
		/**
		 * @param visible
		 *   true to show the cursor at the current X,Y location, false to hide it.
		 */
		virtual void cursor(bool visible)
			throw () = 0;

		/// Read in a string from the user.
		/**
		 * A message is displayed on the status bar, followed by the characters the
		 * user is typing, up to a maximum of maxLen chars.
		 *
		 * @param strPrompt
		 *   Text to display in status bar before user input appears.
		 *
		 * @param maxLen
		 *   Maximum length of the string, not including the terminating NULL.
		 *
		 * @return The string entered by the user, or an empty string if the user
		 *   pressed Escape to abort.
		 */
		virtual std::string getString(const std::string& strPrompt, int maxLen)
			throw () = 0;

};

#endif // ICONSOLE_HPP_
