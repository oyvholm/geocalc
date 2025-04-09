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
 * round_number() - Rounds number in `*dest` to `decimals` number of decimals 
 * and gets rid of negative zero. Returns nothing.
 */

void round_number(double *dest, const int decimals)
{
	double m = pow(10.0, (double)decimals);

	*dest = round(*dest * m) / m;
	if (*dest == -0.0)
		*dest = 0.0;
}

/*
 * print_coordinate() - Prints a coordinate to stdout using the format in 
 * `opt.outpformat`. `name` and `cmt` are used for the GPX format. If `cmt` 
 * isn't used, use NULL. Returns 1 if anything failed, otherwise 0.
 */

static int print_coordinate(const double lat, const double lon,
                            const char *name, const char *cmt)
{
	double nlat = lat, nlon = lon;

	round_number(&nlat, 6);
	round_number(&nlon, 6);
	if (opt.outpformat == OF_DEFAULT) {
		printf("%f,%f\n", nlat, nlon);
	} else if (opt.outpformat == OF_GPX) {
		char *s;
		if (!name) {
			myerror("%s(): Cannot print GPX waypoint," /* gncov */
			        " `name` is NULL", __func__);
			return 1; /* gncov */
		}
		s = gpx_wpt(nlat, nlon, name, cmt);
		if (!s) {
			myerror("%s(): gpx_wpt() failed", /* gncov */
			        __func__);
			return 1; /* gncov */
		}
		fputs(s, stdout);
		free(s);
	} else {
		myerror("%s(): opt.outpformat has unknown value:" /* gncov */
		        " %d", __func__, opt.outpformat); /* gncov */
		return 1; /* gncov */
	}

	return 0;
}

/*
 * print_eor_coor() - Prints "end of run" coordinate. All commands use this 
 * function if the final result is only a coordinate, so the proper output 
 * format can be used. Returns 1 if allocations failed, or an unknown value is 
 * stored in `opt.outpformat`. Otherwise it returns 0.
 */

static int print_eor_coor(const double lat, const double lon, const char *cmd,
                          const char *par1, const char *par2, const char *par3)
{
	char *cmt, *s;
	double nlat = lat, nlon = lon;

	round_number(&nlat, 6);
	round_number(&nlon, 6);

	switch (opt.outpformat) {
	case OF_DEFAULT:
		printf("%f,%f\n", nlat, nlon);
		break;
	case OF_GPX:
		cmt = allocstr("%s %s %s %s", cmd, par1, par2, par3);
		if (!cmt)
			return 1; /* gncov */
		s = gpx_wpt(nlat, nlon, cmd, cmt);
		if (!s) {
			free(cmt); /* gncov */
			return 1; /* gncov */
		}
		printf("%s%s</gpx>\n", GPX_HEADER, s);
		free(s);
		free(cmt);
		break;
	default: /* gncov */
		return 1; /* gncov */
	}

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

	msg(7, "%s(\"%s\", \"%s\", \"%s\")", __func__, cmd, coor1, coor2);

	if (opt.outpformat == OF_GPX) {
		myerror("No way to display this info in GPX format");
		return EXIT_FAILURE;
	}

	if (parse_coordinate(coor1, &lat1, &lon1)
	    || parse_coordinate(coor2, &lat2, &lon2)) {
		myerror("Invalid number specified");
		return EXIT_FAILURE;
	}

	result = !strcmp(cmd, "bear") ? initial_bearing(lat1, lon1, lat2, lon2)
	                              : distance(opt.distformula,
	                                         lat1, lon1, lat2, lon2);
	if (result == -1.0) {
		myerror("Value out of range");
		return EXIT_FAILURE;
	}
	if (result == -2.0) {
		myerror("Antipodal points, answer is undefined");
		return EXIT_FAILURE;
	}
	if (opt.km && !strcmp(cmd, "dist"))
		result /= 1000.0;
	printf("%f\n", result);

	return EXIT_SUCCESS;
}

/*
 * cmd_bpos() - Executes the `bpos` command. Returns `EXIT_SUCCESS` or 
 * `EXIT_FAILURE`.
 */

