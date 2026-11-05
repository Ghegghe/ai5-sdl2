/* Copyright (C) 2026 Ghegghe
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://gnu.org/licenses/>.
 */

#ifndef AI5_SDL2_CHARMAP_H
#define AI5_SDL2_CHARMAP_H

#include <stddef.h>
#include <stdint.h>
#include "nulib/string.h"

/*
 * Character remapping for fan translations.
 *
 * Shift-JIS has no accented Latin letters, so a translation can encode them
 * as half-width katakana (bytes 0xA0-0xDF), which a Latin-script text never
 * uses, and supply a charmap file that tells the engine which character to
 * draw for each of those bytes. Without a charmap nothing changes.
 *
 * File format (UTF-8): one mapping per line, "<hex byte>=<character>",
 * e.g. "B1=à". Blank lines and lines starting with '#' are ignored.
 */

void charmap_init(const char *path);
uint32_t charmap_apply(uint32_t ch);
string charmap_sjis_cstring_to_utf8(const char *src, size_t len);

#endif // AI5_SDL2_CHARMAP_H
