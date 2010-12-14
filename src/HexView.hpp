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

#include <fstream>
#include <sstream>
#include <iomanip>
#include <camoto/bitstream.hpp>
#include "IConsole.hpp"
#include "IView.hpp"

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))

/// Hex editor view.
class HexView: virtual public IView
{
	std::string strFilename;
	camoto::bitstream file;
	IConsole *pConsole;
	bool bStatusAlertVisible;
	int iLineWidth;           ///< Size of line shown to user, initially 16 chars
	int *pLineBuffer;         ///< Line buffer, initially 16 chars
	int iLineAlloc;           ///< Size of pLineBuffer in bytes (may be > iLineWidth)
	int bitWidth;             ///< Number of bits in each char/cell
	int intraByteOffset;      ///< Bit-level seek offset within cell (0..bitWidth-1)

	std::fstream::off_type iOffset; // Offset into file of first character in content window
	std::fstream::off_type iFileSize;

	public:
		HexView(std::string strFilename, camoto::iostream_sptr file,
			std::fstream::off_type iFileSize, IConsole *pConsole)
			throw ();

		~HexView()
			throw ();

		bool processKey(Key c)
			throw ();

		// Set an alert message on the status bar, or remove one if cMsg == NULL
		void statusAlert(const char *cMsg)
			throw ();

		void scrollAbs(unsigned long iNewOffset)
			throw ();

		void scrollRel(int iDelta)
			throw ();

		void redrawLines(int iTop, int iBottom)
			throw ();

		// Display the file data (starting at the current offset) in the content
		// window.
		void drawLine(int iLine, unsigned long iOffset, const int *pData, int iLen)
			throw ();

		/// Set the size of each cell in bits.
		/**
		 * When this is set to eight, a normal byte-level view will be shown.
		 */
		void setBitWidth(int newWidth)
			throw ();

		/// Set the bit-level offset within the cell.
		/**
		 * Using eight bit bytes as an example, when this is set to zero bytes
		 * will be shown as normal.  When this is set to 1, the first *bit* in
		 * the file will be discarded and the following eight bits (last seven
		 * bits in the first input byte, and first bit in the second input byte)
		 * will appear as the first byte on the screen.
		 */
		void setIntraByteOffset(int delta)
			throw ();

		/// Regenerate the entire content on the display.
		/**
		 * This is normally only called after a change that affects the entire
		 * display, e.g. a change in the number of bits shown per byte.
		 */
		void redrawScreen()
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

};

#endif // HEXVIEW_HPP_
