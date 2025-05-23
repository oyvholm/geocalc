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

	assert(dest);
	*dest = round(*dest * m) / m;
	if (*dest == -0.0)
		*dest = 0.0;
}

/*
 * print_coordinate() - Prints a coordinate to stdout using the format in 
 * `o->outpformat`. `name` and `cmt` are used for the GPX format. If `cmt` 
 * isn't used, use NULL. Returns 1 if anything failed, otherwise 0.
 */

static int print_coordinate(const struct Options *o,
                            const double lat, const double lon,
                            const char *name, const char *cmt)
{
	double nlat = lat, nlon = lon;

	assert(o);

	round_number(&nlat, 6);
	round_number(&nlon, 6);
	if (o->outpformat == OF_DEFAULT) {
		printf("%f,%f\n", nlat, nlon);
	} else if (o->outpformat == OF_GPX) {
		char *s;
		if (!name) {
			myerror("%s(): Cannot print GPX waypoint," /* gncov */
			        " `name` is NULL", __func__);
			return 1; /* gncov */
		}
		s = gpx_wpt(nlat, nlon, name, cmt);
		if (!s) {
			failed("gpx_wpt()"); /* gncov */
			return 1; /* gncov */
		}
		fputs(s, stdout);
		free(s);
	} else {
		myerror("%s(): o->outpformat has unknown value:" /* gncov */
		        " %d", __func__, o->outpformat); /* gncov */
		return 1; /* gncov */
	}

	return 0;
}

/*
 * print_eor_coor() - Prints "end of run" coordinate. All commands use this 
 * function if the final result is only a coordinate, so the proper output 
 * format can be used. Returns 1 if allocations failed, or an unknown value is 
 * stored in `o->outpformat`. Otherwise it returns 0.
 */

