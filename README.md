Linux List
----------
A Linux clone of Vernon D. Buerg's List file viewer  
Copyright 2009-2016 Adam Nielsen <malvineous@shikadi.net>  
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

 * Can seek at the byte level or the bit level, which is useful for tracing
   algorithms that operate on a stream of bits rather than on bytes.

The utility is compiled and installed in the usual way:

    ./autogen.sh          # Only if compiling from git
    ./configure && make
    sudo make install

You will need the following prerequisites already installed:

  * [libgamecommon](https://github.com/Malvineous/libgamecommon) >= 2.0

This program is released under the GPLv3 license.

![Screenshot of hex view](http://www.shikadi.net/wiki/main/images/e/ed/ll-xwindow.png)
