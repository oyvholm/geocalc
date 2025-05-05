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

#define chp  (char *[])
#define failed_ok(a)  do { \
	if (errno) \
		ok(1, "%s():%d: %s failed: %s", \
		      __func__, __LINE__, (a), std_strerror(errno)); \
	else \
		ok(1, "%s():%d: %s failed", __func__, __LINE__, (a)); \
	errno = 0; \
} while (0)

static int failcount = 0;
static int testnum = 0;

/*
 * ok() - Print a log line to stdout. If `i` is 0, an "ok" line is printed, 
 * otherwise a "not ok" line is printed. `desc` is the test description and can 
 * use printf sequences.
 *
 * If `desc` is NULL, it returns 1. Otherwise, it returns `i`.
 */

static int ok(const int i, const char *desc, ...)
{
	va_list ap;

	if (!desc)
		return 1;
	va_start(ap, desc);
	printf("%sok %d - ", (i ? "not " : ""), ++testnum);
	vprintf(desc, ap);
	puts("");
	va_end(ap);
	fflush(stdout);
	failcount += !!i;

	return i;
}

/*
 * diag_output_va() - Receives a printf-like string and returns an allocated 
 * string, prefixed with "# " and all '\n' characters converted to "\n# ". 
 * Returns NULL if anything fails or `format` is NULL.
 */

static char *diag_output_va(const char *format, va_list ap)
{
	const char *src;
	char *buffer, *converted_buffer, *dst;
	size_t converted_size;

	if (!format)
		return NULL; /* gncov */

	buffer = allocstr_va(format, ap);
	if (!buffer) {
		failed_ok("allocstr_va()"); /* gncov */
		return NULL; /* gncov */
	}

	/* Prepare for worst case, every char is a newline. */
	converted_size = strlen("# ") + strlen(buffer) * 3 + 1;
	converted_buffer = malloc(converted_size);
	if (!converted_buffer) {
		free(buffer); /* gncov */
		return NULL; /* gncov */
	}

	src = buffer;
	dst = converted_buffer;
	*dst++ = '#';
	*dst++ = ' ';
	while (*src) {
		if (*src == '\n') {
			*dst++ = '\n';
			*dst++ = '#';
			*dst++ = ' ';
		} else {
			*dst++ = *src;
		}
		src++;
	}
	*dst = '\0';
	free(buffer);

	return converted_buffer;
}

/*
 * diag_output() - Frontend against diag_output_va(), used by the tests. 
 * Returns the value from diag_output_va(); a pointer to the allocated string, 
 * or NULL if anything failed.
 */

static char *diag_output(const char *format, ...)
{
	va_list ap;
	char *result;

	if (!format)
		return NULL;

	va_start(ap, format);
	result = diag_output_va(format, ap);
	va_end(ap);

	return result;
}

/*
 * diag() - Prints a diagnostic message prefixed with "# " to stdout. `printf` 
 * sequences can be used. All `\n` characters are converted to "\n# ".
 *
 * A terminating `\n` is automatically added to the string. Returns 0 if 
 * successful, or 1 if `format` is NULL or diag_output_va() failed.
 */

static int diag(const char *format, ...)
{
	va_list ap;
	char *converted_buffer;

	if (!format)
		return 1;

	va_start(ap, format);
	converted_buffer = diag_output_va(format, ap);
	va_end(ap);
	if (!converted_buffer) {
		failed_ok("diag_output_va()"); /* gncov */
		return 1; /* gncov */
	}
	fprintf(stderr, "%s\n", converted_buffer);
	fflush(stderr);
	free(converted_buffer);

	return 0;
}

/*
 * gotexp_output() - Generate the output used by print_gotexp(). The output is 
 * returned as an allocated string that must be free()'ed after use. Returns 
 * NULL if `got` or `exp` is NULL or allocstr() fails. Otherwise, it returns a 
 * pointer to the string with the output.
 */

static char *gotexp_output(const char *got, const char *exp)
{
	char *s;

	if (!got || !exp)
		return NULL;

	s = allocstr("         got: '%s'\n"
	             "    expected: '%s'",
	             got, exp);
	if (!s)
		failed_ok("allocstr()"); /* gncov */

	return s;
}

/*
 * print_gotexp() - Print the value of the actual and exepected data. Used when 
 * a test fails. Returns 1 if `got` or `exp` is NULL, otherwise 0.
 */

static int print_gotexp(const char *got, const char *exp)
{
	char *s;

	if (!got || !exp)
		return 1;
	if (!strcmp(got, exp))
		return 0;

	s = gotexp_output(got, exp); /* gncov */
	diag(s); /* gncov */
	free(s); /* gncov */

	return 0; /* gncov */
}

/*
 * tc_cmp() - Comparison function used by test_command(). There are 2 types of 
 * verification: One that demands that the whole output must be identical to 
 * the expected value, and the other is just a substring search. `got` is the 
 * actual output from the program, and `exp` is the expected output or 
 * substring.
 *
 * If `identical` is 0 (substring search) and `exp` is empty, the output in 
 * `got` must also be empty for the test to succeed.
 *
 * Returns 0 if the string was found, otherwise 1.
 */

static int tc_cmp(const int identical, const char *got, const char *exp)
{
	assert(got);
	assert(exp);
	if (!got || !exp)
		return 1; /* gncov */

	if (identical || !strlen(exp))
		return !!strcmp(got, exp);

	return !strstr(got, exp);
}

/*
 * valgrind_lines() - Searches for Valgrind markers ("\n==DIGITS==") in `s`, 
 * used by test_command(). If a marker is found or `s` is NULL, it returns 1. 
 * Otherwise, it returns 0.
 */

static int valgrind_lines(const char *s)
{
	const char *p = s;

	if (!s)
		return ok(1, "%s(): s == NULL", __func__); /* gncov */

	while (*p) {
		p = strstr(p, "\n==");
		if (!p)
			return 0;
		p += 3;
		if (!*p)
			return 0;
		if (!isdigit(*p))
			continue;
		while (isdigit(*p))
			p++;
		if (!*p)
			return 0;
		if (!strncmp(p, "==", 2))
			return 1;
		p++;
	}

	return 0;
}

/*
 * test_command() - Run the executable with arguments in `cmd` and verify 
 * stdout, stderr and the return value against `exp_stdout`, `exp_stderr` and 
 * `exp_retval`. Returns nothing.
 */

static void test_command(const char identical, char *cmd[],
                         const char *exp_stdout, const char *exp_stderr,
                         const int exp_retval, const char *desc, va_list ap)
{
	struct streams ss;
	char *descbuf;

	assert(cmd);
	if (!cmd) {
		ok(1, "%s(): cmd is NULL", __func__); /* gncov */
		return; /* gncov */
	}

	if (opt.verbose >= 4) {
		int i = -1; /* gncov */
		fprintf(stderr, "# %s(", __func__); /* gncov */
		while (cmd[++i]) /* gncov */
			fprintf(stderr, "%s\"%s\"", /* gncov */
			                i ? ", " : "", cmd[i]); /* gncov */
		fprintf(stderr, ")\n"); /* gncov */
	}

	descbuf = allocstr_va(desc, ap);
	if (!descbuf) {
		failed_ok("allocstr_va()"); /* gncov */
		return; /* gncov */
	}
	streams_init(&ss);
	streams_exec(&ss, cmd);
	if (exp_stdout) {
		ok(tc_cmp(identical, ss.out.buf, exp_stdout),
		   "%s (stdout)", descbuf);
		if (tc_cmp(identical, ss.out.buf, exp_stdout))
			print_gotexp(ss.out.buf, exp_stdout); /* gncov */
	}
	if (exp_stderr) {
		ok(tc_cmp(identical, ss.err.buf, exp_stderr),
		   "%s (stderr)", descbuf);
		if (tc_cmp(identical, ss.err.buf, exp_stderr))
			print_gotexp(ss.err.buf, exp_stderr); /* gncov */
	}
	ok(!(ss.ret == exp_retval), "%s (retval)", descbuf);
	free(descbuf);
	if (ss.ret != exp_retval) {
		char *g = allocstr("%d", ss.ret), /* gncov */
		     *e = allocstr("%d", exp_retval); /* gncov */
		if (!g || !e) /* gncov */
			failed_ok("allocstr()"); /* gncov */
		else
			print_gotexp(g, e); /* gncov */
		free(e); /* gncov */
		free(g); /* gncov */
	}
	if (valgrind_lines(ss.err.buf))
		ok(1, "Found valgrind output"); /* gncov */
	streams_free(&ss);
}

/*
 * sc() - Execute command `cmd` and verify that stdout, stderr and the return 
 * value corresponds to the expected values. The `exp_*` variables are 
 * substrings that must occur in the actual output. Returns nothing.
 */

static void sc(char *cmd[], const char *exp_stdout, const char *exp_stderr,
               const int exp_retval, const char *desc, ...)
{
	va_list ap;

	va_start(ap, desc);
	test_command(0, cmd, exp_stdout, exp_stderr, exp_retval, desc, ap);
	va_end(ap);
}

/*
 * tc() - Execute command `cmd` and verify that stdout, stderr and the return 
 * value are identical to the expected values. The `exp_*` variables are 
 * strings that must be identical to the actual output. Returns nothing.
 */

static void tc(char *cmd[], const char *exp_stdout, const char *exp_stderr,
               const int exp_retval, const char *desc, ...)
{
	va_list ap;

	va_start(ap, desc);
	test_command(1, cmd, exp_stdout, exp_stderr, exp_retval, desc, ap);
	va_end(ap);
}

/*
 ******************
 * Function tests *
 ******************
 */

/*
 * selftest functions
 */

/*
 * verify_constants() - Check that certain constants are unmodified. Some of 
 * these constants are used in the tests themselves, so the tests will be 
 * automatically updated to match the new text or value. This function contains 
 * hardcoded versions of the expected output or return values. Returns nothing.
 */

static void verify_constants(void)
{
	const char *e;

	diag("Verify constants");

	e = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	    "<gpx"
	    " xmlns=\"http://www.topografix.com/GPX/1/1\""
	    " version=\"1.1\""
	    " creator=\"Geocalc - https://gitlab.com/oyvholm/geocalc\""
	    ">\n";
	ok(!!strcmp(GPX_HEADER, e), "GPX_HEADER is correct");
	print_gotexp(GPX_HEADER, e);

	e = "Geocalc";
	ok(!!strcmp(PROJ_NAME, e), "PROJ_NAME is correct");
	print_gotexp(PROJ_NAME, e);

	e = "https://gitlab.com/oyvholm/geocalc";
	ok(!!strcmp(PROJ_URL, e), "PROJ_URL is correct");
	print_gotexp(PROJ_URL, e);
}

/*
 * test_diag_big() - Tests diag_output() with a string larger than BUFSIZ. 
 * Returns nothing.
 */

static void test_diag_big(void)
{
	size_t size;
	char *p, *outp;

	size = BUFSIZ * 2;
	p = malloc(size + 1);
	if (!p) {
		failed_ok("malloc()"); /* gncov */
		return; /* gncov */
	}

	memset(p, 'a', size);
	p[3] = 'b';
	p[4] = 'c';
	p[size] = '\0';

	outp = diag_output("%s", p);
	ok(!outp, "diag_big: diag_output() returns ok");
	ok(!(strlen(outp) == size + 2), "diag_big: String length is correct");
	ok(!!strncmp(outp, "# aaabcaaa", 10), "diag_big: Beginning is ok");
	free(outp);
	free(p);
}

/*
 * test_diag() - Tests the diag_output() function. diag() can't be tested 
 * directly because it would pollute the the test output. Returns nothing.
 */

