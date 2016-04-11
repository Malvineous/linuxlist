/**
 * @file   BaseConsole.hpp
 * @brief  Shared console functions.
 *
 * Copyright (C) 2009-2016 Adam Nielsen <malvineous@shikadi.net>
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

#ifndef BASECONSOLE_HPP_
#define BASECONSOLE_HPP_

#include <stdint.h>
#include "IConsole.hpp"
#include "IView.hpp"

/// Shared console functions.
class BaseConsole: virtual public IConsole
{
	public:
		BaseConsole();
		virtual ~BaseConsole();

		void setView(IViewPtr newView);
		void pushView(IViewPtr newView);
		void popView();
		std::string getString(const std::string& prompt, int maxLen);

		/// Provide view-independent key handling.
		/**
		 * This is used to handle user text entry in the same way for all views.
		 *
		 * @param c
		 *   Key to process.
		 */
		bool processKey(Key c);

	protected:
		ViewVector views;             ///< Views in use
		IViewPtr view;                ///< Currently active view (not yet in \ref views)
		IViewPtr nextView;            ///< If non-NULL, next view to replace \ref view

		enum Mode {
			Normal,                     ///< Pass keystrokes to view for action
			TextEntry,                  ///< Collect keystrokes into a string
		};
		Mode mode;                    ///< User input mode
		std::string textEntry;        ///< Text collected from user
		unsigned int textEntryPos;    ///< Cursor position within textEntry
		std::string textEntryPrompt;  ///< User prompt
		unsigned int textEntryMaxLen; ///< Maximum length of collected text, in chars
};

#endif // BASECONSOLE_HPP_
