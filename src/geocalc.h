/*
 * geocalc.h
 * File ID: 9409ee62-87f2-11ef-a52d-83850402c3ce
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

#ifndef _GEOCALC_H
#define _GEOCALC_H

#include "version.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <math.h>
#include <regex.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "binbuf.h"
#include "geomath.h"
#include "gpx.h"

#define PROJ_NAME  "Geocalc"
#define PROJ_URL  "https://gitlab.com/oyvholm/geocalc"

#define BENCH_LOOP_SECS  2

#if 1
#  define DEBL  msg(2, "DEBL: %s, line %u in %s()", \
                       __FILE__, __LINE__, __func__)
#else
#  define DEBL  ;
#endif

#define check_errno  do { \
	if (errno) { \
		myerror("%s():%s:%d: errno = %d", \
		        __func__, __FILE__, __LINE__, errno); \
	} \
} while (0)

#define failed(a)  myerror("%s():%d: %s failed", __func__, __LINE__, (a))

typedef enum {
	OF_DEFAULT = 0,
	OF_GPX,
	OF_SQL
} OutputFormat;

struct Options {
	/* sort -d -k2 */
	long count;
	DistFormula distformula;
	char *format;
	bool help;
	bool km;
	bool license;
	OutputFormat outpformat;
	char *seed;
	long seedval;
	bool selftest;
	bool testexec;
	bool testfunc;
	bool valgrind;
	int verbose;
	bool version;
};

struct streams {
	struct binbuf in;
	struct binbuf out;
	struct binbuf err;
	int ret;
};

struct bench_result {
	const char *name;
	struct timespec start;
	struct timespec end;
	double start_d;
	double end_d;
	double secs;
	unsigned long rounds;
	double lat1;
	double lon1;
	double lat2;
	double lon2;
	double dist;
};

/*
 * Public function prototypes
 */

/* geocalc.c */
extern char *progname;
extern struct Options opt;
int msg(const int verbose, const char *format, ...);
const char *std_strerror(const int errnum);
int myerror(const char *format, ...);
void init_opt(struct Options *dest);

/* cmds.c */
void round_number(double *dest, const int decimals);
int cmd_bear_dist(const char *cmd, const char *coor1, const char *coor2);
int cmd_bpos(const char *coor, const char *bearing_s, const char *dist_s);
int cmd_course(const char *coor1, const char *coor2, const char *numpoints_s);
int cmd_lpos(const char *coor1, const char *coor2, const char *fracdist_s);
int cmd_randpos(const char *coor, const char *maxdist, const char *mindist);
int cmd_bench(const char *seconds);

/* gpx.c */
char *xml_escape_string(const char *text);
char *gpx_wpt(const double lat, const double lon,
              const char *name, const char *cmt);

/* io.c */
void streams_init(struct streams *dest);
void streams_free(struct streams *dest);
char *read_from_fp(FILE *fp, struct binbuf *dest);
int streams_exec(struct streams *dest, char *cmd[]);
int exec_output(struct binbuf *dest, char *cmd[]);

/* selftest.c */
int opt_selftest(char *execname);

/* strings.c */
int string_to_double(const char *s, double *dest);
char *mystrdup(const char *s);
char *allocstr_va(const char *format, va_list ap);
char *allocstr(const char *format, ...);
int parse_coordinate(const char *s, bool validate,
                     double *dest_lat, double *dest_lon);

#endif /* ifndef _GEOCALC_H */

/* vim: set ts=8 sw=8 sts=8 noet fo+=w tw=79 fenc=UTF-8 : */