static void test_diag(void) {
	char *p, *s;

	diag("Test diag()");

	ok(!diag(NULL), "diag(NULL)");
	ok(!(diag_output(NULL) == NULL), "diag_output() receives NULL");

	p = diag_output("Text with\nnewline");
	ok(!p, "diag_output() with newline didn't return NULL");
	s = "# Text with\n# newline";
	ok(p ? !!strcmp(p, s) : 1, "diag_output() with newline, output is ok");
	print_gotexp(p, s);
	free(p);

	p = diag_output("\n\n\n\n\n\n\n\n\n\n");
	ok(!p, "diag_output() with only newlines didn't return NULL");
	s = "# \n# \n# \n# \n# \n# \n# \n# \n# \n# \n# ";
	ok(p ? !!strcmp(p, s) : 1, "diag_output() with only newlines");
	print_gotexp(p, s);
	free(p);

	p = diag_output("%d = %s, %d = %s, %d = %s",
	                1, "one", 2, "two", 3, "three");
	ok(!p, "diag_output() with %%d and %%s didn't return NULL");
	s = "# 1 = one, 2 = two, 3 = three";
	ok(p ? !!strcmp(p, s) : 1, "diag_output() with %%d and %%s");
	print_gotexp(p, s);
	free(p);

	test_diag_big();
}

/*
 * test_gotexp_output() - Tests the gotexp_output() function. print_gotexp() 
 * can't be tested directly because it would pollute stderr. Returns nothing.
 */

static void test_gotexp_output(void)
{
	char *p, *s;

	diag("Test gotexp_output()");

	ok(!!gotexp_output(NULL, "a"), "gotexp_output(NULL, \"a\")");

	ok(!!strcmp((p = gotexp_output("got this", "expected this")),
	            "         got: 'got this'\n"
	            "    expected: 'expected this'"),
	   "gotexp_output(\"got this\", \"expected this\")");
	free(p);

	ok(!print_gotexp(NULL, "expected this"),
	   "print_gotexp(): Arg is NULL");

	s = "gotexp_output(\"a\", \"a\")";
	ok(!(p = gotexp_output("a", "a")), "%s doesn't return NULL", s);
	ok(!!strcmp(p, "         got: 'a'\n    expected: 'a'"),
	   "%s: Contents is ok", s);
	free(p);

	s = "gotexp_output() with newline";
	ok(!(p = gotexp_output("with\nnewline", "also with\nnewline")),
	   "%s: Doesn't return NULL", s);
	ok(!!strcmp(p, "         got: 'with\nnewline'\n"
	               "    expected: 'also with\nnewline'"),
	   "%s: Contents is ok", s);
	free(p);
}

/*
 * test_valgrind_lines() - Test the behavior of valgrind_lines(). Returns 
 * nothing.
 */

static void test_valgrind_lines(void)
{
	int i;
	const char
	*has[] = {
		"\n==123==",
		"\n==154363456657465745674567456523==maybe",
		"\n==\n==123==maybe",
		"\n==\n==123==maybe==456==",
		"indeed\n==1==",
		NULL
	},
	*hasnot[] = {
		"",
		"==123==",
		"\n=",
		"\n=123== \n234==",
		"\n=123==",
		"\n== 234==",
		"\n==",
		"\n==12.3==",
		"\n==123",
		"\n==123=",
		"\n==jj==",
		"abc",
		"abc\n==",
		NULL
	};

	diag("Test valgrind_lines()");

	i = 0;
	while (has[i]) {
		ok(!valgrind_lines(has[i]),
		   "valgrind_lines(): Has valgrind marker, string %d", i);
		i++;
	}

	i = 0;
	while (hasnot[i]) {
		ok(valgrind_lines(hasnot[i]),
		   "valgrind_lines(): No valgrind marker, string %d", i);
		i++;
	}
}

/*
 * Various functions
 */

/*
 * test_allocstr() - Tests the allocstr() function. Returns nothing.
 */

static void test_allocstr(void)
{
	const size_t bufsize = BUFSIZ * 2 + 1;
	char *p, *p2, *p3;
	size_t alen;

	diag("Test allocstr()");
	p = malloc(bufsize);
	if (!p) {
		failed_ok("malloc()"); /* gncov */
		return; /* gncov */
	}
	memset(p, 'a', bufsize - 1);
	p[bufsize - 1] = '\0';
	p2 = allocstr("%s", p);
	if (!p2) {
		failed_ok("allocstr() with BUFSIZ * 2"); /* gncov */
		goto free_p; /* gncov */
	}
	alen = strlen(p2);
	ok(!(alen == BUFSIZ * 2), "allocstr(): strlen is correct");
	p3 = p2;
	while (*p3) {
		if (*p3 != 'a') {
			p3 = NULL; /* gncov */
			break; /* gncov */
		}
		p3++;
	}
	ok(!(p3 != NULL), "allocstr(): Content of string is correct");
	free(p2);
free_p:
	free(p);
}

/*
 * chk_round() - Used by test_round_number(). Verifies that `round_number(num, 
 * decimals)` results in `exp_result`. Returns nothing.
 */

static void chk_round(const double num, const int decimals,
                      const double exp_result)
{
	double res = num;

	round_number(&res, decimals);
	ok(!(res == exp_result), "round_number(%.13f, %d)", num, decimals);
	if (res != exp_result) {
		diag("got: %.13f", res); /* gncov */
		diag("exp: %.13f", exp_result); /* gncov */
	}
}

/*
 * test_round_number() - Tests the round_number() function. Returns nothing.
 */

static void test_round_number(void)
{
	diag("Test round_number()");

	chk_round(-0.0, 0, 0.0);
	chk_round(-0.0, 2, 0.0);
	chk_round(-0.0000001, 2, 0.0);
	chk_round(-13.124, 2, -13.12);
	chk_round(-13.125, 2, -13.13);
	chk_round(-99.9949999, 3, -99.995);
	chk_round(-99.9959999, 2, -100.0);
	chk_round(-99.999999, 0, -100.0);
	chk_round(1.124, 2, 1.12);
	chk_round(1.125, 2, 1.13);
	chk_round(91.123, 0, 91.0);
	chk_round(99.999499, 3, 99.999);
	chk_round(99.999999, 3, 100.0);
	chk_round(99.999999999999, 9, 100.0);
}

/*
 * chk_rand_pos() - Used by test_rand_pos(). Executes rand_pos() with the 
 * values in `coor`, `maxdist` and `mindist` and checks that they're in the 
 * range defined by `exp_maxdist` and `exp_mindist`. Returns nothing.
 */

static void chk_rand_pos(const char *coor,
                         const double maxdist, const double mindist,
                         const double exp_maxdist, const double exp_mindist)
{
	int errcount = 0, maxtests = 20;
	unsigned long l, numloop = 1e+5;
	double r_exp_max = round(exp_maxdist), r_exp_min = round(exp_mindist);

	for (l = 0; l < numloop; l++) {
		double clat = 1000.0, clon = 1000.0, rlat, rlon, dist, r_dist;
		if (coor && parse_coordinate(coor, &clat, &clon)) {
			ok(1, "%s():%d: parse_coordinate() failed," /* gncov */
			      " coor = \"%s\", errno = %d (%s)",
			      __func__, __LINE__, coor,
			      errno, std_strerror(errno)); /* gncov */
			errno = 0; /* gncov */
			return; /* gncov */
		}
		rand_pos(&rlat, &rlon, clat, clon, maxdist, mindist);
		if (fabs(rlat) > 90.0) {
			ok(1, "rand_pos(): Coordinate %lu:" /* gncov */
			      " lat is outside [-90,90] range,"
			      " lat = %.15f", l, rlat);
			errcount++; /* gncov */
		}
		if (fabs(rlon) > 180.0) {
			ok(1, "rand_pos(): Coordinate %lu:" /* gncov */
			      " lon is outside [-180,180] range,"
			      " lon = %.15f", l, rlon);
			errcount++; /* gncov */
		}
		if (errcount >= maxtests) {
			diag("Aborting rand_pos() range test after" /* gncov */
			     " %d errors", maxtests);
			break; /* gncov */
		}
		if (!coor)
			continue;
		dist = haversine(clat, clon, rlat, rlon);
		r_dist = round(dist);
		if (r_dist < r_exp_min || r_dist > r_exp_max) {
			errcount <= maxtests /* gncov */
			? ok(1, "randpos out of range (%.0f" /* gncov */
			        " to %.0f m), center = %f,%f randpos ="
			        " %f,%f dist = %f",
			        exp_mindist, exp_maxdist, clat, clon,
			        rlat, rlon, dist)
			: 1; /* gncov */
			errcount++; /* gncov */
		}
	}
	ok(!!errcount, "rand_pos(): All %lu coordinates %.0f-%.0f m are in"
	               " range, failed = %lu (%f%%)",
	               numloop, exp_mindist, exp_maxdist, errcount,
	               100.0 * (double)errcount / (double)numloop);
}

/*
 * test_rand_pos() - Tests the rand_pos() function. Returns nothing.
 */

static void test_rand_pos(void)
{
	double MED = MAX_EARTH_DISTANCE;

	diag("Test rand_pos()");

	chk_rand_pos("-3.14,-123.45", 1e+20, 1e+20, MED, MED);
	chk_rand_pos("-3.14,-123.45", 1e+20, 1e+7, MED, 1e+7);
	chk_rand_pos("-3.14,-123.45", 1e+7 + 1, 1e+20, MED, 1e+7 + 1);
	chk_rand_pos("-55.91,-107.32", 0, 2e+7, MED, 2e+7);
	chk_rand_pos("-59.2105,44.47485", 1000.0, 1000.0, 1000.0, 1000.0);
	chk_rand_pos("-90,0", 10.0, 0.0, 10.0, 0.0);
	chk_rand_pos("12,34", 0.1, 0.0, 0.0, 0.0);
	chk_rand_pos("12,34", 1.0, 0.0, 1.0, 0.0);
	chk_rand_pos("12,34", 10.0, 20.0, 20.0, 10.0);
	chk_rand_pos("65,7", 2000.0, 1000.0, 2000.0, 1000.0);
	chk_rand_pos("90,0", 0.0, 1e+6, MED, 1e+6);
	chk_rand_pos(NULL, 0.0, 0.0, MED, 0.0);
}

/*
 * test_streams_exec() - Tests the streams_exec() function. Returns nothing.
 */

static void test_streams_exec(char *execname)
{
	bool orig_valgrind;
	struct streams ss;
	char *s;

	diag("Test streams_exec()");

	diag("Send input to the program");
	streams_init(&ss);
	ss.in.buf = "This is sent to stdin.\n";
	ss.in.len = strlen(ss.in.buf);
	orig_valgrind = opt.valgrind;
	opt.valgrind = false;
	streams_exec(&ss, chp{ execname, NULL });
	opt.valgrind = orig_valgrind;
	s = "streams_exec(execname) with stdin data";
	ok(!!strcmp(ss.out.buf, ""), "%s (stdout)", s);
	ok(!strstr(ss.err.buf, ": No arguments specified\n"),
	   "%s (stderr)", s);
	ok(!(ss.ret == EXIT_FAILURE), "%s (retval)", s);
	streams_free(&ss);
}

/*
 * chk_coor() - Try to parse the coordinate in `s` with `parse_coordinate()` 
 * and test that `parse_coordinate()` returns the expected values.
 * Returns nothing.
 */

static void chk_coor(const char *s, const int exp_ret,
                     const double exp_lat, const double exp_lon)
{
	double lat, lon;
	int result;

	result = parse_coordinate(s, &lat, &lon);
	ok(!(result == exp_ret), "parse_coordinate(\"%s\"), expected to %s",
	                         s, exp_ret ? "fail" : "succeed");

	if (result)
		return;

	ok(!(lat == exp_lat), "parse_coordinate(\"%s\"): lat is ok", s);
	ok(!(lon == exp_lon), "parse_coordinate(\"%s\"): lon is ok", s);
}

/*
 * test_parse_coordinate() - Various tests for `parse_coordinate()`. Returns 
 * nothing.
 */

