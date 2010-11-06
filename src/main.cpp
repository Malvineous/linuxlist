/*
 * main.cpp - entry for Linux List
 *
 * Copyright (C) 2009 Adam Nielsen <malvineous@shikadi.net>
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
#include <sstream>
#include <iomanip>
#include <cassert>

#ifdef HAVE_NCURSESW_H
#include <ncursesw/ncurses.h>
#endif
#ifdef HAVE_ICONV
#include <iconv.h>
#endif

#include <errno.h>
#include <iostream> // for errors before we get to nCurses

#include <camoto/bitstream.hpp>

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))

struct CGAColour
{
	unsigned char iFG;
	unsigned char iBG;
};
struct Config
{
	CGAColour clrStatusBar;
	CGAColour clrContent;
	CGAColour clrHighlight;
};
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
Config cfg;

typedef unsigned char byte;

// Position of text on the status bars
enum SB_Y {
	SB_TOP = 0,
	SB_BOTTOM = 1
};
enum SB_X {
	SB_LEFT,
	SB_CENTRE,
	SB_RIGHT
};

class IConsole
{
	public:
		virtual ~IConsole()
			throw ()
		{
		}

		// Update the content area (not status bars) after changes have been made
		virtual void update(void)
			throw () = 0;

		/*// Same as update but only affects the status bars
		virtual void updateStatusBars(void)
			throw () = 0;*/

		// Blank out the text on the specified status bar.  Not shown until update()
		virtual void clearStatusBar(SB_Y eY)
			throw () = 0;

		// Set the content of the top status bar.  Not shown until next update().
		virtual void setStatusBar(SB_Y eY, SB_X eX, const std::string& strMessage)
			throw () = 0;

		//
		// Display routines
		//
		virtual void gotoxy(int x, int y) throw () = 0;
		virtual void putstr(const std::string& strContent) throw () = 0;
		virtual void getContentDims(int *iWidth, int *iHeight) throw () = 0;
		virtual void scrollContent(int iX, int iY) throw () = 0;
		virtual void eraseToEOL(void) throw () = 0; // erase from cursor pos to end of line

};
class NCursesConsole: virtual public IConsole
{
	private:
		// The status bars need to be separate windows, otherwise updating them
		// will overwrite the content window!
		WINDOW *winStatus[2]; // 0 == top, 1 == bottom
		WINDOW *winContent; // main display area (between the status bars)

		iconv_t cd;

// Colour pairs
#define CLR_STATUSBAR 1
#define CLR_CONTENT 2
		attr_t iAttribute[3]; // attributes (bold etc.) for matching colour pairs

