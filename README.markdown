Linux List - a Linux clone of Vernon D. Buerg's List file viewer  
Copyright 2009-2015 Adam Nielsen <malvineous@shikadi.net>  
http://www.shikadi.net/ll

This program is still very early in development, so there is still much to
implement before it becomes as useful as the original List.

Features:

 * Hex viewer, with full 8-bit CP437 glyphs (UTF8 console required), so that
   binary files look like they did under DOS.

 * Adjustable byte size.  Viewing data as nine-bits per "byte" helps
   considerably when examining LZW-compressed data.  Both big and little
   endian splitting is supported.

 * Hex view supports hex editing (via byte values and direct text entry)

The utility is compiled and installed in the usual way:

  ./configure && make
  sudo make install

You will need libgamecommon installed first.  If you downloaded the git
release, run ./autogen.sh before the commands above.

This program is released under the GPLv3 license.

![Screenshot of hex view](http://www.shikadi.net/gfx/ll/ll-hexview.png)