static void test_parse_coordinate(void) {
	diag("Test parse_coordinate()");
	chk_coor("12.34,56.78", 0, 12.34, 56.78);
	chk_coor("12.34", 1, 0, 0);
	chk_coor("", 1, 0, 0);
	chk_coor("995.456,,456.345", 1, 0, 0);
	chk_coor("56,78", 0, 56, 78);
	chk_coor("-56,-78", 0, -56, -78);
	chk_coor("-56.234,-78.345", 0, -56.234, -78.345);
	chk_coor(" -56.234,-78.345", 0, -56.234, -78.345);
	chk_coor("-56.234, -78.345", 0, -56.234, -78.345);
	chk_coor("56.2r4,-78.345", 1, 0, 0);
	chk_coor("+56.24,-78.345", 0, 56.24, -78.345);
	chk_coor(NULL, 1, 0, 0);
}

/*
 * chk_xmlesc() - Helper function used by test_xml_escape_string(). `s` is the 
 * string to test, and `exp` is the expected outcome. Returns nothing.
 */

static void chk_xmlesc(const char *s, const char *exp)
{
	char *got;

	assert(s);
	assert(exp);
	if (!s || !exp) {
		ok(1, "%s() received NULL", __func__); /* gncov */
		return; /* gncov */
	}

	got = xml_escape_string(s);
	if (!got) {
		failed_ok("xml_escape_string()"); /* gncov */
		return; /* gncov */
	}
	ok(!!strcmp(got, exp), "xml escape: \"%s\"", s);
	free(got);
}

/*
 * chk_antip() - Used by test_are_antipodal(), checks that are_antipodal() 
 * returns the correct answer for the coordinates. The expected return value is 
 * stored in `exp`. Returns nothing.
 */

static void chk_antip(const char *coor1, const char *coor2, const int exp)
{
	int result;
	double lat1, lon1, lat2, lon2;

	if (!coor1 || !coor2) {
		ok(1, "%s(): coor1 or coor2 is NULL", __func__); /* gncov */
		return; /* gncov */
	}

	if (parse_coordinate(coor1, &lat1, &lon1)
	    || parse_coordinate(coor2, &lat2, &lon2)) {
		failed_ok("parse_coordinate()"); /* gncov */ 
		return; /* gncov */
	}

	result = are_antipodal(lat1, lon1, lat2, lon2);
	ok(!(result == exp), "are_antipodal(): \"%s\" and \"%s\", expects %s",
	                     coor1, coor2, exp ? "yes" : "no");
}

/*
 * test_are_antipodal() - Tests the are_antipodal() function. Returns nothing.
 */

static void test_are_antipodal(void)
{
	diag("Test are_antipodal()");
	chk_antip("-0.00000000001,0", "0,180.0", 1);
	chk_antip("-0.0000000001,0", "0,180.0", 0);
	chk_antip("0,0", "0,179.999999999", 0);
	chk_antip("0,0", "0,179.9999999999", 1);
	chk_antip("0,0", "0,180", 1);
	chk_antip("0.00000000001,0", "0,180.0", 1);
	chk_antip("0.0000000001,0", "0,180.0", 0);
	chk_antip("36.988716,-9.604127", "-36.988716,170.395873", 1);
	chk_antip("36.988716,-9.6041270001", "-36.988716,170.395873", 1);
	chk_antip("36.988716,-9.604127001", "-36.988716,170.395873", 0);
	chk_antip("60,5", "-60,-175", 1);
	chk_antip("89.9999999999,0", "-90,0", 0);
	chk_antip("89.99999999999,0", "-90,0", 1);
	chk_antip("90,0", "-90,0", 1);
}

/*
 * test_xml_escape_string() - Tests the xml_escape_string() function. Returns 
 * nothing.
 */

static void test_xml_escape_string(void)
{
	diag("Test xml_escape_string()");
	chk_xmlesc("", "");
	chk_xmlesc("&", "&amp;");
	chk_xmlesc("<", "&lt;");
	chk_xmlesc(">", "&gt;");
	chk_xmlesc("\\", "\\");
	chk_xmlesc("a&c", "a&amp;c");
	chk_xmlesc("a<c", "a&lt;c");
	chk_xmlesc("a>c", "a&gt;c");
	chk_xmlesc("abc", "abc");
	ok(!!xml_escape_string(NULL), "xml_escape_string(NULL)");
}

/*
 * test_gpx_wpt() - Tests the gpx_wpt() function. Returns nothing.
 */

static void test_gpx_wpt(void)
{
	char *p, /* String sent to gpx_wpt() */
	     *c, /* Converted version of `p` */
	     *e, /* Expected string from gpx_wpt() */
	     *s; /* Result from gpx_wpt() */

	diag("Test gpx_wpt()");

	e = "  <wpt lat=\"12.340000\" lon=\"56.780000\">\n"
	    "    <name>abc def</name>\n"
	    "    <cmt>ghi jkl MN</cmt>\n"
	    "  </wpt>\n";
	s = gpx_wpt(12.34, 56.78, "abc def", "ghi jkl MN");
	ok((s ? !!strcmp(s, e) : 1), "gpx_wpt() without special chars");
	print_gotexp(s, e);
	free(s);

	e = "  <wpt lat=\"12.340000\" lon=\"56.780000\">\n"
	    "    <name>&amp;</name>\n"
	    "    <cmt>&amp;</cmt>\n"
	    "  </wpt>\n";
	s = gpx_wpt(12.34, 56.78, "&", "&");
	ok((s ? !!strcmp(s, e) : 1), "gpx_wpt() with ampersand");
	print_gotexp(s, e);
	free(s);

	e = "  <wpt lat=\"12.340000\" lon=\"56.780000\">\n"
	    "    <name>&lt;</name>\n"
	    "    <cmt>&lt;</cmt>\n"
	    "  </wpt>\n";
	s = gpx_wpt(12.34, 56.78, "<", "<");
	ok((s ? !!strcmp(s, e) : 1), "gpx_wpt() with lt");
	print_gotexp(s, e);
	free(s);

	e = "  <wpt lat=\"12.340000\" lon=\"56.780000\">\n"
	    "    <name>&gt;</name>\n"
	    "    <cmt>&gt;</cmt>\n"
	    "  </wpt>\n";
	s = gpx_wpt(12.34, 56.78, ">", ">");
	ok((s ? !!strcmp(s, e) : 1), "gpx_wpt() with gt");
	print_gotexp(s, e);
	free(s);

	p = "asdf < & > <\" &&lt; < & abc\n"
	    " >;;;&lt;;;åæø;abc🤘def\n"
	    "def>/<€>;&amp;&<&gt;>&\n"
	    "\\$e=19;\n";
	c = "asdf &lt; &amp; &gt; &lt;\" &amp;&amp;lt; &lt; &amp; abc\n"
	    " &gt;;;;&amp;lt;;;åæø;abc🤘def\n"
	    "def&gt;/&lt;€&gt;;&amp;amp;&amp;&lt;&amp;gt;&gt;&amp;\n"
	    "\\$e=19;\n";
	e = allocstr("  <wpt lat=\"12.340000\" lon=\"56.780000\">\n"
	             "    <name>%s</name>\n"
	             "    <cmt>%s</cmt>\n"
	             "  </wpt>\n", c, c);
	s = gpx_wpt(12.34, 56.78, p, p);
	ok((s ? !!strcmp(s, e) : 1), "gpx_wpt() with amp, gt, lt, and more");
	print_gotexp(s, e);
	free(s);
	free(e);

	p = NULL;
	e = NULL;
	s = gpx_wpt(12.34, 56.78, p, p);
	ok(!!s, "gpx_wpt() with NULL in name and cmt");
	print_gotexp(s, e);
	if (s) {
		ok(1, "%s():%d: `s` was allocated", /* gncov */
		      __func__, __LINE__);
		free(s); /* gncov */
	}

	p = NULL;
	e = NULL;
	s = gpx_wpt(12.34, 56.78, p, "def");
	ok(!!s, "gpx_wpt() with NULL in name");
	print_gotexp(s, e);
	if (s) {
		ok(1, "%s():%d: `s` was allocated", /* gncov */
		      __func__, __LINE__);
		free(s); /* gncov */
	}

	s = gpx_wpt(12.34, 56.78, "abc", NULL);
	ok(!s, "gpx_wpt() with NULL in cmt");
	free(s);
}

/*
 * chk_karney() - Used by test_karney_distance(). Verifies that 
 * `karney_distance(coor1, coor2)` returns the value in `exp_result`. Returns 
 * nothing.
 */

static void chk_karney(const char *coor1, const char *coor2,
                       const double exp_result)
{
	double lat1, lon1, lat2, lon2;
	double exp_res = exp_result, result;
	char *res_s, *exp_s;

	if (parse_coordinate(coor1, &lat1, &lon1)
	    || parse_coordinate(coor2, &lat2, &lon2)) {
		ok(1, "%s() received invalid coordinate", /* gncov */
		      __func__);
		return; /* gncov */
	}
	result = karney_distance(lat1, lon1, lat2, lon2);
	res_s = allocstr("%.8f", result);
	exp_s = allocstr("%.8f", exp_result);
	if (!res_s || !exp_s) {
		failed_ok("allocstr()"); /* gncov */
		free(exp_s); /* gncov */
		free(res_s); /* gncov */
		return; /* gncov */
	}
	ok(!!strcmp(res_s, exp_s), "karney_distance(): %s %s", coor1, coor2);
	if (strcmp(res_s, exp_s)) {
		print_gotexp(res_s, exp_s); /* gncov */
		diag("        diff: %.15f", result - exp_res); /* gncov */
	}
	free(exp_s);
	free(res_s);
}

/*
 * test_karney_distance() - Tests the karney_distance() function. Returns 
 * nothing.
 */

static void test_karney_distance(void)
{
	diag("Test karney_distance()");

	chk_karney("0,0", "0,0.00000000000001", 0.0);
	chk_karney("0,0", "0,0.0000000000001", 0.00000001);
	chk_karney("0,0", "0,180", nan(""));
	chk_karney("0,0", "0.00000000000001,0", 0.0);
	chk_karney("0,0", "0.0000000000001,0", 0.00000001);
	chk_karney("45,9", "-45,-171", nan(""));
	chk_karney("7,7", "7,7", 0.0);
	chk_karney("90,0", "-90,0", 20003931.4586235844);
	chk_karney("90,180", "-90,180", 20003931.4586235844);
	chk_karney("90,37.37", "-90,37.37", 20003931.4586235844);

	/*
	 * Generated with
	 *
	 * `for f in $(seq 1 20); do (c1=$(./geocalc randpos); c2=$(./geocalc 
	 * randpos); echo "chk_karney(\"$c1\", \"$c2\", $(./geocalc -K dist $c1 
	 * $c2));"); done`
	 *
	 * with a version compiled with KARNEY_DECIMALS=10
	 */

	chk_karney("-0.086162,167.759028", "11.437259,153.119746", 2060263.15110668);
	chk_karney("-0.316970,-125.026617", "11.201875,-139.665899", 2060454.26482576);
	chk_karney("-0.547782,-57.812261", "10.966681,-72.451543", 2060637.67237515);
	chk_karney("-23.521104,-16.569585", "-11.495340,-31.208868", 2043439.35405501);
	chk_karney("-23.773066,50.644770", "-11.730970,36.005488", 2043024.45155100);
	chk_karney("-52.433055,91.887446", "-36.359012,77.248164", 2124196.47358499);
	chk_karney("-53.196823,-133.683843", "-36.934352,-148.323125", 2134461.58793276);
	chk_karney("-53.583847,-66.469488", "-37.223653,-81.108770", 2139944.21629653);
	chk_karney("-53.974449,0.744868", "-37.514068,-13.894415", 2145674.54815034);
	chk_karney("0.144644,100.544672", "11.672840,85.905390", 2060064.68360481);
	chk_karney("0.375452,33.330317", "11.908621,18.691035", 2059858.80968563);
	chk_karney("23.082171,59.301997", "36.288361,44.662714", 2032333.49135906);
	chk_karney("23.333298,-7.912359", "36.575232,-22.551641", 2032541.47454734);
	chk_karney("23.584900,-75.126714", "36.863173,-89.765997", 2032782.84901715);
	chk_karney("24.089562,150.444575", "37.442341,135.805292", 2033370.13841218);
	chk_karney("52.151310,-116.369390", "81.663040,-131.008672", 3328004.48296593);
	chk_karney("52.529079,176.416254", "83.444023,161.776972", 3475511.53412101);
	chk_karney("52.910126,109.201899", "85.938907,94.562617", 3700004.85902363);
	chk_karney("53.294554,41.987543", "-86.843207,27.348261", 15567459.61860570);
	chk_karney("53.682474,-25.226812", "-83.963037,-39.866094", 15297527.07716241);
}

