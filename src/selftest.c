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

static int testnum = 0;

/*
 * ok() - Print a log line to stdout. If `i` is 0, an "ok" line is printed, 
 * otherwise a "not ok" line is printed. `desc` is the test description and can 
 * use `printf` sequences. If `desc` is NULL, it returns 1. Otherwise, it 
 * returns `i`.
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

	return i;
}

/*
 * diag_output_va() - Receives a printf-like string and returns an allocated 
 * string, prefixed it with "# " and all '\n' characters converted to "\n# ". 
 * Returns NULL if anything fails or `format` is NULL.
 */

static char *diag_output_va(const char *format, va_list ap)
{
	const char *src;
	char *buffer, *converted_buffer, *dst;
	int needed;
	size_t buffer_size = BUFSIZ, converted_size;
	va_list ap_copy;

	if (!format)
		return NULL; /* gncov */

	buffer = malloc(buffer_size);
	if (!buffer)
		return NULL; /* gncov */

	va_copy(ap_copy, ap);
	needed = vsnprintf(buffer, buffer_size, format, ap);

	if ((size_t)needed >= buffer_size) {
		free(buffer);
		buffer_size = (size_t)needed + 1;
		buffer = malloc(buffer_size);
		if (!buffer)
			return NULL; /* gncov */
		vsnprintf(buffer, buffer_size, format, ap_copy);
	}
	va_end(ap_copy);

	/* Prepare for worst case, every char is a newline. */
	converted_size = strlen(buffer) * 3 + 1;
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
 * Returns number of failed tests.
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
 * successful, or 1 if `format` is NULL.
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
	if (!converted_buffer)
		return ok(1, "%s(): diag_output_va() failed", /* gncov */
		             __func__);
	fprintf(stderr, "%s\n", converted_buffer);
	fflush(stderr);
	free(converted_buffer);

	return 0;
}

/*
 * print_gotexp() - Print the value of the actual and exepected data. Used when 
 * a test fails. Returns 1 if `got` or `exp` is NULL, otherwise 0.
 */

static int print_gotexp(const char *got, const char *exp)
{
	if (!got || !exp)
		return 1;

	if (strcmp(got, exp)) {
		diag("         got: '%s'", got);
		diag("    expected: '%s'", exp);
	}

	return 0;
}

/*
 * test_diag_big() - Tests diag_output() with a string larger than BUFSIZ. 
 * Returns the number of failed tests.
 */

static int test_diag_big(void)
{
	int r = 0;
	size_t size;
	char *p, *outp;

	size = BUFSIZ * 2;
	diag("%s(): BUFSIZ = %d, size = %zu", __func__, BUFSIZ, size);
	p = malloc(size + 1);
	if (!p)
		return ok(1, "%s(): malloc(%zu) failed", /* gncov */
		             __func__, size + 1);

	memset(p, 'a', size);
	p[3] = 'b';
	p[4] = 'c';
	p[size] = '\0';

	outp = diag_output("%s", p);
	r += ok(!outp, "diag_big: diag_output() returns ok");
	r += ok(!(strlen(outp) == size + 2),
	        "diag_big: String length is correct");
	r += ok(strncmp(outp, "# aaabcaaa", 10) ? 1 : 0,
	        "diag_big: Beginning is ok");
	free(outp);
	free(p);

	return r;
}

/*
 * test_diag() - Tests the diag_output() function. diag() can't be tested 
 * directly because it would pollute the the test output. Returns the number of 
 * failed tests.
 */

static int test_diag(void) {
	int r = 0;
	char *p, *s;

	r += ok(!diag(NULL), "diag(NULL)");
	r += ok(!(diag_output(NULL) == NULL), "diag_output() receives NULL");

	p = diag_output("Text with\nnewline");
	r += ok(p ? 0 : 1, "diag_output() with newline didn't return NULL");
	s = "# Text with\n# newline";
	r += ok(p ? (strcmp(p, s) ? 1 : 0) : 1,
	        "diag_output() with newline, output is ok");
	print_gotexp(p, s);
	free(p);

	p = diag_output("%d = %s, %d = %s, %d = %s",
	                1, "one", 2, "two", 3, "three");
	r += ok(p ? 0 : 1, "diag_output() with %%d and %%s didn't return"
	                   " NULL");
	s = "# 1 = one, 2 = two, 3 = three";
	r += ok(p ? (strcmp(p, s) ? 1 : 0) : 1,
	        "diag_output() with %%d and %%s");
	print_gotexp(p, s);
	free(p);

	r += test_diag_big();

	return r;
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
		return strcmp(got, exp) ? 1 : 0;

	return strstr(got, exp) ? 0 : 1;
}

/*
 * test_command() - Run the executable with arguments in `cmd` and verify 
 * stdout, stderr and the return value against `exp_stdout`, `exp_stderr` and 
 * `exp_retval`. Returns the number of failed tests, or 1 if `cmd` is NULL.
 */

