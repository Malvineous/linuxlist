/**
 * @file   cfg.hpp
 * @brief  Storage of user configuration.
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

#ifndef CFG_HPP_
#define CFG_HPP_

struct CGAColour
{
	unsigned char iFG;
	unsigned char iBG;
};

enum InitialView {
	View_Text = 0,
	View_Hex  = 1,
};

struct Config
{
	CGAColour clrStatusBar;
	CGAColour clrContent;
	CGAColour clrHighlight;
	InitialView view;
};

extern Config cfg;

typedef unsigned char byte;

#endif // CFG_HPP_
