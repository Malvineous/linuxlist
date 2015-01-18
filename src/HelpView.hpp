/**
 * @file   HelpView.hpp
 * @brief  TextView extension for showing help screen.
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

#ifndef HELPVIEW_HPP_
#define HELPVIEW_HPP_

#include <sstream>
#include "TextView.hpp"

/// Help screen.
class HelpView: public TextView
{
	public:
		HelpView(IConsole *pConsole);
		~HelpView();

		bool processKey(Key c);
		void generateHeader(std::ostringstream& ss);

	protected:
		static std::stringstream ss;
};

#endif // HELPVIEW_HPP_
