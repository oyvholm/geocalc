/*
 * io.c
 * File ID: afb87148-98c7-11ef-85f1-83850402c3ce
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
 * read_from_fp() - Read data from fp into an allocated buffer and return a 
 * pointer to the allocated memory or NULL if something failed.
 */

char *read_from_fp(FILE *fp, struct binbuf *dest)
{
	struct binbuf buf;
	size_t bufsize = BUFSIZ;

	assert(fp);

	binbuf_init(&buf);

	do {
		char *p = NULL;
		char *new_mem = realloc(buf.buf, bufsize + buf.len);
		size_t bytes_read;

		if (!new_mem) {
			myerror("%s(): Cannot allocate" /* gncov */
			        " memory for stream buffer", __func__);
			binbuf_free(&buf); /* gncov */
			return NULL; /* gncov */
		}
		buf.alloc = bufsize + buf.len;
		buf.buf = new_mem;
		p = buf.buf + buf.len;
		bytes_read = fread(p, 1, bufsize - 1, fp);
		buf.len += bytes_read;
		p[bytes_read] = '\0';
		if (ferror(fp)) {
			myerror("%s(): Read error", __func__); /* gncov */
			binbuf_free(&buf); /* gncov */
			return NULL; /* gncov */
		}
	} while (!feof(fp));

	if (dest)
		*dest = buf;

	return buf.buf;
}

/* vim: set ts=8 sw=8 sts=8 noet fo+=w tw=79 fenc=UTF-8 : */