/*
 ****************
 * Option tests *
 ****************
 */

/*
 * test_valgrind_option() - Tests the --valgrind command line option. Returns 
 * nothing.
 */

static void test_valgrind_option(char *execname)
{
	struct streams ss;

	diag("Test --valgrind");

	if (opt.valgrind) {
		opt.valgrind = false; /* gncov */
		streams_init(&ss); /* gncov */
		streams_exec(&ss, chp{"valgrind", "--version", /* gncov */
		                      NULL});
		if (!strstr(ss.out.buf, "valgrind-")) { /* gncov */
			ok(1, "Valgrind is not installed," /* gncov */
			      " disabling Valgrind checks");
		} else {
			ok(0, "Valgrind is installed"); /* gncov */
			opt.valgrind = true; /* gncov */
		}
		streams_free(&ss); /* gncov */
	}

	sc(chp{execname, "--valgrind", "-h", NULL},
	   "Show this",
	   "",
	   EXIT_SUCCESS,
	   "--valgrind -h");
}

/*
 * test_standard_options() - Tests the various generic options available in 
 * most programs. Returns nothing.
 */

static void test_standard_options(char *execname)
{
	char *s;

	diag("Test standard options");

	diag("Test -h/--help");
	sc(chp{ execname, "-h", NULL },
	   "  Show this help",
	   "",
	   EXIT_SUCCESS,
	   "-h");
	sc(chp{ execname, "--help", NULL },
	   "  Show this help",
	   "",
	   EXIT_SUCCESS,
	   "--help");

	diag("Test -v/--verbose");
	sc(chp{ execname, "-h", "--verbose", NULL },
	   "  Show this help",
	   "",
	   EXIT_SUCCESS,
	   "-hv: Help text is displayed");
	sc(chp{ execname, "-hv", NULL },
	   EXEC_VERSION,
	   "",
	   EXIT_SUCCESS,
	   "-hv: Version number is printed along with the help text");
	sc(chp{ execname, "-vvv", "--verbose", "--help", NULL },
	   "  Show this help",
	   ": main(): Using verbose level 4\n",
	   EXIT_SUCCESS,
	   "-vvv --verbose: Using correct verbose level");
	sc(chp{ execname, "-vvvvq", "--verbose", "--verbose", "--help", NULL },
	   "  Show this help",
	   ": main(): Using verbose level 5\n",
	   EXIT_SUCCESS,
	   "--verbose: One -q reduces the verbosity level");

	diag("Test --version");
	s = allocstr("%s %s (%s)\n", execname, EXEC_VERSION, EXEC_DATE);
	if (s) {
		sc(chp{ execname, "--version", NULL },
		   s,
		   "",
		   EXIT_SUCCESS,
		   "--version");
		free(s);
	} else {
		failed_ok("allocstr()"); /* gncov */
	}
	tc(chp{ execname, "--version", "-q", NULL },
	   EXEC_VERSION "\n",
	   "",
	   EXIT_SUCCESS,
	   "--version with -q shows only the version number");

	diag("Test --license");
	sc(chp{ execname, "--license", NULL },
	   "GNU General Public License",
	   "",
	   EXIT_SUCCESS,
	   "--license: It's GPL");
	sc(chp{ execname, "--license", NULL },
	   "either version 2 of the License",
	   "",
	   EXIT_SUCCESS,
	   "--license: It's version 2 of the GPL");

	diag("Unknown option");
	sc(chp{ execname, "--gurgle", NULL },
	   "",
	   ": Option error\n",
	   EXIT_FAILURE,
	   "Unknown option: \"Option error\" message is printed");
	sc(chp{ execname, "--gurgle", NULL },
	   "",
	   " --help\" for help screen. Returning with value 1.\n",
	   EXIT_FAILURE,
	   "Unknown option mentions --help");
}

/*
 * test_format_option() - Tests the -F/--format option. Returns nothing.
 */

static void test_format_option(char *execname)
{
	diag("Test -F/--format");
	sc(chp{execname, "-vvvv", "-F", "FoRmAt", NULL},
	   "",
	   ": setup_options(): o.format = \"FoRmAt\"\n",
	   EXIT_FAILURE,
	   "-vvvv -F FoRmAt: o.format is correct");
	sc(chp{execname, "-vvvv", "--format", "FoRmAt", NULL},
	   "",
	   ": setup_options(): o.format = \"FoRmAt\"\n",
	   EXIT_FAILURE,
	   "-vvvv --format FoRmAt: o.format is correct");
	sc(chp{execname, "-vvvv", "-F", "FoRmAt", NULL},
	   NULL,
	   ": FoRmAt: Unknown output format\n",
	   EXIT_FAILURE,
	   "-vvvv -F FoRmAt: It says it's unknown");
	tc(chp{execname, "-vvvv", "-F", "", "lpos", "54,7", "12,22",
	       "0.23", NULL},
	   "44.531328,12.145870\n",
	   NULL,
	   EXIT_SUCCESS,
	   "-F with an empty argument");
}

/*
 * test_cmd_bench() - Tests the `bench` command. Returns nothing.
 */

static void test_cmd_bench(char *execname)
{
	diag("Test bench command");
	sc(chp{ execname, "bench", "0", NULL },
	   " haversine\n",
	   "\nLooping haversine() for ",
	   EXIT_SUCCESS,
	   "bench 0");
	sc(chp{ execname, "bench", "0", "0", NULL },
	   "",
	   ": Too many arguments\n",
	   EXIT_FAILURE,
	   "bench has 1 extra argument");
	sc(chp{ execname, "--format", "sql", "bench", "0", NULL },
	   "INSERT INTO bench VALUES ",
	   "Looping haversine() for ",
	   EXIT_SUCCESS,
	   "--format sql bench");
}

/*
 * test_cmd_bpos() - Tests the `bpos` command. Returns nothing.
 */

static void test_cmd_bpos(char *execname)
{
	diag("Test bpos command");
	tc(chp{ execname, "bpos", "45,0", "45", "1000", NULL },
	   "45.006359,0.008994\n",
	   "",
	   EXIT_SUCCESS,
	   "bpos 45,0 45 1000");
	tc(chp{ execname, "--km", "bpos", "45,0", "45", "1", NULL },
	   "45.006359,0.008994\n",
	   "",
	   EXIT_SUCCESS,
	   "--km bpos 45,0 45 1");
	tc(chp{ execname, "bpos", "0,0", "90.0000001", "1", NULL },
	   "0.000000,0.000009\n",
	   "",
	   EXIT_SUCCESS,
	   "bpos: No negative zero in lat");
	tc(chp{ execname, "bpos", "0,0", "359.9999999", "1", NULL },
	   "0.000009,0.000000\n",
	   "",
	   EXIT_SUCCESS,
	   "bpos: No negative zero in lon");
	tc(chp{ execname, "--km", "bpos", "-34,179", "2", "19716", NULL },
	   "36.688059,-1.117018\n",
	   "",
	   EXIT_SUCCESS,
	   "bpos: Longitude is normalized");
	tc(chp{ execname, "--km", "bpos", "90,0", "180", "20000", NULL },
	   "-89.864321,0.000000\n",
	   "",
	   EXIT_SUCCESS,
	   "bpos: North Pole, point is moved 1 cm");
	tc(chp{ execname, "bpos", "90,97.97", "180", "1234567.89", NULL },
	   "78.897264,97.970000\n",
	   "",
	   EXIT_SUCCESS,
	   "bpos: North Pole, longitude is kept");
	tc(chp{ execname, "--km", "bpos", "-90,0", "359", "7000", NULL },
	   "-27.047487,-1.000000\n",
	   "",
	   EXIT_SUCCESS,
	   "bpos: South Pole, bearing 359");
	sc(chp{ execname, "bpos", "1,2", "r", "1000", NULL },
	   "",
	   ": Invalid number specified: Invalid argument\n",
	   EXIT_FAILURE,
	   "bpos: bearing is not a number");
	sc(chp{ execname, "bpos", "1,2w", "3", "4", NULL },
	   "",
	   ": Invalid number specified: Invalid argument\n",
	   EXIT_FAILURE,
	   "bpos: lon has trailing letter");
	sc(chp{ execname, "bpos", "90.0000000001,2", "3", "4", NULL },
	   "",
	   ": Value out of range\n",
	   EXIT_FAILURE,
	   "bpos: lat is out of range");
	sc(chp{ execname, "bpos", "1,2", "3", "4", "5", NULL },
	   "",
	   ": Too many arguments\n",
	   EXIT_FAILURE,
	   "bpos: 1 extra argument");
	tc(chp{ execname, "-F", "default", "bpos", "40.80542,-73.96546",
	        "188.7", "4817.84", NULL },
	   "40.762590,-73.974113\n",
	   "",
	   EXIT_SUCCESS,
	   "-F default bpos");
	tc(chp{ execname, "--format", "gpx", "bpos", "40.80542,-73.96546",
	        "188.7", "4817.84", NULL },
	         GPX_HEADER
	         "  <wpt lat=\"40.762590\" lon=\"-73.974113\">\n"
	         "    <name>bpos</name>\n"
	         "    <cmt>bpos 40.80542,-73.96546 188.7 4817.84</cmt>\n"
	         "  </wpt>\n"
	         "</gpx>\n",
	   "",
	   EXIT_SUCCESS,
	   "--format gpx bpos");
	sc(chp{ execname, "-K", "bpos", "1,2", "3", "4", NULL },
	   "",
	   ": -K/--karney is not supported by the bpos command\n",
	   EXIT_FAILURE,
	   "-K bpos");

	diag("--format sql bpos");
	tc(chp{ execname, "-F", "sql", "bpos", "0,0", "90", "1000", NULL },
	   "BEGIN;\n"
	   "CREATE TABLE IF NOT EXISTS bpos (lat1 REAL, lon1 REAL, lat2 REAL, lon2 REAL, bear REAL, dist REAL);\n"
	   "INSERT INTO bpos VALUES (0.000000, 0.000000, 0.000000, 0.008993, 90.000000, 1000.000000);\n"
	   "COMMIT;\n",
	   "",
	   EXIT_SUCCESS,
	   "-F sql bpos 0,0 90 1000");
	tc(chp{ execname, "-F", "sql", "--km", "bpos", "0,0", "90", "1",
	        NULL },
	   "BEGIN;\n"
	   "CREATE TABLE IF NOT EXISTS bpos (lat1 REAL, lon1 REAL, lat2 REAL, lon2 REAL, bear REAL, dist REAL);\n"
	   "INSERT INTO bpos VALUES (0.000000, 0.000000, 0.000000, 0.008993, 90.000000, 1000.000000);\n"
	   "COMMIT;\n",
	   "",
	   EXIT_SUCCESS,
	   "-F sql --km bpos 0,0 90 1");
	tc(chp{ execname, "-F", "sql", "--km", "bpos",
	        "76.2379187,-134.9876543", "43.99999", "15000", NULL },
	   "BEGIN;\n"
	   "CREATE TABLE IF NOT EXISTS bpos (lat1 REAL, lon1 REAL, lat2 REAL, lon2 REAL, bear REAL, dist REAL);\n"
	   "INSERT INTO bpos VALUES (76.237919, -134.987654, -34.358442, 8.423430, 43.999990, 15000000.000000);\n"
	   "COMMIT;\n",
	   "",
	   EXIT_SUCCESS,
	   "-F sql --km bpos: dist = 15000 km");
}

/*
 * test_cmd_course() - Tests the `course` command. Returns nothing.
 */

