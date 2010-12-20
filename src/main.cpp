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

#include <fstream>
#include <iostream>
#include "cfg.hpp"

#ifdef HAVE_NCURSESW_H
#include "NCursesConsole.hpp"
#endif

#ifdef HAVE_LIBX11
#include "XConsole.hpp"
#endif

#include "HexView.hpp"
#include "TextView.hpp"

Config cfg;

typedef unsigned char byte;

bool readConfig(std::iostream& config)
{
	if (!config.good()) return false;
	config.read((char*)&::cfg, sizeof(::cfg));
	if (config.gcount() != sizeof(::cfg)) return false;
	return true;
}

int main(int iArgC, char *cArgV[])
{
	if (iArgC != 2) {
		std::cerr << "Usage: ll <filename>" << std::endl;
		return 1;
	}

	// Load config
	std::fstream config(CONFIG_FILE, std::ios::in);
	if (!readConfig(config)) {
		// Set defaults
		::cfg.clrStatusBar.iFG = 15;
		::cfg.clrStatusBar.iBG = 4;
		::cfg.clrContent.iFG = 15;
		::cfg.clrContent.iBG = 1;
		::cfg.clrHighlight.iFG = 10;
		::cfg.clrHighlight.iBG = 0;
		::cfg.view = View_Text;
	} else {
		config.close();
	}

	IConsole *pConsole = NULL;

	// Try X11 interface first, if present
#ifdef HAVE_LIBX11
	Display *display = XOpenDisplay("");
	if (display) {
		pConsole = new XConsole(display);
	}
#endif

	// Otherwise fall back to curses
#ifdef HAVE_NCURSESW_H
	if (!pConsole) {
		pConsole = new NCursesConsole();
	}
#endif

	std::string strFilename = cArgV[1];
	camoto::iostream_sptr fsFile(new std::fstream(strFilename.c_str(), std::fstream::binary | std::fstream::in | std::fstream::out));

	fsFile->seekg(0, std::ios::end);
	std::fstream::off_type iFileSize = fsFile->tellg();

	IViewPtr pView;
	switch (::cfg.view) {
		case View_Hex:
			pView.reset(new HexView(strFilename, fsFile, iFileSize, pConsole));
			break;
		default: // View_Text
			pView.reset(new TextView(strFilename, fsFile, iFileSize, pConsole));
			break;
	}

	pConsole->setView(pView); // pConsole now owns pView (so we don't delete it)
	pConsole->mainLoop();

	delete pConsole;

	// Save config file
	config.open(CONFIG_FILE, std::ios::out);
	config.seekp(0, std::ios::beg);
	config.write((char*)&::cfg, sizeof(::cfg));
	config.close();
	return 0;
}
