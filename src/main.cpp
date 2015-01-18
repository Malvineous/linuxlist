/**
 * @file   main.cpp
 * @brief  Entry for Linux List.
 *
 * Copyright (C) 2009-2015 Adam Nielsen <malvineous@shikadi.net>
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

#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <camoto/stream_file.hpp>
#include <config.h>
#include "cfg.hpp"

/// Path to config file, within home directory
#define CONFIG_FILE "/.config/ll"

#ifdef HAVE_NCURSESW
#include "NCursesConsole.hpp"
#endif

#ifdef USE_X11
#include "XConsole.hpp"
#endif

#include "HexView.hpp"
#include "TextView.hpp"

Config cfg;

typedef unsigned char byte;

IConsole::~IConsole()
{
}

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
	bool gotConfig = false;
	std::string configFilename(getenv("HOME"));
	if (!configFilename.empty()) {
		configFilename.append(CONFIG_FILE);
		std::fstream config(configFilename.c_str(), std::ios::in);
		gotConfig = readConfig(config);
		config.close();
	}
	if (!gotConfig) {
		// Set defaults
		::cfg.clrStatusBar.iFG = 15;
		::cfg.clrStatusBar.iBG = 4;
		::cfg.clrContent.iFG = 15;
		::cfg.clrContent.iBG = 1;
		::cfg.clrHighlight.iFG = 10;
		::cfg.clrHighlight.iBG = 0;
		::cfg.view = View_Text;
	}

	IConsole *pConsole = NULL;

	// Try X11 interface first, if present
#ifdef USE_X11
	Display *display = XOpenDisplay("");
	if (display) {
		pConsole = new XConsole(display);
	}
#endif

	// Otherwise fall back to curses
#ifdef HAVE_NCURSESW
	if (!pConsole) {
		pConsole = new NCursesConsole();
	}
#endif

	if (!pConsole) {
		std::cerr << "Unable to find a usable display method from one of [ "
#ifdef USE_X11
			"X11 "
#endif
#ifdef HAVE_NCURSESW
			"NCurses "
#endif
			"]"
			<< std::endl;
		return 1;
	}

	std::string strFilename = cArgV[1];
	camoto::stream::file_sptr fsFile(new camoto::stream::file());
	try {
		fsFile->open(strFilename);
	} catch (const camoto::stream::open_error&) {
		fsFile->open_readonly(strFilename);
	}

	IViewPtr pView;
	switch (::cfg.view) {
		case View_Hex:
			pView.reset(new HexView(strFilename, fsFile, pConsole));
			break;
		default: // View_Text
			pView.reset(new TextView(strFilename, fsFile, pConsole));
			break;
	}

	pConsole->setView(pView);
	pConsole->mainLoop();

	delete pConsole;

	// Save config file
	if (!configFilename.empty()) {
		std::fstream config(configFilename.c_str(), std::ios::out);
		config.seekp(0, std::ios::beg);
		config.write((char*)&::cfg, sizeof(::cfg));
		config.close();
	}

#ifdef USE_X11
	if (display) XCloseDisplay(display);
#endif

	return 0;
}