static void test_cmd_course(char *execname)
{
	char *exp_stdout;

	diag("Test course command");
	tc(chp{ execname, "course", "45,0", "45,180", "1", NULL },
	   "45.000000,0.000000\n"
	   "90.000000,0.000000\n"
	   "45.000000,180.000000\n",
	   "",
	   EXIT_SUCCESS,
	   "course: Across the North Pole");
	sc(chp{ execname, "course", "0,0", "0,180", "7", NULL },
	   "",
	   ": Antipodal points, answer is undefined\n",
	   EXIT_FAILURE,
	   "course 0,0 0,180 7");
	tc(chp{ execname, "course", "90,0", "0,0", "3", NULL },
	   "90.000000,0.000000\n"
	   "67.500000,0.000000\n"
	   "45.000000,0.000000\n"
	   "22.500000,0.000000\n"
	   "0.000000,0.000000\n",
	   "",
	   EXIT_SUCCESS,
	   "course 90,0 0,0 3");
	exp_stdout = "60.392990,5.324150\n"
	             "66.169926,16.700678\n"
	             "70.664233,33.818071\n"
	             "72.834329,57.579125\n"
	             "71.826607,82.903321\n"
	             "68.075305,102.664288\n"
	             "62.689678,115.951619\n"
	             "56.449510,124.884500\n"
	             "49.752575,131.215240\n"
	             "42.795549,135.979229\n"
	             "35.681389,139.766944\n";
	tc(chp{ execname, "course", "60.39299,5.32415",
	        "35.681389,139.766944", "9", NULL },
	   exp_stdout,
	   "",
	   EXIT_SUCCESS,
	   "course: From Bergen to Tokyo");
	tc(chp{ execname, "--km", "course", "60.39299,5.32415",
	        "35.681389,139.766944", "9", NULL },
	   exp_stdout,
	   "",
	   EXIT_SUCCESS,
	   "--km course: From Bergen to Tokyo");
	sc(chp{ execname, "course", "1,2", "3,4", NULL },
	   "",
	   ": Missing arguments\n",
	   EXIT_FAILURE,
	   "course: Missing 1 argument");
	sc(chp{ execname, "course", "1,2", "3,4", "5", "6", NULL },
	   "",
	   ": Too many arguments\n",
	   EXIT_FAILURE,
	   "course: 1 argument too much");
	sc(chp{ execname, "course", "90.00001,0", "12,34", "1", NULL },
	   "",
	   ": Value out of range\n",
	   EXIT_FAILURE,
	   "course: lat1 is outside range");
	sc(chp{ execname, "course", "17,0", "12,34", "-1", NULL },
	   "",
	   ": Value out of range\n",
	   EXIT_FAILURE,
	   "course: numpoints is -1");
	sc(chp{ execname, "course", "17,6", "12,34", "-0.5", NULL },
	   "",
	   ": Value out of range\n",
	   EXIT_FAILURE,
	   "course: numpoints is -0.5");
	tc(chp{ execname, "course", "22,33", "44,55", "0", NULL },
	   "22.000000,33.000000\n"
	   "44.000000,55.000000\n",
	   "",
	   EXIT_SUCCESS,
	   "course: numpoints is 0");
	sc(chp{ execname, "course", "17,6%", "12,34", "-1", NULL },
	   "",
	   ": Invalid number specified: Invalid argument\n",
	   EXIT_FAILURE,
	   "course: lon1 is invalid number");
	tc(chp{execname, "-F", "default", "course",
	       "52.3731,4.891", "35.681,139.767", "5", NULL},
	   "52.373100,4.891000\n"
	   "62.685860,22.579780\n"
	   "68.869393,53.549146\n"
	   "67.245712,91.173953\n"
	   "59.021383,116.487394\n"
	   "47.913547,130.771879\n"
	   "35.681000,139.767000\n",
	   "",
	   EXIT_SUCCESS,
	   "-F default course, Amsterdam to Tokyo");
	tc(chp{execname, "-F", "gpx", "course",
	       "52.3731,4.891", "35.681,139.767", "5", NULL},
	   GPX_HEADER
	   "  <rte>\n"
	   "    <rtept lat=\"52.373100\" lon=\"4.891000\">\n"
	   "    </rtept>\n"
	   "    <rtept lat=\"62.685860\" lon=\"22.579780\">\n"
	   "    </rtept>\n"
	   "    <rtept lat=\"68.869393\" lon=\"53.549146\">\n"
	   "    </rtept>\n"
	   "    <rtept lat=\"67.245712\" lon=\"91.173953\">\n"
	   "    </rtept>\n"
	   "    <rtept lat=\"59.021383\" lon=\"116.487394\">\n"
	   "    </rtept>\n"
	   "    <rtept lat=\"47.913547\" lon=\"130.771879\">\n"
	   "    </rtept>\n"
	   "    <rtept lat=\"35.681000\" lon=\"139.767000\">\n"
	   "    </rtept>\n"
	   "  </rte>\n"
	   "</gpx>\n",
	   "",
	   EXIT_SUCCESS,
	   "-F gpx course, Amsterdam to Tokyo");
	sc(chp{ execname, "--karney", "course", "1,2", "3,4", "5", NULL },
	   "",
	   ": -K/--karney is not supported by the course command\n",
	   EXIT_FAILURE,
	   "--karney course");

	diag("--format sql course");
	tc(chp{ execname, "-F", "sql", "course", "-45,-123", "45,-123", "5",
	        NULL },
	   "BEGIN;\n"
	   "CREATE TABLE IF NOT EXISTS course (num INTEGER, lat REAL, lon REAL, dist REAL, frac REAL, bear REAL);\n"
	   "INSERT INTO course VALUES (0, -45.000000, -123.000000, 0.000000, 0.000000, 0.000000);\n"
	   "INSERT INTO course VALUES (1, -30.000000, -123.000000, 1667923.899668, 0.166667, 0.000000);\n"
	   "INSERT INTO course VALUES (2, -15.000000, -123.000000, 3335847.799337, 0.333333, 0.000000);\n"
	   "INSERT INTO course VALUES (3, 0.000000, -123.000000, 5003771.699005, 0.500000, 0.000000);\n"
	   "INSERT INTO course VALUES (4, 15.000000, -123.000000, 6671695.598674, 0.666667, 0.000000);\n"
	   "INSERT INTO course VALUES (5, 30.000000, -123.000000, 8339619.498342, 0.833333, 0.000000);\n"
	   "INSERT INTO course VALUES (6, 45.000000, -123.000000, 10007543.398010, 1.000000, NULL);\n"
	   "COMMIT;\n",
	   "",
	   EXIT_SUCCESS,
	   "-F sql course -45,-123 45,-123 5");
	tc(chp{ execname, "-F", "sql", "course", "60,5", "-35,135", "5",
	        NULL },
	   "BEGIN;\n"
	   "CREATE TABLE IF NOT EXISTS course (num INTEGER, lat REAL, lon REAL, dist REAL, frac REAL, bear REAL);\n"
	   "INSERT INTO course VALUES (0, 60.000000, 5.000000, 0.000000, 0.000000, 74.908926);\n"
	   "INSERT INTO course VALUES (1, 57.898298, 50.808536, 2584622.088993, 0.166667, 114.711921);\n"
	   "INSERT INTO course VALUES (2, 43.683265, 80.527411, 5169244.094446, 0.333333, 138.121194);\n"
	   "INSERT INTO course VALUES (3, 24.968228, 97.421916, 7753866.163712, 0.500000, 147.823758);\n"
	   "INSERT INTO course VALUES (4, 4.878069, 109.598447, 10338488.288345, 0.666667, 151.019525);\n"
	   "INSERT INTO course VALUES (5, -15.417397, 121.038906, 12923110.365395, 0.833333, 149.948567);\n"
	   "INSERT INTO course VALUES (6, -35.000000, 135.000000, 15507732.399720, 1.000000, NULL);\n"
	   "COMMIT;\n",
	   "",
	   EXIT_SUCCESS,
	   "-F sql course 60,5 -35,135 5");
}

/*
 * test_cmd_lpos() - Tests the `lpos` command. Returns nothing.
 */

static void test_cmd_lpos(char *execname)
{
	diag("Test lpos command");
	tc(chp{ execname, "lpos", "45,0", "45,180", "0.5", NULL },
	   "90.000000,0.000000\n",
	   "",
	   EXIT_SUCCESS,
	   "lpos: At the North Pole");
	tc(chp{ execname, "--km", "lpos", "45,0", "45,180", "0.5", NULL },
	   "90.000000,0.000000\n",
	   "",
	   EXIT_SUCCESS,
	   "--km lpos: At the North Pole");
	tc(chp{ execname, "--format", "gpx", "lpos", "45,0", "45,180",
	        "0.5", NULL },
	             GPX_HEADER
	             "  <wpt lat=\"90.000000\" lon=\"0.000000\">\n"
	             "    <name>lpos</name>\n"
	             "    <cmt>lpos 45,0 45,180 0.5</cmt>\n"
	             "  </wpt>\n"
	             "</gpx>\n",
	   "",
	   EXIT_SUCCESS,
	   "--format gpx lpos: At the North Pole");
	tc(chp{ execname, "lpos", "0,0", "-0.0000001,0", "1", NULL },
	   "0.000000,0.000000\n",
	   "",
	   EXIT_SUCCESS,
	   "lpos: No negative zero in lat");
	tc(chp{ execname, "lpos", "0,0", "0,-0.0000001", "1", NULL },
	   "0.000000,0.000000\n",
	   "",
	   EXIT_SUCCESS,
	   "lpos: No negative zero in lon");
	tc(chp{ execname, "--km", "lpos", "-24.598059,178.290755",
	        "-12.500692,163.651473", "9.8", NULL },
	   "24.508273,-1.833156\n",
	   "",
	   EXIT_SUCCESS,
	   "lpos: Longitude is normalized");
	tc(chp{ execname, "lpos", "90,0", "0,0", "0.5", NULL },
	   "45.000000,0.000000\n",
	   "",
	   EXIT_SUCCESS,
	   "lpos: Point is moved 1 cm");
	sc(chp{ execname, "lpos", "-90,0", "90,0", "3", NULL },
	   "",
	   ": Antipodal points, answer is undefined\n",
	   EXIT_FAILURE,
	   "lpos: South Pole to the North Pole");
	sc(chp{ execname, "lpos", "1,2", "3,4", NULL },
	   "",
	   ": Missing arguments\n",
	   EXIT_FAILURE,
	   "lpos: Missing 1 argument");
	sc(chp{ execname, "lpos", "1,2", "3,4", "5", "6", NULL },
	   "",
	   ": Too many arguments\n",
	   EXIT_FAILURE,
	   "lpos: 1 argument too much");
	sc(chp{ execname, "lpos", "-90.00001,0", "12,34", "1", NULL },
	   "",
	   ": Value out of range\n",
	   EXIT_FAILURE,
	   "lpos: lat1 is outside range");
	sc(chp{ execname, "lpos", "17,6%", "12,34", "-1", NULL },
	   "",
	   ": Invalid number specified: Invalid argument\n",
	   EXIT_FAILURE,
	   "lpos: lon1 is invalid number");
	tc(chp{ execname, "lpos", "1,2", "3,4", "0", NULL },
	   "1.000000,2.000000\n",
	   "",
	   EXIT_SUCCESS,
	   "lpos: fracdist is 0");
	tc(chp{ execname, "lpos", "11.231,-34.55", "29.97777,47.311001",
	        "1", NULL },
	   "29.977770,47.311001\n",
	   "",
	   EXIT_SUCCESS,
	   "lpos: fracdist is 1");
	tc(chp{ execname, "--km", "lpos", "11.231,-34.55",
	        "29.97777,47.311001", "1", NULL },
	   "29.977770,47.311001\n",
	   "",
	   EXIT_SUCCESS,
	   "--km lpos: fracdist is 1");
	sc(chp{ execname, "lpos", "1,2", "3,4", "INF", NULL },
	   "",
	   ": Invalid number specified: Numerical result out of range\n",
	   EXIT_FAILURE,
	   "lpos: fracdist is INF");
	sc(chp{ execname, "lpos", "0,0", "0,180", "0.5", NULL },
	   "",
	   ": Antipodal points, answer is undefined\n",
	   EXIT_FAILURE,
	   "lpos: Antipodal positions, 0,0 and 0,180");
	sc(chp{ execname, "lpos", "90,0", "-90,0", "0.5", NULL },
	   "",
	   ": Antipodal points, answer is undefined\n",
	   EXIT_FAILURE,
	   "lpos: Antipodal positions, 90,0 and -90,0");
	sc(chp{ execname, "--karney", "lpos", "1,2", "3,4", "0.2", NULL },
	   "",
	   ": -K/--karney is not supported by the lpos command\n",
	   EXIT_FAILURE,
	   "--karney lpos");
	tc(chp{ execname, "-F", "sql", "lpos", "1,2", "87.188,-130.77", "0.2",
	        NULL },
	   "BEGIN;\n"
	   "CREATE TABLE IF NOT EXISTS lpos (lat1 REAL, lon1 REAL, lat2 REAL, lon2 REAL, frac REAL, dlat REAL, dlon REAL, dist REAL, bear REAL);\n"
	   "INSERT INTO lpos VALUES (1.000000, 2.000000, 87.188000, -130.770000, 0.200000, 19.169669, 1.318240, 14327441.236686, 64.166424);\n"
	   "COMMIT;\n",
	   "",
	   EXIT_SUCCESS,
	   "-F sql lpos");
}

