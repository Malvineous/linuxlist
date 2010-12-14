/**
 * @file   main.cpp
 * @brief  Entry for Linux List.
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

#include <config.h>

#include <stdlib.h> // for abs()
#include <string>
#include <fstream>
#include <cassert>

#include <iostream>
#include "cfg.hpp"

#ifdef HAVE_NCURSESW_H
#include "NCursesConsole.hpp"
#endif
#include "HexView.hpp"

Config cfg;

typedef unsigned char byte;

int main(int iArgC, char *cArgV[])
{
	// TODO: Use boost::program_options
	if (iArgC != 2) {
		std::cerr << "Usage: ll <filename>" << std::endl;
		return 1;
	}

	// Load config - TODO: boost::program_options defaults!
	::cfg.clrStatusBar.iFG = 15;
	::cfg.clrStatusBar.iBG = 4;
	::cfg.clrContent.iFG = 15;
	::cfg.clrContent.iBG = 1;
	::cfg.clrHighlight.iFG = 10;
	::cfg.clrHighlight.iBG = 0;

#ifdef HAVE_NCURSESW_H
	IConsole *pConsole = new NCursesConsole();
#endif

	//std::string strFilename = "/home/adam/dos/ctl.txt";
	std::string strFilename = cArgV[1];
	camoto::iostream_sptr fsFile(new std::fstream(strFilename.c_str(), std::fstream::binary | std::fstream::in | std::fstream::out));

	fsFile->seekg(0, std::ios::end);
	std::fstream::off_type iFileSize = fsFile->tellg();

	IView *pView = new HexView(strFilename, fsFile, iFileSize, pConsole);

	Key c;
	do {
		c = (Key)getch();
		// Convert platform-specific keys into generic keys
		switch (c) {
			case KEY_UP:    c = Key_Up; break;
			case KEY_DOWN:  c = Key_Down; break;
			case KEY_LEFT:  c = Key_Left; break;
			case KEY_RIGHT: c = Key_Right; break;
			case KEY_PPAGE: c = Key_PageUp; break;
			case KEY_NPAGE: c = Key_PageDown; break;
			case KEY_HOME:  c = Key_Home; break;
			case KEY_END:   c = Key_End; break;
			default: break; // TODO: ignore unknown key to avoid clash
		}
	} while (pView->processKey(c));

	delete pView;
	delete pConsole;

	return 0;
}
