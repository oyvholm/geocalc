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
		ok(1, "%s(): allocstr() failed", __func__); /* gncov */

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
 * verify_definitions() - Check that certain constants are unmodified. Some of 
 * these constants are used in the tests themselves, so the tests will be 
 * automatically updated to match the new text or value. This function contains 
 * hardcoded versions of the expected output or return values. Returns the 
 * number of failed tests.
 */

static int verify_definitions(void)
{
	int r = 0;
	const char *e;

	diag("Verify format definitions");

	e = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	    "<gpx"
	    " xmlns=\"http://www.topografix.com/GPX/1/1\""
	    " version=\"1.1\""
	    " creator=\"Geocalc - https://gitlab.com/oyvholm/geocalc\""
	    ">\n";
	r += ok(strcmp(gpx_header, e) ? 1 : 0, "gpx_header is correct");
	print_gotexp(gpx_header, e);

	e = "Geocalc";
	r += ok(strcmp(PROJ_NAME, e) ? 1 : 0, "PROJ_NAME is correct");
	print_gotexp(PROJ_NAME, e);

	e = "https://gitlab.com/oyvholm/geocalc";
	r += ok(strcmp(PROJ_URL, e) ? 1 : 0, "PROJ_URL is correct");
	print_gotexp(PROJ_URL, e);

	return r;
}

/*
 * test_gotexp_output() - Tests the gotexp_output() function. print_gotexp() 
 * can't be tested directly because it would pollute stderr. Returns the number 
 * of failed tests.
 */