/*
 * test_multiple() - Tests the `bear` or `dist` command. Returns nothing.
 */

static void test_multiple(char *execname, char *cmd)
{
	assert(cmd);
	if (!cmd) {
		ok(1, "%s(): cmd is NULL", __func__); /* gncov */
		return; /* gncov */
	}

	diag("Test %s command", !strcmp(cmd, "bear") ? "bear" : "dist");
	sc(chp{ execname, "-vv", cmd, NULL },
	   "",
	   ": Missing arguments\n",
	   EXIT_FAILURE,
	   "%s with no arguments", cmd);
	sc(chp{ execname, cmd, "1,2", "3", NULL },
	   "",
	   ": Invalid number specified\n",
	   EXIT_FAILURE,
	   "%s: Argument 2 is not a coordinate", cmd);
	tc(chp{ execname, cmd, "1,2", "3,4", NULL },
	   !strcmp(cmd, "bear") ? "44.951998\n" : "314402.951024\n",
	   "",
	   EXIT_SUCCESS,
	   "%s 1,2 3,4", cmd);
	if (!strcmp(cmd, "bear")) {
		sc(chp{ execname, "bear", "12,34", "-12,-146", NULL },
		   "",
		   ": Antipodal points, answer is undefined\n",
		   EXIT_FAILURE,
		   "bear 12,34 -12,-146 - antipodal points");
	} else {
		tc(chp{ execname, "dist", "12,34", "-12,-146", NULL },
		   "20015086.796021\n",
		   "",
		   EXIT_SUCCESS,
		   "dist 12,34 -12,-146 - antipodal points");
	}
	sc(chp{ execname, cmd, "1,2", "3,4", "5", NULL },
	   "",
	   ": Too many arguments\n",
	   EXIT_FAILURE,
	   "%s with 1 argument too much", cmd);
	sc(chp{ execname, cmd, "1,2", "3,1e+900", NULL },
	   "",
	   ": Invalid number specified: Numerical result out of range\n",
	   EXIT_FAILURE,
	   "%s with 1 number too large", cmd);
	sc(chp{ execname, cmd, "1,2", "urgh,4", NULL },
	   "",
	   ": Invalid number specified: Invalid argument\n",
	   EXIT_FAILURE,
	   "%s with 1 non-number", cmd);
	sc(chp{ execname, cmd, "1,2.9y", "3,4", NULL },
	   "",
	   ": Invalid number specified: Invalid argument\n",
	   EXIT_FAILURE,
	   "%s with non-digit after number", cmd);
	sc(chp{ execname, cmd, "1,2 g", "3,4", NULL },
	   "",
	   ": Invalid number specified: Invalid argument\n",
	   EXIT_FAILURE,
	   "%s with whitespace and non-digit after number", cmd);
	tc(chp{ execname, cmd, "10,2,", "3,4", NULL },
	   !strcmp(cmd, "bear") ? "164.027619\n" : "809080.682265\n",
	   "",
	   EXIT_SUCCESS,
	   "%s with comma after number", cmd);
	sc(chp{ execname, cmd, "1,2", "3,NAN", NULL },
	   "",
	   ": Invalid number specified:"
	   " Invalid argument\n",
	   EXIT_FAILURE,
	   "%s with NAN", cmd);
	sc(chp{ execname, cmd, "1,2", "3,INF", NULL },
	   "",
	   ": Invalid number specified: Numerical result out of range\n",
	   EXIT_FAILURE,
	   "%s with INF", cmd);
	sc(chp{ execname, cmd, "1,2", "", NULL },
	   "",
	   ": Invalid number specified\n",
	   EXIT_FAILURE,
	   "%s with empty argument", cmd);
	sc(chp{ execname, cmd, "1,180.001", "3,4", NULL },
	   "",
	   ": Value out of range\n",
	   EXIT_FAILURE,
	   "%s: lon1 out of range", cmd);
	if (!strcmp(cmd, "bear")) {
		sc(chp{ execname, "bear", "90,0", "-90,0", NULL },
		   "",
		   ": Antipodal points, answer is undefined",
		   EXIT_FAILURE,
		   "bear 90,0 -90,0");
		sc(chp{ execname, "--km", cmd, "90,0", "-90,0", NULL },
		   "",
		   ": Antipodal points, answer is undefined\n",
		   EXIT_FAILURE,
		   "--km bear 90,0 -90,0");
	} else {
		tc(chp{ execname, "dist", "90,0", "-90,0", NULL },
		   "20015086.796021\n",
		   "",
		   EXIT_SUCCESS,
		   "dist 90,0 -90,0");
		tc(chp{ execname, "--km", cmd, "90,0", "-90,0", NULL },
		   "20015.086796\n",
		   "",
		   EXIT_SUCCESS,
		   "--km dist 90,0 -90,0");
	}
	tc(chp{ execname, "-F", "default", cmd, "34,56", "-78,9", NULL },
	   !strcmp(cmd, "bear") ? "189.693136\n" : "12835310.777042\n",
	   "",
	   EXIT_SUCCESS,
	   "-F default %s", cmd);
	sc(chp{ execname, "--format", "gpx", cmd, "34,56", "-78,9", NULL },
	   "",
	   ": No way to display this info in GPX format\n",
	   EXIT_FAILURE,
	   "--format gpx %s", cmd);
	if (!strcmp(cmd, "bear"))
	{
		sc(chp{ execname, "-K", "bear", "1,2", "3,4", NULL },
		   "",
		   ": -K/--karney is not supported by the bear command\n",
		   EXIT_FAILURE,
		   "-K bear");
	}

	diag("--format sql %s", cmd);
	if (!strcmp(cmd, "bear")) {
		tc(chp{ execname, "--format", "sql", cmd, "34,56", "-78,9",
		        NULL },
		   "BEGIN;\n"
		   "CREATE TABLE IF NOT EXISTS bear (lat1 REAL, lon1 REAL, lat2 REAL, lon2 REAL, bear REAL, dist REAL);\n"
		   "INSERT INTO bear VALUES (34.000000, 56.000000, -78.000000, 9.000000, 189.693136, 12835310.777042);\n"
		   "COMMIT;\n",
		   "",
		   EXIT_SUCCESS,
		   "--format sql %s", cmd);
	} else {
		tc(chp{ execname, "--format", "sql", cmd, "34,56", "-78,9",
		        NULL },
		   "BEGIN;\n"
		   "CREATE TABLE IF NOT EXISTS dist (lat1 REAL, lon1 REAL, lat2 REAL, lon2 REAL, dist REAL, bear REAL);\n"
		   "INSERT INTO dist VALUES (34.000000000000000, 56.000000000000000, -78.000000000000000, 9.000000000000000, 12835310.77704204, 189.69313615);\n"
		   "COMMIT;\n",
		   "",
		   EXIT_SUCCESS,
		   "--format sql %s", cmd);
	}
}

/*
 * verify_coor_dist() - Verifies that the distance between `coor` and all 
 * "\n"-separated coordinates in `str` are in the range `mindist`-`maxdist` 
 * meters. Returns nothing.
 */

static void verify_coor_dist(const char *str, const char *coor,
                             const double mindist, const double maxdist)
{
	int errcount = 0;
	char *s, *p;
	double clat, clon,
	       mindist_r = round(mindist), maxdist_r = round(maxdist);
	unsigned long coorcount = 0;

	if (!str || !coor) {
		ok(1, "%s(): `str` or `coor` is NULL", /* gncov */
		      __func__);
		return; /* gncov */
	}
	if (!strchr(str, '\n')) {
		ok(1, "%s(): No '\\n' found in `str`", /* gncov */
		      __func__);
		return; /* gncov */
	}
	if (parse_coordinate(coor, &clat, &clon)) {
		failed_ok("parse_coordinate()"); /* gncov */
		diag("%s(): coor = \"%s\"", __func__, coor); /* gncov */
		return; /* gncov */
	}

	s = mystrdup(str);
	if (!s) {
		failed_ok("mystrdup()"); /* gncov */
		return; /* gncov */
	}
	p = strtok(s, "\n");
	while (p) {
		double lat, lon, dist;
		coorcount++;
		if (parse_coordinate(p, &lat, &lon)) {
			failed_ok("parse_coordinate()"); /* gncov */
			diag("%s(): p = \"%s\"", __func__, p); /* gncov */
			errcount++; /* gncov */
			goto next; /* gncov */
		}
		dist = haversine(clat, clon, lat, lon);
		if (dist == -1.0) {
			ok(1, "%s(): haversine() failed," /* gncov */
			      " values out of range", __func__);
			diag("%s(): coor = \"%s\", lat = %f," /* gncov */
			     " lon = %f", __func__, coor, lat, lon);
			errcount++; /* gncov */
			goto next; /* gncov */
		}
		dist = round(dist);
		if (dist < mindist_r || dist > maxdist_r) {
			errcount < 11 /* gncov */
			? ok(1, "Coordinate out of range (%f" /* gncov */
			        " to %f m). center: %f,%f, coor = %f,%f,"
			        " dist = %f",
			        mindist, maxdist,
			        clat, clon, lat, lon, dist)
			: 1; /* gncov */
			errcount++; /* gncov */
		}
next:
		if (errcount >= 10) {
			ok(1, "Aborting this test after 10" /* gncov */
			      " errors");
			break; /* gncov */
		}
		p = strtok(NULL, "\n");
	}
	ok(!!errcount, "randpos: All %lu coordinates were inside range of"
	               " %.0f to %.0f meters",
	               coorcount, mindist, maxdist);
	free(s);
}

/*
 * chk_coor_outp() - Verify the output from the randpos command. Returns 
 * 1 if any error, 0 if everything is ok.
 */

