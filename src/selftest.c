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
 * selftest() - Run internal testing to check that it works on the current 
 * system. Executed if --selftest is used. Returns `EXIT_FAILURE` if any tests 
 * fail; otherwise, it returns `EXIT_SUCCESS`.
 */

int selftest(void)
{
	int errcount = 0;

	diag("Running tests for %s %s (%s)",
	     progname, EXEC_VERSION, EXEC_DATE);
	errno = EACCES;
	puts("# myerror(\"errno is EACCES\")");
	myerror("errno is EACCES");
	errno = 0;
	errcount += ok(!diag(NULL), "diag(NULL)");
	errcount += ok(!ok(0, NULL), "ok(0, NULL)");

	return errcount ? EXIT_FAILURE : EXIT_SUCCESS;
}

/* vim: set ts=8 sw=8 sts=8 noet fo+=w tw=79 fenc=UTF-8 : */