static int test_command(const char identical, char *cmd[],
                        const char *exp_stdout, const char *exp_stderr,
                        const int exp_retval, const char *desc)
{
	int r = 0;
	struct streams ss;

	assert(cmd);
	if (!cmd)
		return 1; /* gncov */

	if (opt.verbose >= VERBOSE_DEBUG) {
		int i = -1; /* gncov */
		fprintf(stderr, "# %s(", __func__); /* gncov */
		while (cmd[++i]) /* gncov */
			fprintf(stderr, "%s\"%s\"", /* gncov */
			                i ? ", " : "", cmd[i]); /* gncov */
		fprintf(stderr, ")\n"); /* gncov */
	}

	streams_init(&ss);
	streams_exec(&ss, cmd);
	if (exp_stdout) {
		r += ok(tc_cmp(identical, ss.out.buf, exp_stdout),
		        "%s (stdout)", desc);
		if (tc_cmp(identical, ss.out.buf, exp_stdout)) {
			print_gotexp(ss.out.buf, exp_stdout); /* gncov */
		}
	}
	if (exp_stderr) {
		r += ok(tc_cmp(identical, ss.err.buf, exp_stderr),
		        "%s (stderr)", desc);
		if (tc_cmp(identical, ss.err.buf, exp_stderr)) {
			print_gotexp(ss.err.buf, exp_stderr); /* gncov */
		}
	}
	r += ok(!(ss.ret == exp_retval), "%s (retval)", desc);
	if (ss.ret != exp_retval) {
		char *g = allocstr("%d", ss.ret), /* gncov */
		     *e = allocstr("%d", exp_retval); /* gncov */
		if (!g || !e) /* gncov */
			r += ok(1, "%s(): allocstr() failed", /* gncov */
			           __func__); /* gncov */
		else
			print_gotexp(g, e); /* gncov */
		free(e); /* gncov */
		free(g); /* gncov */
	}
	streams_free(&ss);

	return r;
}

/*
 * sc() - Execute command `cmd` and verify that stdout, stderr and the return 
 * value corresponds to the expected values. The `exp_*` variables are 
 * substrings that must occur in the actual output. Returns the number of 
 * failed tests.
 */

static int sc(char *cmd[], const char *exp_stdout, const char *exp_stderr,
              const int exp_retval, const char *desc)
{
	return test_command(0, cmd, exp_stdout, exp_stderr, exp_retval, desc);
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
 * test_allocstr() - Tests the allocstr() function. Returns the number of 
 * failed tests.
 */

static int test_allocstr(void)
{
	const size_t bufsize = BUFSIZ * 2 + 1;
	char *p, *p2, *p3;
	int r = 0;
	size_t alen;

	p = malloc(bufsize);
	if (!p) {
		r += ok(1, "%s(): malloc() failed", __func__); /* gncov */
		return r; /* gncov */
	}
	memset(p, 'a', bufsize - 1);
	p[bufsize - 1] = '\0';
	p2 = allocstr("%s", p);
	if (!p2) {
		r += ok(1, "%s(): allocstr() failed" /* gncov */
		           " with BUFSIZ * 2",
		           __func__);
		goto free_p; /* gncov */
	}
	alen = strlen(p2);
	r += ok(!(alen == BUFSIZ * 2), "allocstr(): strlen is correct");
	p3 = p2;
	while (*p3) {
		if (*p3 != 'a') {
			p3 = NULL; /* gncov */
			break; /* gncov */
		}
		p3++;
	}
	r += ok(!(p3 != NULL), "allocstr(): Content of string is correct");
	free(p2);
free_p:
	free(p);

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
 * test_executable() - Run various tests with the executable and verify that 
 * stdout, stderr and the return value are as expected. Returns the number of 
 * failed tests.
 */

static int test_executable(void)
{
	int r = 0;

	diag("Test the executable");
	r += sc(chp{ progname, "abc", NULL },
	        "",
	        ": Unknown command: abc\n",
	        EXIT_FAILURE,
	        "Unknown command");
	diag("Test --valgrind");
	r += sc(chp{progname, "--valgrind", "-h", NULL},
	        "Show this",
	        "",
	        EXIT_SUCCESS,
	        "--valgrind -h");

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
	r += ok(!ok(0, NULL), "ok(0, NULL)");
	r += test_diag();

	diag("Test print_gotexp()");
	r += ok(print_gotexp("got this", "expected this"),
	        "print_gotexp() demo");
	r += ok(!print_gotexp(NULL, "expected this"),
	        "print_gotexp(): Arg is NULL");

	diag("Test various routines");
	diag("Test myerror()");
	errno = EACCES;
	r += ok(!(myerror("errno is EACCES") > 37),
	        "myerror(): errno is EACCES");
	errno = 0;
	r += ok(!(std_strerror(0) != NULL), "std_strerror(0)");
	r += ok(!(mystrdup(NULL) == NULL), "mystrdup(NULL) == NULL");
	r += test_allocstr();
	r += test_parse_coordinate();

	r += test_executable();

	printf("1..%d\n", testnum);
	if (r)
		diag("Looks like you failed %d test%s of %d.", /* gncov */
		     r, (r == 1) ? "" : "s", testnum);

	return r ? EXIT_FAILURE : EXIT_SUCCESS;
}

#undef chp

/* vim: set ts=8 sw=8 sts=8 noet fo+=w tw=79 fenc=UTF-8 : */