static int print_eor_coor(const struct Options *o,
                          const double lat, const double lon, const char *cmd,
                          const char *par1, const char *par2, const char *par3)
{
	char *cmt, *s;
	double nlat = lat, nlon = lon;

	assert(o);

	round_number(&nlat, 6);
	round_number(&nlon, 6);

	switch (o->outpformat) {
	case OF_DEFAULT:
		printf("%f,%f\n", nlat, nlon);
		break;
	case OF_GPX:
		if (!cmd || !par1 || !par2 || !par3) {
			myerror("%s() received NULL argument," /* gncov */
			        " cannot generate GPX output", __func__);
			return 1; /* gncov */
		}
		cmt = allocstr("%s %s %s %s", cmd, par1, par2, par3);
		if (!cmt) {
			failed("allocstr()"); /* gncov */
			return 1; /* gncov */
		}
		s = gpx_wpt(nlat, nlon, cmd, cmt);
		if (!s) {
			failed("gpx_wpt()"); /* gncov */
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

int cmd_bear_dist(const char *cmd, const struct Options *o,
                  const char *coor1, const char *coor2)
{
	double lat1, lon1, lat2, lon2, result;
	char *s;

	assert(cmd);
	assert(o);
	assert(!strcmp(cmd, "bear") || !strcmp(cmd, "dist"));
	assert(coor1);
	assert(coor2);

	msg(7, "%s(\"%s\", \"%s\", \"%s\")", __func__, cmd, coor1, coor2);

	if (parse_coordinate(coor1, true, &lat1, &lon1)) {
		myerror("%s: Invalid coordinate", coor1);
		return EXIT_FAILURE;
	}
	if (parse_coordinate(coor2, true, &lat2, &lon2)) {
		myerror("%s: Invalid coordinate", coor2);
		return EXIT_FAILURE;
	}

	result = !strcmp(cmd, "bear") ? initial_bearing(lat1, lon1, lat2, lon2)
	                              : distance(o->distformula,
	                                         lat1, lon1, lat2, lon2);
	if (result == -2.0) {
		myerror("Antipodal points, answer is undefined");
		return EXIT_FAILURE;
	}
	if (isnan(result) && o->distformula == FRM_KARNEY
	    && !strcmp(cmd, "dist"))
	{
		myerror("Formula did not converge, antipodal points");
		return EXIT_FAILURE;
	}

	if (o->km && !strcmp(cmd, "dist"))
		result /= 1000.0;
	switch (o->outpformat) {
	case OF_DEFAULT:
		s = allocstr("%%.%uf\n",
		             o->distformula == FRM_KARNEY
		               ? KARNEY_DECIMALS
		               : HAVERSINE_DECIMALS);
		if (!s) {
			failed("allocstr()"); /* gncov */
			return EXIT_FAILURE; /* gncov */
		}
		printf(s, result);
		free(s);
		return EXIT_SUCCESS;
	case OF_GPX: /* gncov */
		return EXIT_FAILURE; /* gncov */
	case OF_SQL:
		puts("BEGIN;");
		if (!strcmp(cmd, "bear")) {
			puts("CREATE TABLE IF NOT EXISTS bear (lat1 REAL,"
			     " lon1 REAL, lat2 REAL, lon2 REAL, bear REAL,"
			     " dist REAL);");
			printf("INSERT INTO bear VALUES (%f, %f, %f, %f, %f,"
			       " %f);\n",
			       lat1, lon1, lat2, lon2,
			       initial_bearing(lat1, lon1, lat2, lon2),
			       haversine(lat1, lon1, lat2, lon2));
		} else {
			puts("CREATE TABLE IF NOT EXISTS dist (lat1 REAL,"
			     " lon1 REAL, lat2 REAL, lon2 REAL, dist REAL,"
			     " bear REAL);");
			printf("INSERT INTO dist VALUES (%.15f, %.15f, %.15f,"
			       " %.15f, %.8f, %.8f);\n",
			       lat1, lon1, lat2, lon2,
			       haversine(lat1, lon1, lat2, lon2),
			       initial_bearing(lat1, lon1, lat2, lon2));
		}
		puts("COMMIT;");
		return EXIT_SUCCESS;
	}

	myerror("%s():%d: o->outpformat has unknown format %d", /* gncov */
	        __func__, __LINE__, o->outpformat); /* gncov */

	return EXIT_FAILURE; /* gncov */
}

/*
 * cmd_bpos() - Executes the `bpos` command. Returns `EXIT_SUCCESS` or 
 * `EXIT_FAILURE`.
 */

int cmd_bpos(const struct Options *o, const char *coor,
             const char *bearing_s, const char *dist_s)
{
	double lat, lon, bearing, dist, nlat, nlon;

	assert(o);
	assert(coor);
	assert(bearing_s);
	assert(dist_s);

	msg(7, "%s(\"%s\", \"%s\", \"%s\")",
	       __func__, coor, bearing_s, dist_s);

	if (parse_coordinate(coor, true, &lat, &lon)) {
		myerror("%s: Invalid coordinate", coor);
		return EXIT_FAILURE;
	}
	if (string_to_double(bearing_s, &bearing)) {
		myerror("%s: Invalid bearing", bearing_s);
		return EXIT_FAILURE;
	}
	if (bearing < 0.0 || bearing > 360.0) {
		myerror("%s: Bearing out of range", bearing_s);
		return EXIT_FAILURE;
	}
	if (string_to_double(dist_s, &dist)) {
		myerror("%s: Invalid distance", dist_s);
		return EXIT_FAILURE;
	}
	if (o->km)
		dist *= 1000.0;
	bearing_position(lat, lon, bearing, dist, &nlat, &nlon);

	switch (o->outpformat) {
	case OF_DEFAULT:
	case OF_GPX:
		return print_eor_coor(o, nlat, nlon, "bpos", coor, bearing_s,
		                      dist_s)
		       ? EXIT_FAILURE : EXIT_SUCCESS;
	case OF_SQL:
		puts("BEGIN;");
		puts("CREATE TABLE IF NOT EXISTS bpos (lat1 REAL, lon1 REAL,"
		     " lat2 REAL, lon2 REAL, bear REAL, dist REAL);");
		printf("INSERT INTO bpos VALUES (%f, %f, %f, %f, %f, %f);\n",
		       lat, lon, nlat, nlon,
		       initial_bearing(lat, lon, nlat, nlon),
		       haversine(lat, lon, nlat, nlon));
		puts("COMMIT;");
		return EXIT_SUCCESS;
	}

	myerror("%s(): o->outpformat has unknown format %d", /* gncov */
	        __func__, o->outpformat); /* gncov */

	return EXIT_FAILURE; /* gncov */
}

/*
 * cmd_course() - Executes the `course` command. Returns `EXIT_SUCCESS` or 
 * `EXIT_FAILURE`.
 */

int cmd_course(const struct Options *o, const char *coor1, const char *coor2,
               const char *numpoints_s)
{
	double lat1, lon1, lat2, lon2, numpoints, nlat = 0.0, nlon = 0.0;
	int i, retval = EXIT_SUCCESS;

	assert(o);
	assert(coor1);
	assert(coor2);
	assert(numpoints_s);

	msg(7, "%s(\"%s\", \"%s\", \"%s\")",
	       __func__, coor1, coor2, numpoints_s);

	if (parse_coordinate(coor1, true, &lat1, &lon1)) {
		myerror("%s: Invalid coordinate", coor1);
		return EXIT_FAILURE;
	}
	if (parse_coordinate(coor2, true, &lat2, &lon2)) {
		myerror("%s: Invalid coordinate", coor2);
		return EXIT_FAILURE;
	}
	if (string_to_double(numpoints_s, &numpoints)) {
		myerror("%s: Invalid number of points", numpoints_s);
		return EXIT_FAILURE;
	}
	if (are_antipodal(lat1, lon1, lat2, lon2)) {
		myerror("Antipodal points, answer is undefined");
		return EXIT_FAILURE;
	}
	if (numpoints++ < 0) {
		myerror("%s: Number of intermediate points cannot be negative",
		        numpoints_s);
		return EXIT_FAILURE;
	}

	switch (o->outpformat) {
	case OF_DEFAULT:
		break;
	case OF_GPX:
		fputs(GPX_HEADER, stdout);
		puts("  <rte>");
		break;
	case OF_SQL:
		puts("BEGIN;");
		puts("CREATE TABLE IF NOT EXISTS course (num INTEGER,"
		     " lat REAL, lon REAL, dist REAL, frac REAL, bear REAL);");
		break;
	}

	for (i = 0; i <= numpoints; i++) {
		double frac = 1.0 * i / numpoints,
		       dist, bear;
		char *bear_s;

		routepoint(lat1, lon1, lat2, lon2, frac, &nlat, &nlon);
		round_number(&nlat, 6);
		round_number(&nlon, 6);
		switch(o->outpformat) {
		case OF_DEFAULT:
			printf("%f,%f\n", nlat, nlon);
			break;
		case OF_GPX:
			printf("    <rtept lat=\"%f\" lon=\"%f\">\n"
			       "    </rtept>\n", nlat, nlon);
			break;
		case OF_SQL:
			dist = haversine(lat1, lon1, nlat, nlon);
			if (nlat != lat2 || nlon != lon2) {
				bear = initial_bearing(nlat, nlon, lat2, lon2);
				bear_s = allocstr("%f", bear);
			} else {
				bear_s = allocstr("NULL");
			}
			if (!bear_s) {
				failed("allocstr()"); /* gncov */
				return EXIT_FAILURE; /* gncov */
			}
			printf("INSERT INTO course VALUES (%d, %f, %f, %f,"
			       " %f, %s);\n",
			       i, nlat, nlon, dist, frac, bear_s);
			free(bear_s);
			break;
		}
	}

	switch (o->outpformat) {
	case OF_DEFAULT:
		break;
	case OF_GPX:
		puts("  </rte>");
		puts("</gpx>");
		break;
	case OF_SQL:
		puts("COMMIT;");
		break;
	}

	return retval;
}

/*
 * cmd_lpos() - Executes the `lpos` command. Returns `EXIT_SUCCESS` or 
 * `EXIT_FAILURE`.
 */

int cmd_lpos(const struct Options *o, const char *coor1, const char *coor2,
             const char *fracdist_s)
{
	double lat1, lon1, lat2, lon2, fracdist, nlat, nlon;

	assert(o);
	assert(coor1);
	assert(coor2);
	assert(fracdist_s);

	msg(7, "%s(\"%s\", \"%s\", \"%s\")",
	       __func__, coor1, coor2, fracdist_s);

	if (parse_coordinate(coor1, true, &lat1, &lon1)) {
		myerror("%s: Invalid coordinate", coor1);
		return EXIT_FAILURE;
	}
	if (parse_coordinate(coor2, true, &lat2, &lon2)) {
		myerror("%s: Invalid coordinate", coor2);
		return EXIT_FAILURE;
	}
	if (string_to_double(fracdist_s, &fracdist)) {
		myerror("%s: Invalid fraction", fracdist_s);
		return EXIT_FAILURE;
	}
	if (are_antipodal(lat1, lon1, lat2, lon2)) {
		myerror("Antipodal points, answer is undefined");
		return EXIT_FAILURE;
	}
	routepoint(lat1, lon1, lat2, lon2, fracdist, &nlat, &nlon);

	switch (o->outpformat) {
	case OF_DEFAULT:
	case OF_GPX:
		return print_eor_coor(o, nlat, nlon, "lpos",
		                      coor1, coor2, fracdist_s)
		       ? EXIT_FAILURE : EXIT_SUCCESS;
	case OF_SQL:
		puts("BEGIN;");
		puts("CREATE TABLE IF NOT EXISTS lpos (lat1 REAL, lon1 REAL,"
		     " lat2 REAL, lon2 REAL, frac REAL, dlat REAL, dlon REAL,"
		     " dist REAL, bear REAL);");
		printf("INSERT INTO lpos VALUES (%f, %f, %f, %f, %f, %f, %f,"
		       " %f, %f);\n",
		       lat1, lon1, lat2, lon2, fracdist, nlat, nlon,
		       haversine(lat1, lon2, nlat, nlon),
		       initial_bearing(lat1, lon2, nlat, nlon));
		puts("COMMIT;");
		return EXIT_SUCCESS;
	}

	myerror("%s(): o->outpformat has unknown format %d", /* gncov */
	        __func__, o->outpformat); /* gncov */

	return EXIT_FAILURE; /* gncov */
}

/*
 * cmd_randpos() - Executes the `randpos` command. Returns `EXIT_SUCCESS` or 
 * `EXIT_FAILURE`.
 */

int cmd_randpos(const struct Options *o, const char *coor,
                const char *maxdist, const char *mindist)
{
	long l;
	double c_lat = 1000, c_lon = 1000, maxdist_d = 0, mindist_d = 0;

	assert(o);

	if (coor) {
		if (parse_coordinate(coor, true, &c_lat, &c_lon)) {
			myerror("%s: Invalid coordinate", coor);
			return EXIT_FAILURE;
		}
		if (maxdist && string_to_double(maxdist, &maxdist_d)) {
			myerror("%s: Invalid max_dist argument", maxdist);
			return EXIT_FAILURE;
		}
		if (mindist && string_to_double(mindist, &mindist_d)) {
			myerror("%s: Invalid min_dist argument", mindist);
			return EXIT_FAILURE;
		}
		if (mindist_d < 0 || maxdist_d < 0) {
			myerror("Distance cannot be negative");
			return EXIT_FAILURE;
		}
		if (o->km) {
			mindist_d *= 1000.0;
			maxdist_d *= 1000.0;
		}
		if (mindist_d > MAX_EARTH_DISTANCE)
			mindist_d = MAX_EARTH_DISTANCE;
		if (maxdist_d > MAX_EARTH_DISTANCE)
			maxdist_d = MAX_EARTH_DISTANCE;
	}

	switch (o->outpformat) {
	case OF_DEFAULT:
		break;
	case OF_GPX:
		fputs(GPX_HEADER, stdout);
		break;
	case OF_SQL:
		puts("BEGIN;");
		puts("CREATE TABLE IF NOT EXISTS randpos (seed INTEGER,"
		     " num INTEGER, lat REAL, lon REAL, dist REAL,"
		     " bear REAL);");
		break;
	}

	for (l = 1; l <= o->count; l++) {
		double lat, lon;
		char *name, *seedstr = NULL;

		if (o->seed) {
			seedstr = allocstr(", seed %ld", o->seedval);
			if (!seedstr) {
				failed("allocstr()"); /* gncov */
				return EXIT_FAILURE; /* gncov */
			}
		}
		rand_pos(&lat, &lon, c_lat, c_lon, maxdist_d, mindist_d);
		name = allocstr("Random %lu%s", l, seedstr ? seedstr : "");
		if (!name) {
			failed("allocstr()"); /* gncov */
			free(seedstr); /* gncov */
			return EXIT_FAILURE; /* gncov */
		}

		if (o->outpformat == OF_SQL) {
			double dist, bear;

			dist = haversine(c_lat, c_lon, lat, lon);
			bear = initial_bearing(c_lat, c_lon, lat, lon);
			if (c_lat > 90.0) {
				printf("INSERT INTO randpos VALUES"
				       " (%ld, %ld, %f, %f, NULL, NULL);\n",
				       o->seedval, l, lat, lon);
			} else {
				printf("INSERT INTO randpos VALUES"
				       " (%ld, %ld, %f, %f, %f, %f);\n",
				       o->seedval, l, lat, lon, dist, bear);
			}
		} else {
			print_coordinate(o, lat, lon, name, NULL);
		}

		free(name);
		free(seedstr);
	}

	switch (o->outpformat) {
	case OF_DEFAULT:
		break;
	case OF_GPX:
		puts("</gpx>");
		break;
	case OF_SQL:
		puts("COMMIT;");
		break;
	}

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
	assert(name);
	assert(fnc);
	assert(br);

	fprintf(stderr, "Looping %s() for %ld second%s...",
	                name, dur, dur == 1 ? "" : "s");
	fflush(stderr);

	br->name = name;
	br->rounds = 0L;
	if (clock_gettime(CLOCK_MONOTONIC, &br->start)) {
		failed("clock_gettime()"); /* gncov */
		return 1; /* gncov */
	}
	do {
		fnc(br->lat1, br->lon1, br->lat2, br->lon2);
		br->rounds++;
		if (clock_gettime(CLOCK_MONOTONIC, &br->end)) {
			failed("clock_gettime()"); /* gncov */
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

int cmd_bench(const struct Options *o, const char *seconds)
{
	time_t secs = seconds ? atoi(seconds) : BENCH_LOOP_SECS;
	struct bench_result br[2];
	const size_t arrsize = sizeof(br) / sizeof(br[0]);
	size_t i;
	int r = 0;
	unsigned long totrounds = 0UL;
	double lat1, lon1, lat2, lon2;

	assert(o);

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
	r += bench_dist_func("karney_distance", karney_distance, secs, &br[1]);
	fputs("\n", stderr);

	for (i = 0; i < arrsize; i++)
		totrounds += br[i].rounds;

	qsort(br, arrsize, sizeof(struct bench_result), cmd_bench_cmp_rounds);
	if (o->outpformat == OF_SQL) {
		puts("BEGIN;");
		puts("CREATE TABLE IF NOT EXISTS bench (name TEXT, start REAL,"
		     " end REAL, secs REAL, rounds INTEGER, lat1 REAL,"
		     " lon1 REAL, lat2 REAL, lon2 REAL, dist REAL);");
	}
	for (i = 0; i < arrsize; i++) {
		if (o->outpformat == OF_SQL) {
			printf("INSERT INTO bench VALUES ('%s', %f, %f, %f,"
			       " %lu, %.15f, %.15f, %.15f, %.15f, %f);\n",
			       br[i].name, br[i].start_d, br[i].end_d,
			       br[i].secs, br[i].rounds, br[i].lat1,
			       br[i].lon1, br[i].lat2, br[i].lon2, br[i].dist);
		} else {
			printf("%lu (%f%%) %f %.8f %s\n",
			       br[i].rounds,
			       totrounds ? 100.0 * (double)br[i].rounds
			                   / (double)totrounds
			                 : 0.0,
			       br[i].secs, br[i].dist, br[i].name);
		}
	}

	if (o->outpformat == OF_SQL)
		puts("COMMIT;");

	return r ? EXIT_FAILURE : EXIT_SUCCESS;
}

/* vim: set ts=8 sw=8 sts=8 noet fo+=w tw=79 fenc=UTF-8 : */
