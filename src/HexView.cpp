/**
 * @file   HexView.cpp
 * @brief  IView implementation for a hex editor view.
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

#include <cassert>
#include <iomanip>
#include "HexView.hpp"
#include "TextView.hpp"
#include "HelpView.hpp"
#include "cfg.hpp"

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))

/// Number of chars wide each num is (e.g. 9-bit nums are three chars wide)
#define CALC_HEXCELL_WIDTH ((this->bitWidth + 3) / 4)

HexView::HexView(std::string strFilename,
	std::shared_ptr<camoto::stream::inout> data, IConsole *pConsole)
	:	FileView(strFilename, data, pConsole),
		iLineWidth(16),
		iLineAlloc(16),
		pLineBuffer(NULL),
		cursorOffset(0),
		editMode(View),
		hexEditOffset(0)
{
	this->pLineBuffer = new unsigned int[this->iLineAlloc];
}

HexView::HexView(const FileView& parent)
	:	FileView(parent),
		iLineWidth(16),
		iLineAlloc(16),
		pLineBuffer(NULL),
		cursorOffset(0),
		editMode(View),
		hexEditOffset(0)
{
	this->pLineBuffer = new unsigned int[this->iLineAlloc];
}

HexView::~HexView()
{
	this->file.flush();
	assert(this->pLineBuffer != NULL);
	delete[] this->pLineBuffer;
}

bool HexView::processKey(Key c)
{
	int iWidth, iHeight;
	this->pConsole->getContentDims(&iWidth, &iHeight);

	// Hide any active status message on any keypress
	this->statusAlert(NULL);

	// Global keys, always active
	switch (c) {
		case Key_None: // ignore
			return true;
		case Key_Esc:
		case Key_F10:
			return false;
		case Key_Tab: this->cycleEditMode(); break;
		case Key_PageUp: this->scrollRel(-this->iLineWidth*iHeight); break;
		case Key_PageDown: this->scrollRel(this->iLineWidth*iHeight); break;
		case CTRL('L'): this->redrawScreen(); break;
		case Key_F1: {
			IViewPtr newView(new HelpView(this->pConsole));
			this->pConsole->pushView(newView);
			break;
		}
	}

	// Keys for both edit views
	if (this->editMode != View) {
		switch (c) {
			case Key_Up:    this->moveCursor(-this->iLineWidth); break;
			case Key_Down:  this->moveCursor(this->iLineWidth); break;
			case Key_Left:  this->moveCursor(-1); break;
			case Key_Right: this->moveCursor(1); break;
			case Key_Home:  this->moveCursor(-this->cursorOffset); break;
			case Key_End:   this->moveCursor(this->iLineWidth * iHeight - this->cursorOffset - 1); break;
		}
	}

	// Mode-specific keys
	switch (this->editMode) {
		case HexEdit: {
			int val = -1;
			if ((c >= '0') && (c <= '9')) val = c - '0';
			else if ((c >= 'a') && (c <= 'f')) val = c - 'a' + 10;
			else if ((c >= 'A') && (c <= 'F')) val = c - 'A' + 10;

			if (val >= 0) {
				// Write this keypress into the data
				int y = this->cursorOffset / this->iLineWidth;
				this->writeByteAtCursor(val);
				this->redrawLines(y, y+1);
			}
			break;
		}
		case BinaryEdit:
			if (c < 256) {
				// Write this keypress into the data
				int y = this->cursorOffset / this->iLineWidth;
				this->writeByteAtCursor(c);
				this->redrawLines(y, y+1);
			}
			break;
		case View:
			switch (c) {
				case 'q': return false;
				case '-': this->adjustLineWidth(-1); break;
				case '+': this->adjustLineWidth(+1); break;
				case 'b':
					this->setBitWidth(max(1, this->bitWidth - 1));

					// Make sure the data hasn't gone off the edge of the screen
					this->adjustLineWidth(0);

					this->redrawScreen();
					break;
				case 'B':
					this->setBitWidth(min(sizeof(int)*8, this->bitWidth + 1));

					// Make sure the data hasn't gone off the edge of the screen
					this->adjustLineWidth(0);

					this->redrawScreen();
					break;
				case 's': this->setIntraByteOffset(-1); break;
				case 'S': this->setIntraByteOffset(1); break;
				case 'e': this->file.changeEndian(camoto::bitstream::littleEndian); this->redrawScreen(); break;
				case 'E': this->file.changeEndian(camoto::bitstream::bigEndian); this->redrawScreen(); break;
				case 'g': this->gotoOffset(); break;
				case ALT('h'): {
					this->file.flush();
					IViewPtr newView(new TextView(*this));
					this->pConsole->setView(newView);
					::cfg.view = View_Text;
					break;
				}
				case Key_Up: this->scrollRel(-this->iLineWidth); break;
				case Key_Down: this->scrollRel(this->iLineWidth); break;
				case Key_Left: this->scrollRel(-1); break;
				case Key_Right: this->scrollRel(1); break;
				case Key_Home: this->scrollAbs(0); break;
				case Key_End: {
					unsigned long sizeInCells = (this->iFileSize << 3) / this->bitWidth;
					int iLastLineLen = sizeInCells % this->iLineWidth;
					if (iLastLineLen == 0) iLastLineLen = this->iLineWidth;
					this->scrollAbs(sizeInCells - iLastLineLen -
						(iHeight - 2) * this->iLineWidth);
					break;
				}
				default: break;
			}
			break;
	} // switch (editMode)
	this->pConsole->update();
	return true; // true == keep going (don't quit)
}

void HexView::redrawScreen()
{
	int iWidth, iHeight;
	this->pConsole->getContentDims(&iWidth, &iHeight);
	this->showCursor(false);

	this->updateHeader();
	this->redrawLines(0, iHeight);

	this->showCursor(true);
	return;
}

void HexView::generateHeader(std::ostringstream& ss)
{
	this->FileView::generateHeader(ss);
	// Append our own text onto the end
	ss << "  Width: " << this->iLineWidth;
	return;
}

void HexView::scrollAbs(camoto::stream::pos iNewOffset)
{
	this->scrollRel(iNewOffset - this->iOffset);
	return;
}

void HexView::scrollRel(camoto::stream::delta iDelta)
{
	if (iDelta == 0) return; // e.g. pressing Home twice

	int iWidth, iHeight;
	this->pConsole->getContentDims(&iWidth, &iHeight);
	int iScreenSize = iHeight * this->iLineWidth;

	//
	// Check and enforce scrolling limits
	//

	// Convert the file size from bytes into whatever bitwidth we're currently using
	camoto::stream::pos sizeInCells = (this->iFileSize << 3) / this->bitWidth;

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

		// Crop the scroll so that it only goes up to the last byte, not past it
		if (this->iOffset + iDelta >= sizeInCells) {
			if (iDelta % this->iLineWidth == 0) {
				// The user is scrolling by lines, so crop at the line level
				unsigned long iMaxBytes = sizeInCells - this->iOffset
					- (this->iLineWidth - (this->iOffset % this->iLineWidth));
				iDelta = iMaxBytes - (iMaxBytes % this->iLineWidth);
				if (iDelta == 0) return; // can't scroll down by a whole line without going past EOF
			} else {
				// The user is scrolling by chars, so allow them to go right up to
				// the last byte.
				iDelta = (sizeInCells - 1) - this->iOffset;
			}
		}

	}

	// If we are past EOF, display a notice to the user.
	if (this->iOffset + iScreenSize + iDelta >= sizeInCells) {
		this->statusAlert("End of file");
	}

	if (iDelta == 0) return;

	// If we're here, then iDelta is within limits and won't scroll too far
	// in either direction.

	// See if we're scrolling by part of a line
	int iChars = iDelta % this->iLineWidth;
	if (iChars) {
		// We're scrolling by a partial line, so the whole screen will need
		// to be redrawn regardless of how far we scroll.
		this->iOffset += iDelta;
		this->redrawLines(0, iHeight);
	} else {

		// No, we're only scrolling by a multiple of exact lines.
		int iLines = iDelta / this->iLineWidth;
		assert(iLines != 0);
		if (abs(iLines) >= iHeight) {
			// But we're scrolling by more than a screenful, so we'll need to
			// redraw the whole screen anyway.
			this->iOffset += iDelta;
			this->redrawLines(0, iHeight);
		} else {

			// If we're here, then we're scrolling by only a handful of lines, so
			// try to do it efficiently.
			this->pConsole->scrollContent(0, iLines);
			this->iOffset += iDelta;
			if (iLines < 0) {
				this->redrawLines(0, -iLines);
			} else {
				this->redrawLines(iHeight-iLines, iHeight);
			}
		}
	}

	this->updateHeader();

	// Make sure cursor stays within limits
	this->moveCursor(0);

	return;
}

void HexView::redrawLines(int iTop, int iBottom)
{
	this->showCursor(false);
	int y = iTop;
	camoto::stream::pos iCurOffset = this->iOffset + iTop * this->iLineWidth;
	file.seek(iCurOffset * this->bitWidth + this->intraByteOffset, camoto::stream::start);

	// Convert the offset from whatever bitwidth we're currently using into bytes
	unsigned long offsetInBytes = (iCurOffset * this->bitWidth) >> 3;

	// Draw the content, unless we're past EOF
	if (offsetInBytes <= this->iFileSize) {
		for (; y < iBottom; y++) {

			int iRead;
			for (iRead = 0; iRead < this->iLineWidth; iRead++) {
				if (!file.read(this->bitWidth, &this->pLineBuffer[iRead])) break;
			}

			this->drawLine(y, iCurOffset, this->pLineBuffer, iRead);
			if (iRead < this->iLineWidth) {
				y++;
				break; // EOF
			}
			iCurOffset += this->iLineWidth;
		}
	}
	// Blank out any leftover lines
	for (; y < iBottom; y++) {
		this->pConsole->gotoxy(0, y);
		this->pConsole->eraseToEOL();
	}
	this->showCursor(true);
	return;
}

void HexView::drawLine(int iLine, unsigned long iOffset,
	const unsigned int *pData, int iLen)
{
	this->pConsole->gotoxy(0, iLine);
	std::ostringstream osHex;
	std::ostringstream osBin;

	// Offset display (left)
	osHex << std::hex << std::setiosflags(std::ios_base::uppercase)
		<< std::setfill('0') << std::setw(8)
		<< iOffset << " ";

	// Number of chars wide each num is (e.g. 9-bit nums are three chars wide)
	int cellNumberWidth = CALC_HEXCELL_WIDTH;

	const unsigned int *pb = pData;
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
	return;
}

void HexView::adjustLineWidth(int delta)
{
	int iWidth, iHeight;
	this->pConsole->getContentDims(&iWidth, &iHeight);

	int newWidth = this->iLineWidth + delta;
	if (newWidth < 1) newWidth = 1;
/*
	// Width required to display a single byte of data (hex digits + binary), e.g.
	// 4 chars per byte by default (hex, hex, space and binary)
	int byteWidth = 2 + CALC_HEXCELL_WIDTH; // 2 == binary + space
	int maxWidth = (iWidth - 11) / byteWidth;
	maxWidth -= 0.5 + maxWidth / (8 * 4); // plus an extra space every 8 chars (have to *4 to cancel out the /4 above)
	if (newWidth > maxWidth) newWidth = maxWidth;
*/
	if (this->iLineWidth != newWidth) {
		this->iLineWidth = newWidth;
		if (this->iLineWidth > this->iLineAlloc) {
			// Enlarge the buffer
			delete[] this->pLineBuffer;
			this->pLineBuffer = new unsigned int[this->iLineWidth];
			this->iLineAlloc = this->iLineWidth;
		}
		this->redrawScreen();
	}
	return;
}