int cmd_bpos(const char *coor, const char *bearing_s, const char *dist_s)
{
	double lat, lon, bearing, dist, nlat, nlon;
	int result;

	msg(7, "%s(\"%s\", \"%s\", \"%s\")",
	       __func__, coor, bearing_s, dist_s);

	if (parse_coordinate(coor, &lat, &lon)
	    || string_to_double(bearing_s, &bearing)
	    || string_to_double(dist_s, &dist)) {
		myerror("Invalid number specified");
		errno = 0;
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
	double lat1, lon1, lat2, lon2, numpoints, nlat = 0.0, nlon = 0.0;
	int i, result, retval = EXIT_SUCCESS;

	msg(7, "%s(\"%s\", \"%s\", \"%s\")",
	       __func__, coor1, coor2, numpoints_s);

	if (parse_coordinate(coor1, &lat1, &lon1)
	    || parse_coordinate(coor2, &lat2, &lon2)
	    || string_to_double(numpoints_s, &numpoints)) {
		myerror("Invalid number specified");
		errno = 0;
		return EXIT_FAILURE;
	}
	if (are_antipodal(lat1, lon1, lat2, lon2)) {
		myerror("Antipodal points, answer is undefined");
		return EXIT_FAILURE;
	}
	if (numpoints++ < 0) {
		myerror("Value out of range");
		return EXIT_FAILURE;
	}
	if (opt.outpformat == OF_GPX) {
		fputs(GPX_HEADER, stdout);
		puts("  <rte>");
	}
	for (i = 0; i <= numpoints; i++) {
		result = routepoint(lat1, lon1, lat2, lon2,
		                    1.0 * i / numpoints, &nlat, &nlon);
		if (result) {
			myerror("Value out of range");
			retval = EXIT_FAILURE;
			break;
		}
		round_number(&nlat, 6);
		round_number(&nlon, 6);
		switch(opt.outpformat) {
		case OF_DEFAULT:
			printf("%f,%f\n", nlat, nlon);
			break;
		case OF_GPX:
			printf("    <rtept lat=\"%f\" lon=\"%f\">\n"
			       "    </rtept>\n", nlat, nlon);
			break;
		}
	}
	if (opt.outpformat == OF_GPX) {
		puts("  </rte>");
		puts("</gpx>");
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

	msg(7, "%s(\"%s\", \"%s\", \"%s\")",
	       __func__, coor1, coor2, fracdist_s);

	if (parse_coordinate(coor1, &lat1, &lon1)
	    || parse_coordinate(coor2, &lat2, &lon2)
	    || string_to_double(fracdist_s, &fracdist)) {
		myerror("Invalid number specified");
		errno = 0;
		return EXIT_FAILURE;
	}
	if (are_antipodal(lat1, lon1, lat2, lon2)) {
		myerror("Antipodal points, answer is undefined");
		return EXIT_FAILURE;
	}
	if (routepoint(lat1, lon1, lat2, lon2, fracdist, &nlat, &nlon)) {
		myerror("Value out of range");
		return EXIT_FAILURE;
	}

	return print_eor_coor(nlat, nlon, "lpos", coor1, coor2, fracdist_s)
	       ? EXIT_FAILURE : EXIT_SUCCESS;
}

/*
 * cmd_randpos() - Executes the `randpos` command. Returns `EXIT_SUCCESS` or 
 * `EXIT_FAILURE`.
 */

int cmd_randpos(const char *coor, const char *maxdist, const char *mindist)
{
	long l;
	double c_lat = 1000, c_lon = 1000, maxdist_d = 0, mindist_d = 0;

	if (coor) {
		if (parse_coordinate(coor, &c_lat, &c_lon)) {
			myerror("Error in center coordinate");
			return EXIT_FAILURE;
		}
		if (maxdist && string_to_double(maxdist, &maxdist_d)) {
			myerror("Error in max_dist argument");
			errno = 0;
			return EXIT_FAILURE;
		}
		if (mindist && string_to_double(mindist, &mindist_d)) {
			myerror("Error in min_dist argument");
			errno = 0;
			return EXIT_FAILURE;
		}
		if (mindist_d < 0 || maxdist_d < 0) {
			myerror("Distance can't be negative");
			return EXIT_FAILURE;
		}
		if (opt.km) {
			mindist_d *= 1000.0;
			maxdist_d *= 1000.0;
		}
		if (mindist_d > MAX_EARTH_DISTANCE)
			mindist_d = MAX_EARTH_DISTANCE;
		if (maxdist_d > MAX_EARTH_DISTANCE)
			maxdist_d = MAX_EARTH_DISTANCE;
	}
	if (opt.outpformat == OF_GPX)
		fputs(GPX_HEADER, stdout);
	for (l = 1; l <= opt.count; l++) {
		double lat, lon;
		char *name, *seedstr = NULL;
		if (opt.seed) {
			seedstr = allocstr(", seed %ld", opt.seedval);
			if (!seedstr) {
				myerror("%s():%d: allocstr()" /* gncov */
				        " failed()", __func__, __LINE__);
				return EXIT_FAILURE; /* gncov */
			}
		}
		rand_pos(&lat, &lon, c_lat, c_lon, maxdist_d, mindist_d);
		name = allocstr("Random %lu%s", l, seedstr ? seedstr : "");
		print_coordinate(lat, lon, name, NULL);
		free(name);
		free(seedstr);
	}
	if (opt.outpformat == OF_GPX)
		puts("</gpx>");

	return EXIT_SUCCESS;
}

/*
 * bench_dist_func() - Used by cmd_bench(). Executes the function specified by 
 * the function pointer `fnc` in a loop that lasts for `dur` seconds.
 *
 * Parameters:
 * - `name`: The name of the function, displayed to the user.
 * - `fnc`: Function pointer to the distance function to use.
 * - `br`: Pointer to a `struct bench_result` where the results are stored. The 
 *   members `lat1`, `lon1`, `lat2`, and `lon2` are expected to already be set 
 *   to the coordinate pair to use.
 *
 * Returns 1 if something failed, or 0 if ok.
 */

static int bench_dist_func(const char *name,
                           double (*fnc)(const double, const double,
                                         const double, const double),
                           const time_t dur,
                           struct bench_result *br)
{
	fprintf(stderr, "Looping %s() for %ld second%s...",
	                name, dur, dur == 1 ? "" : "s");
	fflush(stderr);

	br->name = name;
	br->rounds = 0L;
	if (clock_gettime(CLOCK_MONOTONIC, &br->start)) {
		myerror("%s():%d: clock_gettime() failed", /* gncov */
		        __func__, __LINE__);
		return 1; /* gncov */
	}
	do {
		fnc(br->lat1, br->lon1, br->lat2, br->lon2);
		br->rounds++;
		if (clock_gettime(CLOCK_MONOTONIC, &br->end)) {
			myerror("%s():%d: clock_gettime() failed", /* gncov */
			        __func__, __LINE__);
			return 1; /* gncov */
		}
		if ((br->end.tv_sec - br->start.tv_sec > dur)
		    || (br->end.tv_sec - br->start.tv_sec == dur
		        && br->end.tv_nsec >= br->start.tv_nsec)) {
			break;
		}
	} while (1);
	fputs("done\n", stderr);

	br->start_d = (double)br->start.tv_sec
	              + (double)br->start.tv_nsec / 1e9;
	br->end_d = (double)br->end.tv_sec + (double)br->end.tv_nsec / 1e9;
	br->secs = br->end_d - br->start_d;
	br->dist = fnc(br->lat1, br->lon1, br->lat2, br->lon2);
	fprintf(stderr, "%s(): %lu rounds, ran for %f seconds. dist = %.8f\n",
	                name, br->rounds, br->end_d - br->start_d, br->dist);

	return 0;
}

/*
 * cmd_bench_cmp_rounds() - Used as comparison function for qsort() in 
 * cmd_bench(). Returns the descending sort value for the `rounds` member in 
 * `bench_result`.
 */

static int cmd_bench_cmp_rounds(const void *s1, const void *s2) /* gncov */
{
	const struct bench_result *br1, *br2;

	assert(s1); /* gncov */
	assert(s2); /* gncov */

	br1 = (const struct bench_result *)s1; /* gncov */
	br2 = (const struct bench_result *)s2; /* gncov */

	return (int)(br2->rounds - br1->rounds); /* gncov */
}

/*
 * cmd_bench() - Run various benchmarks and report the result. Returns 
 * EXIT_SUCCESS or EXIT_FAILURE.
 */

int cmd_bench(const char *seconds)
{
	time_t secs = seconds ? atoi(seconds) : BENCH_LOOP_SECS;
	struct bench_result br[1];
	const size_t arrsize = sizeof(br) / sizeof(br[0]);
	size_t i;
	int r = 0;
	unsigned long totrounds = 0UL;
	double lat1, lon1, lat2, lon2;

	rand_pos(&lat1, &lon1, 1000, 1000, 0, 0);
	rand_pos(&lat2, &lon2, 1000, 1000, 0, 0);
	fprintf(stderr, "Random coordinates: %.15f,%.15f %.15f,%.15f\n",
	                lat1, lon1, lat2, lon2);
	for (i = 0; i < arrsize; i++) {
		br[i] = (struct bench_result){
			.lat1 = lat1, .lon1 = lon1, .lat2 = lat2, .lon2 = lon2
		};
	}

	r += bench_dist_func("haversine", haversine, secs, &br[0]);
	fputs("\n", stderr);

	for (i = 0; i < arrsize; i++)
		totrounds += br[i].rounds;

	qsort(br, arrsize, sizeof(struct bench_result), cmd_bench_cmp_rounds);
	for (i = 0; i < arrsize; i++) {
		printf("%lu (%f%%) %f %.8f %s\n",
		       br[i].rounds,
		       totrounds ? 100.0 * (double)br[i].rounds
		                   / (double)totrounds
		                 : 0.0,
		       br[i].secs, br[i].dist, br[i].name);
	}

	return r ? EXIT_FAILURE : EXIT_SUCCESS;
}

/* vim: set ts=8 sw=8 sts=8 noet fo+=w tw=79 fenc=UTF-8 : */
