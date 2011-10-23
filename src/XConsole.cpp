/**
 * @file   XConsole.cpp
 * @brief  X11 implementation of IConsole.
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
#include <cassert>
#include <iostream> // for errors before we get to nCurses

#include <X11/Xutil.h>

#include "cfg.hpp"
#include "XConsole.hpp"
#include "font.hpp"

/// EGA palette
int pal[] = {
	0x000000,
	0x0000AA,
	0x00AA00,
	0x00AAAA,
	0xAA0000,
	0xAA00AA,
	0xAA5500,
	0xAAAAAA,
	0x555555,
	0x5555FF,
	0x55FF55,
	0x55FFFF,
	0xFF5555,
	0xFF55FF,
	0xFFFF55,
	0xFFFFFF,
};

XConsole::XConsole(Display *display)
	throw (std::ios::failure) :
		display(display),
		fontWidth(8),
		fontHeight(14),
		cursorX(0),
		cursorY(0),
		cursorVisible(false),
		text(NULL),
		screenWidth(80),
		screenHeight(25)
{
	int screen = DefaultScreen(this->display);

	// Mark colours as not-yet-allocated, then allocate them
	this->pixels[0] = -1;
	this->setColoursFromConfig();

	this->win = XCreateSimpleWindow(this->display,
		DefaultRootWindow(this->display),
		0, 0,
		this->screenWidth * this->fontWidth,
		this->screenHeight * this->fontHeight,
		0, 0,
		BlackPixel(this->display, screen) // default BG
	);

	this->gc = XCreateGC(this->display, this->win, 0, 0);

	// Swap the bits in the font as X11 wants them mirrored
	for (int i = 0; i < sizeof(int10_font_14); i++) {
		int10_font_14[i] =
			((int10_font_14[i] & 0x80) >> 7) |
			((int10_font_14[i] & 0x40) >> 5) |
			((int10_font_14[i] & 0x20) >> 3) |
			((int10_font_14[i] & 0x10) >> 1) |
			((int10_font_14[i] & 0x08) << 1) |
			((int10_font_14[i] & 0x04) << 3) |
			((int10_font_14[i] & 0x02) << 5) |
			((int10_font_14[i] & 0x01) << 7)
		;
	}
	this->font = XCreateBitmapFromData(this->display, this->win,
		(char *)int10_font_14, this->fontWidth, 256 * this->fontHeight
	);

	XSelectInput(this->display, this->win, KeyPressMask | ExposureMask | StructureNotifyMask);

	XMapRaised(this->display, this->win);

	int screensize = this->screenWidth * this->screenHeight;
	this->text    = new uint8_t[screensize];
	this->changed = new uint8_t[screensize];
	memset(this->text,    0, screensize);
	memset(this->changed, 0, screensize);
}

XConsole::~XConsole()
	throw ()
{
	delete[] this->changed;
	delete[] this->text;

	XDestroyWindow(this->display, this->win);
	XFreeGC(this->display, this->gc);
	XFreePixmap(this->display, this->font);

	int screen = DefaultScreen(this->display);
	Colormap cmap = XDefaultColormap(this->display, screen);
	XFreeColors(this->display, cmap, this->pixels, PX_TOTAL, 0);
}

void XConsole::setView(IViewPtr pView)
	throw ()
{
	this->nextView = pView;
	return;
}

void XConsole::mainLoop()
	throw ()
{
	assert(this->nextView);
	Key c;
	XEvent ev;
	do {
		c = Key_None;
		if (this->nextView) {
			this->pView = this->nextView;
			this->nextView.reset();
			this->pView->init();
			this->pView->redrawScreen();
			this->update();
		}

		if (XNextEvent(this->display, &ev) < 0) break;
		switch (ev.type) {
			case Expose:
				this->redrawCells(
					ev.xexpose.x / this->fontWidth,
					ev.xexpose.y / this->fontHeight,
					(ev.xexpose.x + ev.xexpose.width + this->fontWidth - 1) / this->fontWidth,
					(ev.xexpose.y + ev.xexpose.height + this->fontHeight - 1) / this->fontHeight,
					false // draw all cells, even unchanged ones
				);
				break;
			case KeymapNotify:
				XRefreshKeyboardMapping(&ev.xmapping);
				break;
			case KeyPress: {
				char ch[32];
				KeySym sym;
				int len = XLookupString(&ev.xkey, ch, sizeof(ch), &sym, NULL);
				switch (sym) {
					//case XK_BackSpace:
					case XK_Tab:       c = Key_Tab; break;
					//case XK_Return:    c = Key_Enter; break;
					case XK_Escape:    c = Key_Esc; break;
					case XK_Home:      c = Key_Home; break;
					case XK_Left:      c = Key_Left; break;
					case XK_Up:        c = Key_Up; break;
					case XK_Right:     c = Key_Right; break;
					case XK_Down:      c = Key_Down; break;
					case XK_Page_Up:   c = Key_PageUp; break;
					case XK_Page_Down: c = Key_PageDown; break;
					case XK_End:       c = Key_End; break;
					case XK_F1:        c = Key_F1; break;
					case XK_F10:       c = Key_F10; break;
					default:
						if (len == 1) c = (Key)ch[0];
						else c = Key_None;
						break;
				}
				if (ev.xkey.state & Mod1Mask) c = (Key)(c | Key_Alt);
				break;
			}
			case ConfigureNotify: {
				int newWidth = ev.xconfigure.width / this->fontWidth;
				int newHeight = ev.xconfigure.height / this->fontHeight;
				if ((this->screenWidth != newWidth) || (this->screenHeight != newHeight)) {
					this->screenWidth = newWidth;
					this->screenHeight = newHeight;
					delete[] this->text;
					delete[] this->changed;
					int screensize = newWidth * newHeight;
					this->text    = new uint8_t[screensize];
					this->changed = new uint8_t[screensize];
					memset(this->text,    0, screensize);
					memset(this->changed, 1, screensize);
					this->pView->init();
					this->pView->redrawScreen();
				}
				break;
			}
		}
	} while (this->pView->processKey(c));

	return;
}

void XConsole::update(void)
	throw ()
{
	this->redrawCells(0, 0, this->screenWidth, this->screenHeight, true);
	return;
}

void XConsole::clearStatusBar(SB_Y eY)
	throw ()
{
	int offset = (eY == SB_BOTTOM ? this->screenHeight - 1 : 0) * this->screenWidth;
	memset(this->text    + offset, 0, this->screenWidth);
	memset(this->changed + offset, 1, this->screenWidth);
	return;
}

void XConsole::setStatusBar(SB_Y eY, SB_X eX, const std::string& strMessage)
	throw ()
{
	int x;
	switch (eX) {
		case SB_CENTRE: x = (this->screenWidth + strMessage.length()) / 2; break;
		case SB_RIGHT:  x = this->screenWidth - strMessage.length(); break;
		default: // SB_LEFT
			x = 0;
			break;
	}
	int y;
	switch (eY) {
		case SB_BOTTOM:
			y = this->screenHeight - 1;
			break;
		default: // SB_TOP
			y = 0;
			break;
	}
	this->writeText(x, y, strMessage);
	return;
}

void XConsole::gotoxy(int x, int y)
	throw ()
{
	int oldX = this->cursorX, oldY = this->cursorY;
	this->cursorX = x;
	this->cursorY = y + 1; // doesn't count status bar
	if (this->cursorVisible) {
		this->redrawCells(oldX, oldY, oldX + 1, oldY + 1, false);
		this->redrawCells(this->cursorX, this->cursorY,
			this->cursorX + 1, this->cursorY + 1, false);
	}
	return;
}

void XConsole::putstr(const std::string& strContent)
	throw ()
{
	int len = strContent.length();
	this->writeText(this->cursorX, this->cursorY, strContent);
	this->cursorX += len % this->screenWidth;
	this->cursorY += len / this->screenWidth;
	return;
}

void XConsole::getContentDims(int *iWidth, int *iHeight)
	throw ()
{
	*iWidth = this->screenWidth;
	*iHeight = this->screenHeight - 2; // doesn't count status bars
	return;
}

void XConsole::scrollContent(int iX, int iY)
	throw ()
{
	int scrollSize = (this->screenHeight - 2 - abs(iY)) * this->screenWidth;
	if (iY < 0) {
		int start = (-iY + 1) * this->screenWidth;
		memmove(
			this->text + start,
			this->text + 1 * this->screenWidth,
			scrollSize
		);
		memset(this->changed + start, 1, scrollSize);
	} else if (iY > 0) {
		int start = 1 * this->screenWidth;
		memmove(
			this->text + start,
			this->text + (iY+1) * this->screenWidth,
			scrollSize
		);
		memset(this->changed + start, 1, scrollSize);
	}
	return;
}

void XConsole::eraseToEOL(void)
	throw ()
{
	int offset = this->cursorY * this->screenWidth + this->cursorX;
	int len = this->screenWidth - this->cursorX;
	memset(this->text    + offset, 0, len);
	memset(this->changed + offset, 1, len);
	return;
}

void XConsole::cursor(bool visible)
	throw ()
{
	this->cursorVisible = visible;
	this->redrawCells(this->cursorX, this->cursorY,
		this->cursorX + 1, this->cursorY + 1, false);
	return;
}

std::string XConsole::getString(const std::string& strPrompt, int maxLen)
	throw ()
{
	std::string s;
	// TODO
	return s;
}

void XConsole::setColoursFromConfig()
	throw ()
{
	int screen = DefaultScreen(this->display);
	Colormap cmap = XDefaultColormap(this->display, screen);
	if (this->pixels[0] != -1) {
		XFreeColors(this->display, cmap, this->pixels, PX_TOTAL, 0);
	}
	int clr;
	XColor c;
	for (int i = 0; i < PX_TOTAL; i++) {
		switch (i) {
			case PX_DOC_FG: clr = ::cfg.clrContent.iFG; break;
			case PX_DOC_BG: clr = ::cfg.clrContent.iBG; break;
			case PX_SB_FG:  clr = ::cfg.clrStatusBar.iFG; break;
			case PX_SB_BG:  clr = ::cfg.clrStatusBar.iBG; break;
			case PX_HL_FG:  clr = ::cfg.clrHighlight.iFG; break;
			case PX_HL_BG:  clr = ::cfg.clrHighlight.iBG; break;
		}
		c.flags = DoRed | DoGreen | DoBlue;
		c.red   = ((pal[clr] >> 8) & 0xFF00)     | ((pal[clr] >> 16) & 0xFF);
		c.green =  (pal[clr]       & 0xFF00)     | ((pal[clr] >> 8)  & 0xFF);
		c.blue  = ((pal[clr]       & 0xFF) << 8) |  (pal[clr]        & 0xFF);
		XAllocColor(this->display, cmap, &c);
		this->pixels[i] = c.pixel;
	}
	return;
}

void XConsole::redrawCells(int startX, int startY, int endX, int endY,
	bool changedOnly)
	throw ()
{
	int fore = -1;
	for (int y = startY; y < endY; y++) {
		if ((y == 0) || (y == this->screenHeight - 1)) {
			if (fore != PX_SB_FG) {
				XSetBackground(this->display, this->gc, this->pixels[PX_SB_BG]);
				XSetForeground(this->display, this->gc, this->pixels[PX_SB_FG]);
				fore = PX_SB_FG;
			}
		} else {
			if (fore != PX_DOC_FG) {
				XSetBackground(this->display, this->gc, this->pixels[PX_DOC_BG]);
				XSetForeground(this->display, this->gc, this->pixels[PX_DOC_FG]);
				fore = PX_DOC_FG;
			}
		}
		bool swapped = false; // did we swap colours to draw the cursor?
		for (int x = startX; x < endX; x++) {
			// If for some reason the drawing is out of range, skip it
			if ((x < this->screenWidth) && (y < this->screenHeight)) {
				if (this->cursorVisible && (this->cursorX == x) && (this->cursorY == y)) {
					XSetBackground(this->display, this->gc, this->pixels[fore]);
					XSetForeground(this->display, this->gc, this->pixels[fore+1]);
					swapped = true;
				} else if (swapped) {
					XSetBackground(this->display, this->gc, this->pixels[fore+1]);
					XSetForeground(this->display, this->gc, this->pixels[fore]);
					swapped = false;
				}
				// In range, draw the text
				int off = y * this->screenWidth + x;
				if (!changedOnly || this->changed[off]) {
					XCopyPlane(this->display, this->font, this->win, this->gc,
						0, this->text[off] * this->fontHeight, // source
						this->fontWidth, this->fontHeight,
						x * this->fontWidth, y * this->fontHeight, // dest
						1 // plane
					);
					this->changed[off] = 0;
				}
			} else {
				XFillRectangle(this->display, this->win, this->gc,
					x * this->fontWidth, y * this->fontHeight,
					this->fontWidth, this->fontHeight
				);
			}
		}
	}
	return;
}

void XConsole::writeText(int x, int y, const std::string& strContent)
	throw ()
{
	// Make sure this won't write past the end of the screen
	assert(y * this->screenWidth + x +
		strContent.length() <= this->screenWidth * this->screenHeight);

	int offset = y * this->screenWidth + x;
	uint8_t *start  = this->text    + offset;
	uint8_t *cstart = this->changed + offset;
	for (std::string::const_iterator i = strContent.begin(); i != strContent.end(); i++) {
		*start++ = *i;
		*cstart++ = 1;
	}
	return;
}

