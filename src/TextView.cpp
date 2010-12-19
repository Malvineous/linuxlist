/**
 * @file   TextView.cpp
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

#include <errno.h>
#include "TextView.hpp"
#include "HexView.hpp"
#include "HelpView.hpp"

/// Maximum number of lines to reach when pressing the 'end' key.  If the file
/// has more lines than this, this is as far as the 'end' key will go.
#define MAX_LINE  (1 << 25)   // ~32 million lines (128MB memory use)

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))

TextView::TextView(std::string strFilename, camoto::iostream_sptr data,
	std::fstream::off_type iFileSize, IConsole *pConsole)
	throw () :
		FileView(strFilename, data, iFileSize, pConsole),
		iLineAlloc(80),
		line(0),
		cacheComplete(false)
{
	this->pLineBuffer = new uint8_t[this->iLineAlloc];
}

TextView::TextView(const FileView& parent)
	throw () :
		FileView(parent),
		iLineAlloc(80),
		line(0),
		cacheComplete(false)
{
	this->pLineBuffer = new uint8_t[this->iLineAlloc];

	// If we weren't at the start of the file when this view was loaded, try to
	// seek to the same spot.
	if (this->iOffset > 0) {
		int iWidth, iHeight;
		this->pConsole->getContentDims(&iWidth, &iHeight);
		// Try to seek to this line
		for (int l = 0; l < MAX_LINE; l++) {
			this->cacheLines(l, iWidth);
			int numLines = this->linePos.size();
			if (numLines < l) {
				// hit EOF already
				this->line = numLines - 1;
				break;
			}
			int bitOffset = this->iOffset * 8;
			if (this->linePos.back() > bitOffset) {
				this->line = this->linePos.size() - 1;
				if (this->line > 0) this->line--;
				break;
			}
		}
	}
}

TextView::~TextView()
	throw ()
{
	assert(this->pLineBuffer != NULL);
	delete[] this->pLineBuffer;
}

bool TextView::processKey(Key c)
	throw ()
{
	int iWidth, iHeight;
	this->pConsole->getContentDims(&iWidth, &iHeight);

	// Hide any active status message on any keypress
	this->statusAlert(NULL);

	// Global keys, always active
	switch (c) {
		case Key_Esc:
		case Key_F10:
		case 'q':
			return false;

		case Key_PageUp: this->scrollLines(-iHeight); break;

		case Key_Enter:
		case Key_PageDown: this->scrollLines(iHeight); break;

		case 'b':
			this->setBitWidth(max(1, this->bitWidth - 1));
			this->redrawScreen();
			break;
		case 'B':
			this->setBitWidth(min(sizeof(int)*8, this->bitWidth + 1));
			this->redrawScreen();
			break;
		case 's': this->setIntraByteOffset(-1); break;
		case 'S': this->setIntraByteOffset(1); break;
		case 'e': this->file.changeEndian(camoto::bitstream::littleEndian); this->redrawScreen(); break;
		case 'E': this->file.changeEndian(camoto::bitstream::bigEndian); this->redrawScreen(); break;
		case ALT('h'): {
			IViewPtr newView(new HexView(*this));
			this->pConsole->setView(newView);
			break;
		}
		case CTRL('L'): this->redrawScreen(); break;
		case Key_Up: this->scrollLines(-1); break;
		case Key_Down: this->scrollLines(1); break;
		case Key_Home:
			if (this->line < iHeight) {
				// Relative scroll
				this->scrollLines(-this->line);
			} else {
				// Absolute scroll
				this->line = 0;
				this->redrawScreen();
			}
			break;
		case Key_End: {
			this->cacheLines(MAX_LINE, iWidth);
			int target = this->linePos.size() - iHeight;
			if ((this->line + iHeight > target) && (this->line - iHeight < target)) {
				// Relative scroll
				this->scrollLines(target - this->line);
			} else {
				// Absolute scroll
				this->line = target;
				this->redrawScreen();
			}
			break;
		}
		case Key_F1: {
			IViewPtr newView(new HelpView(shared_from_this(), this->pConsole));
			this->pConsole->setView(newView);
			break;
		}
		default: break;
	}

	this->pConsole->update();
	return true; // true == keep going (don't quit)
}

void TextView::redrawScreen()
	throw ()
{
	int iWidth, iHeight;
	this->pConsole->getContentDims(&iWidth, &iHeight);
	this->pConsole->cursor(false);

	this->redrawLines(0, iHeight, iWidth);

	// Update the header after drawing the lines as that function will update
	// the cached line list, which causes the line count to be updated (which we
	// then want to display in the header.)
	this->updateHeader();

	return;
}

void TextView::generateHeader(std::ostringstream& ss)
	throw ()
{
	this->iOffset = this->linePos[this->line] / 8;
	this->FileView::generateHeader(ss);
	// Append our own text onto the end
	ss << " Line: " << this->line + 1 << '/' << this->linePos.size();
	if (!this->cacheComplete) ss << '+';
	return;
}

void TextView::setBitWidth(int newWidth)
	throw ()
{
	// Clear all the line markings so they are re-read
	this->linePos.clear();
	this->line = 0;
	this->iOffset = 0;

	this->FileView::setBitWidth(newWidth);
	return;
}

void TextView::setIntraByteOffset(int delta)
	throw ()
{
	// Clear all the line markings so they are re-read
	this->linePos.clear();
	this->line = 0;
	this->iOffset = 0;

	this->FileView::setIntraByteOffset(delta);
	return;
}

void TextView::scrollLines(int iDelta)
	throw ()
{
	if (iDelta == 0) return; // e.g. pressing Home twice

	int iWidth, iHeight;
	this->pConsole->getContentDims(&iWidth, &iHeight);

	// If the user wants to scroll up, towards the start of the file...
	if (iDelta < 0) {

		// If the scroll operation will be cropped to the start of the file,
		// display a notice to the user.
		if (-iDelta > this->line) {
			this->statusAlert("Top of file");
			// Crop the scroll so that it only goes back to the start, not past it
			iDelta = -this->line;
		}

		// But prevent them from actually scrolling past the start.
		if (this->line == 0) return;

	} else {
		// The user wants to scroll down, towards the end of the file.

		this->cacheLines(this->line + iDelta + iHeight, iWidth);

		// Prevent scrolling past last line in file
		if (this->line + iDelta >= this->linePos.size()) {
			iDelta = this->linePos.size() - 1 - this->line;
		}
	}

	this->line += iDelta;

	// If we are past EOF, display a notice to the user.
	if (this->line + iHeight >= this->linePos.size()) {
		this->statusAlert("End of file");
	}

	if (iDelta == 0) return;

	// If we're here, then iDelta is within limits and won't scroll too far
	// in either direction.

	if (abs(iDelta) > iHeight) {
		// But we're scrolling by more than a screenful, so we'll need to
		// redraw the whole screen anyway.
		this->redrawLines(0, iHeight, iWidth);
	} else {

		// If we're here, then we're scrolling by only a handful of lines, so
		// try to do it efficiently.
		this->pConsole->scrollContent(0, iDelta);
		if (iDelta < 0) {
			this->redrawLines(0, -iDelta, iWidth);
		} else {
			this->redrawLines(iHeight-iDelta, iHeight, iWidth);
		}
	}

	this->updateHeader();

	return;
}

void TextView::redrawLines(int iTop, int iBottom, int width)
	throw ()
{
	if (width > this->iLineAlloc) {
		// Enlarge the buffer
		delete[] this->pLineBuffer;
		this->iLineAlloc = width;
		this->pLineBuffer = new uint8_t[this->iLineAlloc];
	}

	int y = iTop;

	// Make sure the lines up to the start of this page have been cached.
	this->cacheLines(this->line + y, width);
	int cachedLines = this->linePos.size();

	if (this->line + y < cachedLines) {
		// There is content to draw (as opposed to drawing past EOF, e.g. when
		// drawing 'new' lines at the bottom of the screen when scrolling.)
		file.clear(); // clear any errors (e.g. reaching EOF previously)
		int lastOffset = this->linePos[this->line + y];//iCurOffset * this->bitWidth + this->intraByteOffset;
		file.seek(lastOffset, std::ios::beg);
		bool eof = false;

		int off = 0;
		for (; (y < iBottom) && !eof; y++) {

			this->pConsole->gotoxy(0, y);

			int prev = -1;
			int x;
			for (x = 0; (x < width) && !eof; x++) {
				int c;
				if (!file.read(this->bitWidth, &c)) {
					eof = true;
					this->cacheComplete = true; // cached all lines in file now
					break;
				}
				off++;

				if (c == 0) {
					this->pLineBuffer[x] = ' ';
				} else if (c == '\r') {
					// Allow a preceding null character to escape this one
					if (prev == 0) {
						this->pLineBuffer[--x] = c;
					} else {
						this->pLineBuffer[x] = ' ';
					}
				} else if (c == '\n') {
					// EOL

					// Allow a preceding null character to escape this one.  This allows
					// the ASCII table in the help text to display all 256 chars,
					// hopefully without causing any text files to mis-display (since
					// they shouldn't have any nulls in them at all.)
					// Allow a preceding null character to escape this one
					if (prev == 0) {
						this->pLineBuffer[--x] = c;
					} else {
						break;
					}
				} else if (c > 255) {
					this->pLineBuffer[x] = '.'; // TODO: some non-ASCII char
				} else {
					this->pLineBuffer[x] = c;
				}
				prev = c;
			}

			this->pLineBuffer[x] = 0; // force EOL

			if (!eof) {
				// Cache this line while we're here
				if (this->line + y == cachedLines - 1) {
					this->linePos.push_back(lastOffset + off * this->bitWidth);
					cachedLines++;
				}

				// But regardless make sure this line has been cached
				assert(this->line + y < cachedLines);
			}

			// Display line
			this->pConsole->putstr((char *)this->pLineBuffer);
			if (x < width) this->pConsole->eraseToEOL();
		}
	}

	// Blank out any leftover lines
	for (; y < iBottom; y++) {
		this->pConsole->gotoxy(0, y);
		this->pConsole->eraseToEOL();
	}

	return;
}

void TextView::cacheLines(int maxLine, int width)
	throw ()
{
	int cachedLines = this->linePos.size();
	if (cachedLines == 0) {
		// First line or bit seek changed
		this->linePos.push_back(this->intraByteOffset);
		cachedLines++;
	}

	if (maxLine >= cachedLines) {
		// Need to read more data to get to this line
		int lastOffset = this->linePos.back();
		file.clear(); // recover from any previous EOF
		file.seek(lastOffset, std::ios::beg);

		int off = 0;
		bool eof = false;
		int prev = -1;
		for (int i = cachedLines; (i <= maxLine) && !eof; i++) {
			for (int x = 0; x < width; x++) {
				int c;
				if (!file.read(this->bitWidth, &c)) {
					eof = true;
					this->cacheComplete = true; // cached all lines in file now
					break;
				}
				off++;
				if ((c == '\n') && (prev != 0)) {
					prev = c;
					break;
				}
				prev = c;
			}
			if (!eof) this->linePos.push_back(lastOffset + off * this->bitWidth);
		}

		assert((this->linePos.size() == maxLine + 1) || eof);
	}
	return;
}
