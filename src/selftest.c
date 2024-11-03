/*
 * selftest.c
 * File ID: 93fe63c6-87f2-11ef-bfa9-83850402c3ce
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
 * The functions in this file are supposed to be compatible with `Test::More` 
 * in Perl 5 as far as possible.
 */

/*
 * diag() - Prints a diagnostic message prefixed with "# " to stdout. `printf` 
 * sequences can be used. No multiline support, and no `\n` should be added to 
 * the string. Returns 0 if successful, or 1 if `format` is NULL.
 */

static int diag(const char *format, ...)
{
	va_list ap;

	if (!format)
		return 1;
	va_start(ap, format);
	printf("# ");
	vprintf(format, ap);
	puts("");
	va_end(ap);

	return 0;
}

/*
 * ok() - Print a log line to stdout. If `i` is 0, an "ok" line is printed, 
 * otherwise a "not ok" line is printed. `desc` is the test description and can 
 * use `printf` sequences. If `desc` is NULL, it returns 1. Otherwise, it 
 * returns `i`.
 */

static int ok(const int i, const char *desc, ...)
{
	static int testnum = 0;
	va_list ap;

	if (!desc)
		return 1;
	va_start(ap, desc);
	printf("%sok %d - ", (i ? "not " : ""), ++testnum);
	vprintf(desc, ap);
	puts("");
	va_end(ap);

	return i;
}

/*
 * chk_coor() - Try to parse the coordinate in `s` with `parse_coordinate()` 
 * and test that `parse_coordinate()` returns the expected values.
 * Returns the number of failed tests.
 */

static int chk_coor(const char *s, const int exp_ret,
                    const double exp_lat, const double exp_lon)
{
	double lat, lon;
	int result, r = 0;

	result = parse_coordinate(s, &lat, &lon);
	r += ok(!(result == exp_ret),
	        "parse_coordinate(\"%s\"), expected to %s",
	        s, exp_ret ? "fail" : "succeed");

	if (result)
		return r;

	r += ok(!(lat == exp_lat), "parse_coordinate(\"%s\"): lat is ok", s);
	r += ok(!(lon == exp_lon), "parse_coordinate(\"%s\"): lon is ok", s);

	return r;
}

/*
 * test_parse_coordinate() - Various tests for `parse_coordinate()`. Returns 
 * the number of failed tests.
 */

static int test_parse_coordinate(void) {
	int r = 0;

	diag("Test parse_coordinate()");
	r += chk_coor("12.34,56.78", 0, 12.34, 56.78);
	r += chk_coor("12.34", 1, 0, 0);
	r += chk_coor("", 1, 0, 0);
	r += chk_coor("995.456,,456.345", 1, 0, 0);
	r += chk_coor("56,78", 0, 56, 78);
	r += chk_coor("-56,-78", 0, -56, -78);
	r += chk_coor("-56.234,-78.345", 0, -56.234, -78.345);
	r += chk_coor(" -56.234,-78.345", 0, -56.234, -78.345);
	r += chk_coor("-56.234, -78.345", 0, -56.234, -78.345);
	r += chk_coor("56.2r4,-78.345", 1, 0, 0);
	r += chk_coor("+56.24,-78.345", 0, 56.24, -78.345);
	r += chk_coor(NULL, 1, 0, 0);

	return r;
}

/*
 * selftest() - Run internal testing to check that it works on the current 
 * system. Executed if --selftest is used. Returns `EXIT_FAILURE` if any tests 
 * fail; otherwise, it returns `EXIT_SUCCESS`.
 */

int selftest(void)
{
	int r = 0;

	diag("Running tests for %s %s (%s)",
	     progname, EXEC_VERSION, EXEC_DATE);

	diag("Test selftest routines");
	r += ok(!diag(NULL), "diag(NULL)");
	r += ok(!ok(0, NULL), "ok(0, NULL)");

	diag("Test myerror()");
	errno = EACCES;
	r += ok(!(myerror("errno is EACCES") > 37),
	        "myerror(): errno is EACCES");
	errno = 0;

	diag("Test std_strerror()");
	ok(!(std_strerror(0) != NULL), "std_strerror(0)");

	r += test_parse_coordinate();
	r += ok(!(mystrdup(NULL) == NULL), "mystrdup(NULL) == NULL");

	diag("%d test%s failed.", r, (r == 1) ? "" : "s");

	return r ? EXIT_FAILURE : EXIT_SUCCESS;
}

/* vim: set ts=8 sw=8 sts=8 noet fo+=w tw=79 fenc=UTF-8 : */
