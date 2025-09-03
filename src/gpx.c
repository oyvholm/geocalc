/*
 * gpx.c
 * File ID: a4ab90e6-a092-11ef-ab3c-83850402c3ce
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
 * xml_escape_string() - Returns pointer to allocated string where the data in 
 * `text` is escaped for use in XML files.
 */

char *xml_escape_string(const char *text)
{
	char *retval, *destp;
	const char *p;
	size_t size;

	if (!text)
		return NULL;

	size = strlen(text);
	retval = malloc(size * 5 + 1); /* Worst case, only ampersands. */
	if (!retval)
		return NULL; /* gncov */

	destp = retval;
	for (p = text; *p; p++) {
		switch (*p) {
		case '&':
			strcpy(destp, "&amp;");
			destp += 5;
			break;
		case '<':
			strcpy(destp, "&lt;");
			destp += 4;
			break;
		case '>':
			strcpy(destp, "&gt;");
			destp += 4;
			break;
		default:
			*destp++ = *p;
			break;
		}
	}
	*destp = '\0';

	return retval;
}

/*
 * gpx_wpt() - Returns a pointer to an allocated string with a GPX waypoint. 
 * `name` is shown on the map, and `cmt` is a short description of the 
 * waypoint. To suppress the `<cmt>` element, set `cmt` to NULL. `name` and 
 * `cmt` are converted to XML-safe strings with xml_escape_string(). Returns 
 * pointer to the allocated string if successful, or NULL if `name` is NULL or 
 * any allocations failed.
 */

char *gpx_wpt(const double lat, const double lon,
              const char *name, const char *cmt)
{
	char *retval = NULL, *name_c = NULL, *cmt_elem = NULL,
	     *lat_s = NULL, *lon_s = NULL;

	if (!name)
		return NULL;

	if (cmt) {
		char *cmt_c = xml_escape_string(cmt);
		if (!cmt_c)
			return NULL; /* gncov */
		cmt_elem = allocstr("    <cmt>%s</cmt>\n", cmt_c);
		free(cmt_c);
		if (!cmt_elem)
			return NULL; /* gncov */
	}
	name_c = xml_escape_string(name);
	if (!name_c)
		return NULL; /* gncov */
	lat_s = allocstr("%f", lat);
	lon_s = allocstr("%f", lon);
	if (!lat_s || !lon_s)
		goto cleanup; /* gncov */
	trim_zeros(lat_s);
	trim_zeros(lon_s);
	retval = allocstr("  <wpt lat=\"%s\" lon=\"%s\">\n"
	                  "    <name>%s</name>\n"
	                  "%s"
	                  "  </wpt>\n",
	                  lat_s, lon_s, name_c, cmt_elem ? cmt_elem : "");
cleanup:
	free(lon_s);
	free(lat_s);
	free(name_c);
	free(cmt_elem);

	return retval;
}

/* vim: set ts=8 sw=8 sts=8 noet fo+=w tw=79 fenc=UTF-8 : */