void HexView::cycleEditMode()
{
	this->editMode = (EditMode)((this->editMode + 1) % NUM_EDIT_MODES);
	if (this->editMode == View) {
		this->pConsole->cursor(false);
	} else {
		// Move the cursor back to the first hex byte, so it doesn't unexpectedly
		// end up in the middle of a byte.
		this->hexEditOffset = 0;

		// Move the cursor by a zero-amount, so it ends up in the correct spot.
		this->moveCursor(0);
		this->showCursor(true);
	}
	return;
}

void HexView::updateCursorPos()
{
	int cursorX = this->cursorOffset % this->iLineWidth;
	int cursorY = this->cursorOffset / this->iLineWidth;

	int byteWidth = 1 + CALC_HEXCELL_WIDTH; // 1 == space
	switch (this->editMode) {
		case HexEdit:
			cursorX = cursorX * byteWidth + cursorX / 8;
			cursorX += 10; // width of offset - TODO: update when offset is large
			cursorX += this->hexEditOffset;
			break;
		case BinaryEdit: {
			int hexWidth = byteWidth * this->iLineWidth + ((this->iLineWidth - 1) / 8);
			cursorX += 10 + hexWidth + 1;
			// 10 == width of offset - TODO: update when offset is large
			break;
		}
	}
	this->pConsole->gotoxy(cursorX, cursorY);
	return;
}

