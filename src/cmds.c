/*
 * cmds.c
 * File ID: 0d849232-8961-11ef-ad57-83850402c3ce
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
 * print_eor_coor() - Prints "end of run" coordinate. All commands use this 
 * function if the final result is only a coordinate, so the proper output 
 * format can be used. Returns 1 if allocations failed, or an unknown value is 
 * stored in `opt.outpformat`. Otherwise it returns 0.
 */

static int print_eor_coor(const double lat, const double lon, const char *cmd,
                          const char *par1, const char *par2, const char *par3)
{
	if (cmd || par1 || par2 || par3) { } /* Temporary, avoid warnings */
	switch (opt.outpformat) {
	case OF_DEFAULT:
		printf("%f,%f\n", lat, lon);
		break;
	default: /* gncov */
		return 1; /* gncov */
	}

	return 0;
}

/*
 * print_eor_number() - Prints "end of run" number. Same philosophy as 
 * print_eor_coor(). Returns only 0 for now.
 */

static int print_eor_number(const double num)
{
	printf("%f\n", num);

	return 0;
}

/*
 * cmd_bear_dist() - Executes the `bear` or `dist` commands, specified in 
 * `cmd`. Returns `EXIT_SUCCESS` or `EXIT_FAILURE`.
 */

int cmd_bear_dist(const char *cmd, const char *coor1, const char *coor2)
{
	double lat1, lon1, lat2, lon2, result;

	assert(cmd);
	assert(!strcmp(cmd, "bear") || !strcmp(cmd, "dist"));
	assert(coor1);
	assert(coor2);

	msg(VERBOSE_TRACE, "%s(\"%s\", \"%s\", \"%s\")",
	    __func__, cmd, coor1, coor2);

	if (parse_coordinate(coor1, &lat1, &lon1)
	    || parse_coordinate(coor2, &lat2, &lon2)) {
		myerror("Invalid number specified");
		return EXIT_FAILURE;
	}

	result = !strcmp(cmd, "bear") ? initial_bearing(lat1, lon1, lat2, lon2)
	                              : haversine(lat1, lon1, lat2, lon2);
	if (result == -1.0) {
		myerror("Value out of range");
		return EXIT_FAILURE;
	}
	if (opt.km && !strcmp(cmd, "dist"))
		result /= 1000.0;

	return print_eor_number(result) ? EXIT_FAILURE : EXIT_SUCCESS;
}

/*
 * cmd_bpos() - Executes the `bpos` command. Returns `EXIT_SUCCESS` or 
 * `EXIT_FAILURE`.
 */

int cmd_bpos(const char *coor, const char *bearing_s, const char *dist_s)
{
	double lat, lon, bearing, dist, nlat, nlon;
	int result;

	msg(VERBOSE_TRACE, "%s(\"%s\", \"%s\", \"%s\")",
	    __func__, coor, bearing_s, dist_s);

	if (parse_coordinate(coor, &lat, &lon)
	    || string_to_double(bearing_s, &bearing)
	    || string_to_double(dist_s, &dist)) {
		myerror("Invalid number specified");
		return EXIT_FAILURE;
	}
	if (opt.km)
		dist *= 1000.0;
	result = bearing_position(lat, lon, bearing, dist, &nlat, &nlon);
	if (result) {
		myerror("Value out of range");
		return EXIT_FAILURE;
	}
	return print_eor_coor(nlat, nlon, "bpos", coor, bearing_s, dist_s)
	       ? EXIT_FAILURE : EXIT_SUCCESS;
}

/*
 * cmd_course() - Executes the `course` command. Returns `EXIT_SUCCESS` or 
 * `EXIT_FAILURE`.
 */

int cmd_course(const char *coor1, const char *coor2, const char *numpoints_s)
{
	double lat1, lon1, lat2, lon2, numpoints, nlat, nlon;
	int i, result, retval = EXIT_SUCCESS;

	msg(VERBOSE_TRACE, "%s(\"%s\", \"%s\", \"%s\")",
	    __func__, coor1, coor2, numpoints_s);

	if (parse_coordinate(coor1, &lat1, &lon1)
	    || parse_coordinate(coor2, &lat2, &lon2)
	    || string_to_double(numpoints_s, &numpoints)) {
		myerror("Invalid number specified");
		return EXIT_FAILURE;
	}
	if (numpoints++ < 0) {
		myerror("Value out of range");
		return EXIT_FAILURE;
	}
	for (i = 0; i <= numpoints; i++) {
		result = routepoint(lat1, lon1, lat2, lon2,
		                    1.0 * i / numpoints, &nlat, &nlon);
		if (!result) {
			switch(opt.outpformat) {
			case OF_DEFAULT:
				printf("%f,%f\n", nlat, nlon);
				break;
			default: /* gncov */
				break; /* gncov */
			}
		} else {
			myerror("Value out of range");
			retval = EXIT_FAILURE;
			break;
		}
	}

	return retval;
}

/*
 * cmd_lpos() - Executes the `lpos` command. Returns `EXIT_SUCCESS` or 
 * `EXIT_FAILURE`.
 */

int cmd_lpos(const char *coor1, const char *coor2, const char *fracdist_s)
{
	double lat1, lon1, lat2, lon2, fracdist, nlat, nlon;

	msg(VERBOSE_TRACE, "%s(\"%s\", \"%s\", \"%s\")",
	    __func__, coor1, coor2, fracdist_s);

	if (parse_coordinate(coor1, &lat1, &lon1)
	    || parse_coordinate(coor2, &lat2, &lon2)
	    || string_to_double(fracdist_s, &fracdist)) {
		myerror("Invalid number specified");
		return EXIT_FAILURE;
	}
	if (routepoint(lat1, lon1, lat2, lon2, fracdist, &nlat, &nlon)) {
		myerror("Value out of range");
		return EXIT_FAILURE;
	}

	return print_eor_coor(nlat, nlon, "lpos", coor1, coor2, fracdist_s)
	       ? EXIT_FAILURE : EXIT_SUCCESS;
}

/* vim: set ts=8 sw=8 sts=8 noet fo+=w tw=79 fenc=UTF-8 : */
