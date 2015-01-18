/**
 * @file   FileView.cpp
 * @brief  IView interface for a file viewer.
 *
 * Copyright (C) 2009-2012 Adam Nielsen <malvineous@shikadi.net>
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

#include <sstream>
#include <camoto/stream_file.hpp>
#include "FileView.hpp"

FileView::FileView(std::string strFilename, camoto::stream::inout_sptr data,
	IConsole *pConsole)
	:	strFilename(strFilename),
		file(data, camoto::bitstream::littleEndian),
		pConsole(pConsole),
		bStatusAlertVisible(true), // trigger an update when next set
		bitWidth(8),
		intraByteOffset(0),
		iOffset(0),
		iFileSize(data->size())
{
	camoto::stream::file *file = dynamic_cast<camoto::stream::file *>(data.get());
	if (file) this->readonly = file->readonly();
	else this->readonly = true; // memory stream
}

FileView::FileView(const FileView& parent)
	:	strFilename(parent.strFilename),
		readonly(parent.readonly),
		file(parent.file),
		pConsole(parent.pConsole),
		bStatusAlertVisible(true), // trigger an update when next set
		bitWidth(parent.bitWidth),
		intraByteOffset(parent.intraByteOffset),
		iOffset(parent.iOffset),
		iFileSize(parent.iFileSize)
{
}

FileView::~FileView()
{
}

void FileView::init()
{
	this->pConsole->clearStatusBar(SB_TOP);
	this->pConsole->setStatusBar(SB_TOP, SB_LEFT, this->strFilename,
		SB_NO_CURSOR_MOVE);
	this->bStatusAlertVisible = true; // force redraw
	if (this->readonly) {
		this->statusAlert("File is read-only");
	} else {
		this->statusAlert(NULL); // draw the bottom status bar with no alert
	}
}

void FileView::updateTextEntry(const std::string& prompt,
	const std::string& text, unsigned int pos)
{
	this->pConsole->clearStatusBar(SB_BOTTOM);
	this->pConsole->setStatusBar(SB_BOTTOM, SB_LEFT, prompt + "> " + text,
		prompt.length() + 2 + pos);
	this->pConsole->cursor(true);
	return;
}

void FileView::clearTextEntry()
{
	this->pConsole->cursor(false);
	// Reset status bar to hide prompt
	this->bStatusAlertVisible = true;
	this->statusAlert(NULL);
	return;
}

void FileView::statusAlert(const char *cMsg)
{
	// If there's no status message and a blank has been requested, do nothing.
	if ((!cMsg) && (!this->bStatusAlertVisible)) return;

	// Blank out the bottom statusbar to hide the old status message
	this->pConsole->clearStatusBar(SB_BOTTOM);

	// Reset the right-hand side after we've blanked it
	this->pConsole->setStatusBar(SB_BOTTOM, SB_RIGHT, "F1=help",
		SB_NO_CURSOR_MOVE);

	if (cMsg) {
		this->pConsole->setStatusBar(SB_BOTTOM, SB_LEFT, std::string("Command>  *** ") + cMsg + " *** ",
			SB_NO_CURSOR_MOVE);
		this->bStatusAlertVisible = true;
	} else {
		this->pConsole->setStatusBar(SB_BOTTOM, SB_LEFT, "Command> ",
			SB_NO_CURSOR_MOVE);
		this->bStatusAlertVisible = false;
	}
	return;
}

void FileView::generateHeader(std::ostringstream& ss)
{
	unsigned long offsetInBytes = (this->iOffset * this->bitWidth) >> 3;
	// TODO: Leading spaces are dodgy, erase the line or something first.
	ss << "         Offset: " << offsetInBytes << '+' << this->intraByteOffset
		<< "b  Cell size: " << this->bitWidth
		<< "b/";
	if (this->file.getEndian() == camoto::bitstream::littleEndian) {
		ss << "LE";
	} else {
		ss << "BE";
	}
	return;
}

void FileView::updateHeader()
{
	std::ostringstream ss;
	this->generateHeader(ss);
	this->pConsole->setStatusBar(SB_TOP, SB_RIGHT, ss.str(), SB_NO_CURSOR_MOVE);
	return;
}


void FileView::setBitWidth(int newWidth)
{
	// Since the first bit on the screen should stay the same after
	// this change, we need to adjust offsets.
	int bitOffset = this->iOffset * this->bitWidth + this->intraByteOffset;

	this->bitWidth = newWidth;
	this->intraByteOffset = bitOffset % this->bitWidth;
	this->iOffset = bitOffset / this->bitWidth;

	assert(bitOffset == this->iOffset * this->bitWidth + this->intraByteOffset);

	return;
}

void FileView::setIntraByteOffset(int delta)
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
