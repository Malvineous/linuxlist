/**
 * @file   IView.hpp
 * @brief  Interface class for a 'view' - something to show on the screen.
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

#ifndef IVIEW_HPP_
#define IVIEW_HPP_

#include <boost/shared_ptr.hpp>

/// Keycode definitions.  Values < 256 are ASCII values.
enum Key {
	Key_None = 0,
	Key_Up = 256,
	Key_Down,
	Key_Left,
	Key_Right,
	Key_PageUp,
	Key_PageDown,
	Key_Home,
	Key_End,
	Key_Esc,
	Key_Tab,
};

/// Something to display on the screen (file content, directory browser, etc.)
class IView
{
	public:
		/// Process the given key.
		/**
		 * @param c
		 *   Key to react to.
		 *
		 * @return true to keep going, false to quit.
		 */
		virtual bool processKey(Key c)
			throw () = 0;

		/// Regenerate the entire content on the display.
		/**
		 * This is normally only called after a change that affects the entire
		 * display, e.g. a change in the number of bits shown per byte.
		 */
		virtual void redrawScreen()
			throw () = 0;

};

typedef boost::shared_ptr<IView> IViewPtr;

#endif // IVIEW_HPP_
