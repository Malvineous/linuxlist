/**
 * @file   FileView.hpp
 * @brief  IView interface for a file viewer.
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

#ifndef FILEVIEW_HPP_
#define FILEVIEW_HPP_

#include <fstream> // for off_type
#include <sstream>
#include <string>
#include <camoto/bitstream.hpp>
#include "IView.hpp"
#include "IConsole.hpp"

/// Common implementation for a file viewer.
/**
 * This class implements things that are used for all types of file viewers
 * (text and hex mode.)  Things like status alerts, etc.
 */
class FileView: virtual public IView
{
	protected:
		std::string strFilename;  ///< Filename of open file
		camoto::bitstream file;   ///< Bitstream for reading data from file
		IConsole *pConsole;       ///< Console used for drawing content
		bool bStatusAlertVisible; ///< true if an alert is visible in the status bar
		int bitWidth;             ///< Number of bits in each char/cell
		int intraByteOffset;      ///< Bit-level seek offset within cell (0..bitWidth-1)

		camoto::stream::pos iOffset; ///< Offset into file of first character in content window
		camoto::stream::pos iFileSize; ///< Length of input stream

	public:
		/// Constructor
		/**
		 * @param strFilename
		 *   Filename of data being displayed.  Shown in header.
		 *
		 * @param data
		 *   Data to display.
		 *
		 * @param iFileSize
		 *   Length of data being displayed in bytes.
		 *
		 * @param pConsole
		 *   Output console where data is drawn.
		 */
		FileView(std::string strFilename, camoto::stream::inout_sptr data,
			IConsole *pConsole)
			throw ();

		FileView(const FileView& parent)
			throw ();

		/// Destructor
		~FileView()
			throw ();

		void init()
			throw ();

		/// Set an alert message on the status bar.
		/**
		 * @param cMsg
		 *   Message to show.  If NULL, message is removed.
		 */
		void statusAlert(const char *cMsg)
			throw ();

		/// Get the text to show in the header (file offset, etc.)
		virtual void generateHeader(std::ostringstream& ss)
			throw ();

		/// Update the top status bar with the current offset etc.
		void updateHeader()
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

};

#endif // FILEVIEW_HPP_