	public:
		NCursesConsole(void)
			throw ()
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
				mvwprintw(this->winStatus[i], 0, 50, "%s", "Content goes here");
			}

			// Create the main window between the status bars
			this->winContent = newwin(LINES - 2, COLS, 1, 0);
			wattrset(this->winContent,
				COLOR_PAIR(CLR_CONTENT) | this->iAttribute[CLR_CONTENT]);
			wbkgdset(this->winContent, COLOR_PAIR(CLR_CONTENT));
			wclear(this->winContent);
			//mvwprintw(this->winContent, 0, 0, "%s", "Content goes here");

			idlok(this->winContent, TRUE); // enable efficient scrolling
			scrollok(this->winContent, TRUE); // Allow scrolling

			// Annoyingly, we have to do a refresh() here even though we haven't
			// finished preparing the output.  If this is left out, the screen
			// never gets drawn :-(  Also dotting getch() calls throughout the init
			// code can have a similar effect.  Isn't dodgy code great?
			refresh();

			/*wrefresh(this->winContent);
			wnoutrefresh(this->winStatus[0]);
			wnoutrefresh(this->winStatus[1]);
			//wnoutrefresh(this->winContent);
			doupdate();*/
			wnoutrefresh(this->winContent);

			//refresh();
			// Set status bar attributes
			//attrset(COLOR_PAIR(CLR_STATUSBAR) | this->iAttribute[CLR_STATUSBAR]);
			//mvprintw(0, 0, "%s", "This is a test");
			//mvchgat(0, 0, -1, this->iAttribute[CLR_STATUSBAR], CLR_STATUSBAR, NULL);
			//mvchgat(LINES-1, 0, -1, this->iAttribute[CLR_STATUSBAR], CLR_STATUSBAR, NULL);


			//print_in_middle(stdscr, LINES / 2, 0, 0, "Viola !!! In color ...");
			//attroff(COLOR_PAIR(1));

		}

		~NCursesConsole()
			throw ()
		{
			delwin(this->winContent);

			// Restore the terminal
			endwin();

			iconv_close(this->cd);
		}

		void update(void)
			throw ()
		{
			wnoutrefresh(this->winStatus[0]);
			wnoutrefresh(this->winStatus[1]);
			wnoutrefresh(this->winContent);
			// Copy the virtual screen onto the terminal
			doupdate();
		}

		void clearStatusBar(SB_Y eY)
			throw ()
		{
			wclear(this->winStatus[eY]);
			return;
		}

		void setStatusBar(SB_Y eY, SB_X eX, const std::string& strMessage)
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

		void gotoxy(int x, int y)
			throw ()
		{
			wmove(this->winContent, y, x);
			return;
		}

		void putstr(const std::string& strContent)
			throw ()
		{
			if (this->cd != (iconv_t)-1) {
				// iconv is valid, convert
				wchar_t *wout = new wchar_t[256];
				char *pIn = (char *)strContent.c_str();
				char *pOut = (char *)wout;
				size_t inleft = strContent.length(), outleft;
				if (iconv(cd, &pIn, &inleft, &pOut, &outleft) == (size_t)-1) {
					perror("iconv");
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

		void getContentDims(int *iWidth, int *iHeight)
			throw ()
		{
			getmaxyx(this->winContent, *iHeight, *iWidth);
			return;
		}

		void scrollContent(int iX, int iY)
			throw ()
		{
			wscrl(this->winContent, iY);
			return;
		}

		void eraseToEOL(void)
			throw ()
		{
			wclrtoeol(this->winContent);
			return;
		}

};

// Something to display on the screen (file content, directory browser, etc.)
class IView
{
	public:
		// Process the given key.  Returns true to keep going, false to quit.
		virtual bool processKey(int c)
			throw () = 0;
};

class HexView: virtual public IView
{
	std::string strFilename;
	camoto::bitstream file;
	IConsole *pConsole;
	bool bStatusAlertVisible;
	int iLineWidth;
	int *pLineBuffer;
	int bitWidth;             ///< Number of bits in each char/cell
	int intraByteOffset;      ///< Bit-level seek offset within cell (0..bitWidth-1)

	std::fstream::off_type iOffset; // Offset into file of first character in content window
	std::fstream::off_type iFileSize;

	public:
		HexView(std::string strFilename, camoto::iostream_sptr file,
			std::fstream::off_type iFileSize, IConsole *pConsole)
			throw () :
				strFilename(strFilename),
				file(file, camoto::bitstream::littleEndian),
				pConsole(pConsole),
				bStatusAlertVisible(true), // trigger an update when next set
				iLineWidth(16),
				pLineBuffer(NULL),
				iOffset(0),
				iFileSize(iFileSize),
				bitWidth(8),
				intraByteOffset(0)
		{
			// Draw the first page of data
			this->pConsole->setStatusBar(SB_TOP, SB_LEFT, strFilename);
			this->statusAlert(NULL); // draw the bottom status bar with no alert
			//this->pConsole->setStatusBar(SB_BOTTOM, SB_LEFT, " Command>  *** End of file ***");

			this->pLineBuffer = new int[this->iLineWidth];

			int iWidth, iHeight;
			this->pConsole->getContentDims(&iWidth, &iHeight);
			this->redrawLines(0, iHeight);

			this->pConsole->update();
//			this->pConsole->updateContent();
		}

		~HexView()
			throw ()
		{
			assert(this->pLineBuffer != NULL);
			delete[] this->pLineBuffer;
		}

		bool processKey(int c)
			throw ()
		{
			int iWidth, iHeight;
			this->pConsole->getContentDims(&iWidth, &iHeight);
			this->statusAlert(NULL);
			switch (c) {
				case 'q': return false;
				case '-': if (this->iLineWidth > 1) this->iLineWidth--; this->redrawScreen(); break;
				case '+': if (this->iLineWidth < (iWidth - 10 - (this->iLineWidth / 8)) / 4) this->iLineWidth++; this->redrawScreen(); break;
				case 'b': this->setBitWidth(max(1, this->bitWidth - 1)); break;
				case 'B': this->setBitWidth(min(sizeof(int)*8, this->bitWidth + 1)); break;
				case 's': this->setIntraByteOffset(-1); break;
				case 'S': this->setIntraByteOffset(1); break;
				case 'e': this->file.changeEndian(camoto::bitstream::littleEndian); this->redrawScreen(); break;
				case 'E': this->file.changeEndian(camoto::bitstream::bigEndian); this->redrawScreen(); break;
				case KEY_UP: this->scrollRel(-this->iLineWidth); break;
				case KEY_DOWN: this->scrollRel(this->iLineWidth); break;
				case KEY_LEFT: this->scrollRel(-1); break;
				case KEY_RIGHT: this->scrollRel(1); break;
				case KEY_PPAGE: this->scrollRel(-this->iLineWidth*iHeight); break;
				case KEY_NPAGE: this->scrollRel(this->iLineWidth*iHeight); break;
				case KEY_HOME: this->scrollAbs(0); break;
				case KEY_END: {
					int fileSizeInCells = (this->iFileSize * 8 / this->bitWidth);
					int iLastLineLen = fileSizeInCells % this->iLineWidth;
					if (iLastLineLen == 0) iLastLineLen = this->iLineWidth;
					this->scrollAbs(fileSizeInCells - iLastLineLen -
						(iHeight - 1) * this->iLineWidth);
					break;
				}
				default: break;
			}
			this->pConsole->update();
			return true; // true == keep going (don't quit)
		}

		// Set an alert message on the status bar, or remove one if cMsg == NULL
		void statusAlert(const char *cMsg)
		{
			// If there's no status message and a blank has been requested, do nothing.
			if ((!cMsg) && (!bStatusAlertVisible)) return;

			// Blank out the bottom statusbar to hide the old status message
			this->pConsole->clearStatusBar(SB_BOTTOM);

			// Reset the right-hand side after we've blanked it
			this->pConsole->setStatusBar(SB_BOTTOM, SB_RIGHT, "F1=help");

			if (cMsg) {
				this->pConsole->setStatusBar(SB_BOTTOM, SB_LEFT, std::string("Command>  *** ") + cMsg + " *** ");
				bStatusAlertVisible = true;
			} else {
				this->pConsole->setStatusBar(SB_BOTTOM, SB_LEFT, "Command> ");
				bStatusAlertVisible = false;
			}
			return;
		}

		void scrollAbs(unsigned long iNewOffset)
			throw ()
		{
			this->scrollRel(iNewOffset - this->iOffset);
			return;
		}

		void scrollRel(int iDelta)
			throw ()
		{
			if (iDelta == 0) return; // e.g. pressing Home twice

			int iWidth, iHeight;
			this->pConsole->getContentDims(&iWidth, &iHeight);
			int iScreenSize = iHeight * this->iLineWidth;

			//
			// Check and enforce scrolling limits
			//

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

				// If the scroll operation will show past the end of the file,
				// display a notice to the user.
				if (this->iOffset + iScreenSize + iDelta > this->iFileSize) {
					this->statusAlert("End of file");
				}

				// But prevent them from actually scrolling past the last byte.
				if (this->iOffset >= this->iFileSize - 1) return;

				// Crop the scroll so that it only goes up to the last byte, not past it
				if (this->iOffset + iDelta > this->iFileSize) {
					if (iDelta % this->iLineWidth == 0) {
						// The user is scrolling by lines, so crop at the line level
						unsigned long iMaxBytes = this->iFileSize - this->iOffset;
						iDelta = iMaxBytes - (iMaxBytes % this->iLineWidth);
						if (iDelta == 0) return; // can't scroll down by a whole line without going past EOF
					} else {
						// The user is scrolling by chars, so allow them to go right up to
						// the last byte.
						iDelta = (this->iFileSize - 1) - this->iOffset;
					}
				}

			}
			assert(iDelta != 0);

			// If we're here, then iDelta is within limits and won't scroll too far
			// in either direction.

			// See if we're scrolling by part of a line
			int iChars = iDelta % this->iLineWidth;
			if (iChars) {
				// We're scrolling by a partial line, so the whole screen will need
				// to be redrawn regardless of how far we scroll.
				this->iOffset += iDelta;
				this->redrawLines(0, iHeight);
				return;
			}

			// No, we're only scrolling by a multiple of exact lines.
			int iLines = iDelta / this->iLineWidth;
			assert(iLines != 0);
			if (abs(iLines) > iHeight) {
				// But we're scrolling by more than a screenful, so we'll need to
				// redraw the whole screen anyway.
				this->iOffset += iDelta;
				this->redrawLines(0, iHeight);
				return;
			}

			// If we're here, then we're scrolling by only a handful of lines, so
			// try to do it efficiently.
			this->pConsole->scrollContent(0, iLines);
			this->iOffset += iDelta;
			if (iLines < 0) {
				this->redrawLines(0, -iLines);
			} else {
				this->redrawLines(iHeight-iLines, iHeight);
			}

			return;
		}

		void redrawLines(int iTop, int iBottom)
		{
			int y = iTop;
			std::fstream::off_type iCurOffset = this->iOffset + iTop * this->iLineWidth;
			file.clear(); // clear any errors (e.g. reaching EOF previously)
			file.seek(iCurOffset * this->bitWidth + this->intraByteOffset, std::ios::beg);
			if (iCurOffset < this->iFileSize) {
				for (; y < iBottom; y++) {

					// Don't read past the end of the file
					int iRead;
					for (iRead = 0; iRead < this->iLineWidth; iRead++) {
						if (!file.read(this->bitWidth, &this->pLineBuffer[iRead])) break;
					}

					this->drawLine(y, iOffset + (y * this->iLineWidth),
						this->pLineBuffer, iRead);
					if (iRead < this->iLineWidth) {
						y++;
						break; // EOF
					}
				}
			}
			// Blank out any leftover lines
			for (; y < iBottom; y++) {
				this->pConsole->gotoxy(0, y);
				//this->pConsole->putstr("blankline");
				this->pConsole->eraseToEOL();
			}
			return;
		}

		// Display the file data (starting at the current offset) in the content
		// window.
		void drawLine(int iLine, unsigned long iOffset, const int *pData, int iLen)
			throw ()
		{
			this->pConsole->gotoxy(0, iLine);
			std::ostringstream osHex;
			std::ostringstream osBin;
			//ostr.setf(std::ios::hex, std::ios::basefield);

			// Offset display (left)
			osHex << std::hex << std::setiosflags(std::ios_base::uppercase)
				<< std::setfill('0') << std::setw(8)
				<< iOffset << " ";

			// Number of chars wide each num is (e.g. 9-bit nums are three chars wide)
			int cellNumberWidth = (this->bitWidth + 3) / 4;

			const int *pb = pData;
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
			//this->pConsole->putstr("\n12345678901234567890123456789012345678901234567890123456789012345678901234567890");
			//this->pConsole->putstr("test");
			return;
		}

		/// Set the size of each cell in bits.
		/**
		 * When this is set to eight, a normal byte-level view will be shown.
		 */
		void setBitWidth(int newWidth)
		{
			// Since the first bit on the screen should stay the same after
			// this change, we need to adjust offsets.
			int bitOffset = this->iOffset * this->bitWidth + this->intraByteOffset;

			this->bitWidth = newWidth;
			this->intraByteOffset = bitOffset % this->bitWidth;
			this->iOffset = bitOffset / this->bitWidth;

			assert(bitOffset == this->iOffset * this->bitWidth + this->intraByteOffset);

			this->redrawScreen();
			return;
		}

		/// Set the bit-level offset within the cell.
		/**
		 * Using eight bit bytes as an example, when this is set to zero bytes
		 * will be shown as normal.  When this is set to 1, the first *bit* in
		 * the file will be discarded and the following eight bits (last seven
		 * bits in the first input byte, and first bit in the second input byte)
		 * will appear as the first byte on the screen.
		 */
		void setIntraByteOffset(int delta)
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

		/// Regenerate the entire content on the display.
		/**
		 * This is normally only called after a change that affects the entire
		 * display, e.g. a change in the number of bits shown per byte.
		 */
		void redrawScreen()
		{
			int iWidth, iHeight;
			this->pConsole->getContentDims(&iWidth, &iHeight);

			std::ostringstream ss;
			ss << "Offset: " << this->iOffset << '+' << this->intraByteOffset
				<< "b  Cell size: " << this->bitWidth
				<< "b/";
			if (this->file.getEndian() == camoto::bitstream::littleEndian) {
				ss << "LE";
			} else {
				ss << "BE";
			}
			ss << "  Width: " << this->iLineWidth;
			this->pConsole->setStatusBar(SB_TOP, SB_RIGHT, ss.str());

			this->redrawLines(0, iHeight);
			return;
		}

};

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

	IConsole *pConsole = new NCursesConsole();

	//std::string strFilename = "/home/adam/dos/ctl.txt";
	std::string strFilename = cArgV[1];
	camoto::iostream_sptr fsFile(new std::fstream(strFilename.c_str(), std::fstream::binary | std::fstream::in | std::fstream::out));

	fsFile->seekg(0, std::ios::end);
	std::fstream::off_type iFileSize = fsFile->tellg();

	IView *pView = new HexView(strFilename, fsFile, iFileSize, pConsole);

	int c;
	do {
		c = getch();
	} while (pView->processKey(c));

	delete pView;
	delete pConsole;

	return 0;
}
