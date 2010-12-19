/**
 * @file   HexView.hpp
 * @brief  IView implementation for a hex editor view.
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

#ifndef HEXVIEW_HPP_
#define HEXVIEW_HPP_

#include <config.h>

#include <iomanip>
#include "FileView.hpp"

/// Hex editor view.
class HexView: public FileView
{
	int iLineWidth;           ///< Size of line shown to user, initially 16 chars
	int *pLineBuffer;         ///< Line buffer, initially 16 chars
	int iLineAlloc;           ///< Size of pLineBuffer in bytes (may be > iLineWidth)
	unsigned int cursorOffset;///< Cursor position (in bytes) relative to iOffset
	int hexEditOffset;        ///< Offset (in on-screen chars) within byte in hex edit mode

	enum EditMode {
		View,       ///< Viewing data (default)
		HexEdit,    ///< Editing hex values
		BinaryEdit, ///< Editing binary data
	};
	#define NUM_EDIT_MODES 3  ///< Number of entries in EditMode
	EditMode editMode; ///< Current editing mode

	public:
		HexView(std::string strFilename, camoto::iostream_sptr data,
			std::fstream::off_type iFileSize, IConsole *pConsole)
			throw ();

		HexView(const FileView& parent)
			throw ();

		~HexView()
			throw ();

		bool processKey(Key c)
			throw ();

		void redrawScreen()
			throw ();

		void generateHeader(std::ostringstream& ss)
			throw ();

		/// Scroll to an absolute offset.
		/**
		 * @param iNewOffset
		 *   Offset to of byte to appear at (0,0)
		 */
		void scrollAbs(unsigned long iNewOffset)
			throw ();

		/// Scroll by this number of chars.
		/**
		 * @param iDelta
		 *   Number of bytes to scroll.  -1 will scroll back one byte, +1 will
		 *   scroll forward one byte.  -this->iLineWidth will scroll up one line,
		 *   +this->iLineWidth will scroll down one line.
		 */
		void scrollRel(int iDelta)
			throw ();

		/// Redraw part of the screen.
		/**
		 * @param iTop
		 *   First line to redraw, 0 is first data line (just below top status bar)
		 *
		 * @param iBottom
		 *   Stop drawing at this line.  iBottom-1 is the actual last line drawn.
		 */
		void redrawLines(int iTop, int iBottom)
			throw ();

		/// Display the file data (starting at the current offset) in the content
		/// window.
		/**
		 * @param iLine
		 *   Line number (0 is first line)
		 *
		 * @param iOffset
		 *   Value to display in file-offset column.
		 *
		 * @param pData
		 *   Data to show
		 *
		 * @param iLen
		 *   Line length/length of pData
		 */
		void drawLine(int iLine, unsigned long iOffset, const int *pData, int iLen)
			throw ();

		/// Increase or decrease the line width.
		/**
		 * This adjusts how long each line of hex data is.
		 *
		 * @param delta
		 *   Amount of change, -1 will shorten the row by one byte, +1 will increase
		 *   it by one byte.
		 *
		 * @post If the value is within range, the screen is redrawn using the new
		 *   width.  If the value would have moved the width out of range, nothing
		 *   will be changed or redrawn.
		 */
		void adjustLineWidth(int delta)
			throw ();

		/// Cycle between edit modes.
		void cycleEditMode()
			throw ();

		/// Move the text cursor to its correct location.
		/**
		 * This function is used after this->cursorOffset has been changed, to move
		 * the cursor to the new location.
		 */
		void updateCursorPos()
			throw ();

		/// Show/hide text cursor.
		/**
		 * @param visible
		 *   true to show, false to hide.
		 *
		 * @note Returns cursor to last location before showing, and is a no-op
		 *   when in view mode (cursor is always hidden then)
		 */
		void showCursor(bool visible)
			throw ();

		/// Move the cursor position by the given amount.
		/**
		 * @param delta
		 *   Number of bytes to move the cursor.  -1 is back one byte,
		 *   +this->iLineWidth is down one line, etc.
		 *
		 * @note The value is clipped to the allowable range, however this function
		 *   may call the scroll functions, e.g. if the function is used to try to
		 *   move past the end of the screen, the cursor will stay put but more
		 *   data will scroll into place.
		 */
		void moveCursor(int delta)
			throw ();

		/// Write the given value into the file at the current cursor offset.
		/**
		 * @param byte
		 *   Byte to write.  Can be > 8-bits if the current cell width is wide.
		 *
		 * @post Underlying file has been changed.  Screen is not updated.
		 */
		void writeByteAtCursor(unsigned int byte)
			throw ();

		/// Prompt the user for an offset, then jump there.
		void gotoOffset()
			throw ();

};

#endif // HEXVIEW_HPP_
