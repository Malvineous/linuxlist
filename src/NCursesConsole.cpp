/**
 * @file   NCursesConsole.cpp
 * @brief  ncurses implementation of IConsole.
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

#include <string.h> // strerror()
#include <errno.h>
#include <iostream> // for errors before we get to nCurses

#include "cfg.hpp"
#include "NCursesConsole.hpp"

// Not sure why these aren't defined elsewhere...is this even portable??
#define NC_KEY_ENTER1 '\r'
#define NC_KEY_ENTER2 '\n'
#define NC_KEY_BACKSPACE 0x7F

static const int cgaColours[] = {COLOR_BLACK, COLOR_BLUE, COLOR_GREEN,
	COLOR_CYAN, COLOR_RED, COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE
};

wchar_t ctl_utf8[] = {
	0x0020, // NULL = space
	0x263A, // Ctrl+A == transparent smiley face
	0x263B, // Ctrl+B == opaque smiley face
	0x2665, // Heart
	0x2666, // Diamond
	0x2663, // Club
	0x2660, // Spade
	0x25CF, // bullet (should be smaller than this though)
	0x25D8, // inverted bullet
	0x25CB, // small circle
	0x25D9, // inverted small circle
	0x2642, // male
	0x2640, // female
	0x266A, // single note
	0x266B, // double note
	0x263C, // O thing
	0x25B6, // solid > triangle
	0x25C0, // solid < triangle
	0x2195, // up/down arrow
	0x203C, // double exclamation mark
	0x00B6, // end of paragraph marker
	0x00A7, // weird S thing
	0x2583, // thick underline
	0x21A8, // underlined up/down arrow
	0x2191, // up arrow
	0x2193, // down arrow
	0x2192, // right arrow
	0x2190, // left arrow
	0x221F, // |_ brackety thing (should be a bit shorter)
	0x2194, // left/right arrow
	0x25B2, // solid ^ triangle
	0x25BC, // solid v triangle
};
// 127 could be 0x25B0 (just a triangle, not square at the bottom)

NCursesConsole::NCursesConsole(void)
	throw () :
		maxLineLen(80)
{
	// Init iconv
	setlocale(LC_ALL, ""); // set locale based on LANG env var
	this->cd = iconv_open("WCHAR_T", "IBM437");
	if (this->cd == (iconv_t)-1) {
		if (errno == EINVAL) {
			printf("iconv open failed, unable to convert from IBM437 to WCHAR_T\n"
				"Extended characters will look funny...\n");
		} else {
			perror("iconv open failed");
		}
		sleep(2);
	}

	// Init ncurses
	initscr();

	raw();  // Line buffering disabled (get keys as they're pressed)
	keypad(stdscr, TRUE);  // Get F1, F2, etc.
	noecho();  // Don't echo keypresses with getch()
	nonl(); // Don't autoconvert the return key
	curs_set(0);  // Hide the cursor

	// Define our colours
	if (has_colors() == FALSE) {
		// TODO: Can we still cope with reverse video or something?
	}
	start_color();
	init_pair(CLR_STATUSBAR,
		cgaColours[cfg.clrStatusBar.iFG & 7],
		cgaColours[cfg.clrStatusBar.iBG & 7]);
	this->iAttribute[CLR_STATUSBAR] =
		(cfg.clrStatusBar.iFG & 8) ? A_BOLD : A_NORMAL;
	init_pair(CLR_CONTENT,
		cgaColours[cfg.clrContent.iFG & 7],
		cgaColours[cfg.clrContent.iBG & 7]);
	this->iAttribute[CLR_CONTENT] =
		(cfg.clrContent.iFG & 8) ? A_BOLD : A_NORMAL;

	// Create the status bars
	this->winStatus[0] = newwin(1, COLS, 0, 0);
	this->winStatus[1] = newwin(1, COLS, LINES-1, 0);
	for (int i = 0; i < 2; i++) {
		wattrset(this->winStatus[i],
			COLOR_PAIR(CLR_STATUSBAR) | this->iAttribute[CLR_STATUSBAR]);
		wbkgdset(this->winStatus[i], COLOR_PAIR(CLR_STATUSBAR));
		wclear(this->winStatus[i]);
	}

	// Create the main window between the status bars
	this->winContent = newwin(LINES - 2, COLS, 1, 0);
	wattrset(this->winContent,
		COLOR_PAIR(CLR_CONTENT) | this->iAttribute[CLR_CONTENT]);
	wbkgdset(this->winContent, COLOR_PAIR(CLR_CONTENT));
	wclear(this->winContent);

	idlok(this->winContent, TRUE); // enable efficient scrolling
	scrollok(this->winContent, TRUE); // Allow scrolling

	wnoutrefresh(this->winStatus[0]);
	wnoutrefresh(this->winStatus[1]);
	wnoutrefresh(this->winContent);

	refresh();
}

NCursesConsole::~NCursesConsole()
	throw ()
{
	delwin(this->winContent);

	// Restore the terminal
	endwin();

	iconv_close(this->cd);
}

void NCursesConsole::mainLoop(IView *pView)
	throw ()
{
	Key c;
	do {
		c = (Key)getch();
		if (c & KEY_CODE_YES) {
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
				default: continue; // ignore unknown key
			}
		} else {
			switch (c) {
				case 9:  c = Key_Tab; break;
				case 27: c = Key_Esc; break;
				default: break; // allow unknown key through as ASCII
			}
		}
	} while (pView->processKey(c));

	return;
}

void NCursesConsole::update(void)
	throw ()
{
	wnoutrefresh(this->winStatus[0]);
	wnoutrefresh(this->winStatus[1]);
	wnoutrefresh(this->winContent);
	// Copy the virtual screen onto the terminal
	doupdate();
}

void NCursesConsole::clearStatusBar(SB_Y eY)
	throw ()
{
	// Erasing the line avoids the entire screen flickering as it does if we
	// use wclear() instead.
	wmove(this->winStatus[eY], 0, 0);
	wclrtoeol(this->winStatus[eY]);
	return;
}

void NCursesConsole::setStatusBar(SB_Y eY, SB_X eX, const std::string& strMessage)
	throw ()
{
	switch (eX) {
		case SB_LEFT:   wmove(this->winStatus[eY], 0, 0); break;
		case SB_CENTRE: wmove(this->winStatus[eY], 0, (COLS + strMessage.length()) / 2); break;
		case SB_RIGHT:  wmove(this->winStatus[eY], 0, COLS - strMessage.length()); break;
	}
	//wprintw(this->winStatus[eY], "%s", strMessage.c_str());
	waddstr(this->winStatus[eY], strMessage.c_str());
	// Copy the change onto the virtual screen
//			wnoutrefresh(this->winStatus[eY]);
	return;
}

void NCursesConsole::gotoxy(int x, int y)
	throw ()
{
	wmove(this->winContent, y, x);
	return;
}

void NCursesConsole::putstr(const std::string& strContent)
	throw ()
{
	if (this->cd != (iconv_t)-1) {
		// iconv is valid, convert
	again:
		wchar_t *wout = new wchar_t[this->maxLineLen];
		char *pIn = (char *)strContent.c_str();
		char *pOut = (char *)wout;
		size_t inleft = strContent.length(), outleft = sizeof(wchar_t) * this->maxLineLen;
		if (iconv(cd, &pIn, &inleft, &pOut, &outleft) == (size_t)-1) {
			if ((errno == E2BIG) && (this->maxLineLen < 0x10000)) {
				this->maxLineLen <<= 1;
				delete[] wout;
				goto again;
			}
			this->setStatusBar(SB_BOTTOM, SB_LEFT, std::string("!!> iconv error: ") + strerror(errno));
		} else {
			//printf("%d chars", ((wchar_t *)pOut) - wout);
			size_t iCount = (wchar_t *)pOut - wout;
			for (int i = 0; i < iCount; i++) {
				if (wout[i] < 32) {
					// This is a control character, so escape it manually
					wout[i] = ctl_utf8[wout[i]];
				} else if (wout[i] == 127) {
					// Also not escaped
					wout[i] = 0x25B0;
				}
			}
			waddnwstr(this->winContent, wout, iCount);
		}
		delete[] wout;
	} else {
		// No conversion, display as-is
		waddstr(this->winContent, strContent.c_str());
	}
	//waddnwstr(this->winContent, (wchar_t *)cOutBuf, strContent.length());
	return;
}

void NCursesConsole::getContentDims(int *iWidth, int *iHeight)
	throw ()
{
	getmaxyx(this->winContent, *iHeight, *iWidth);
	return;
}

void NCursesConsole::scrollContent(int iX, int iY)
	throw ()
{
	wscrl(this->winContent, iY);
	return;
}

void NCursesConsole::eraseToEOL(void)
	throw ()
{
	wclrtoeol(this->winContent);
	return;
}

void NCursesConsole::cursor(bool visible)
	throw ()
{
	curs_set(visible ? 1 : 0);
	return;
}

std::string NCursesConsole::getString(const std::string& strPrompt, int maxLen)
	throw ()
{
	wmove(this->winStatus[SB_BOTTOM], 0, 0);
	waddstr(this->winStatus[SB_BOTTOM], strPrompt.c_str());
	waddstr(this->winStatus[SB_BOTTOM], "> ");
	wclrtoeol(this->winStatus[SB_BOTTOM]);
	std::string s;
	char c;
	curs_set(1);
	for (;;) {
		c = wgetch(this->winStatus[SB_BOTTOM]);
		if ((c == NC_KEY_ENTER1) || (c == NC_KEY_ENTER2)) break;
		if (c == NC_KEY_BACKSPACE) {
			if (s.length()) {
				int len = s.length() - 1;
				s = s.substr(0, len);
				len += strPrompt.length() + 2;
				wmove(this->winStatus[SB_BOTTOM], 0, len);
				wechochar(this->winStatus[SB_BOTTOM], ' ');
				wmove(this->winStatus[SB_BOTTOM], 0, len);
			}
			continue;
		}
		if (c & KEY_CODE_YES) continue; // ignore special keys

		if (s.length() < maxLen) {
			s += c;
			wechochar(this->winStatus[SB_BOTTOM], c);
		}
		//printf("%02X\n", c);
		//wnoutrefresh(this->winStatus[SB_BOTTOM]);
	}
	curs_set(0);
	return s;
}
