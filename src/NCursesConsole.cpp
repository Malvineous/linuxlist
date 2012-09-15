/**
 * @file   NCursesConsole.cpp
 * @brief  ncurses implementation of IConsole.
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

#include <string.h> // strerror()
#include <errno.h>
#include <cassert>
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
	0x2022, // bullet
	0x25D8, // inverted bullet
	0x25CB, // small circle
	0x25D9, // inverted small circle
	0x2642, // male
	0x2640, // female
	0x266A, // single note
	0x266B, // double note
	0x263C, // O thing
	0x25BA, // solid > triangle
	0x25C4, // solid < triangle
	0x2195, // up/down arrow
	0x203C, // double exclamation mark
	0x00B6, // end of paragraph marker
	0x00A7, // weird S thing
	0x25AC, // thick underline
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
const wchar_t ctl_utf8_0x7f = 0x2302;

NCursesConsole::NCursesConsole(void)
	:	maxLineLen(80),
		cursorInWindow(true)
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

	// Create the status bars
	this->winStatus[0] = newwin(1, COLS, 0, 0);
	this->winStatus[1] = newwin(1, COLS, LINES-1, 0);

	// Create the main window between the status bars
	this->winContent = newwin(LINES - 2, COLS, 1, 0);

	// Define our colours
	if (has_colors() == FALSE) {
		// TODO: Can we still cope with reverse video or something?
	}
	start_color();
	this->setColoursFromConfig();

	idlok(this->winContent, TRUE); // enable efficient scrolling
	scrollok(this->winContent, TRUE); // Allow scrolling

	wnoutrefresh(this->winStatus[0]);
	wnoutrefresh(this->winStatus[1]);
	wnoutrefresh(this->winContent);

	refresh();
}

NCursesConsole::~NCursesConsole()
{
	delwin(this->winContent);
	delwin(this->winStatus[1]);
	delwin(this->winStatus[0]);

	// Restore the terminal
	endwin();

	iconv_close(this->cd);
}

void NCursesConsole::mainLoop()
{
	Key c;
	bool escape = false; // was last keypress ESC?
	do {
		c = (Key)getch();
		if (c & KEY_CODE_YES) {
			// Convert platform-specific keys into generic keys
			switch (c) {
				case KEY_BACKSPACE: c = Key_Backspace; break;
				case KEY_UP:        c = Key_Up; break;
				case KEY_DOWN:      c = Key_Down; break;
				case KEY_LEFT:      c = Key_Left; break;
				case KEY_RIGHT:     c = Key_Right; break;
				case KEY_PPAGE:     c = Key_PageUp; break;
				case KEY_NPAGE:     c = Key_PageDown; break;
				case KEY_HOME:      c = Key_Home; break;
				case KEY_END:       c = Key_End; break;
				case KEY_DC:        c = Key_Del; break;
				case KEY_F(1):      c = Key_F1; break;
				case KEY_F(10):     c = Key_F10; break;
				case KEY_RESIZE:
					// Terminal resized
					delwin(this->winContent);
					delwin(this->winStatus[1]);
					delwin(this->winStatus[0]);
					this->winStatus[0] = newwin(1, COLS, 0, 0);
					this->winStatus[1] = newwin(1, COLS, LINES-1, 0);
					this->winContent = newwin(LINES - 2, COLS, 1, 0);

					this->setColoursFromConfig();

					idlok(this->winContent, TRUE); // enable efficient scrolling
					scrollok(this->winContent, TRUE); // Allow scrolling

					this->view->redrawScreen();
					continue;
				default: continue; // ignore unknown key
			}
			escape = false;
		} else {
			if (escape) {
				escape = false;
				if ((c >= 'a') && (c <= 'z')) c = (Key)(Key_Alt | c);
				else if (c == 27) c = Key_Esc;
			} else {
				switch (c) {
					case 9:   c = Key_Tab; break;
					case 13:  c = Key_Enter; break;
					case 27:  c = Key_None; escape = true; break;
					case 127: c = Key_Backspace; break;
					default: break; // allow unknown key through as ASCII
				}
			}
		}
	} while (this->processKey(c));

	return;
}

void NCursesConsole::update(void)
{
	wnoutrefresh(this->winStatus[0]);
	wnoutrefresh(this->winStatus[1]);
	wnoutrefresh(this->winContent);
	if (!this->cursorInWindow) {
		// Refresh the status bar again so the cursor ends up in this window
		wnoutrefresh(this->winStatus[1]);
	}
	// Copy the virtual screen onto the terminal
	doupdate();
}

void NCursesConsole::clearStatusBar(SB_Y eY)
{
	// Erasing the line avoids the entire screen flickering as it does if we
	// use wclear() instead.
	wmove(this->winStatus[eY], 0, 0);
	wclrtoeol(this->winStatus[eY]);
	this->cursorInWindow = true;
	return;
}

void NCursesConsole::setStatusBar(SB_Y eY, SB_X eX,
	const std::string& strMessage, int cursor)
{
	int x;
	switch (eX) {
		case SB_LEFT:   x = 0; break;
		case SB_CENTRE: x = (COLS + strMessage.length() / 2); break;
		case SB_RIGHT:  x = COLS - strMessage.length(); break;
	}
	wmove(this->winStatus[eY], 0, x);
	waddstr(this->winStatus[eY], strMessage.c_str());
	if (cursor >= 0) {
		wmove(this->winStatus[eY], 0, x + cursor);
		this->cursorInWindow = false;
	} else {
		this->cursorInWindow = true;
	}
	return;
}

void NCursesConsole::gotoxy(int x, int y)
{
	wmove(this->winContent, y, x);
	this->cursorInWindow = true;
	return;
}

void NCursesConsole::putstr(const std::string& strContent)
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
			this->setStatusBar(SB_BOTTOM, SB_LEFT, std::string("!!> iconv error: ") + strerror(errno),
				SB_NO_CURSOR_MOVE);
		} else {
			size_t iCount = (wchar_t *)pOut - wout;
			for (int i = 0; i < iCount; i++) {
				if (wout[i] < 32) {
					// This is a control character, so escape it manually
					wout[i] = ctl_utf8[wout[i]];
				} else if (wout[i] == 127) {
					// Also not escaped
					wout[i] = ctl_utf8_0x7f;
				}
			}
			waddnwstr(this->winContent, wout, iCount);
		}
		delete[] wout;
	} else {
		// No conversion, display as-is
		waddstr(this->winContent, strContent.c_str());
	}
	return;
}

void NCursesConsole::getContentDims(int *iWidth, int *iHeight)
{
	getmaxyx(this->winContent, *iHeight, *iWidth);
	return;
}

void NCursesConsole::scrollContent(int iX, int iY)
{
	wscrl(this->winContent, iY);
	return;
}

void NCursesConsole::eraseToEOL(void)
{
	wclrtoeol(this->winContent);
	return;
}

void NCursesConsole::cursor(bool visible)
{
	curs_set(visible ? 1 : 0);
	return;
}

void NCursesConsole::setColoursFromConfig()
{
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

	for (int i = 0; i < 2; i++) {
		wattrset(this->winStatus[i],
			COLOR_PAIR(CLR_STATUSBAR) | this->iAttribute[CLR_STATUSBAR]);
		wbkgdset(this->winStatus[i], COLOR_PAIR(CLR_STATUSBAR));
		wclear(this->winStatus[i]);
	}

	wattrset(this->winContent,
		COLOR_PAIR(CLR_CONTENT) | this->iAttribute[CLR_CONTENT]);
	wbkgdset(this->winContent, COLOR_PAIR(CLR_CONTENT));
	wclear(this->winContent);

	return;
}
