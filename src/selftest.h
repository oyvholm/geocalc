/*
 * selftest.h
 * File ID: 8ae1ff72-4d86-11f1-a201-83850402c3ce
 *
 * Library version: 0.1.0+0.git-9fab35005016 (2026-10-23)
 *
 * Available from:
 *
 *   https://github.com/oyvholm/libselftest
 *   https://gitlab.com/oyvholm/libselftest
 *
 * (C)opyleft 2026- Øyvind A. Holm <sunny@sunbase.org>
 *
 * This library is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU Lesser General Public License as published by the 
 * Free Software Foundation; either version 2.1 of the License, or (at your 
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License 
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License 
 * along with this library; if not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SELFTEST_H
#define SELFTEST_H

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <regex.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define LST_VERSION  "0.1.0+0.git-9fab35005016"
#define LST_DATE  "2026-10-23"

/*
 * Main test macros, meant to be a human-friendly frontend against lst_ok(). 
 * Unlike most other testing frameworks that return 1 for success and 0 for 
 * error, these functions and macros return 0 for success and 1 if the test 
 * fails. The reasoning behind this is:
 *
 * We don't look for successful tests, but tests that fail. By returning 1 when 
 * the test fails, the return value can be used to increase a fail counter to 
 * find the total number of failed tests, or take special action based on the 
 * outcome of a single test or a series of previous tests. If the macros or 
 * lst_ok() had returned 0 for failure and non-zero for success, we would need 
 * an additional counter to keep track of the number of executed tests and then 
 * use subtraction to see if any tests failed. Besides, C typically returns 0 
 * for success and non-zero for failure, so this convention should feel natural 
 * to C programmers. The `lst_ok(expr, desc, ...)` function is a frontend to 
 * `lst_ok_va()`, which evaluates `expr`, printing an "ok" line to stdout if 
 * `expr` is 0 (success, returning 0) or "not ok" if `expr` is non-zero 
 * (failure, returning 1), using `desc` with any additional arguments for the 
 * test description.
 *
 * All macros have a description parameter at the end which supports printf() 
 * sequences and additional optional arguments.
 *
 * In addition to the regular macros, the same macros exist as `OK_*_L()` 
 * variants. The functionality is identical, but they have an extra `linenum` 
 * parameter before the `desc` parameter. This is used to communicate to 
 * lst_ok() and lst_ok_va() the line number of the test. If the test is inside 
 * a helper function that's repeated many times, the line number inside the 
 * helper function is sent to lst_ok(), and that's not what's usually wanted. 
 * By having a `const int linenumber` parameter in the helper function, the 
 * scopes above can deliver the actual line the test is executed from.
 *
 * This is a list of the various macros, with a description of which types of 
 * tests they're intended for, and the criteria for success:
 *
 * OK_EQUAL(a, b, desc, ...) - Verifies that the values `a` and `b` are 
 * identical. It uses `==` for comparison and is intended for variables of any 
 * type that supports the `==` operator.
 * Example: OK_EQUAL(num_found, expected, "Found %u elements", expected);
 *
 * OK_ERROR(msg, ...) - Generates a test failure with `msg` as the description. 
 * Used for unexpected errors that can't be ignored, incrementing the failure 
 * counter and failing the test run. Typically used in conditional checks.
 * Example: if (valgrind_lines(stderr_output))
 *                  OK_ERROR("Found Valgrind output in stderr");
 *
 * OK_FAILURE(func, desc, ...) - Used for functions that return 0 for success 
 * and non-zero for failure. Expects the function to fail (return non-zero). 
 * Example: OK_FAILURE(stat(file), "File is unreadable or doesn't exist");
 *
 * OK_FALSE(val, desc, ...) - Used for boolean values or expressions expected 
 * to be false. Negated expressions can be confusing, so `OK_TRUE` is usually a 
 * clearer choice for complex checks.
 * Examples: OK_FALSE(user_exists(user), "User %s doesn't exist", user);
 *           OK_FALSE(result == 5, "Result is not 5");
 *
 * OK_MEMCMP(a, b, len, desc, ...) - Compares `len` bytes of the buffers `a` 
 * and `b` and succeeds if the buffers are identical. This macro differs from 
 * OK_STRNCMP in that the buffers can contain null bytes.
 * Example: OK_MEMCMP(buf1, buf2, 512, "Buffers are identical");
 *
 * OK_NOTEQUAL(a, b, desc, ...) - Expects the values `a` and `b` to be 
 * different. The `!=` operator is used for the comparison.
 * Example: OK_NOTEQUAL(userid1, userid2, "The users have different IDs");
 *
 * OK_NOTNULL(p, desc, ...) - Succeeds if the pointer `p` is non-NULL.
 * Examples: OK_NOTNULL(strstr(txt, substr), "Substring was found in text");
 *           OK_NOTNULL(fp, "File pointer is not NULL");
 *
 * OK_NULL(p, desc, ...) - Expects the pointer `p` to be NULL.
 * Examples: OK_NULL(getenv(var), "Environment variable %s is undefined", var);
 *           OK_NULL(strchr(file, '/'), "No slash in file name \"%s\"", file);
 *
 * OK_STRCMP(a, b, desc, ...) - Compares the strings `a` and `b` and succeeds 
 * if they're identical.
 * Example: OK_STRCMP(file, "index.html", "File name is correct");
 *
 * OK_STRNCMP(a, b, len, desc, ...) - Compares the first `len` characters of 
 * the strings `a` and `b` and succeeds if the substrings are identical.
 * Example: OK_STRNCMP(file, "tmp", 3, "File name has \"tmp\" prefix");
 *
 * OK_SUCCESS(func, desc, ...) - Used for functions that return 0 for success 
 * and non-zero for failure. Expects the function to succeed (return zero).
 * Example: OK_SUCCESS(rmdir(tempdir), "Delete temporary directory");
 *
 * OK_TRUE(val, desc, ...) - Expects the boolean value `val` to be true. This 
 * macro can also be used for comparisons or expressions not covered by other 
 * macros, like checking if a value is larger or smaller than another.
 * Examples: OK_TRUE(file_exists(file), "File %s was created", file);
 *           OK_TRUE(errcount < 10, "Error count %d is below 10", errcount);
 */