void HexView::showCursor(bool visible)
{
	if (this->editMode != View) {
		if (visible) {
			this->updateCursorPos();
			this->pConsole->cursor(true);
		} else {
			this->pConsole->cursor(false);
		}
	}
	return;
}

void HexView::moveCursor(int delta)
{
	int iWidth, iHeight;
	this->pConsole->getContentDims(&iWidth, &iHeight);
	int byteWidth = CALC_HEXCELL_WIDTH;

	if ((this->editMode == HexEdit) && ((delta == 1) || (delta == -1)))  {
		// We're editing the hex data and moving by only one char, so see if we can
		// move around within the byte first.
		if (
			(delta == -1) &&
			(this->hexEditOffset == 0) &&
			(this->cursorOffset == 0) &&
			(this->iOffset == 0)
		) {
			// Can't scroll past start of file, and don't want to jump to next hex
			// digit when at file start.
			return;
		}
		int orig = this->hexEditOffset;
		this->hexEditOffset = (this->hexEditOffset + delta + byteWidth) % byteWidth;

		// Don't do any scrolling if we stayed within the same byte.
		if (
			((delta == 1) && (orig < this->hexEditOffset))
			||
			((delta == -1) && (orig > this->hexEditOffset))
		) {
			this->updateCursorPos();
			return;
		}
	}

	// Make sure the cursor doesn't somehow end up past the end of the screen.
	// This can happen when shrinking the line width if the cursor is near the
	// end of the page, for example.
	if (this->cursorOffset >= this->iLineWidth * iHeight) {
		this->cursorOffset = this->iLineWidth * iHeight - 1;
	}

	int newOffset = this->cursorOffset + delta;
	if (newOffset < 0) {
		this->scrollRel(delta);

	} else if (newOffset >= this->iLineWidth * iHeight) {
		this->scrollRel(delta);

	} else {
		// Scroll amount remains on the same page

		// Make sure the user can't scroll past EOF
		if (this->iOffset + newOffset >= this->iFileSize) {
			int horiz = this->iOffset % this->iLineWidth;
			int eofpos = this->iFileSize - ((this->iFileSize - horiz) % this->iLineWidth);
			if (horiz == 0) eofpos -= this->iLineWidth;
			if (this->iOffset + this->cursorOffset < eofpos) {
				this->cursorOffset = this->iFileSize - this->iOffset - 1;
			} // else cursor is on last row, don't move it
		} else {
			this->cursorOffset = newOffset;
		}
	}

	// Further EOF check in case user used page down to skip way past EOF
	if (this->iOffset + this->cursorOffset >= this->iFileSize) {
		this->cursorOffset = this->iFileSize - this->iOffset - 1;
		this->hexEditOffset = byteWidth - 1;
	}

	this->updateCursorPos();
	return;
}

