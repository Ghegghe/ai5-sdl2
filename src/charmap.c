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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "nulib.h"
#include "nulib/file.h"
#include "nulib/string.h"
#include "nulib/utfsjis.h"

#include "charmap.h"

// Half-width katakana: Shift-JIS bytes 0xA0-0xDF decode to U+FF60-U+FF9F.
#define CHARMAP_FIRST_BYTE 0xa0
#define CHARMAP_LAST_BYTE  0xdf
#define CHARMAP_FIRST_CP   0xff60
#define CHARMAP_SIZE (CHARMAP_LAST_BYTE - CHARMAP_FIRST_BYTE + 1)

static uint32_t charmap[CHARMAP_SIZE] = {0};
static bool charmap_loaded = false;

// Decode one UTF-8 character. Returns its length in bytes, or 0 if invalid.
static unsigned utf8_decode(const uint8_t *s, uint32_t *out)
{
	if (s[0] < 0x80) {
		*out = s[0];
		return s[0] ? 1 : 0;
	}
	unsigned len;
	uint32_t cp;
	if ((s[0] & 0xe0) == 0xc0) {
		len = 2;
		cp = s[0] & 0x1f;
	} else if ((s[0] & 0xf0) == 0xe0) {
		len = 3;
		cp = s[0] & 0x0f;
	} else if ((s[0] & 0xf8) == 0xf0) {
		len = 4;
		cp = s[0] & 0x07;
	} else {
		return 0;
	}
	for (unsigned i = 1; i < len; i++) {
		if ((s[i] & 0xc0) != 0x80)
			return 0;
		cp = (cp << 6) | (s[i] & 0x3f);
	}
	*out = cp;
	return len;
}

static char *trim(char *s)
{
	while (isspace((unsigned char)*s))
		s++;
	char *end = s + strlen(s);
	while (end > s && isspace((unsigned char)end[-1]))
		end--;
	*end = '\0';
	return s;
}

static void parse_line(const char *path, unsigned line_no, char *line)
{
	line = trim(line);
	if (!*line || *line == '#')
		return;

	char *eq = strchr(line, '=');
	if (!eq) {
		WARNING("%s:%u: expected \"<hex byte>=<character>\"", path, line_no);
		return;
	}
	*eq = '\0';
	char *key = trim(line);
	char *val = trim(eq + 1);

	char *endptr;
	long b = strtol(key, &endptr, 16);
	if (!*key || *endptr || b < CHARMAP_FIRST_BYTE || b > CHARMAP_LAST_BYTE) {
		WARNING("%s:%u: byte must be between A0 and DF: \"%s\"", path, line_no, key);
		return;
	}

	uint32_t cp;
	unsigned len = utf8_decode((const uint8_t*)val, &cp);
	if (!len || val[len] != '\0') {
		WARNING("%s:%u: expected a single UTF-8 character: \"%s\"", path, line_no, val);
		return;
	}
	charmap[b - CHARMAP_FIRST_BYTE] = cp;
}

void charmap_init(const char *path)
{
	if (!path)
		return;

	size_t size;
	char *data = file_read(path, &size);
	if (!data) {
		WARNING("Failed to read charmap \"%s\"", path);
		return;
	}

	char *text = xmalloc(size + 1);
	memcpy(text, data, size);
	text[size] = '\0';
	free(data);

	// skip UTF-8 BOM
	char *p = text;
	if (size >= 3 && !memcmp(p, "\xef\xbb\xbf", 3))
		p += 3;

	unsigned line_no = 0;
	while (p) {
		char *next = strchr(p, '\n');
		if (next)
			*next++ = '\0';
		parse_line(path, ++line_no, p);
		p = next;
	}
	free(text);
	charmap_loaded = true;
}

uint32_t charmap_apply(uint32_t ch)
{
	if (!charmap_loaded || ch < CHARMAP_FIRST_CP || ch >= CHARMAP_FIRST_CP + CHARMAP_SIZE)
		return ch;
	uint32_t mapped = charmap[ch - CHARMAP_FIRST_CP];
	return mapped ? mapped : ch;
}

static string push_utf8(string dst, uint32_t c)
{
	char buf[4];
	size_t len;
	if (c <= 0x7f) {
		buf[0] = c;
		len = 1;
	} else if (c <= 0x7ff) {
		buf[0] = 0xc0 | c >> 6;
		buf[1] = 0x80 | (c & 0x3f);
		len = 2;
	} else if (c <= 0xffff) {
		buf[0] = 0xe0 | c >> 12;
		buf[1] = 0x80 | (c >> 6 & 0x3f);
		buf[2] = 0x80 | (c & 0x3f);
		len = 3;
	} else {
		buf[0] = 0xf0 | c >> 18;
		buf[1] = 0x80 | (c >> 12 & 0x3f);
		buf[2] = 0x80 | (c >> 6 & 0x3f);
		buf[3] = 0x80 | (c & 0x3f);
		len = 4;
	}
	return string_concat_len(dst, buf, len);
}

string charmap_sjis_cstring_to_utf8(const char *src, size_t len)
{
	if (!charmap_loaded)
		return sjis_cstring_to_utf8(src, len);

	const char *end = src + len;
	string dst = string_new("");
	while (src < end && *src) {
		int c;
		src = sjis_char2unicode(src, &c);
		dst = push_utf8(dst, charmap_apply(c));
	}
	return dst;
}
