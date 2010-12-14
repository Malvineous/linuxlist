/**
 * @file   HexView.cpp
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

#include "HexView.hpp"

/// Number of chars wide each num is (e.g. 9-bit nums are three chars wide)
#define CALC_HEXCELL_WIDTH ((this->bitWidth + 3) / 4)

HexView::HexView(std::string strFilename, camoto::iostream_sptr file,
	std::fstream::off_type iFileSize, IConsole *pConsole)
	throw () :
		strFilename(strFilename),
		file(file, camoto::bitstream::littleEndian),
		pConsole(pConsole),
		bStatusAlertVisible(true), // trigger an update when next set
		iLineWidth(16),
		iLineAlloc(16),
		pLineBuffer(NULL),
		iOffset(0),
		iFileSize(iFileSize),
		bitWidth(8),
		intraByteOffset(0)
{
	// Draw the first page of data
	this->pConsole->setStatusBar(SB_TOP, SB_LEFT, strFilename);
	this->statusAlert(NULL); // draw the bottom status bar with no alert
	//this->pConsole->setStatusBar(SB_BOTTOM, SB_LEFT, " Command>  *** End of file ***");

	this->pLineBuffer = new int[this->iLineWidth];

	this->redrawScreen();
	this->pConsole->update();
}

HexView::~HexView()
	throw ()
{
	assert(this->pLineBuffer != NULL);
	delete[] this->pLineBuffer;
}

bool HexView::processKey(Key c)
	throw ()
{
	int iWidth, iHeight;
	this->pConsole->getContentDims(&iWidth, &iHeight);
	this->statusAlert(NULL);
	switch (c) {
		case 'q': return false;
		case '-': this->adjustLineWidth(-1); break;
		case '+': this->adjustLineWidth(+1); break;
		case 'b': this->setBitWidth(max(1, this->bitWidth - 1)); break;
		case 'B': this->setBitWidth(min(sizeof(int)*8, this->bitWidth + 1)); break;
		case 's': this->setIntraByteOffset(-1); break;
		case 'S': this->setIntraByteOffset(1); break;
		case 'e': this->file.changeEndian(camoto::bitstream::littleEndian); this->redrawScreen(); break;
		case 'E': this->file.changeEndian(camoto::bitstream::bigEndian); this->redrawScreen(); break;
		case Key_Up: this->scrollRel(-this->iLineWidth); break;
		case Key_Down: this->scrollRel(this->iLineWidth); break;
		case Key_Left: this->scrollRel(-1); break;
		case Key_Right: this->scrollRel(1); break;
		case Key_PageUp: this->scrollRel(-this->iLineWidth*iHeight); break;
		case Key_PageDown: this->scrollRel(this->iLineWidth*iHeight); break;
		case Key_Home: this->scrollAbs(0); break;
		case Key_End: {
			int fileSizeInCells = (this->iFileSize * 8 / this->bitWidth);
			int iLastLineLen = fileSizeInCells % this->iLineWidth;
			if (iLastLineLen == 0) iLastLineLen = this->iLineWidth;
			this->scrollAbs(fileSizeInCells - iLastLineLen -
				(iHeight - 1) * this->iLineWidth);
			break;
		}
		default: break;
	}
	this->pConsole->update();
	return true; // true == keep going (don't quit)
}

// Set an alert message on the status bar, or remove one if cMsg == NULL
void HexView::statusAlert(const char *cMsg)
	throw ()
{
	// If there's no status message and a blank has been requested, do nothing.
	if ((!cMsg) && (!bStatusAlertVisible)) return;

	// Blank out the bottom statusbar to hide the old status message
	this->pConsole->clearStatusBar(SB_BOTTOM);

	// Reset the right-hand side after we've blanked it
	this->pConsole->setStatusBar(SB_BOTTOM, SB_RIGHT, "F1=help");

	if (cMsg) {
		this->pConsole->setStatusBar(SB_BOTTOM, SB_LEFT, std::string("Command>  *** ") + cMsg + " *** ");
		bStatusAlertVisible = true;
	} else {
		this->pConsole->setStatusBar(SB_BOTTOM, SB_LEFT, "Command> ");
		bStatusAlertVisible = false;
	}
	return;
}

void HexView::scrollAbs(unsigned long iNewOffset)
	throw ()
{
	this->scrollRel(iNewOffset - this->iOffset);
	return;
}

void HexView::scrollRel(int iDelta)
	throw ()
{
	if (iDelta == 0) return; // e.g. pressing Home twice

	int iWidth, iHeight;
	this->pConsole->getContentDims(&iWidth, &iHeight);
	int iScreenSize = iHeight * this->iLineWidth;

	//
	// Check and enforce scrolling limits
	//

	// If the user wants to scroll up, towards the start of the file...
	if (iDelta < 0) {

		// If the scroll operation will be cropped to the start of the file,
		// display a notice to the user.
		if (-iDelta > this->iOffset) {
			this->statusAlert("Top of file");
			// Crop the scroll so that it only goes back to the start, not past it
			iDelta = -this->iOffset;
		}

		// But prevent them from actually scrolling past the start.
		if (this->iOffset == 0) return;

	} else {
		// The user wants to scroll down, towards the end of the file.

		// If the scroll operation will show past the end of the file,
		// display a notice to the user.
		if (this->iOffset + iScreenSize + iDelta > this->iFileSize) {
			this->statusAlert("End of file");
		}

		// But prevent them from actually scrolling past the last byte.
		if (this->iOffset >= this->iFileSize - 1) return;

		// Crop the scroll so that it only goes up to the last byte, not past it
		if (this->iOffset + iDelta > this->iFileSize) {
			if (iDelta % this->iLineWidth == 0) {
				// The user is scrolling by lines, so crop at the line level
				unsigned long iMaxBytes = this->iFileSize - this->iOffset;
				iDelta = iMaxBytes - (iMaxBytes % this->iLineWidth);
				if (iDelta == 0) return; // can't scroll down by a whole line without going past EOF
			} else {
				// The user is scrolling by chars, so allow them to go right up to
				// the last byte.
				iDelta = (this->iFileSize - 1) - this->iOffset;
			}
		}

	}
	assert(iDelta != 0);

	// If we're here, then iDelta is within limits and won't scroll too far
	// in either direction.

	// See if we're scrolling by part of a line
	int iChars = iDelta % this->iLineWidth;
	if (iChars) {
		// We're scrolling by a partial line, so the whole screen will need
		// to be redrawn regardless of how far we scroll.
		this->iOffset += iDelta;
		this->redrawLines(0, iHeight);
		return;
	}

	// No, we're only scrolling by a multiple of exact lines.
	int iLines = iDelta / this->iLineWidth;
	assert(iLines != 0);
	if (abs(iLines) > iHeight) {
		// But we're scrolling by more than a screenful, so we'll need to
		// redraw the whole screen anyway.
		this->iOffset += iDelta;
		this->redrawLines(0, iHeight);
		return;
	}

	// If we're here, then we're scrolling by only a handful of lines, so
	// try to do it efficiently.
	this->pConsole->scrollContent(0, iLines);
	this->iOffset += iDelta;
	if (iLines < 0) {
		this->redrawLines(0, -iLines);
	} else {
		this->redrawLines(iHeight-iLines, iHeight);
	}

	return;
}

void HexView::redrawLines(int iTop, int iBottom)
	throw ()
{
	int y = iTop;
	std::fstream::off_type iCurOffset = this->iOffset + iTop * this->iLineWidth;
	file.clear(); // clear any errors (e.g. reaching EOF previously)
	file.seek(iCurOffset * this->bitWidth + this->intraByteOffset, std::ios::beg);
	if (iCurOffset < this->iFileSize) {
		for (; y < iBottom; y++) {

			// Don't read past the end of the file
			int iRead;
			int bytesPerLine = (this->iLineWidth * this->bitWidth) >> 3;
			for (iRead = 0; iRead < this->iLineWidth; iRead++) {
				if (!file.read(this->bitWidth, &this->pLineBuffer[iRead])) break;
			}

			this->drawLine(y, iOffset + y * bytesPerLine,
				this->pLineBuffer, iRead);
			if (iRead < this->iLineWidth) {
				y++;
				break; // EOF
			}
		}
	}
	// Blank out any leftover lines
	for (; y < iBottom; y++) {
		this->pConsole->gotoxy(0, y);
		//this->pConsole->putstr("blankline");
		this->pConsole->eraseToEOL();
	}
	return;
}

void HexView::drawLine(int iLine, unsigned long iOffset, const int *pData, int iLen)
	throw ()
{
	this->pConsole->gotoxy(0, iLine);
	std::ostringstream osHex;
	std::ostringstream osBin;
	//ostr.setf(std::ios::hex, std::ios::basefield);

	// Offset display (left)
	osHex << std::hex << std::setiosflags(std::ios_base::uppercase)
		<< std::setfill('0') << std::setw(8)
		<< iOffset << " ";

	// Number of chars wide each num is (e.g. 9-bit nums are three chars wide)
	int cellNumberWidth = CALC_HEXCELL_WIDTH;

	const int *pb = pData;
	for (int i = 0; i < iLen; pb++, i++) {

		// Hex display (middle)

		osHex << ((i && (i % 8 == 0)) ? "  " : " ");
		//else if (i % 4 == 0) ostr << ' ';
		osHex << std::setfill('0') << std::setw(cellNumberWidth) << (int)*pb;

		// Binary display (right)

		if (*pb < 32) {
			// This is a control character, so it needs to be manually converted
			if (*pb == 0) osBin << ' ';
			else osBin << (char)*pb;
		} else if (*pb < 256) {
			osBin << (char)*pb;
		} else {
			osBin << '.'; // TODO: some non-ASCII char
		}

	}
	// Pad out any data at the end of the file
	for (int i = iLen; i < this->iLineWidth; i++) {
		osHex << ((i && (i % 8 == 0)) ? "  " : " ");
		for (int j = 0; j < cellNumberWidth; j++) osHex << ' ';
		osBin << ' ';
	}
	osHex << "  " << osBin.str();// << 'x';
	this->pConsole->putstr(osHex.str().c_str());
	this->pConsole->eraseToEOL();
	//this->pConsole->putstr("\n12345678901234567890123456789012345678901234567890123456789012345678901234567890");
	//this->pConsole->putstr("test");
	return;
}

void HexView::setBitWidth(int newWidth)
	throw ()
{
	// Since the first bit on the screen should stay the same after
	// this change, we need to adjust offsets.
	int bitOffset = this->iOffset * this->bitWidth + this->intraByteOffset;

	this->bitWidth = newWidth;
	this->intraByteOffset = bitOffset % this->bitWidth;
	this->iOffset = bitOffset / this->bitWidth;

	assert(bitOffset == this->iOffset * this->bitWidth + this->intraByteOffset);

	// Make sure the data hasn't gone off the edge of the screen
	this->adjustLineWidth(0);

	this->redrawScreen();
	return;
}

void HexView::setIntraByteOffset(int delta)
	throw ()
{
	if ((this->intraByteOffset + delta) >= this->bitWidth) {
		this->iOffset++;
	} else if ((this->intraByteOffset + delta) < 0) {
		if (this->iOffset > 0) this->iOffset--;
		else return; // can't go past start of file
	}
	this->intraByteOffset = (this->intraByteOffset + this->bitWidth + delta) % this->bitWidth;
	this->redrawScreen();
	return;
}

void HexView::redrawScreen()
	throw ()
{
	int iWidth, iHeight;
	this->pConsole->getContentDims(&iWidth, &iHeight);

	std::ostringstream ss;
	ss << "Offset: " << this->iOffset << '+' << this->intraByteOffset
		<< "b  Cell size: " << this->bitWidth
		<< "b/";
	if (this->file.getEndian() == camoto::bitstream::littleEndian) {
		ss << "LE";
	} else {
		ss << "BE";
	}
	ss << "  Width: " << this->iLineWidth;
	this->pConsole->setStatusBar(SB_TOP, SB_RIGHT, ss.str());

	this->redrawLines(0, iHeight);
	return;
}

void HexView::adjustLineWidth(int delta)
	throw ()
{
	int iWidth, iHeight;
	this->pConsole->getContentDims(&iWidth, &iHeight);

	int newWidth = this->iLineWidth + delta;
	if (newWidth < 1) newWidth = 1;
	// Width required to display a single byte of data (hex digits + binary), e.g.
	// 4 chars per byte by default (hex, hex, space and binary)
	int byteWidth = 2 + CALC_HEXCELL_WIDTH; // 2 == binary + space
	int maxWidth = (iWidth - 11) / byteWidth;
	maxWidth -= 0.5 + maxWidth / (8 * 4); // plus an extra space every 8 chars (have to *4 to cancel out the /4 above)
	if (newWidth > maxWidth) newWidth = maxWidth;
	if (this->iLineWidth != newWidth) {
		this->iLineWidth = newWidth;
		if (this->iLineWidth > this->iLineAlloc) {
			// Enlarge the buffer
			delete[] this->pLineBuffer;
			this->pLineBuffer = new int[this->iLineWidth];
			this->iLineAlloc = this->iLineWidth;
		}
		this->redrawScreen();
	}
	return;
}
