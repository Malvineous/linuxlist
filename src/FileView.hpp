/**
 * @file   FileView.hpp
 * @brief  IView interface for a file viewer.
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

#ifndef FILEVIEW_HPP_
#define FILEVIEW_HPP_

#include <string>
#include <camoto/stream.hpp>
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
	public:
		/// Constructor.
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
			IConsole *pConsole);

		/// Copy constructor.
		/**
		 * This allows creating an identical view of a file.  Descendent classes
		 * can use this to switch between views of the same file at the same
		 * seek location (e.g. plain text and hex views.)
		 */
		FileView(const FileView& parent);

		virtual ~FileView();

		virtual void init();
		void updateTextEntry(const std::string& prompt, const std::string& text,
			unsigned int pos);
		void clearTextEntry();

		/// Set an alert message on the status bar.
		/**
		 * @param cMsg
		 *   Message to show.  If NULL, message is removed.
		 */
		void statusAlert(const char *cMsg);

		/// Get the text to show in the header (file offset, etc.)
		virtual void generateHeader(std::ostringstream& ss);

		/// Update the top status bar with the current offset etc.
		void updateHeader();

		/// Set the size of each cell in bits.
		/**
		 * When this is set to eight, a normal byte-level view will be shown.
		 *
		 * @param newWidth
		 *   Width of bits to show in each byte/cell.
		 */
		void setBitWidth(int newWidth);

		/// Set the bit-level offset within the cell.
		/**
		 * Using eight bit bytes as an example, when this is set to zero bytes
		 * will be shown as normal.  When this is set to 1, the first *bit* in
		 * the file will be discarded and the following eight bits (last seven
		 * bits in the first input byte, and first bit in the second input byte)
		 * will appear as the first byte on the screen.
		 *
		 * @pre delta must be less than the value passed to setBitWidth().
		 *
		 * @param delta
		 *   Number of bits to seek/offset.
		 */
		void setIntraByteOffset(int delta);

	protected:
		std::string strFilename;  ///< Filename of open file
		bool readonly;            ///< Is the file open in read-only mode?
		camoto::bitstream file;   ///< Bitstream for reading data from file
		IConsole *pConsole;       ///< Console used for drawing content
		bool bStatusAlertVisible; ///< true if an alert is visible in the status bar
		int bitWidth;             ///< Number of bits in each char/cell
		int intraByteOffset;      ///< Bit-level seek offset within cell (0..bitWidth-1)

		camoto::stream::pos iOffset; ///< Offset into file of first character in content window
		camoto::stream::pos iFileSize; ///< Length of input stream
};

#endif // FILEVIEW_HPP_