static int chk_coor_outp(const OutputFormat format, const char *output,
                         const unsigned int num, const char *coor,
                         const double mindist, const double maxdist)
{
	int errcount = 0;
	regex_t regex;
	char *regstr, *pattern;
	static int count = 0;

	count++;
	if (format == OF_DEFAULT && coor)
		verify_coor_dist(output, coor, mindist, maxdist);

	switch (format) {
	case OF_DEFAULT:
		regstr = "^(-?[0-9]+\\.[0-9]+"
		         ","
		         "-?[0-9]+\\.[0-9]+\n){%u}$";
		break;
	case OF_GPX:
		regstr = "^<\\?xml version=\"1\\.0\" encoding=\"UTF-8\"\\?>\n"
		         "<gpx"
		         " xmlns=\"http://www\\.topografix\\.com/GPX/1/1\""
		         " version=\"1\\.1\""
		         " creator=\"" PROJ_NAME " - " PROJ_URL "\""
		         ">\n"
		         "("
		         "  <wpt"
		         " lat=\"-?[0-9]+\\.[0-9]+\""
		         " lon=\"-?[0-9]+\\.[0-9]+\""
		         ">\n"
		         "    <name>Random [0-9]+</name>\n"
		         "  </wpt>\n"
		         "){%u}"
		         "</gpx>\n$";
		break;
	default: /* gncov */
		regstr = "NON-EXISTENT VALUE"; /* gncov */
		break; /* gncov */
	}

	pattern = allocstr(regstr, num);
	if (!pattern) {
		failed_ok("allocstr()"); /* gncov */
		return 1; /* gncov */
	}

	if (regcomp(&regex, pattern, REG_EXTENDED)) {
		failed_ok("regcomp()"); /* gncov */
		errcount++; /* gncov */
		goto cleanup; /* gncov */
	}

	ok(!!regexec(&regex, output, 0, NULL, 0),
	   "Regexp matches, count = %d", count);
	regfree(&regex);

cleanup:
	free(pattern);

	return !!errcount;
}

/*
 * te_randpos() - Execute `cmd` and verify that all coordinates are valid and 
 * inside the various ranges specified. Returns nothing.
 */

static void te_randpos(const OutputFormat format, char **cmd,
                       const unsigned int num, const char *coor,
                       const double mindist, const double maxdist,
                       const char *desc)
{
	struct streams ss;

	streams_init(&ss);
	streams_exec(&ss, cmd);
	ok(chk_coor_outp(format, ss.out.buf, num, coor, mindist, maxdist),
	   desc);
	streams_free(&ss);
}

/*
 * test_randpos_dist_max() - Test the randpos command with only the maximum 
 * distance. Returns nothing.
 */

static void test_randpos_dist_max(char *execname)
{
	char **as;

	diag("randpos with max_dist");

	as = chp{ execname, "--count", "50", "randpos", "1.234,5.6789", "100",
	          NULL };
	te_randpos(OF_DEFAULT, as, 50, "1.234,5.6789", 0.0, 100.0,
	           "randpos: 50 pos inside a radius of 100m");

	as = chp{ execname, "--count", "51", "randpos", "1.234,5.6789",
	          "100000000", NULL };
	te_randpos(OF_DEFAULT, as, 51, "1.234,5.6789", 0.0,
	           MAX_EARTH_DISTANCE,
	           "randpos: max_dist is larger than MAX_EARTH_DISTANCE");

	as = chp{ execname, "--count", "52", "randpos", "1.234,5.6789", "0",
	          "100000000", NULL };
	te_randpos(OF_DEFAULT, as, 52, "1.234,5.6789", 0.0,
	           MAX_EARTH_DISTANCE,
	           "randpos: min_dist is larger than MAX_EARTH_DISTANCE");

	as = chp{ execname, "--count", "53", "randpos", "1.234,5.67",
	          "100000000", "100000000", NULL };
	te_randpos(OF_DEFAULT, as, 53, "1.234,5.67", MAX_EARTH_DISTANCE,
	           MAX_EARTH_DISTANCE,
	           "randpos: min_dist and max_dist are larger than"
	           " MAX_EARTH_DISTANCE, stdout looks ok");

	as = chp{ execname, "-F", "gpx", "--count", "14", "randpos",
	          "19.63,-19.70", "25", NULL };
	te_randpos(OF_GPX, as, 14, NULL, 0, 0,
	           "-F gpx --count 14 randpos 19.63,-19.70 25");

	as = chp{ execname, "--km", "--count", "50", "randpos", "1.234,5.6789",
	          "100", NULL };
	te_randpos(OF_DEFAULT, as, 50,
	           "1.234,5.6789", 0.0, 100000.0,
	           "--km randpos: 50 pos inside a radius of 100km,"
	           " stdout looks ok");

	as = chp{ execname, "--km", "--count", "50", "randpos", "1.234,5.6789",
	          "100000", NULL };
	te_randpos(OF_DEFAULT, as, 50,
	           "1.234,5.6789", 0.0, MAX_EARTH_DISTANCE,
	           "--km randpos: max_dist is larger than MAX_EARTH_DISTANCE,"
	           " stdout looks ok");
}

/*
 * test_randpos_dist_minmax() - Tests the randpos command with both maximum and 
 * minimum distance. Returns nothing.
 */

static void test_randpos_dist_minmax(char *execname)
{
	char **as;

	diag("randpos with max_dist and min_dist");

	as = chp{ execname, "randpos", "12.34,56.78", "100", "200", NULL };
	te_randpos(OF_DEFAULT, as, 1, "12.34,56.78", 100.0, 200.0,
	           "randpos: min_dist is larger than max_dist");

	as = chp{ execname, "--count", "27", "randpos", "1.234,5.6789", "2000",
	          "2000", NULL };
	te_randpos(OF_DEFAULT, as, 27, "1.234,5.6789", 2000.0, 2000.0,
	           "randpos: max_dist is equal to min_dist, stdout looks ok");

	as = chp{ execname, "-F", "gpx", "--count", "21", "randpos",
	          "90,0", "7741", "7777", NULL };
	te_randpos(OF_GPX, as, 21, "90,0", 7741, 7777,
	           "randpos, North Pole, 25 pos as GPX, dists swapped");

	as = chp{ execname, "--count", "33", "randpos", "90,0", "10000",
	          NULL };
	te_randpos(OF_DEFAULT, as, 33, "90,0", 0.0, 10000.0,
	           "randpos: Exactly at the North Pole");

	as = chp{ execname, "--count", "34", "randpos", "-90,0", "10001",
	          NULL };
	te_randpos(OF_DEFAULT, as, 34, "-90,0", 0.0, 10001.0,
	           "randpos: Exactly at the South Pole");
}

/*
 * test_randpos_dist() - Tests the randpos command with maximum and minimum 
 * distance. Returns nothing.
 */

static void test_randpos_dist(char *execname)
{
	test_randpos_dist_max(execname);
	test_randpos_dist_minmax(execname);
}

/*
 * test_cmd_randpos() - Tests the randpos command. Returns nothing.
 */

static void test_cmd_randpos(char *execname)
{
	int res;
	struct streams ss;
	double lat, lon;
	char **as;

	diag("Test randpos command");

	sc(chp{ execname, "randpos", "1,2", "100", "90", "5", NULL },
	   "",
	   ": Too many arguments\n",
	   EXIT_FAILURE,
	   "randpos with 1 extra argument");

	streams_init(&ss);
	streams_exec(&ss, chp{ execname, "randpos", NULL });
	lat = lon = 0;
	res = parse_coordinate(ss.out.buf, &lat, &lon);
	ok(!!res, "randpos: Coordinate is valid");
	ok(!(fabs(lat) <= 90), "randpos: lat is in range");
	ok(!(fabs(lat) <= 180), "randpos: lon is in range");
	streams_free(&ss);

	as = chp{ execname, "--count", "5", "randpos", NULL };
	te_randpos(OF_DEFAULT, as, 5, NULL, 0, 0,
	           "--count 5 randpos, stdout is ok");

	as = chp{ execname, "-F", "gpx", "--count", "9", "randpos", NULL };
	te_randpos(OF_GPX, as, 9, NULL, 0, 0,
	           "-F gpx --count 9 randpos, stdout is ok");

	diag("--count with invalid argument");
	sc(chp{ execname, "--count", "", "randpos", NULL },
	   "",
	   ": : Invalid --count argument\n",
	   EXIT_FAILURE,
	   "Empty argument to --count");
	sc(chp{ execname, "--count", "g", "randpos", NULL },
	   "",
	   ": g: Invalid --count argument\n",
	   EXIT_FAILURE,
	   "--count receives non-number");
	sc(chp{ execname, "--count", "11y", "randpos", NULL },
	   "",
	   ": 11y: Invalid --count argument\n",
	   EXIT_FAILURE,
	   "--count 11y");
	sc(chp{ execname, "--count", "11.3", "randpos", NULL },
	   "",
	   ": 11.3: Invalid --count argument\n",
	   EXIT_FAILURE,
	   "--count 11.3");
	sc(chp{ execname, "--count", "-2", "randpos", NULL },
	   "",
	   ": -2: Invalid --count argument\n",
	   EXIT_FAILURE,
	   "--count -2");

	diag("--count 0");
	tc(chp{ execname, "--count", "0", "randpos", NULL },
	   "",
	   "",
	   EXIT_SUCCESS,
	   "--count 0");
	tc(chp{ execname, "-F", "gpx", "--count", "0", "randpos", NULL },
	   GPX_HEADER "</gpx>\n",
	   "",
	   EXIT_SUCCESS,
	   "-F gpx --count 0");

	test_randpos_dist(execname);

	diag("randpos with max_dist, invalid arguments");

	sc(chp{ execname, "randpos", "12.34,56.34y", "10", NULL },
	   "",
	   ": Error in center coordinate: Invalid argument\n",
	   EXIT_FAILURE,
	   "randpos with error in coordinate");

	sc(chp{ execname, "randpos", "12.34,56.34", "10y", NULL },
	   "",
	   ": Error in max_dist argument: Invalid argument\n",
	   EXIT_FAILURE,
	   "randpos with error in max_dist");

	sc(chp{ execname, "randpos", "12.34,56.34", "-17.9", NULL },
	   "",
	   ": Distance can't be negative\n",
	   EXIT_FAILURE,
	   "randpos with negative max_dist");

	diag("randpos with max_dist and min_dist, invalid arguments");

	sc(chp{ execname, "randpos", "12.34,56.34", "10", "3y", NULL },
	   "",
	   ": Error in min_dist argument: Invalid argument\n",
	   EXIT_FAILURE,
	   "randpos with error in min_dist");

	sc(chp{ execname, "randpos", "12.34,56.34", "9", "-2", NULL },
	   "",
	   ": Distance can't be negative\n",
	   EXIT_FAILURE,
	   "randpos with negative min_dist");

	sc(chp{ execname, "--karney", "randpos", "1,2", "200", "100", NULL },
	   "",
	   ": -K/--karney is not supported by the randpos command\n",
	   EXIT_FAILURE,
	   "--karney randpos");

	diag("--format sql randpos");
	tc(chp{ execname, "--format", "sql", "--seed", "19", "--count", "20",
	        "randpos", NULL },
	   "BEGIN;\n"
	   "CREATE TABLE IF NOT EXISTS randpos (seed INTEGER, num INTEGER, lat REAL, lon REAL, dist REAL, bear REAL);\n"
	   "INSERT INTO randpos VALUES (19, 1, 25.603688, -130.636512, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 2, -48.273060, 77.529775, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 3, -17.117300, 140.106483, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 4, -52.240484, -115.036322, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 5, 3.344781, 17.095447, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 6, 16.755787, 9.521758, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 7, -19.490223, 12.660125, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 8, 69.696210, 156.752235, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 9, 52.694091, 90.355201, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 10, -6.449310, -117.032350, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 11, 13.210432, -169.948761, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 12, -22.225278, 129.713413, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 13, -1.595068, -106.917102, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 14, -22.137051, -55.379762, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 15, 0.672918, -35.768761, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 16, 8.184838, -1.136274, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 17, -57.901507, 155.489707, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 18, 14.890397, 131.859757, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 19, 31.066093, -2.976285, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 20, 31.662070, 0.677547, NULL, NULL);\n"
	   "COMMIT;\n",
	   "",
	   EXIT_SUCCESS,
	   "--format sql randpos without coordinate or dists");
	tc(chp{ execname, "-F", "sql", "--seed", "19", "--count", "20",
	        "randpos", "1,2", "200", "100", NULL },
	   "BEGIN;\n"
	   "CREATE TABLE IF NOT EXISTS randpos (seed INTEGER, num INTEGER, lat REAL, lon REAL, dist REAL, bear REAL);\n"
	   "INSERT INTO randpos VALUES (19, 1, 0.999739, 1.998795, 137.029826, 257.785884);\n"
	   "INSERT INTO randpos VALUES (19, 2, 1.001160, 2.001187, 184.578986, 45.661444);\n"
	   "INSERT INTO randpos VALUES (19, 3, 0.998948, 2.001395, 194.296589, 127.020796);\n"
	   "INSERT INTO randpos VALUES (19, 4, 1.001014, 2.000784, 142.479955, 37.694182);\n"
	   "INSERT INTO randpos VALUES (19, 5, 0.998461, 1.999715, 173.992388, 190.501973);\n"
	   "INSERT INTO randpos VALUES (19, 6, 0.999042, 1.998779, 172.556828, 231.892738);\n"
	   "INSERT INTO randpos VALUES (19, 7, 0.999223, 2.001350, 173.155110, 119.943720);\n"
	   "INSERT INTO randpos VALUES (19, 8, 1.001736, 1.999657, 196.717262, 348.815877);\n"
	   "INSERT INTO randpos VALUES (19, 9, 1.001344, 1.998994, 186.659487, 323.173976);\n"
	   "INSERT INTO randpos VALUES (19, 10, 0.998803, 2.000441, 141.822259, 159.781653);\n"
	   "INSERT INTO randpos VALUES (19, 11, 0.999209, 1.999309, 116.709311, 221.135064);\n"
	   "INSERT INTO randpos VALUES (19, 12, 0.999353, 2.001608, 192.753169, 111.915138);\n"
	   "INSERT INTO randpos VALUES (19, 13, 0.998700, 2.000114, 145.056415, 174.989594);\n"
	   "INSERT INTO randpos VALUES (19, 14, 0.999461, 2.001323, 158.835986, 112.171800);\n"
	   "INSERT INTO randpos VALUES (19, 15, 0.998532, 1.999946, 163.296314, 182.113987);\n"
	   "INSERT INTO randpos VALUES (19, 16, 0.998618, 1.999337, 170.487140, 205.626063);\n"
	   "INSERT INTO randpos VALUES (19, 17, 1.001568, 2.000817, 196.535789, 27.515539);\n"
	   "INSERT INTO randpos VALUES (19, 18, 0.998799, 1.998745, 193.074008, 226.254748);\n"
	   "INSERT INTO randpos VALUES (19, 19, 1.000077, 1.998472, 170.123644, 272.884771);\n"
	   "INSERT INTO randpos VALUES (19, 20, 1.000120, 1.998468, 170.843636, 274.483494);\n"
	   "COMMIT;\n",
	   "",
	   EXIT_SUCCESS,
	   "-F sql randpos with maxdist and mindist");
}

