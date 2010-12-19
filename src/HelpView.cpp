/**
 * @file   HelpView.cpp
 * @brief  TextView extension for showing help screen.
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

#include "HelpView.hpp"

#define HELP_TEXT \
	"Linux List - a Linux clone of Vernon D. Buerg's List file viewer\n" \
	"Copyright 2009-2010 Adam Nielsen <malvineous@shikadi.net>\n" \
	"http://www.shikadi.net/ll\n" \
	"\n" \
	"-= Keys =-\n" \
	"\n" \
	"  Basic keys                     Advanced keys\n" \
	"  ~~~~~~~~~~                     ~~~~~~~~~~~~~\n" \
	"  F10/q/Esc  Quit                Alt+H Toggle hex view\n" \
	"  Arrows     Scroll              S/s   Seek forward/back one bit\n" \
	"  Home/End   Jump to start/end   E/e   Set big/little endian\n" \
	"  Ctrl+L     Redraw screen       B/b   +/- num bits per cell\n" \
	"\n" \
	"                                 Hex-view keys\n" \
	"                                 ~~~~~~~~~~~~~\n" \
	"                                 Tab   Cycle edit mode\n" \
  "                                 +/-   Alter line width\n" \
	"                                 g     Go to offset (prefix 0=oct, 0x=hex)\n" \
	"\n" \
	"-= ASCII table =-\n" \
	"\n" \
	"      0 1 2 3 4 5 6 7 8 9 A B C D E F\n" \
	"    +--------------------------------\n" \
	"  0 | \x00 \x01 \x02 \x03 \x04 \x05 \x06 \x07 \x08 \x09 \x00\x0a \x0b \x0c \x00\x0d \x0e \x0f\n" \
	"  1 | \x10 \x11 \x12 \x13 \x14 \x15 \x16 \x17 \x18 \x19 \x1a \x1b \x1c \x1d \x1e \x1f\n" \
	"  2 | \x20 \x21 \x22 \x23 \x24 \x25 \x26 \x27 \x28 \x29 \x2a \x2b \x2c \x2d \x2e \x2f\n" \
	"  3 | \x30 \x31 \x32 \x33 \x34 \x35 \x36 \x37 \x38 \x39 \x3a \x3b \x3c \x3d \x3e \x3f\n" \
	"  4 | \x40 \x41 \x42 \x43 \x44 \x45 \x46 \x47 \x48 \x49 \x4a \x4b \x4c \x4d \x4e \x4f\n" \
	"  5 | \x50 \x51 \x52 \x53 \x54 \x55 \x56 \x57 \x58 \x59 \x5a \x5b \x5c \x5d \x5e \x5f\n" \
	"  6 | \x60 \x61 \x62 \x63 \x64 \x65 \x66 \x67 \x68 \x69 \x6a \x6b \x6c \x6d \x6e \x6f\n" \
	"  7 | \x70 \x71 \x72 \x73 \x74 \x75 \x76 \x77 \x78 \x79 \x7a \x7b \x7c \x7d \x7e \x7f\n" \
	"  8 | \x80 \x81 \x82 \x83 \x84 \x85 \x86 \x87 \x88 \x89 \x8a \x8b \x8c \x8d \x8e \x8f\n" \
	"  9 | \x90 \x91 \x92 \x93 \x94 \x95 \x96 \x97 \x98 \x99 \x9a \x9b \x9c \x9d \x9e \x9f\n" \
	"  a | \xa0 \xa1 \xa2 \xa3 \xa4 \xa5 \xa6 \xa7 \xa8 \xa9 \xaa \xab \xac \xad \xae \xaf\n" \
	"  b | \xb0 \xb1 \xb2 \xb3 \xb4 \xb5 \xb6 \xb7 \xb8 \xb9 \xba \xbb \xbc \xbd \xbe \xbf\n" \
	"  c | \xc0 \xc1 \xc2 \xc3 \xc4 \xc5 \xc6 \xc7 \xc8 \xc9 \xca \xcb \xcc \xcd \xce \xcf\n" \
	"  d | \xd0 \xd1 \xd2 \xd3 \xd4 \xd5 \xd6 \xd7 \xd8 \xd9 \xda \xdb \xdc \xdd \xde \xdf\n" \
	"  e | \xe0 \xe1 \xe2 \xe3 \xe4 \xe5 \xe6 \xe7 \xe8 \xe9 \xea \xeb \xec \xed \xee \xef\n" \
	"  f | \xf0 \xf1 \xf2 \xf3 \xf4 \xf5 \xf6 \xf7 \xf8 \xf9 \xfa \xfb \xfc \xfd \xfe \xff\n" \
	"\n" \
	"-= License =-\n" \
	"\n" \
	"  This program is free software: you can redistribute it and/or modify\n" \
	"  it under the terms of the GNU General Public License as published by\n" \
	"  the Free Software Foundation, either version 3 of the License, or\n" \
	"  (at your option) any later version.\n" \
	"\n" \
	"  This program is distributed in the hope that it will be useful,\n" \
	"  but WITHOUT ANY WARRANTY; without even the implied warranty of\n" \
	"  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n" \
	"  GNU General Public License for more details.\n" \
	"\n" \
	"  You should have received a copy of the GNU General Public License\n" \
	"  along with this program.  If not, see <http://www.gnu.org/licenses/>.\n"

std::stringstream HelpView::ss(std::string(HELP_TEXT, sizeof(HELP_TEXT) - 1));

class deleter {
	public:
		void operator()(std::stringstream *p) { }
};

HelpView::HelpView(IViewPtr oldView, IConsole *pConsole)
	throw () :
		TextView("Help (F10 to exit)", camoto::iostream_sptr(&ss, deleter()),
			sizeof(HELP_TEXT) - 1, pConsole),
		oldView(oldView)
{
}

HelpView::~HelpView()
	throw ()
{
}

bool HelpView::processKey(Key c)
	throw ()
{
	switch (c) {
		case Key_F1:
		case Key_Esc:
		case Key_F10:
		case 'q':
			this->pConsole->setView(this->oldView);
			break;

		// These keys are passed through to the text view and processed as usual.
		case Key_Up:
		case Key_Down:
		case Key_Left:
		case Key_Right:
		case Key_Home:
		case Key_End:
		case Key_PageUp:
		case Key_PageDown:
			this->TextView::processKey(c);
			break;
	}
	return true; // true == keep going (don't quit)
}

void HelpView::generateHeader(std::ostringstream& ss)
	throw ()
{
	ss << "   Line: " << this->line + 1 << '/' << this->linePos.size();
	if (!this->cacheComplete) ss << '+';
	return;
}