#define OK_EQUAL_L(a, b, file, linenum, ...)  \
        lst_ok(!((a) == (b)), (file), (linenum), __VA_ARGS__)
#define OK_ERROR_L(file, linenum, ...)  \
        lst_ok(1, (file), (linenum), __VA_ARGS__)
#define OK_FAILURE_L(func, file, linenum, ...)  \
        lst_ok(!(func), (file), (linenum), __VA_ARGS__)
#define OK_FALSE_L(val, file, linenum, ...)  \
        lst_ok(!!(val), (file), (linenum), __VA_ARGS__)
#define OK_MEMCMP_L(a, b, len, file, linenum, ...)  \
        lst_ok(!!memcmp((a), (b), (len)), (file), (linenum), __VA_ARGS__)
#define OK_NOTEQUAL_L(a, b, file, linenum, ...)  \
        lst_ok(!((a) != (b)), (file), (linenum), __VA_ARGS__)
#define OK_NOTNULL_L(p, file, linenum, ...)  \
        lst_ok(!(p), (file), (linenum), __VA_ARGS__)
#define OK_NULL_L(p, file, linenum, ...)  \
        lst_ok(!!(p), (file), (linenum), __VA_ARGS__)
#define OK_STRCMP_L(a, b, file, linenum, ...)  \
        lst_ok(!!strcmp((a), (b)), (file), (linenum), __VA_ARGS__)
#define OK_STRNCMP_L(a, b, len, file, linenum, ...)  \
        lst_ok(!!strncmp((a), (b), (len)), (file), (linenum), __VA_ARGS__)