/*
 * test_seed_option() - Tests the --seed option. Returns nothing.
 */

static void test_seed_option(char *execname)
{
	struct binbuf bb1, bb2, bb3;

	diag("Test --seed");

	binbuf_init(&bb1);
	binbuf_init(&bb2);
	binbuf_init(&bb3);
	exec_output(&bb1, chp{ execname, "--seed", "64738",
	                       "--count", "20", "randpos", NULL });
	exec_output(&bb2, chp{ execname, "--seed", "64739",
	                       "--count", "20", "randpos", NULL });
	exec_output(&bb3, chp{ execname, "--seed", "64738",
	                       "--count", "20", "randpos", NULL });
	ok(!strcmp(bb1.buf, bb2.buf),
	   "randpos with seed 64738 and 64739 are different");
	ok(!!strcmp(bb3.buf, bb1.buf),
	   "randpos with seed 64738 is identical to the first");
	binbuf_free(&bb3);
	binbuf_free(&bb2);
	binbuf_free(&bb1);

	tc(chp{ execname, "--seed", "-29271", "--count", "10", "randpos",
	        NULL },
	   "56.398026,65.672317\n"
	   "-62.545731,39.973376\n"
	   "-12.953591,-109.219631\n"
	   "71.625404,10.907732\n"
	   "-49.720325,-97.898594\n"
	   "-9.863466,161.067034\n"
	   "-21.125676,-179.454884\n"
	   "-20.152272,-58.458280\n"
	   "11.179247,79.685331\n"
	   "-47.058531,149.678499\n",
	   "",
	   EXIT_SUCCESS,
	   "--seed -29271 --count 10 randpos");

	tc(chp{ execname, "--seed", "29271", "--count", "10", "randpos",
	        NULL },
	   "-8.603169,114.257108\n"
	   "-46.646685,-133.238413\n"
	   "32.233828,-45.004903\n"
	   "-10.383696,-105.396018\n"
	   "14.981908,-85.632935\n"
	   "-2.551390,93.617273\n"
	   "-45.194786,6.814725\n"
	   "-4.491350,-102.252955\n"
	   "-54.462817,-25.117826\n"
	   "-23.940993,-89.915442\n",
	   "",
	   EXIT_SUCCESS,
	   "--seed 29271 --count 10 randpos");

	tc(chp{ execname, "--format", "gpx", "--seed", "19999",
	        "--count", "4", "randpos", NULL },
	   "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	   "<gpx xmlns=\"http://www.topografix.com/GPX/1/1\""
	   " version=\"1.1\" creator=\"Geocalc -"
	   " https://gitlab.com/oyvholm/geocalc\">\n"
	   "  <wpt lat=\"-17.914770\" lon=\"127.654700\">\n"
	   "    <name>Random 1, seed 19999</name>\n"
	   "  </wpt>\n"
	   "  <wpt lat=\"-27.493377\" lon=\"115.562443\">\n"
	   "    <name>Random 2, seed 19999</name>\n"
	   "  </wpt>\n"
	   "  <wpt lat=\"-22.699248\" lon=\"152.244953\">\n"
	   "    <name>Random 3, seed 19999</name>\n"
	   "  </wpt>\n"
	   "  <wpt lat=\"-52.359745\" lon=\"-142.812430\">\n"
	   "    <name>Random 4, seed 19999</name>\n"
	   "  </wpt>\n"
	   "</gpx>\n",
	   "",
	   EXIT_SUCCESS,
	   "--format gpx --seed 19999 --count 4 randpos");

	sc(chp{ execname, "--seed", "", "randpos", NULL },
	   "",
	   ": : Invalid --seed argument\n",
	   EXIT_FAILURE,
	   "Empty argument to --seed");

	sc(chp{ execname, "--seed", "9.14", "randpos", NULL },
	   "",
	   ": 9.14: Invalid --seed argument\n",
	   EXIT_FAILURE,
	   "--seed 9.14 randpos");
}

/*
 * test_haversine_option() - Tests the -H/--haversine option. Returns nothing.
 */

static void test_haversine_option(char *execname)
{
	diag("Test -H/--haversine");

	tc(chp{ execname, "-H", "dist", "13.389820,-71.453489",
	        "-24.171099,-162.897613", NULL },
	   "10755873.395009\n",
	   "",
	   EXIT_SUCCESS,
	   "-H dist 13.389820,-71.453489 -24.171099,-162.897613");

	tc(chp{ execname, "--haversine", "dist", "-51.548124,19.706076",
	        "-35.721304,13.064358", NULL },
	   "1837351.151434\n",
	   "",
	   EXIT_SUCCESS,
	   "--haversine dist -51.548124,19.706076 -35.721304,13.064358");
}

/*
 * test_karney_option() - Tests the -K/--karney option. Returns nothing.
 */

static void test_karney_option(char *execname)
{
	diag("Test -K/--karney");

	tc(chp{ execname, "-K", "dist", "13.389820,-71.453489",
	        "-24.171099,-162.897613", NULL },
	   "10759030.94409290\n",
	   "",
	   EXIT_SUCCESS,
	   "-K dist 13.389820,-71.453489 -24.171099,-162.897613");

	tc(chp{ execname, "--karney", "dist", "-51.548124,19.706076",
	        "-35.721304,13.064358", NULL },
	   "1836406.16934653\n",
	   "",
	   EXIT_SUCCESS,
	   "--karney dist -51.548124,19.706076 -35.721304,13.064358");

	tc(chp{ execname, "-K", "dist", "12.34,56.789", "12.34,56.789", NULL },
	   "0.00000000\n",
	   "",
	   EXIT_SUCCESS,
	   "-K dist: Coincident points");

	sc(chp{ execname, "-K", "dist", "37,7", "-37,-173", NULL },
	   "",
	   ": Formula did not converge, antipodal points\n",
	   EXIT_FAILURE,
	   "--karney dist: Antipodal points");
}

/*
 * test_functions() - Tests various functions directly. Returns nothing.
 */

static void test_functions(void)
{
	if (!opt.testfunc)
		return; /* gncov */

	diag("Test selftest routines");
	ok(!ok(0, NULL), "ok(0, NULL)");
	verify_constants();
	test_diag();
	test_gotexp_output();
	test_valgrind_lines();

	diag("Test various routines");
	diag("Test myerror()");
	errno = EACCES;
	ok(!(myerror("errno is EACCES") > 37), "myerror(): errno is EACCES");
	ok(!!errno, "errno is set to 0 by myerror()");
	diag("Test std_strerror()");
	ok(!(std_strerror(0) != NULL), "std_strerror(0)");
	ok(!!strcmp(std_strerror(EACCES), "Permission denied"),
	   "std_strerror(EACCES) is as expected");
	diag("Test mystrdup()");
	ok(!(mystrdup(NULL) == NULL), "mystrdup(NULL) == NULL");
	test_allocstr();
	test_round_number();
	test_rand_pos();
	test_parse_coordinate();
	test_are_antipodal();
	test_xml_escape_string();
	test_gpx_wpt();
	test_karney_distance();
}

/*
 * print_version_info() - Display output from the --version command. Returns 0 
 * if ok, or 1 if streams_exec() failed.
 */

static int print_version_info(char *execname)
{
	struct streams ss;
	int res;

	streams_init(&ss);
	res = streams_exec(&ss, chp{ execname, "--version", NULL });
	if (res) {
		failed_ok("streams_exec()"); /* gncov */
		if (ss.err.buf) /* gncov */
			diag(ss.err.buf); /* gncov */
		return 1; /* gncov */
	}
	diag("========== BEGIN version info ==========\n"
	     "%s"
	     "=========== END version info ===========",
	     ss.out.buf ? ss.out.buf : "(null)");
	streams_free(&ss);

	return 0;
}

/*
 * test_executable() - Run various tests with the executable and verify that 
 * stdout, stderr and the return value are as expected. Returns nothing.
 */

static void test_executable(char *execname)
{
	if (!opt.testexec)
		return; /* gncov */

	diag("Test the executable");
	test_valgrind_option(execname);
	print_version_info(execname);
	test_streams_exec(execname);
	sc(chp{ execname, "abc", NULL },
	   "",
	   ": Unknown command: abc\n",
	   EXIT_FAILURE,
	   "Unknown command");
	test_standard_options(execname);
	test_format_option(execname);
	test_cmd_bench(execname);
	test_cmd_bpos(execname);
	test_cmd_course(execname);
	test_cmd_lpos(execname);
	test_multiple(execname, "bear");
	test_multiple(execname, "dist");
	test_cmd_randpos(execname);
	test_seed_option(execname);
	test_haversine_option(execname);
	test_karney_option(execname);
	print_version_info(execname);
}

/*
 * opt_selftest() - Run internal testing to check that it works on the current 
 * system. Executed if --selftest is used. Returns `EXIT_FAILURE` if any tests 
 * fail; otherwise, it returns `EXIT_SUCCESS`.
 */

int opt_selftest(char *execname)
{
	diag("Running tests for %s %s (%s)",
	     execname, EXEC_VERSION, EXEC_DATE);

	test_functions();
	test_executable(execname);

	printf("1..%d\n", testnum);
	if (failcount) {
		diag("Looks like you failed %d test%s of %d.", /* gncov */
		     failcount, (failcount == 1) ? "" : "s", /* gncov */
		     testnum);
	}

	return failcount ? EXIT_FAILURE : EXIT_SUCCESS;
}

#undef chp
#undef failed_ok

/* vim: set ts=8 sw=8 sts=8 noet fo+=w tw=79 fenc=UTF-8 : */
