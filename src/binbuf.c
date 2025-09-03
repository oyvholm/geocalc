/*
 * binbuf.c
 * File ID: a9c04cc4-98be-11ef-a370-83850402c3ce
 *
 * (C)opyleft 2024- Øyvind A. Holm <sunny@sunbase.org>
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU General Public License as published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option) 
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for 
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with 
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "geocalc.h"

/*
 * binbuf_init() - Prepares a `struct binbuf` for use, returns nothing.
 */

void binbuf_init(struct binbuf *sb)
{
	assert(sb);
	sb->alloc = sb->len = 0;
	sb->buf = NULL;
}

/*
 * binbuf_free() - Deallocates a `struct binbuf` and sets the struct values to 
 * their initial state.
 */

void binbuf_free(struct binbuf *sb)
{
	assert(sb);
	if (sb->alloc)
		free(sb->buf);
	binbuf_init(sb);
}

/*
 * binbuf_cpy() - Creates a binbuf copy of `src` and stores it in `dest`. 
 * Returns a char pointer to `dest->buf` if successful, or NULL if allocation 
 * failed or `dest` or `src` are NULL.
 */

char *binbuf_cpy(struct binbuf *dest, const struct binbuf *src)
{
	struct binbuf sb;

	assert(dest);
	assert(src);
	if (!dest || !src) {
		myerror("%s(): `dest` or `src` is NULL", __func__); /* gncov */
		return NULL; /* gncov */
	}
	binbuf_init(&sb);
	sb.alloc = src->alloc;
	sb.buf = malloc(sb.alloc);
	if (!sb.buf) {
		failed("malloc()"); /* gncov */
		return NULL; /* gncov */
	}
	sb.len = src->len;
	if (sb.len > sb.alloc) {
		myerror("%s(): sb.len (%zu) is larger than" /* gncov */
		        " sb.alloc (%zu)",
		        __func__, sb.len, sb.alloc);
		return NULL; /* gncov */
	}
	memcpy(sb.buf, src->buf, src->alloc);
	*dest = sb;

	return dest->buf;
}

/* vim: set ts=8 sw=8 sts=8 noet fo+=w tw=79 fenc=UTF-8 : */
