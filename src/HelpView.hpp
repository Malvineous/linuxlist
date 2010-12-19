/**
 * @file   HelpView.hpp
 * @brief  TextView extension for showing help screen.
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

#ifndef HELPVIEW_HPP_
#define HELPVIEW_HPP_

#include <config.h>

#include <sstream>
#include <boost/shared_ptr.hpp>
#include "TextView.hpp"

/// Help screen.
class HelpView: public TextView
{
	private:
		static std::stringstream ss;
		IViewPtr oldView;

	public:
		HelpView(IViewPtr oldView, IConsole *pConsole)
			throw ();

		~HelpView()
			throw ();

		bool processKey(Key c)
			throw ();

		void generateHeader(std::ostringstream& ss)
			throw ();
};

#endif // HELPVIEW_HPP_