#define OK_SUCCESS_L(func, file, linenum, ...)  \
        lst_ok(!!(func), (file), (linenum), __VA_ARGS__)
#define OK_TRUE_L(val, file, linenum, ...)  \
        lst_ok(!(val), (file), (linenum), __VA_ARGS__)

#define OK_EQUAL(a, b, ...)  \
        OK_EQUAL_L((a), (b), __FILE__, __LINE__, __VA_ARGS__)
#define OK_ERROR(...)  \
        OK_ERROR_L(__FILE__, __LINE__, __VA_ARGS__)
#define OK_FAILURE(func, ...)  \
        OK_FAILURE_L((func), __FILE__, __LINE__, __VA_ARGS__)
#define OK_FALSE(val, ...)  \
        OK_FALSE_L((val), __FILE__, __LINE__, __VA_ARGS__)
#define OK_MEMCMP(a, b, len, ...)  \
        OK_MEMCMP_L((a), (b), (len), __FILE__, __LINE__, __VA_ARGS__)
#define OK_NOTEQUAL(a, b, ...)  \
        OK_NOTEQUAL_L((a), (b), __FILE__, __LINE__, __VA_ARGS__)
#define OK_NOTNULL(p, ...)  \
        OK_NOTNULL_L((p), __FILE__, __LINE__, __VA_ARGS__)
#define OK_NULL(p, ...)  \
        OK_NULL_L((p), __FILE__, __LINE__, __VA_ARGS__)
#define OK_STRCMP(a, b, ...)  \
        OK_STRCMP_L((a), (b), __FILE__, __LINE__, __VA_ARGS__)
#define OK_STRNCMP(a, b, len, ...)  \
        OK_STRNCMP_L((a), (b), (len), __FILE__, __LINE__, __VA_ARGS__)
#define OK_SUCCESS(func, ...)  \
        OK_SUCCESS_L((func), __FILE__, __LINE__, __VA_ARGS__)
#define OK_TRUE(val, ...)  \
        OK_TRUE_L((val), __FILE__, __LINE__, __VA_ARGS__)

#define no_null(a)  ((a) ? (a) : "(null)")

#define EXECSTR  "__EXSTR__"
#define chp  (char *[])
#define print_gotexp_nostr(seq, got, exp)  do { \
	char *pgn_g = lst_allocstr((seq), (got)), \
	     *pgn_e = lst_allocstr((seq), (exp)); \
	if (!pgn_g || !pgn_e) \
		failed_ok("lst_allocstr()"); \
	else \
		print_gotexp(pgn_g, pgn_e); \
	free(pgn_e); \
	free(pgn_g); \
} while (0)
#define print_gotexp_double(got, exp)  do { \
	const double pgd_got = (got), pgd_exp = (exp); \
	if (pgd_got != pgd_exp) \
		print_gotexp_nostr("%.8f", pgd_got, pgd_exp); \
} while (0)
#define print_gotexp_int(got, exp)  do { \
	const int pgi_got = (got), pgi_exp = (exp); \
	if (pgi_got != pgi_exp) \
		print_gotexp_nostr("%d", pgi_got, pgi_exp); \
} while (0)
#define print_gotexp_size_t(got, exp)  do { \
	const size_t pgst_got = (got), pgst_exp = (exp); \
	if (pgst_got != pgst_exp) \
		print_gotexp_nostr("%zu", pgst_got, pgst_exp); \
} while (0)
#define print_gotexp_ulong(got, exp)  do { \
	const unsigned long pgu_got = (got), pgu_exp = (exp); \
	if (pgu_got != pgu_exp) \
		print_gotexp_nostr("%lu", pgu_got, pgu_exp); \
} while (0)
#define DIAG_DEBL  lst_diag("DEBL: %s:%s():%d", __FILE__, __func__, __LINE__)
#define diag_errno()  do { \
	lst_diag("errno = %d (%s)", errno, strerror(errno)); \
	errno = 0; \
} while (0)
#define failed_ok(a)  do { \
	if (errno) \
		OK_ERROR("%s():%d: %s failed: %s", \
		         __func__, __LINE__, (a), strerror(errno)); \
	else \
		OK_ERROR("%s():%d: %s failed", __func__, __LINE__, (a)); \
	errno = 0; \
} while (0)