void HexView::writeByteAtCursor(unsigned int byte)
{
	if (this->readonly) {
		this->statusAlert("File is read-only");
		return;
	}
	camoto::stream::pos iCurOffset = this->iOffset + this->cursorOffset;
	int dest = iCurOffset * this->bitWidth + this->intraByteOffset;
	switch (this->editMode) {
		case HexEdit: {
			// TODO: Just make use of the bitstream functions to handle all this!
			this->file.seek(dest, camoto::stream::start);
			unsigned int cur;
			if (!this->file.read(this->bitWidth, &cur)) {
				this->statusAlert("Read error getting byte to update :-(");
				return;
			}
			int byteWidth = CALC_HEXCELL_WIDTH;
			int shift = (byteWidth - 1 - this->hexEditOffset) * 4;
			//int mask = ((1 << this->bitWidth) - 1) << shift;
			// The bitwidth for a single hex digit will be at most 4
			int mask = ((1 << min(4, this->bitWidth)) - 1) << shift;
			cur &= ~mask;
			cur |= byte << shift;
			// Save byte and limit it to the current bits (so typing "f" on first
			// char of 9-bit value 1FF will only go in as 1)
			byte = cur & ((1 << this->bitWidth) - 1);
			break;
		}
	}
	this->file.seek(dest, camoto::stream::start);
	if (!this->file.write(this->bitWidth, byte)) {
		this->statusAlert("Write error :-(");
	} else {
		this->moveCursor(1);
	}
	return;
}

void HexView::gotoOffset()
{
	std::string val = this->pConsole->getString("Offset", 15);

	// Reset status bar to hide prompt
	this->bStatusAlertVisible = true;
	this->statusAlert(NULL);

	if (val.length() > 0) {
		// Get value and scroll if ok
		const char *nptr = val.c_str();
		char *endptr;
		bool relative = (*nptr == '+') || (*nptr == '-');
		long off = strtol(nptr, &endptr, 0);
		if (
			(endptr != nptr) &&  // if text was entered, and
			(*endptr  == '\0')   // it was all valid
		) {
			// Perform the jump
			if (relative) {
				this->scrollRel(off);
			} else {
				this->scrollAbs(off);
			}
		}
	}

	// The cursor is turned on for the prompt, then turned off afterwards, so
	// now set it back to the correct state for the current edit mode.
	this->showCursor(true);

	return;
}