static int test_gotexp_output(void)
{
	int r = 0;
	char *p, *s;

	diag("Test gotexp_output()");

	r += ok(gotexp_output(NULL, "a") ? 1 : 0,
	        "gotexp_output(NULL, \"a\")");

	r += ok(strcmp((p = gotexp_output("got this", "expected this")),
	               "         got: 'got this'\n"
	               "    expected: 'expected this'") ? 1 : 0,
	        "gotexp_output(\"got this\", \"expected this\")");
	free(p);

	r += ok(!print_gotexp(NULL, "expected this"),
	        "print_gotexp(): Arg is NULL");

	s = "gotexp_output(\"a\", \"a\")";
	r += ok((p = gotexp_output("a", "a")) ? 0 : 1,
	        "%s doesn't return NULL", s);
	r += ok(strcmp(p, "         got: 'a'\n    expected: 'a'") ? 1 : 0,
	        "%s: Contents is ok", s);
	free(p);

	s = "gotexp_output() with newline";
	r += ok((p = gotexp_output("with\nnewline", "also with\nnewline"))
	        ? 0 : 1,
	        "%s: Doesn't return NULL", s);
	r += ok(strcmp(p, "         got: 'with\nnewline'\n"
	                  "    expected: 'also with\nnewline'") ? 1 : 0,
	        "%s: Contents is ok", s);
	free(p);

	return r;
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
 * test_valgrind_lines() - Test the behavior of valgrind_lines(). Returns the 
 * number of failed tests.
 */

static int test_valgrind_lines(void)
{
	int r = 0, i;
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

	i = 0;
	while (has[i]) {
		r += ok(!valgrind_lines(has[i]),
		        "valgrind_lines(): Has valgrind marker, string %d", i);
		i++;
	}

	i = 0;
	while (hasnot[i]) {
		r += ok(valgrind_lines(hasnot[i]),
		        "valgrind_lines(): No valgrind marker, string %d", i);
		i++;
	}

	return r;
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
	if (valgrind_lines(ss.err.buf))
		r += ok(1, "Found valgrind output"); /* gncov */
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
 * tc() - Execute command `cmd` and verify that stdout, stderr and the return 
 * value are identical to the expected values. The `exp_*` variables are 
 * strings that must be identical to the actual output. Returns the number of 
 * failed tests.
 */

static int tc(char *cmd[], const char *exp_stdout, const char *exp_stderr,
              const int exp_retval, const char *desc)
{
	return test_command(1, cmd, exp_stdout, exp_stderr, exp_retval, desc);
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
 * test_streams_exec() - Tests the streams_exec() function. Returns the number 
 * of failed tests.
 */

static int test_streams_exec(void)
{
	int r = 0;
	bool orig_valgrind;
	struct streams ss;
	char *s;

	diag("Test streams_exec()");

	streams_init(&ss);
	ss.in.buf = "This is sent to stdin.\n";
	ss.in.len = strlen(ss.in.buf);
	orig_valgrind = opt.valgrind;
	opt.valgrind = false;
	streams_exec(&ss, chp{ progname, NULL });
	opt.valgrind = orig_valgrind;
	s = "streams_exec(progname) with stdin data";
	r += ok(!!strcmp(ss.out.buf, ""), "%s (stdout)", s);
	r += ok(!strstr(ss.err.buf, ": No arguments specified\n"),
	        "%s (stderr)", s);
	r += ok(!(ss.ret == EXIT_FAILURE), "%s (retval)", s);
	streams_free(&ss);

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
 * chk_xmlesc() - Helper function used by test_xml_escape_string(). `s` is the 
 * string to test, and `exp` is the expected outcome. Returns the number of 
 * failed tests.
 */

static int chk_xmlesc(const char *s, const char *exp)
{
	int r = 0;
	char *got;

	assert(s);
	assert(exp);
	if (!s || !exp)
		return ok(1, "%s() received NULL", __func__); /* gncov */

	got = xml_escape_string(s);
	if (!got)
		return ok(1, "xml_escape_string() failed"); /* gncov */
	r += ok(strcmp(got, exp) ? 1 : 0, "xml escape: \"%s\"", s);
	free(got);

	return r;
}

/*
 * test_xml_escape_string() - Tests the xml_escape_string() function. Returns 
 * the number of failed tests.
 */

static int test_xml_escape_string(void)
{
	int r = 0;

	r += chk_xmlesc("", "");
	r += chk_xmlesc("&", "&amp;");
	r += chk_xmlesc("<", "&lt;");
	r += chk_xmlesc(">", "&gt;");
	r += chk_xmlesc("\\", "\\");
	r += chk_xmlesc("a&c", "a&amp;c");
	r += chk_xmlesc("a<c", "a&lt;c");
	r += chk_xmlesc("a>c", "a&gt;c");
	r += chk_xmlesc("abc", "abc");
	r += ok(xml_escape_string(NULL) ? 1 : 0, "xml_escape_string(NULL)");

	return r;
}

/*
 * test_gpx_wpt() - Tests the gpx_wpt() function. Returns the number of failed 
 * tests.
 */

static int test_gpx_wpt(void)
{
	int r = 0;
	char *p, /* String sent to gpx_wpt() */
	     *c, /* Converted version of `p` */
	     *e, /* Expected string from gpx_wpt() */
	     *s; /* Result from gpx_wpt() */

	e = "  <wpt lat=\"12.340000\" lon=\"56.780000\">\n"
	    "    <name>abc def</name>\n"
	    "    <cmt>ghi jkl MN</cmt>\n"
	    "  </wpt>\n";
	s = gpx_wpt(12.34, 56.78, "abc def", "ghi jkl MN");
	r += ok((s ? (strcmp(s, e) ? 1 : 0) : 1),
	        "gpx_wpt() without special chars");
	print_gotexp(s, e);
	free(s);

	e = "  <wpt lat=\"12.340000\" lon=\"56.780000\">\n"
	    "    <name>&amp;</name>\n"
	    "    <cmt>&amp;</cmt>\n"
	    "  </wpt>\n";
	s = gpx_wpt(12.34, 56.78, "&", "&");
	r += ok((s ? (strcmp(s, e) ? 1 : 0) : 1),
	        "gpx_wpt() with ampersand");
	print_gotexp(s, e);
	free(s);

	e = "  <wpt lat=\"12.340000\" lon=\"56.780000\">\n"
	    "    <name>&lt;</name>\n"
	    "    <cmt>&lt;</cmt>\n"
	    "  </wpt>\n";
	s = gpx_wpt(12.34, 56.78, "<", "<");
	r += ok((s ? (strcmp(s, e) ? 1 : 0) : 1),
	        "gpx_wpt() with lt");
	print_gotexp(s, e);
	free(s);

	e = "  <wpt lat=\"12.340000\" lon=\"56.780000\">\n"
	    "    <name>&gt;</name>\n"
	    "    <cmt>&gt;</cmt>\n"
	    "  </wpt>\n";
	s = gpx_wpt(12.34, 56.78, ">", ">");
	r += ok((s ? (strcmp(s, e) ? 1 : 0) : 1),
	        "gpx_wpt() with gt");
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
	r += ok((s ? (strcmp(s, e) ? 1 : 0) : 1),
	        "gpx_wpt() with amp, gt, lt, and more");
	print_gotexp(s, e);
	free(s);
	free(e);

	p = NULL;
	e = NULL;
	s = gpx_wpt(12.34, 56.78, p, p);
	r += ok(s ? 1 : 0, "gpx_wpt() with NULL in name and cmt");
	print_gotexp(s, e);
	if (s) {
		ok(1, "%s():%d: `s` was allocated", /* gncov */
		      __func__, __LINE__);
		free(s); /* gncov */
	}

	p = NULL;
	e = NULL;
	s = gpx_wpt(12.34, 56.78, p, "def");
	r += ok(s ? 1 : 0, "gpx_wpt() with NULL in name");
	print_gotexp(s, e);
	if (s) {
		ok(1, "%s():%d: `s` was allocated", /* gncov */
		      __func__, __LINE__);
		free(s); /* gncov */
	}

	s = gpx_wpt(12.34, 56.78, "abc", NULL);
	r += ok(s ? 0 : 1, "gpx_wpt() with NULL in cmt");
	free(s);

	return r;
}

/*
 * test_valgrind_option() - Tests the --valgrind command line option. Returns 
 * the number of failed tests.
 */

static int test_valgrind_option(void)
{
	int r = 0;
	struct streams ss;

	diag("Test --valgrind");

	if (opt.valgrind) {
		opt.valgrind = false; /* gncov */
		streams_init(&ss); /* gncov */
		streams_exec(&ss, chp{"valgrind", "--version", /* gncov */
		                      NULL});
		if (!strstr(ss.out.buf, "valgrind-")) { /* gncov */
			r += ok(1, "Valgrind is not installed," /* gncov */
			           " disabling Valgrind checks.");
		} else {
			ok(0, "Valgrind is installed"); /* gncov */
			opt.valgrind = true; /* gncov */
		}
		streams_free(&ss); /* gncov */
	}

	r += sc(chp{progname, "--valgrind", "-h", NULL},
	        "Show this",
	        "",
	        EXIT_SUCCESS,
	        "--valgrind -h");

	return r;
}

/*
 * test_standard_options() - Tests the various generic options available in 
 * most programs. Returns the number of failed tests.
 */

static int test_standard_options(void) {
	int r = 0;
	char *s;

	diag("Test -h/--help");
	r += sc(chp{ progname, "-h", NULL },
	        "  Show this help",
	        "",
	        EXIT_SUCCESS,
	        "-h");
	r += sc(chp{ progname, "--help", NULL },
	        "  Show this help",
	        "",
	        EXIT_SUCCESS,
	        "--help");

	diag("Test -v/--verbose");
	r += sc(chp{ progname, "-h", "--verbose", NULL },
	        "  Show this help",
	        "",
	        EXIT_SUCCESS,
	        "-hv: Help text is displayed");
	r += sc(chp{ progname, "-hv", NULL },
	        EXEC_VERSION,
	        "",
	        EXIT_SUCCESS,
	        "-hv: Version number is printed along with the help text");
	r += sc(chp{ progname, "-vvv", "--verbose", NULL },
	        "",
	        ": main(): Using verbose level 4\n",
	        EXIT_FAILURE,
	        "-vvv --verbose: Using correct verbose level");
	r += sc(chp{ progname, "-vvvvq", "--verbose", "--verbose", NULL },
	        "",
	        ": main(): Using verbose level 5\n",
	        EXIT_FAILURE,
	        "--verbose: One -q reduces the verbosity level");

	diag("Test --version");
	s = allocstr("%s %s (%s)\n", progname, EXEC_VERSION, EXEC_DATE);
	if (s) {
		r += sc(chp{ progname, "--version", NULL },
		        s,
		        "",
		        EXIT_SUCCESS,
		        "--version");
		free(s);
	} else {
		r += ok(1, "%s(): allocstr() 1 failed", __func__); /* gncov */
	}
	s = allocstr("%s\n", EXEC_VERSION);
	if (s) {
		r += tc(chp{ progname, "--version", "-q", NULL },
		        s,
		        "",
		        EXIT_SUCCESS,
		        "--version with -q shows only the version number");
		free(s);
	} else {
		r += ok(1, "%s(): allocstr() 2 failed", __func__); /* gncov */
	}

	diag("Test --license");
	r += sc(chp{ progname, "--license", NULL },
	        "GNU General Public License",
	        "",
	        EXIT_SUCCESS,
	        "--license: It's GPL");
	r += sc(chp{ progname, "--license", NULL },
	        "either version 2 of the License",
	        "",
	        EXIT_SUCCESS,
	        "--license: It's version 2 of the GPL");

	diag("Unknown option");
	r += sc(chp{ progname, "--gurgle", NULL },
	        "",
	        ": Option error\n",
	        EXIT_FAILURE,
	        "Unknown option: \"Option error\" message is printed");
	r += sc(chp{ progname, "--gurgle", NULL },
	        "",
	        " --help\" for help screen. Returning with value 1.\n",
	        EXIT_FAILURE,
	        "Unknown option mentions --help");

	return r;
}

/*
 * test_format_option() - Tests the -F/--format option. Returns the number of 
 * failed tests.
 */

static int test_format_option(void)
{
	int r = 0;

	diag("Test -F/--format");
	r += sc(chp{progname, "-vvvv", "-F", "FoRmAt", NULL},
	        "",
	        ": setup_options(): o.format = \"FoRmAt\"\n",
	        EXIT_FAILURE,
	        "-vvvv --format FoRmAt");
	r += sc(chp{progname, "-vvvv", "-F", "FoRmAt", NULL},
	        NULL,
	        ": FoRmAt: Unknown output format\n",
	        EXIT_FAILURE,
	        "-vvvv --format FoRmAt: It says it's unknown");
	r += sc(chp{progname, "-vvvv", "--format", "FoRmAt", NULL},
	        "",
	        ": setup_options(): o.format = \"FoRmAt\"\n",
	        EXIT_FAILURE,
	        "-vvvv --format FoRmAt");
	r += tc(chp{progname, "-vvvv", "-F", "", "lpos", "54,7", "12,22",
	            "0.23", NULL},
	        "44.531328,12.145870\n",
	        NULL,
	        EXIT_SUCCESS,
	        "-F with an empty argument");

	return r;
}

/*
 * test_cmd_bpos() - Tests the `bpos` command. Returns the number of failed 
 * tests.
 */

static int test_cmd_bpos(void)
{
	int r = 0;
	char *p1;

	diag("Test bpos command");
	r += tc(chp{ progname, "bpos", "45,0", "45", "1000", NULL },
	        "45.006359,0.008994\n",
	        "",
	        EXIT_SUCCESS,
	        "bpos 45,0 45 1000");
	r += tc(chp{ progname, "--km", "bpos", "45,0", "45", "1", NULL },
	        "45.006359,0.008994\n",
	        "",
	        EXIT_SUCCESS,
	        "--km bpos 45,0 45 1");
	r += sc(chp{ progname, "bpos", "1,2", "r", "1000", NULL },
	        "",
	        ": Invalid number specified: Invalid argument\n",
	        EXIT_FAILURE,
	        "bpos: bearing is not a number");
	r += sc(chp{ progname, "bpos", "1,2w", "3", "4", NULL },
	        "",
	        ": Invalid number specified: Invalid argument\n",
	        EXIT_FAILURE,
	        "bpos: lon has trailing letter");
	r += sc(chp{ progname, "bpos", "90.0000000001,2", "3", "4", NULL },
	        "",
	        ": Value out of range\n",
	        EXIT_FAILURE,
	        "bpos: lat is out of range");
	r += sc(chp{ progname, "bpos", "1,2", "3", "4", "5", NULL },
	        "",
	        ": Too many arguments\n",
	        EXIT_FAILURE,
	        "bpos: 1 extra argument");
	r += tc(chp{ progname, "-F", "default", "bpos", "40.80542,-73.96546",
	             "188.7", "4817.84", NULL },
	        "40.762590,-73.974113\n",
	        "",
	        EXIT_SUCCESS,
	        "-F default bpos");
	r += tc(chp{ progname, "--format", "gpx", "bpos", "40.80542,-73.96546",
	             "188.7", "4817.84", NULL },
	        (p1 = allocstr(
	              "%s"
	              "  <wpt lat=\"40.762590\" lon=\"-73.974113\">\n"
	              "    <name>bpos</name>\n"
	              "    <cmt>bpos 40.80542,-73.96546 188.7 4817.84</cmt>\n"
	              "  </wpt>\n"
	              "</gpx>\n", gpx_header)),
	        "",
	        EXIT_SUCCESS,
	        "--format gpx bpos");
	free(p1);

	return r;
}

/*
 * test_cmd_course() - Tests the `course` command. Returns the number of failed 
 * tests.
 */

static int test_cmd_course(void)
{
	int r = 0;
	char *exp_stdout;

	diag("Test course command");
	r += tc(chp{ progname, "course", "45,0", "45,180", "1", NULL },
	        "45.000000,0.000000\n"
	        "90.000000,0.000000\n"
	        "45.000000,180.000000\n",
	        "",
	        EXIT_SUCCESS,
	        "course: Across the North Pole");
	r += tc(chp{ progname, "course", "0,0", "0,180", "7", NULL },
	        "0.000000,0.000000\n"
	        "0.000000,22.500000\n"
	        "0.000000,45.000000\n"
	        "0.000000,67.500000\n"
	        "0.000000,90.000000\n"
	        "0.000000,112.500000\n"
	        "0.000000,135.000000\n"
	        "0.000000,157.500000\n"
	        "0.000000,180.000000\n",
	        "",
	        EXIT_SUCCESS,
	        "course 0,0 0,180 7");
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
	r += tc(chp{ progname, "course", "60.39299,5.32415",
	             "35.681389,139.766944", "9", NULL },
	        exp_stdout,
	        "",
	        EXIT_SUCCESS,
	        "course: From Bergen to Tokyo");
	r += tc(chp{ progname, "--km", "course", "60.39299,5.32415",
	             "35.681389,139.766944", "9", NULL },
	        exp_stdout,
	        "",
	        EXIT_SUCCESS,
	        "--km course: From Bergen to Tokyo");
	r += sc(chp{ progname, "course", "1,2", "3,4", NULL },
	        "",
	        ": Missing arguments\n",
	        EXIT_FAILURE,
	        "course: Missing 1 argument");
	r += sc(chp{ progname, "course", "1,2", "3,4", "5", "6", NULL },
	        "",
	        ": Too many arguments\n",
	        EXIT_FAILURE,
	        "course: 1 argument too much");
	r += sc(chp{ progname, "course", "90.00001,0", "12,34", "1", NULL },
	        "",
	        ": Value out of range\n",
	        EXIT_FAILURE,
	        "course: lat1 is outside range");
	r += sc(chp{ progname, "course", "17,0", "12,34", "-1", NULL },
	        "",
	        ": Value out of range\n",
	        EXIT_FAILURE,
	        "course: numpoints is -1");
	r += sc(chp{ progname, "course", "17,6", "12,34", "-0.5", NULL },
	        "",
	        ": Value out of range\n",
	        EXIT_FAILURE,
	        "course: numpoints is -0.5");
	r += tc(chp{ progname, "course", "22,33", "44,55", "0", NULL },
	        "22.000000,33.000000\n"
	        "44.000000,55.000000\n",
	        "",
	        EXIT_SUCCESS,
	        "course: numpoints is 0");
	r += sc(chp{ progname, "course", "17,6%", "12,34", "-1", NULL },
	        "",
	        ": Invalid number specified: Invalid argument\n",
	        EXIT_FAILURE,
	        "course: lon1 is invalid number");
	r += tc(chp{progname, "-F", "default", "course",
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
	r += tc(chp{progname, "-F", "gpx", "course",
	            "52.3731,4.891", "35.681,139.767", "5", NULL},
	        (exp_stdout = allocstr(
	        "%s"
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
	        "</gpx>\n", gpx_header)),
	        "",
	        EXIT_SUCCESS,
	        "-F gpx course, Amsterdam to Tokyo");
	free(exp_stdout);

	return r;
}

/*
 * test_cmd_lpos() - Tests the `lpos` command. Returns the number of failed 
 * tests.
 */

static int test_cmd_lpos(void)
{
	int r = 0;
	char *p;

	diag("Test lpos command");
	r += tc(chp{ progname, "lpos", "45,0", "45,180", "0.5", NULL },
	        "90.000000,0.000000\n",
	        "",
	        EXIT_SUCCESS,
	        "lpos: At the North Pole");
	r += tc(chp{ progname, "--km", "lpos", "45,0", "45,180", "0.5", NULL },
	        "90.000000,0.000000\n",
	        "",
	        EXIT_SUCCESS,
	        "--km lpos: At the North Pole");
	r += tc(chp{ progname, "--format", "gpx", "lpos", "45,0", "45,180",
	             "0.5", NULL },
	             (p = allocstr(
	                  "%s"
	                  "  <wpt lat=\"90.000000\" lon=\"0.000000\">\n"
	                  "    <name>lpos</name>\n"
	                  "    <cmt>lpos 45,0 45,180 0.5</cmt>\n"
	                  "  </wpt>\n"
	                  "</gpx>\n", gpx_header)),
	        "",
	        EXIT_SUCCESS,
	        "--format gpx lpos: At the North Pole");
	free(p);
	r += sc(chp{ progname, "lpos", "1,2", "3,4", NULL },
	        "",
	        ": Missing arguments\n",
	        EXIT_FAILURE,
	        "lpos: Missing 1 argument");
	r += sc(chp{ progname, "lpos", "1,2", "3,4", "5", "6", NULL },
	        "",
	        ": Too many arguments\n",
	        EXIT_FAILURE,
	        "lpos: 1 argument too much");
	r += sc(chp{ progname, "lpos", "-90.00001,0", "12,34", "1", NULL },
	        "",
	        ": Value out of range\n",
	        EXIT_FAILURE,
	        "lpos: lat1 is outside range");
	r += sc(chp{ progname, "lpos", "17,6%", "12,34", "-1", NULL },
	        "",
	        ": Invalid number specified: Invalid argument\n",
	        EXIT_FAILURE,
	        "lpos: lon1 is invalid number");
	r += tc(chp{ progname, "lpos", "1,2", "3,4", "0", NULL },
	        "1.000000,2.000000\n",
	        "",
	        EXIT_SUCCESS,
	        "lpos: fracdist is 0");
	r += tc(chp{ progname, "lpos", "11.231,-34.55", "29.97777,47.311001",
	             "1", NULL },
	        "29.977770,47.311001\n",
	        "",
	        EXIT_SUCCESS,
	        "lpos: fracdist is 1");
	r += tc(chp{ progname, "--km", "lpos", "11.231,-34.55",
	             "29.97777,47.311001", "1", NULL },
	        "29.977770,47.311001\n",
	        "",
	        EXIT_SUCCESS,
	        "--km lpos: fracdist is 1");
	r += sc(chp{ progname, "lpos", "1,2", "3,4", "INF", NULL },
	        "",
	        ": Invalid number specified: Numerical result out of range\n",
	        EXIT_FAILURE,
	        "lpos: fracdist is INF");

	return r;
}

/*
 * test_multiple() - Tests the `bear` or `dist` command. Returns the number of 
 * failed tests.
 */

static int test_multiple(char *cmd)
{
	int r = 0;
	char *p1, *p2;

	assert(cmd);
	if (!cmd)
		return ok(1, "%s(): cmd is NULL", __func__); /* gncov */

	diag("Test %s command", !strcmp(cmd, "bear") ? "bear" : "dist");
	r += sc(chp{ progname, "-vv", cmd, NULL },
	        "",
	        ": Missing arguments\n",
	        EXIT_FAILURE,
	        (p1 = allocstr("%s with no arguments", cmd)));
	free(p1);
	r += sc(chp{ progname, cmd, "1,2", "3", NULL },
	        "",
	        ": Invalid number specified\n",
	        EXIT_FAILURE,
	        (p1 = allocstr("%s: Argument 2 is not a coordinate", cmd)));
	free(p1);
	r += tc(chp{ progname, cmd, "1,2", "3,4", NULL },
	        (p2 = allocstr("%s\n",
	                       !strcmp(cmd, "bear") ? "44.951998"
	                                            : "314402.951024")),
	        "",
	        EXIT_SUCCESS,
	        (p1 = allocstr("%s 1,2 3,4", cmd)));
	free(p1);
	free(p2);
	r += sc(chp{ progname, cmd, "1,2", "3,4", "5", NULL },
	        "",
	        ": Too many arguments\n",
	        EXIT_FAILURE,
	        (p1 = allocstr("%s with 1 argument too much", cmd)));
	free(p1);
	r += sc(chp{ progname, cmd, "1,2", "3,1e+900", NULL },
	        "",
	        ": Invalid number specified:"
	        " Numerical result out of range\n",
	        EXIT_FAILURE,
	        (p1 = allocstr("%s with 1 number too large", cmd)));
	free(p1);
	r += sc(chp{ progname, cmd, "1,2", "urgh,4", NULL },
	        "",
	        ": Invalid number specified:"
	        " Invalid argument\n",
	        EXIT_FAILURE,
	        (p1 = allocstr("%s with 1 non-number", cmd)));
	free(p1);
	r += sc(chp{ progname, cmd, "1,2.9y", "3,4", NULL },
	        "",
	        ": Invalid number specified:"
	        " Invalid argument\n",
	        EXIT_FAILURE,
	        (p1 = allocstr("%s with non-digit after number", cmd)));
	free(p1);
	r += sc(chp{ progname, cmd, "1,2 g", "3,4", NULL },
	        "",
	        ": Invalid number specified:"
	        " Invalid argument\n",
	        EXIT_FAILURE,
	        (p1 = allocstr("%s with whitespace and non-digit after number",
	                       cmd)));
	free(p1);
	r += tc(chp{ progname, cmd, "10,2,", "3,4", NULL },
	        (p2 = allocstr("%s\n",
	                       !strcmp(cmd, "bear") ? "164.027619"
	                                            : "809080.682265")),
	        "",
	        EXIT_SUCCESS,
	        (p1 = allocstr("%s with comma after number", cmd)));
	free(p1);
	free(p2);
	r += sc(chp{ progname, cmd, "1,2", "3,NAN", NULL },
	        "",
	        ": Invalid number specified:"
	        " Invalid argument\n",
	        EXIT_FAILURE,
	        (p1 = allocstr("%s with NAN", cmd)));
	free(p1);
	r += sc(chp{ progname, cmd, "1,2", "3,INF", NULL },
	        "",
	        ": Invalid number specified:"
	        " Numerical result out of range\n",
	        EXIT_FAILURE,
	        (p1 = allocstr("%s with INF", cmd)));
	free(p1);
	r += sc(chp{ progname, cmd, "1,2", "", NULL },
	        "",
	        ": Invalid number specified\n",
	        EXIT_FAILURE,
	        (p1 = allocstr("%s with empty argument", cmd)));
	free(p1);
	r += sc(chp{ progname, cmd, "1,180.001", "3,4", NULL },
	        "",
	        ": Value out of range\n",
	        EXIT_FAILURE,
	        (p1 = allocstr("%s: lon1 out of range", cmd)));
	free(p1);
	r += tc(chp{ progname, cmd, "90,0", "-90,0", NULL },
	        (p2 = allocstr("%s\n",
	                       !strcmp(cmd, "bear") ? "180.000000"
	                                            : "20015086.796021")),
	        "",
	        EXIT_SUCCESS,
	        (p1 = allocstr("%s 90,0 -90,0", cmd)));
	free(p1);
	free(p2);
	r += tc(chp{ progname, "--km", cmd, "90,0", "-90,0", NULL },
	        !strcmp(cmd, "bear") ? "180.000000\n" : "20015.086796\n",
	        "",
	        EXIT_SUCCESS,
	        (p1 = allocstr("--km %s 90,0 -90,0", cmd)));
	free(p1);
	r += tc(chp{ progname, "-F", "default", cmd, "34,56", "-78,9", NULL },
	        !strcmp(cmd, "bear") ? "189.693136\n" : "12835310.777042\n",
	        "",
	        EXIT_SUCCESS,
	        (p1 = allocstr("-F default %s", cmd)));
	free(p1);
	r += sc(chp{ progname, "--format", "gpx", cmd, "34,56", "-78,9",
	             NULL },
	        "",
	        ": No way to display this info in GPX format\n",
	        EXIT_FAILURE,
	        (p1 = allocstr("--format gpx %s", cmd)));
	free(p1);

	return r;
}

/*
 * test_functions() - Tests various functions directly. Returns the number of 
 * failed tests.
 */

static int test_functions(void)
{
	int r = 0;

	diag("Test selftest routines");
	r += ok(!ok(0, NULL), "ok(0, NULL)");
	r += verify_definitions();
	r += test_diag();
	r += test_gotexp_output();
	r += test_valgrind_lines();

	diag("Test various routines");
	diag("Test myerror()");
	errno = EACCES;
	r += ok(!(myerror("errno is EACCES") > 37),
	        "myerror(): errno is EACCES");
	errno = 0;
	r += ok(!(std_strerror(0) != NULL), "std_strerror(0)");
	r += ok(!(mystrdup(NULL) == NULL), "mystrdup(NULL) == NULL");
	r += test_allocstr();
	r += test_streams_exec();
	r += test_parse_coordinate();
	r += test_xml_escape_string();
	r += test_gpx_wpt();

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
	r += test_valgrind_option();
	r += sc(chp{ progname, "abc", NULL },
	        "",
	        ": Unknown command: abc\n",
	        EXIT_FAILURE,
	        "Unknown command");
	r += test_standard_options();
	r += test_format_option();
	r += test_cmd_bpos();
	r += test_cmd_course();
	r += test_cmd_lpos();
	r += test_multiple("bear");
	r += test_multiple("dist");

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

	r += test_functions();
	r += test_executable();

	printf("1..%d\n", testnum);
	if (r)
		diag("Looks like you failed %d test%s of %d.", /* gncov */
		     r, (r == 1) ? "" : "s", testnum);

	return r ? EXIT_FAILURE : EXIT_SUCCESS;
}

#undef chp

/* vim: set ts=8 sw=8 sts=8 noet fo+=w tw=79 fenc=UTF-8 : */