#ifndef LST_TMPDIR
#define LST_TMPDIR  ".selftest.tmp"
#endif

#define sc(cmd, exp_stdout, exp_stderr, ...)  \
        sc_func(__FILE__, __LINE__, (cmd), (exp_stdout), (exp_stderr), \
                __VA_ARGS__)
#define tc(cmd, exp_stdout, exp_stderr, ...)  \
        tc_func(__FILE__, __LINE__, (cmd), (exp_stdout), (exp_stderr), \
                __VA_ARGS__)
#define rc(cmd, exp_stdout, exp_stderr, ...)  \
        rc_func(__FILE__, __LINE__, (cmd), (exp_stdout), (exp_stderr), \
                __VA_ARGS__)
#define verify_output_files(desc, exp_stdout, exp_stderr)  \
        verify_output_files_func(__FILE__, __LINE__, \
                                 desc, exp_stdout, exp_stderr)

struct lst_binbuf {
	size_t alloc;
	size_t len;
	char *buf;
};

struct lst_streams {
	struct lst_binbuf in;
	struct lst_binbuf out;
	struct lst_binbuf err;
	int ret;
};

/* selftest.c */
extern bool lst_use_valgrind;
extern char *lst_execname;
extern int lst_testnum;
void lst_binbuf_init(struct lst_binbuf *sb);
void lst_binbuf_free(struct lst_binbuf *sb);
char *lst_binbuf_cpy(struct lst_binbuf *dest, const struct lst_binbuf *src);
bool lst_file_exists(const char *s);
char *lst_read_from_fp(FILE *fp, struct lst_binbuf *dest);
char *lst_read_from_file(const char *fname);
int lst_init_home(void);
void lst_streams_init(struct lst_streams *dest);
void lst_streams_free(struct lst_streams *dest);
int lst_streams_exec(struct lst_streams *dest, char *cmd[]);
int lst_is_root(void);
char *lst_allocstr_va(const char *format, va_list ap);
char *lst_allocstr(const char *format, ...);
size_t lst_count_substr(const char *s, const char *substr);
char *lst_str_replace(const char *s, const char *s1, const char *s2);
int lst_re_check(const char *pattern, const char *s);
void lst_bail_out(const char *msg, ...);
int lst_ok_va(const int i, const char *file, const int linenum,
              const char *desc, va_list ap);
int lst_ok(const int i, const char *file, const int linenum,
           const char *desc, ...);
int lst_diag(const char *format, ...);
int print_gotexp(const char *got, const char *exp);
void sc_func(const char *file, const int linenum, char *cmd[],
             const char *exp_stdout, const char *exp_stderr,
             const int exp_retval, const char *desc, ...);
void tc_func(const char *file, const int linenum, char *cmd[],
             const char *exp_stdout, const char *exp_stderr,
             const int exp_retval, const char *desc, ...);
void rc_func(const char *file, const int linenum, char *cmd[],
             const char *exp_stdout, const char *exp_stderr,
             const int exp_retval, const char *desc, ...);
int lst_init_output_files(void);
void lst_restore_output_files(void);
void verify_output_files_func(const char *file, const int linenum,
                              const char *desc,
                              const char *exp_stdout, const char *exp_stderr);
void lst_cleanup_tempdir(const char *file, const int linenum);
int selftest_failcount(void);
int print_selftest_footer(void);
char *get_execname(const char *ename);
void test_selftest_funcs(void);

#endif /* ifndef SELFTEST_H */

/* vim: set ts=8 sw=8 sts=8 noet fo+=w tw=79 fenc=UTF-8 : */
