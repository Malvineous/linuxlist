/**
 * @file   TextView.hpp
 * @brief  IView implementation for a text view.
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

#ifndef TEXTVIEW_HPP_
#define TEXTVIEW_HPP_

#include <config.h>

#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <camoto/bitstream.hpp>
#include "IConsole.hpp"
#include "FileView.hpp"

/// Text view.
class TextView: virtual public FileView
{
	uint8_t *pLineBuffer;     ///< Line buffer, initially 80 chars
	int iLineAlloc;           ///< Size of pLineBuffer in bytes (may be > iLineWidth)

	int line;                 ///< Current line at top of screen, 0 == first line
	std::vector<int> linePos; ///< List of offsets where each line begins
	bool cacheComplete;       ///< True if linePos covers the entire file


	public:
		TextView(std::string strFilename, camoto::iostream_sptr file,
			std::fstream::off_type iFileSize, IConsole *pConsole)
			throw ();

		TextView(const FileView& parent)
			throw ();

		~TextView()
			throw ();

		bool processKey(Key c)
			throw ();

		void redrawScreen()
			throw ();

		void generateHeader(std::ostringstream& ss)
			throw ();

		void setBitWidth(int newWidth)
			throw ();

		void setIntraByteOffset(int delta)
			throw ();

		/// Scroll vertically by this number of lines.
		/**
		 * @param iDelta
		 *   Number of lines to scroll.  -1 will scroll back one byte, +1 will
		 *   scroll forward one byte.  -this->iLineWidth will scroll up one line,
		 *   +this->iLineWidth will scroll down one line.
		 */
		void scrollLines(int iDelta)
			throw ();

		/// Redraw part of the screen.
		/**
		 * @param iTop
		 *   First line to redraw, 0 is first data line (just below top status bar)
		 *
		 * @param iBottom
		 *   Stop drawing at this line.  iBottom-1 is the actual last line drawn.
		 *
		 * @param width
		 *   Current console width.
		 */
		void redrawLines(int iTop, int iBottom, int width)
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

		/// Populate linePos with offsets of each line from 0 to maxLine.
		/**
		 * @param maxLine
		 *   Line to populate up until.  Actual population may be less if maxLine
		 *   is past EOF.
		 *
		 * @param width
		 *   Width of output console (for wrapping long lines)
		 */
		void cacheLines(int maxLine, int width)
			throw ();

};

#endif // TEXTVIEW_HPP_
