/*
 * string.c
 * File ID: f7f16c5c-96b1-11ef-bd3d-83850402c3ce
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
 * string_to_double() - Converts a number from `char *` to `double` and checks 
 * for errors. If any error occurs, it sets `errno` and returns 1, otherwise it 
 * returns 0.
 */

int string_to_double(const char *s, double *dest)
{
	char *endptr;
	errno = 0;

	*dest = strtod(s, &endptr);

	if (errno == ERANGE) {
		/* Number is too large or small */
		return 1;
	}

	if (endptr == s) {
		/* No valid conversion possible */
		errno = EINVAL;
		return 1;
	}

	/*
	 * Check for extra characters after the number, whitespace and `,` are 
	 * allowed for the time being in case it's copy+paste.
	 */
	while (*endptr != '\0') {
		if (*endptr != ',' && !isspace((unsigned char)*endptr)) {
			errno = EINVAL;
			return 1;
		}
		endptr++;
	}

	if (isnan(*dest)) {
		errno = EINVAL;
		return 1;
	}

	if (isinf(*dest)) {
		errno = ERANGE;
		return 1;
	}

	return 0;
}

/* vim: set ts=8 sw=8 sts=8 noet fo+=w tw=79 fenc=UTF-8 : */
