/**
 * @file   BaseConsole.cpp
 * @brief  Common console functions.
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

#include "BaseConsole.hpp"

BaseConsole::BaseConsole()
	:	mode(Normal)
{
}

BaseConsole::~BaseConsole()
{
}

void BaseConsole::setView(IViewPtr newView)
{
	if (this->view) {
		// We need to ensure this->view is kept valid because we're likely to return
		// to it after exiting this function.
		this->nextView = newView;
	} else {
		this->view = newView;
		this->view->init();
		this->view->redrawScreen();
		this->update();
	}
	return;
}

void BaseConsole::pushView(IViewPtr newView)
{
	this->views.push_back(this->view);
	this->setView(newView);
	return;
}

void BaseConsole::popView()
{
	this->setView(this->views.back());
	this->views.pop_back();
	return;
}

std::string BaseConsole::getString(const std::string& prompt, int maxLen)
{
	this->textEntry.clear();
	this->textEntryPos = 0;
	this->textEntryPrompt = prompt;
	this->textEntryMaxLen = maxLen;
	this->mode = TextEntry;

	this->view->updateTextEntry(this->textEntryPrompt, this->textEntry,
		this->textEntryPos);
	this->update();

	// Loop, calling processKey(), until processKey() returns false
	this->mainLoop();

	this->mode = Normal;
	return this->textEntry;
}

bool BaseConsole::processKey(Key c)
{
	bool ret;
	switch (this->mode) {
		case Normal:
			// Pass the keystroke onto the view.
			ret = this->view->processKey(c);
			break;
		case TextEntry:
			ret = true; // true == keep going (don't quit)
			if (c == Key_None) break; // ignore non-keys
			switch (c) {
				case Key_Esc:
					this->textEntry.clear();
					this->textEntryPos = 0;
					// fall through
				case Key_Enter:
					this->view->clearTextEntry();
					this->update();
					return false; // false == done
				case Key_Left:
					if (this->textEntryPos > 0) this->textEntryPos--;
					break;
				case Key_Right:
					if (this->textEntryPos < this->textEntry.length()) this->textEntryPos++;
					break;
				case Key_Backspace:
					if (this->textEntryPos > 0) {
						this->textEntry.erase(--this->textEntryPos, 1);
					}
					break;
				case Key_Del:
					if (this->textEntryPos < this->textEntry.length()) {
						this->textEntry.erase(this->textEntryPos, 1);
					}
					break;
				case Key_Home:
					this->textEntryPos = 0;
					break;
				case Key_End:
					this->textEntryPos = this->textEntry.length();
					break;
				default:
					if ((c > 31) && (c < 127) && (this->textEntry.length() < this->textEntryMaxLen)) {
						this->textEntry.insert(this->textEntryPos++, 1, c);
					}
					break;
			}
			this->view->updateTextEntry(this->textEntryPrompt, this->textEntry,
				this->textEntryPos);
			this->update();
			break;
	}
	if (this->nextView) {
		this->view = this->nextView;
		this->nextView.reset();
		this->view->init();
		this->view->redrawScreen();
		this->update();
	}
	return ret;
}
