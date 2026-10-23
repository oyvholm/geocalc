/*
 * selftest.c
 * File ID: 9c088252-4a58-11f1-bac1-83850402c3ce
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

#include "selftest.h"

/*
 * The functions in this file are supposed to be compatible with `Test::More` 
 * in Perl 5 as far as possible.
 */

bool lst_use_valgrind = false;
char *lst_execname;
int lst_testnum = 0;
static int failcount = 0;

static const char *stderr_file = LST_TMPDIR "/stderr.txt";
static const char *stdout_file = LST_TMPDIR "/stdout.txt";
static int orig_stderr_fd = -1;
static int orig_stdout_fd = -1;

/******************************************************************************
                               Utility functions
******************************************************************************/

/*
 * lst_binbuf_init() - Prepares a `struct lst_binbuf` for use, returns nothing.
 */

void lst_binbuf_init(struct lst_binbuf *sb)
{
	assert(sb);
	sb->alloc = sb->len = 0;
	sb->buf = NULL;
}

/*
 * lst_binbuf_free() - Deallocates a `struct lst_binbuf` and sets the struct 
 * values to their initial state.
 */

void lst_binbuf_free(struct lst_binbuf *sb)
{
	assert(sb);
	if (sb->alloc)
		free(sb->buf);
	lst_binbuf_init(sb);
}

/*
 * lst_binbuf_cpy() - Creates a lst_binbuf copy of `src` and stores it in 
 * `dest`. Returns a char pointer to `dest->buf` if successful, or NULL if 
 * allocation failed or `dest`, `src`, or `src->alloc` are NULL.
 */

char *lst_binbuf_cpy(struct lst_binbuf *dest, const struct lst_binbuf *src)
{
	struct lst_binbuf sb;

	assert(dest);
	assert(src);
	if (!dest || !src) {
		OK_ERROR("%s(): `dest` or `src` is NULL", /* gncov */
		         __func__);
		return NULL; /* gncov */
	}
	if (!src->alloc)
		return NULL;
	if (src->len > src->alloc)
		return NULL;

	lst_binbuf_init(&sb);
	sb.alloc = src->alloc;
	sb.buf = malloc(sb.alloc);
	if (!sb.buf) {
		failed_ok("malloc()"); /* gncov */
		return NULL; /* gncov */
	}
	sb.len = src->len;
	memcpy(sb.buf, src->buf, src->alloc);
	*dest = sb;

	return dest->buf;
}

/*
 * lst_file_exists() - Returns `true` if a filesystem entry (file, directory, 
 * symlink, etc.) exists at path `s`, or `false` if it does not. Symlinks are 
 * treated as ordinary entries, so if the symlink exists but is broken, the 
 * function still returns `true`.
 */

bool lst_file_exists(const char *s)
{
	struct stat st;

	assert(s);
	assert(*s);

	if (lstat(s, &st)) {
		if (errno == ENOENT)
			errno = 0;
		return false;
	}

	return true;
}

/*
 * lst_read_from_fp() - Reads data from fp into an allocated buffer and returns 
 * a pointer to the allocated memory or NULL if something failed.
 */

char *lst_read_from_fp(FILE *fp, struct lst_binbuf *dest)
{
	struct lst_binbuf buf;
	size_t bufsize = BUFSIZ;

	assert(fp);

	lst_binbuf_init(&buf);

	do {
		char *p = NULL;
		char *new_mem = realloc(buf.buf, bufsize + buf.len);
		size_t bytes_read;

		if (!new_mem) {
			lst_binbuf_free(&buf); /* gncov */
			return NULL; /* gncov */
		}
		buf.alloc = bufsize + buf.len;
		buf.buf = new_mem;
		p = buf.buf + buf.len;
		bytes_read = fread(p, 1, bufsize - 1, fp);
		buf.len += bytes_read;
		p[bytes_read] = '\0';
		if (ferror(fp)) {
			lst_binbuf_free(&buf); /* gncov */
			return NULL; /* gncov */
		}
	} while (!feof(fp));

	if (dest)
		*dest = buf;

	return buf.buf;
}

/*
 * lst_read_from_file() - Reads contents of file `fname` and returns a pointer 
 * to an allocated string with the contents, or NULL if error.
 */

char *lst_read_from_file(const char *fname)
{
	FILE *fp;
	char *retval;

	assert(fname);
	assert(*fname);

	fp = fopen(fname, "rb");
	if (!fp)
		return NULL;
	retval = lst_read_from_fp(fp, NULL);
	fclose(fp);

	return retval;
}

/*
 * lst_init_home() - Sets the HOME directory to the location of the temporary 
 * directory defined in LST_TMPDIR. Returns 1 if anything fails, or 0 if OK.
 */

int lst_init_home(void)
{
	char *p = NULL;
	int retval = 1;

	if (OK_SUCCESS(mkdir(LST_TMPDIR, 0777), "mkdir %s", LST_TMPDIR)) {
		failed_ok("mkdir()"); /* gncov */
		return 1; /* gncov */
	}
	OK_NOTNULL(p = realpath(LST_TMPDIR, NULL),
	           "Convert LST_TMPDIR to absolute path");
	OK_SUCCESS(rmdir(LST_TMPDIR), "rmdir %s", LST_TMPDIR);
	if (!p) {
		failed_ok("realpath()"); /* gncov */
		return 1; /* gncov */
	}
	if (OK_SUCCESS(setenv("HOME", p, 1),
	               "Set HOME to the temporary directory")) {
		lst_diag("Cannot set HOME to tempdir: %s", /* gncov */
		         strerror(errno)); /* gncov */
		goto cleanup; /* gncov */
	}
	retval = 0;

cleanup:
	free(p);

	return retval;
}

/*
 * lst_streams_init() - Initializes a `struct lst_streams` struct. Returns 
 * nothing.
 */

void lst_streams_init(struct lst_streams *dest)
{
	assert(dest);

	lst_binbuf_init(&dest->in);
	lst_binbuf_init(&dest->out);
	lst_binbuf_init(&dest->err);
	dest->ret = 0;
}

/*
 * lst_streams_free() - Deallocates and sets a `struct lst_streams` struct to 
 * default values. Returns nothing.
 */

void lst_streams_free(struct lst_streams *dest)
{
	assert(dest);

	lst_binbuf_free(&dest->in);
	lst_binbuf_free(&dest->out);
	lst_binbuf_free(&dest->err);
	lst_streams_init(dest);
}

/*
 * lst_prepare_valgrind_cmd() - Creates a command array for valgrind execution. 
 * Returns a new allocated array that starts with `valgrind_args` followed by 
 * `cmd`. Returns NULL on error. Caller must free the returned array after use.
 */

static char **lst_prepare_valgrind_cmd(char *cmd[]) /* gncov */
{
	static const char *valgrind_args[] = {
		"valgrind",
		"-q",
		"--error-exitcode=1",
		"--leak-check=full",
		"--show-leak-kinds=all",
		"--"
	};
	const size_t argnum = sizeof(valgrind_args) /* gncov */
	                      / sizeof(valgrind_args[0]);
	size_t cmd_len = 0; /* gncov */
	char **valgrind_cmd;

	assert(cmd); /* gncov */
	while (cmd[cmd_len]) /* gncov */
		cmd_len++; /* gncov */
	valgrind_cmd = malloc((cmd_len + argnum + 1) /* gncov */
	                      * sizeof(char *));
	if (!valgrind_cmd) { /* gncov */
		failed_ok("malloc()"); /* gncov */
		return NULL; /* gncov */
	}

	memcpy(valgrind_cmd, valgrind_args, /* gncov */
	       argnum * sizeof(char *));
	memcpy(valgrind_cmd + argnum, cmd, /* gncov */
	       (cmd_len + 1) * sizeof(char *)); /* gncov */

	return valgrind_cmd; /* gncov */
}

/*
 * write_stdin_to_child() - Writes `len` bytes from `buf` to `fp` and uses a 
 * loop to make sure the whole input is received by the child process. Returns 
 * 0 if successful, or 1 if write() fails.
 */

static int write_stdin_to_child(const int fd, const char *buf, /* gncov */
                                const size_t len)
{
	size_t total_written = 0; /* gncov */

	assert(buf); /* gncov */

	while (total_written < len) { /* gncov */
		ssize_t written = write(fd, buf + total_written, /* gncov */
		                        len - total_written);

		if (written == -1) { /* gncov */
			if (errno == EINTR) /* gncov */
				continue; /* gncov */
			failed_ok("write() to stdin pipe"); /* gncov */
			return 1; /* gncov */
		}
		total_written += (size_t)written; /* gncov */
	}

	return 0; /* gncov */
}

/*
 * lst_streams_exec() - Executes a command and stores stdout, stderr and the 
 * return value into `dest`. `cmd` is an array of arguments, and the last 
 * element must be NULL. If `lst_use_valgrind` is `true`, the command is 
 * executed with Valgrind. Returns the exit value from the process.
 */

int lst_streams_exec(struct lst_streams *dest, char *cmd[])
{
	int retval = 1;
	int infd[2] = { -1, -1 };
	int outfd[2] = { -1, -1 };
	int errfd[2] = { -1, -1 };
	pid_t pid;
	FILE *outfp = NULL, *errfp = NULL;
	struct sigaction old_action, new_action;

	assert(dest);
	assert(cmd);

	if (pipe(infd) == -1) {
		failed_ok("Creation of stdin pipe"); /* gncov */
		goto cleanup; /* gncov */
	}
	if (pipe(outfd) == -1) {
		failed_ok("Creation of stdout pipe"); /* gncov */
		goto cleanup; /* gncov */
	}
	if (pipe(errfd) == -1) {
		failed_ok("Creation of stderr pipe"); /* gncov */
		goto cleanup; /* gncov */
	}

	if ((pid = fork()) == -1) {
		failed_ok("fork()"); /* gncov */
		goto cleanup; /* gncov */
	}

	if (!pid) {
		/* Child */
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		if (dup2(infd[0], STDIN_FILENO) == -1
		    || dup2(outfd[1], STDOUT_FILENO) == -1
		    || dup2(errfd[1], STDERR_FILENO) == -1) {
			failed_ok("dup2()"); /* gncov */
			_exit(EXIT_FAILURE); /* gncov */
		}

		close(infd[0]);
		close(infd[1]);
		close(outfd[0]);
		close(outfd[1]);
		close(errfd[0]);
		close(errfd[1]);

		if (lst_use_valgrind) { /* gncov */
			char **valgrind_cmd
			= lst_prepare_valgrind_cmd(cmd); /* gncov */
			execvp(valgrind_cmd[0], valgrind_cmd); /* gncov */
			free(valgrind_cmd); /* gncov */
		} else {
			execvp(cmd[0], cmd); /* gncov */
		}

		failed_ok("execvp()"); /* gncov */
		_exit(EXIT_FAILURE); /* gncov */
	}

	/* Parent */
	close(infd[0]);
	close(outfd[1]);
	close(errfd[1]);

	if (!dest) {
		wait(&retval); /* gncov */
		goto cleanup; /* gncov */
	}

	if (!(outfp = fdopen(outfd[0], "r"))
	    || !(errfp = fdopen(errfd[0], "r"))) {
		failed_ok("fdopen()"); /* gncov */
		goto cleanup; /* gncov */
	}

	/* Write to stdin using direct write() call and close immediately */
	if (dest->in.buf && dest->in.len) {
		if (write_stdin_to_child(infd[1], dest->in.buf, /* gncov */
		                         dest->in.len)) {
			goto cleanup; /* gncov */
		}
	}
	close(infd[1]);
	infd[1] = -1;

	/* Now read from stdout and stderr */
	lst_read_from_fp(errfp, &dest->err);
	lst_read_from_fp(outfp, &dest->out);

	wait(&dest->ret);
	dest->ret = dest->ret >> 8;
	retval = dest->ret;

cleanup:
	/* Protect against SIGPIPE when closing streams */
	new_action.sa_handler = SIG_IGN;
	sigemptyset(&new_action.sa_mask);
	new_action.sa_flags = 0;
	if (sigaction(SIGPIPE, &new_action, &old_action) == -1)
		failed_ok("Cannot set SIGPIPE handler," /* gncov */
		          " sigaction()");

	if (errfp)
		fclose(errfp);
	if (outfp)
		fclose(outfp);
	if (infd[1] != -1)
		close(infd[1]); /* gncov */

	/* Restore original signal handling */
	if (sigaction(SIGPIPE, &old_action, NULL) == -1)
		failed_ok("Restoration of SIGPIPE handler"); /* gncov */

	return retval;
}

/*
 * lst_is_root() - Returns 1 if the current user is root, otherwise it returns 
 * 0.
 */

int lst_is_root(void)
{
	return !getuid();
}

/*
 * lst_allocstr_va() - Returns pointer to allocated string generated by 
 * providing printf()-like format and va_list. Returns NULL on error.
 */

char *lst_allocstr_va(const char *format, va_list ap)
{
	char *p;
	int needed;
	va_list ap_copy;
	size_t size;

	assert(format);
	if (!format)
		return NULL; /* gncov */

	va_copy(ap_copy, ap);
	needed = vsnprintf(NULL, 0, format, ap_copy);
	va_end(ap_copy);

	if (needed < 0)
		return NULL; /* gncov */

	size = (size_t)needed + 1;
	p = malloc(size);
	if (!p)
		return NULL; /* gncov */

	needed = vsnprintf(p, size, format, ap);
	if (needed < 0) {
		free(p); /* gncov */
		return NULL; /* gncov */
	}

	return p;
}

/*
 * lst_allocstr() - Returns pointer to an allocated string generated by 
 * providing printf()-like arguments. Returns NULL on error.
 */

char *lst_allocstr(const char *format, ...)
{
	va_list ap;
	char *retval;

	assert(format);
	if (!format)
		return NULL; /* gncov */

	va_start(ap, format);
	retval = lst_allocstr_va(format, ap);
	va_end(ap);

	return retval;
}

/*
 * lst_count_substr() - Returns the number of non-overlapping occurrences of 
 * substring `substr` in string `s`.
 */

size_t lst_count_substr(const char *s, const char *substr)
{
	size_t count = 0, len_ss;
	const char *p = s;

	if (!s || !substr || !*s || !*substr)
		return 0;

	len_ss = strlen(substr);
	while ((p = strstr(p, substr)) != NULL) {
		count++;
		p += len_ss;
	}

	return count;
}

/*
 * lst_str_replace() - Returns an allocated string where all substrings `s1` 
 * are replaced with the string `s2` in the string `s`.
 *
 * Returns NULL if any errors occur or if `s`, `s1` or `s2` are NULL.
 *
 * If `s1` is empty, an allocated copy of `s` is returned.
 */

char *lst_str_replace(const char *s, const char *s1, const char *s2)
{
	size_t len_s1, len_s2, count, new_len;
	const char *tmp, *src;
	char *buf, *dest;

	if (!s || !s1 || !s2)
		return NULL;

	len_s1 = strlen(s1);
	len_s2 = strlen(s2);
	count = lst_count_substr(s, s1);

	if (!*s || !*s1 || !count)
		return strdup(s);

	new_len = strlen(s) + count * (len_s2 - len_s1);
	buf = malloc(new_len + 1);
	if (!buf)
		return NULL; /* gncov */

	dest = buf;
	src = s;
	while ((tmp = strstr(src, s1)) != NULL) {
		size_t len_before = (size_t)(tmp - src);

		memcpy(dest, src, len_before);
		dest += len_before;

		memcpy(dest, s2, len_s2);
		dest += len_s2;

		src = tmp + len_s1;
	}
	strcpy(dest, src);

	return buf;
}

/*
 * lst_re_check() - Tests that the regular expression in `pattern` matches the 
 * input string `s`. Returns 0 if it matches, or 1 if it doesn't. If regcomp() 
 * fails, it prints an error message and returns -1.
 */

int lst_re_check(const char *pattern, const char *s)
{
	regex_t regexp;
	int result;

	assert(pattern);
	assert(*pattern);
	assert(s);

	result = regcomp(&regexp, pattern, REG_EXTENDED);
	if (result) {
		char errbuf[1024];

		regerror(result, &regexp, errbuf, sizeof(errbuf));
		fprintf(stderr, "%s(): regcomp() failed: %s",
		                __func__, errbuf);
		return -1;
	}
	result = regexec(&regexp, s, 0, NULL, 0);
	regfree(&regexp);

	return !!result;
}

/******************************************************************************
                             --selftest functions
******************************************************************************/

/*
 * lst_bail_out() - Aborts the test suite with an optional message written to 
 * stdout. To abort without a message, use NULL in `msg`. Returns nothing.
 */

void lst_bail_out(const char *msg, ...) /* gncov */
{
	va_list ap;

	fputs("Bail out!", stdout); /* gncov */
	if (msg) { /* gncov */
		fputs("  ", stdout); /* gncov */
		va_start(ap, msg); /* gncov */
		vprintf(msg, ap); /* gncov */
		va_end(ap); /* gncov */
	}
	fputc('\n', stdout); /* gncov */

	exit(EXIT_FAILURE); /* gncov */
}

/*
 * lst_ok_va() - Prints a log line to stdout. If `i` is 0, an "ok" line is 
 * printed, otherwise a "not ok" line is printed. `desc` is the test 
 * description and can use printf sequences.
 *
 * Returns 0 if `i` is 0, otherwise it returns 1.
 */

int lst_ok_va(const int i, const char *file, const int linenum,
              const char *desc, va_list ap)
{
	va_list ap_copy;
	char *s, *s2;

	assert(file);
	assert(*file);
	assert(desc);
	assert(*desc);

	if (!desc)
		lst_bail_out("%s(): desc is NULL", __func__); /* gncov */

	printf("%sok %d - %s:%d: ",
	       (i ? "not " : ""), ++lst_testnum, file, linenum);
	va_copy(ap_copy, ap);
	s = lst_allocstr_va(desc, ap_copy);
	va_end(ap_copy);
	if (!s)
		lst_bail_out("lst_allocstr_va() failed: %s", /* gncov */
		             strerror(errno)); /* gncov */
	s2 = lst_str_replace(s, "\n", "\\n");
	if (!s2) {
		free(s); /* gncov */
		lst_bail_out("lst_str_replace() failed: %s", /* gncov */
		             strerror(errno)); /* gncov */
	}
	puts(s2);
	free(s2);
	free(s);
	failcount += !!i;

	return !!i;
}

/*
 * lst_ok() - Frontend against lst_ok_va(). Refer to the description for that 
 * function for more info. Returns 0 if `i` is 0, otherwise it returns 1.
 */

int lst_ok(const int i, const char *file, const int linenum, const char *desc,
           ...)
{
	va_list ap;

	assert(file);
	assert(*file);
	assert(desc);
	assert(*desc);

	if (!desc)
		lst_bail_out("%s(): desc is NULL", __func__); /* gncov */

	va_start(ap, desc);
	lst_ok_va(i, file, linenum, desc, ap);
	va_end(ap);

	return !!i;
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

	assert(format);
	if (!format)
		return NULL; /* gncov */

	buffer = lst_allocstr_va(format, ap);
	if (!buffer) {
		failed_ok("lst_allocstr_va()"); /* gncov */
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
 * Returns the value from diag_output_va() (a pointer to the allocated string), 
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
 * lst_diag() - Prints a diagnostic message prefixed with "# " to stdout. 
 * `printf` sequences can be used. All `\n` characters are converted to "\n# ".
 *
 * A terminating `\n` is automatically added to the string. Returns 0 if 
 * successful, or 1 if `format` is NULL or diag_output_va() failed.
 */

int lst_diag(const char *format, ...)
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
	printf("%s\n", converted_buffer);
	free(converted_buffer);

	return 0;
}

/*
 * gotexp_output() - Generates the output used by print_gotexp(). Returns NULL 
 * if lst_allocstr() or strdup() fails. Otherwise, it returns a pointer to an 
 * allocated string with the output.
 */

static char *gotexp_output(const char *got, const char *exp)
{
	char *s;

	if (got == exp || (got && exp && !strcmp(got, exp))) {
		s = strdup("");
		if (!s)
			failed_ok("strdup()"); /* gncov */
	} else {
		s = lst_allocstr("         got: '%s'\n"
		                 "    expected: '%s'",
		                 no_null(got), no_null(exp));
		if (!s)
			failed_ok("lst_allocstr()"); /* gncov */
	}

	return s;
}

/*
 * print_gotexp() - Prints the value of the actual and expected data if the 
 * strings `got` and `exp` are different. Used when a test fails. Returns 1 if 
 * gotexp_output() or lst_diag() failed, otherwise 0.
 */

int print_gotexp(const char *got, const char *exp)
{
	char *s;
	int r = 0;

	s = gotexp_output(got, exp);
	if (!s)
		return 1; /* gncov */
	if (*s)
		r += lst_diag(s); /* gncov */
	free(s);

	return !!r;
}

/*
 * valgrind_marker_at() - Returns 1 if `p` starts with a Valgrind marker 
 * ("==DIGITS=="), otherwise 0.
 */

static int valgrind_marker_at(const char *p)
{
	assert(p);

	if (strncmp(p, "==", 2))
		return 0;
	p += 2;
	if (!*p || !isdigit((unsigned char)*p))
		return 0;
	while (isdigit((unsigned char)*p))
		p++;
	if (!*p)
		return 0;

	return !strncmp(p, "==", 2);
}

/*
 * valgrind_lines() - Searches for Valgrind markers ("==DIGITS==") in `s`, used 
 * by test_command(). Markers are recognised at the start of the buffer or 
 * after a newline, matching real Valgrind `-q` output. If a marker is found or 
 * `s` is NULL, it returns 1. Otherwise, it returns 0.
 */

static int valgrind_lines(const char *s)
{
	const char *p = s;

	if (!s)
		return OK_ERROR("%s(): s == NULL", __func__); /* gncov */

	if (valgrind_marker_at(p))
		return 1;

	while ((p = strstr(p, "\n==")) != NULL) {
		p++; /* Point at the first '=' of "==..." */
		if (valgrind_marker_at(p))
			return 1;
		p++; /* Advance so the next search can progress */
	}

	return 0;
}

/*
 * tc_cmp() - Comparison function used by test_command(). There are 3 types of 
 * verification, set via the `type` parameter:
 *
 * - 0: substring search, `exp` must exist within `got`
 * - 1: the whole output must be identical to the expected value
 * - 2: regexp search, `got` must match the pattern `exp`
 *
 * `got` is the actual output from the program, and `exp` is the expected 
 * output, substring, or regexp pattern.
 *
 * If `exp` is empty, the output in `got` must also be empty for the test to 
 * succeed, for every comparison type.
 *
 * Returns 0 if the string was found, otherwise 1.
 */

static int tc_cmp(const int type, const char *got, const char *exp)
{
	assert(got);
	assert(exp);
	if (!got || !exp)
		return 1; /* gncov */

	if (type == 1 || !*exp)
		return !!strcmp(got, exp);
	if (type == 2)
		return !!lst_re_check(exp, got);

	return !strstr(got, exp);
}

/*
 * test_command() - Runs the executable with arguments in `cmd` and verifies 
 * stdout, stderr and the return value against `exp_stdout`, `exp_stderr` and 
 * `exp_retval`. Returns nothing.
 */

static void test_command(const char *file, const int linenum, const char type,
                         char *cmd[], const char *exp_stdout,
                         const char *exp_stderr, const int exp_retval,
                         const char *desc, va_list ap)
{
	struct lst_streams ss;
	char *e_stdout, *e_stderr, *descbuf;

	assert(file);
	assert(*file);
	assert(cmd);
	assert(*cmd);
	assert(desc);
	assert(*desc);
	if (!cmd) {
		OK_ERROR_L(file, linenum, "%s(): cmd is NULL", /* gncov */
		           __func__);
		return; /* gncov */
	}

	e_stdout = lst_str_replace(exp_stdout, EXECSTR, lst_execname);
	e_stderr = lst_str_replace(exp_stderr, EXECSTR, lst_execname);
	descbuf = lst_allocstr_va(desc, ap);
	if (!descbuf) {
		failed_ok("lst_allocstr_va()"); /* gncov */
		return; /* gncov */
	}
	lst_streams_init(&ss);
	lst_streams_exec(&ss, cmd);
	if (e_stdout) {
		OK_SUCCESS_L(tc_cmp(type, ss.out.buf, e_stdout), file, linenum,
		             "%s (stdout)", descbuf);
		if (tc_cmp(type, ss.out.buf, e_stdout))
			print_gotexp(ss.out.buf, e_stdout); /* gncov */
	}
	if (e_stderr) {
		OK_SUCCESS_L(tc_cmp(type, ss.err.buf, e_stderr), file, linenum,
		             "%s (stderr)", descbuf);
		if (tc_cmp(type, ss.err.buf, e_stderr))
			print_gotexp(ss.err.buf, e_stderr); /* gncov */
	}
	OK_EQUAL_L(ss.ret, exp_retval, file, linenum, "%s (retval)", descbuf);
	print_gotexp_int(ss.ret, exp_retval);
	free(descbuf);
	free(e_stderr);
	free(e_stdout);
	if (valgrind_lines(ss.err.buf))
		OK_ERROR_L(file, linenum, "Found valgrind output"); /* gncov */
	lst_streams_free(&ss);
}

/*
 * sc_func() - Executes command `cmd` and verifies that stdout, stderr and the 
 * return value corresponds to the expected values. The `exp_*` variables are 
 * substrings that must occur in the actual output. Not meant to be called 
 * directly, but via the sc() macro that logs the line number automatically. 
 * Returns nothing.
 */

void sc_func(const char *file, const int linenum, char *cmd[],
             const char *exp_stdout, const char *exp_stderr,
             const int exp_retval, const char *desc, ...)
{
	va_list ap;

	assert(file);
	assert(*file);
	assert(cmd);
	assert(*cmd);
	assert(desc);
	assert(*desc);

	va_start(ap, desc);
	test_command(file, linenum, 0, cmd, exp_stdout, exp_stderr,
	             exp_retval, desc, ap);
	va_end(ap);
}

/*
 * tc_func() - Executes command `cmd` and verifies that stdout, stderr and the 
 * return value are identical to the expected values. The `exp_*` variables are 
 * strings that must be identical to the actual output. Not meant to be called 
 * directly, but via the tc() macro that logs the line number automatically. 
 * Returns nothing.
 */

void tc_func(const char *file, const int linenum, char *cmd[],
             const char *exp_stdout, const char *exp_stderr,
             const int exp_retval, const char *desc, ...)
{
	va_list ap;

	assert(file);
	assert(*file);
	assert(cmd);
	assert(*cmd);
	assert(desc);
	assert(*desc);

	va_start(ap, desc);
	test_command(file, linenum, 1, cmd, exp_stdout, exp_stderr, exp_retval,
	             desc, ap);
	va_end(ap);
}

/*
 * rc_func() - Executes command `cmd` and verifies that stdout and stderr 
 * matches the regexp patterns in `exp_stdout` and `exp_stderr` and that the 
 * return value is identical to `exp_retval`. Not meant to be called directly, 
 * but via the rc() macro that logs the line number automatically. Returns 
 * nothing.
 */

void rc_func(const char *file, const int linenum, char *cmd[],
             const char *exp_stdout, const char *exp_stderr,
             const int exp_retval, const char *desc, ...)
{
	va_list ap;

	assert(file);
	assert(*file);
	assert(cmd);
	assert(*cmd);
	assert(desc);
	assert(*desc);

	va_start(ap, desc);
	test_command(file, linenum, 2, cmd, exp_stdout, exp_stderr,
	             exp_retval, desc, ap);
	va_end(ap);
}

/*
 * lst_init_output_files() - Redirects stdout and stderr to files in 
 * LST_TMPDIR. Used for testing functions that prints to stderr or stdout. 
 * Returns 0 on success, or 1 on failure.
 */

int lst_init_output_files(void)
{
	int stdout_fd, stderr_fd;

	assert(lst_file_exists(LST_TMPDIR));

	if (orig_stdout_fd != -1 || orig_stderr_fd != -1) {
		/*
		 * Already initialized, perhaps a previous test didn't restore 
		 * properly.
		 */
		OK_ERROR("%s(): Already initialized", __func__); /* gncov */
		return 1; /* gncov */
	}

	stdout_fd = open(stdout_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (stdout_fd == -1) {
		failed_ok("open() for stdout"); /* gncov */
		lst_diag("stdout_file = \"%s\"", stdout_file); /* gncov */
		return 1; /* gncov */
	}

	stderr_fd = open(stderr_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (stderr_fd == -1) {
		failed_ok("open() for stderr"); /* gncov */
		lst_diag("stderr_file = \"%s\"", stderr_file); /* gncov */
		close(stdout_fd); /* gncov */
		return 1; /* gncov */
	}

	orig_stdout_fd = dup(1);
	if (orig_stdout_fd == -1) {
		failed_ok("dup() for stdout"); /* gncov */
		close(stdout_fd); /* gncov */
		close(stderr_fd); /* gncov */
		return 1; /* gncov */
	}

	orig_stderr_fd = dup(2);
	if (orig_stderr_fd == -1) {
		failed_ok("dup() for stderr"); /* gncov */
		close(stdout_fd); /* gncov */
		close(stderr_fd); /* gncov */
		close(orig_stdout_fd); /* gncov */
		return 1; /* gncov */
	}

	fflush(stdout);
	fflush(stderr);

	if (dup2(stdout_fd, 1) == -1) {
		failed_ok("dup2() for stdout"); /* gncov */
		close(stdout_fd); /* gncov */
		close(stderr_fd); /* gncov */
		close(orig_stdout_fd); /* gncov */
		close(orig_stderr_fd); /* gncov */
		orig_stdout_fd = -1; /* gncov */
		orig_stderr_fd = -1; /* gncov */
		return 1; /* gncov */
	}
	close(stdout_fd);

	if (dup2(stderr_fd, 2) == -1) {
		failed_ok("dup2() for stderr"); /* gncov */
		close(stderr_fd); /* gncov */
		/*
		 * Restore stdout before closing the saved descriptor, 
		 * otherwise dup2() uses a closed fd and stdout stays 
		 * redirected to the temporary file.
		 */
		dup2(orig_stdout_fd, 1); /* gncov */
		close(orig_stdout_fd); /* gncov */
		close(orig_stderr_fd); /* gncov */
		orig_stdout_fd = -1; /* gncov */
		orig_stderr_fd = -1; /* gncov */
		return 1; /* gncov */
	}
	close(stderr_fd);

	return 0;
}

/*
 * lst_restore_output_files() - Restores stdout and stderr to their original 
 * file descriptors. Returns nothing, but logs failures.
 */

void lst_restore_output_files(void)
{
	if (orig_stdout_fd == -1 && orig_stderr_fd == -1) {
		/* Not initialized, nothing to restore */
		return; /* gncov */
	}

	fflush(stdout);
	fflush(stderr);

	if (orig_stdout_fd != -1) {
		if (dup2(orig_stdout_fd, 1) == -1) {
			failed_ok("dup2() (restore stdout)"); /* gncov */
		}
		close(orig_stdout_fd);
		orig_stdout_fd = -1;
	}

	if (orig_stderr_fd != -1) {
		if (dup2(orig_stderr_fd, 2) == -1) {
			failed_ok("dup2() (restore stderr)"); /* gncov */
		}
		close(orig_stderr_fd);
		orig_stderr_fd = -1;
	}
}

/*
 * verify_output_files_func() - Verify that the contents of stdout_file and 
 * stderr_file is equal to `exp_stdout` and `exp_stderr`, respectively. To 
 * avoid checking either stdout or stderr, use NULL as the value. Returns 
 * nothing.
 */

void verify_output_files_func(const char *file, const int linenum,
                              const char *desc,
                              const char *exp_stdout, const char *exp_stderr)
{
	char *result;

	assert(file);
	assert(*file);
	assert(desc);
	assert(*desc);

	if (exp_stdout) {
		result = lst_read_from_file(stdout_file);
		if (result) {
			OK_STRCMP_L(result, exp_stdout, file, linenum,
			            "%s (stdout)", desc);
			print_gotexp(result, exp_stdout);
			free(result);
		} else {
			failed_ok("lst_read_from_file" /* gncov */
			          "(stdout_file)");
		}
	}

	if (exp_stderr) {
		result = lst_read_from_file(stderr_file);
		if (result) {
			OK_STRCMP_L(result, exp_stderr, file, linenum,
			            "%s (stderr)", desc);
			print_gotexp(result, exp_stderr);
			free(result);
		} else {
			failed_ok("lst_read_from_file" /* gncov */
			          "(stderr_file)");
		}
	}
}

/*
 * lst_cleanup_tempdir() - Delete all known filesystem entries in the temporary 
 * work directory defined in LST_TMPDIR. Expects `linenum` to be `__LINE__` or 
 * the `linenum` value from a parent function. Returns nothing.
 */

void lst_cleanup_tempdir(const char *file, const int linenum)
{
	assert(file);
	assert(*file);

	if (lst_file_exists(stderr_file))
		OK_SUCCESS_L(remove(stderr_file), file, linenum,
		             "Delete %s", stderr_file);
	if (lst_file_exists(stdout_file))
		OK_SUCCESS_L(remove(stdout_file), file, linenum,
		             "Delete %s", stdout_file);
}

/*
 * selftest_failcount() - Returns the number of failed tests.
 */

int selftest_failcount(void)
{
	return failcount;
}

/*
 * print_selftest_footer() - Prints a TAP status report to stdout if `print` is 
 * 1. If `print` is 0, nothing is printed. Returns the number of executed 
 * tests.
 */

int print_selftest_footer(void)
{
	printf("1..%d\n", lst_testnum);
	if (failcount) {
		lst_diag("Looks like you failed %d test%s of %d.", /* gncov */
		         failcount, (failcount == 1) ? "" : "s", /* gncov */
		         lst_testnum);
	}

	return lst_testnum;
}

/******************************************************************************
                 Function tests, no temporary directory needed
******************************************************************************/

                             /*** selftest.c ***/

/*
 * test_ok_macros() - Tests the OK_*() macros. Returns nothing.
 */

static void test_ok_macros(void)
{
	lst_diag("Test the OK_*() macros");

	OK_EQUAL(3.14, 3.14, "OK_EQUAL(%.2f, %.2f, ...)", 3.14, 3.14);
	/* OK_ERROR("OK_ERROR(...), can't be tested"); */
	OK_FAILURE(1, "OK_FAILURE(1, ...)");
	OK_FALSE(5 == 9, "OK_FALSE(5 == 9, ...)");
	OK_MEMCMP("with\0null", "with\0null", strlen("with null") + 1,
	          "OK_MEMCMP(\"with\\0null\", \"with\\0null\","
	          " strlen(\"with null\") + 1, ...)");
	OK_NOTEQUAL(19716, 1916, "OK_NOTEQUAL(%u, %u, ...)", 19716, 1916);
	OK_NOTNULL(strstr("abcdef", "cde"),
	           "OK_NOTNULL(strstr(\"abcdef\", \"cde\"), ...)");
	OK_NULL(strstr("abcdef", "notfound"),
	               "OK_NULL(strstr(\"abcdef\", \"notfound\"), ...)");
	OK_STRCMP("str", "str",
	          "OK_STRCMP(\"%s\", \"%s\", ...)", "str", "str");
	OK_STRNCMP("abcde", "abcyz", 3,
	           "OK_STRNCMP(\"abcde\", \"abcyz\", 3, ...)");
	OK_SUCCESS(0, "OK_SUCCESS(0, ...)");
	OK_TRUE(9 > -4, "OK_TRUE(%d > %d, ...)", 9, -4);
}

/*
 * test_lst_binbuf_cpy() - Tests the test_lst_binbuf_cpy() function. Returns 
 * nothing.
 */

static void test_lst_binbuf_cpy(void)
{
	struct lst_binbuf src, dest;
	const char *txt;
	char *res;

	lst_diag("Test lst_binbuf_cpy()");

	lst_diag("Store `txt` in `src` and duplicate it to `dest`");
	lst_binbuf_init(&src);
	lst_binbuf_init(&dest);
	txt = "The best test ever.\n";
	src.buf = strdup(txt);
	if (!src.buf) {
		failed_ok("strdup()"); /* gncov */
		return; /* gncov */
	}
	src.len = strlen(txt);
	src.alloc = src.len + 1;
	res = lst_binbuf_cpy(&dest, &src);
	OK_NOTNULL(res, "lst_binbuf_cpy(&dest, &src) doesn't return NULL");
	OK_STRCMP(dest.buf, src.buf, "dest.buf is identical to src.buf");
	OK_EQUAL(dest.len, src.len, "dest.len and src.len are identical");
	OK_EQUAL(dest.alloc, src.alloc,
	         "dest.alloc and src.alloc are identical");
	lst_binbuf_free(&dest);
	lst_binbuf_free(&src);

	lst_diag("`src.buf` is empty");
	lst_binbuf_init(&src);
	lst_binbuf_init(&dest);
	src.buf = strdup("");
	if (!src.buf) {
		failed_ok("strdup()"); /* gncov */
		return; /* gncov */
	}
	src.len = 0;
	src.alloc = src.len + 1;
	res = lst_binbuf_cpy(&dest, &src);
	OK_NOTNULL(res, "lst_binbuf_cpy() with empty string");
	OK_STRCMP(dest.buf, "", "dest.buf is empty");
	OK_EQUAL(dest.len, 0, "dest.len is 0");
	print_gotexp_size_t(dest.len, 0);
	OK_EQUAL(dest.alloc, 1, "dest.alloc is 1");
	print_gotexp_size_t(dest.alloc, 1);
	lst_binbuf_free(&dest);
	lst_binbuf_free(&src);

	lst_diag("`src` has null values");
	lst_binbuf_init(&src);
	lst_binbuf_init(&dest);
	res = lst_binbuf_cpy(&dest, &src);
	OK_NULL(res, "lst_binbuf_cpy() with null values in src");
	OK_NULL(dest.buf, "dest.buf is NULL");
	OK_EQUAL(dest.len, 0, "dest.len is 0");
	print_gotexp_size_t(dest.len, 0);
	OK_EQUAL(dest.alloc, 0, "dest.alloc is 0");
	print_gotexp_size_t(dest.alloc, 0);
	lst_binbuf_free(&dest);
	lst_binbuf_free(&src);

	lst_diag("`src.len` is larger than `src.alloc`");
	lst_binbuf_init(&src);
	lst_binbuf_init(&dest);
	src.len = 2;
	src.alloc = 1;
	res = lst_binbuf_cpy(&dest, &src);
	OK_NULL(res, "lst_binbuf_cpy() when src.len is larger than src.alloc");
	OK_NULL(dest.buf, "dest.buf is NULL");
	OK_EQUAL(dest.len, 0, "dest.len is 0");
	print_gotexp_size_t(dest.len, 0);
	OK_EQUAL(dest.alloc, 0, "dest.alloc is 0");
	print_gotexp_size_t(dest.alloc, 0);
	lst_binbuf_free(&dest);
	lst_binbuf_free(&src);

	lst_diag("String in `src.buf` is not null-terminated");
	lst_binbuf_init(&src);
	lst_binbuf_init(&dest);
	txt = "Indeed";
	src.len = strlen(txt);
	src.buf = malloc(src.len);
	if (!src.buf) {
		failed_ok("malloc()"); /* gncov */
		return; /* gncov */
	}
	src.alloc = src.len;
	memcpy(src.buf, txt, src.len);
	res = lst_binbuf_cpy(&dest, &src);
	OK_NOTNULL(res, "lst_binbuf_cpy(), src.buf is not null-terminated");
	OK_NOTNULL(dest.buf, "dest.buf is not NULL");
	OK_EQUAL(dest.len, src.len, "dest.len is equal to src.len");
	print_gotexp_size_t(dest.len, src.len);
	OK_EQUAL(dest.alloc, src.len, "dest.alloc is equal to src.len");
	print_gotexp_size_t(dest.alloc, src.len);
	OK_MEMCMP(dest.buf, src.buf, src.len,
	          "dest.buf and src.buf are identical");
	lst_binbuf_free(&dest);
	lst_binbuf_free(&src);

	lst_diag("Non-ASCIIZ string in src.buf contains nullbyte");
	lst_binbuf_init(&src);
	lst_binbuf_init(&dest);
	txt = "Has nullbyte.";
	src.len = strlen(txt);
	src.buf = malloc(src.len);
	if (!src.buf) {
		failed_ok("malloc()"); /* gncov */
		return; /* gncov */
	}
	src.alloc = src.len;
	memcpy(src.buf, txt, src.len);
	src.buf[3] = '\0';
	res = lst_binbuf_cpy(&dest, &src);
	OK_NOTNULL(res, "lst_binbuf_cpy(), src.buf contains nullbyte");
	OK_NOTNULL(dest.buf, "dest.buf is not NULL");
	OK_EQUAL(dest.len, src.len, "dest.len is equal to src.len");
	print_gotexp_size_t(dest.len, src.len);
	OK_EQUAL(dest.alloc, src.len, "dest.alloc is equal to src.len");
	print_gotexp_size_t(dest.alloc, src.len);
	OK_MEMCMP(dest.buf, src.buf, src.len,
	          "dest.buf and src.buf are identical");
	lst_binbuf_free(&dest);
	lst_binbuf_free(&src);
}

/*
 * test_lst_allocstr() - Tests the lst_allocstr() function. Returns nothing.
 */

static void test_lst_allocstr(void)
{
	const size_t bufsize = BUFSIZ * 2 + 1;
	char *p, *p2, *p3;
	size_t alen;

	lst_diag("Test lst_allocstr()");
	p = malloc(bufsize);
	if (!p) {
		failed_ok("malloc()"); /* gncov */
		return; /* gncov */
	}
	memset(p, 'a', bufsize - 1);
	p[bufsize - 1] = '\0';
	p2 = lst_allocstr("%s", p);
	if (!p2) {
		failed_ok("lst_allocstr() with BUFSIZ * 2"); /* gncov */
		goto cleanup; /* gncov */
	}
	alen = strlen(p2);
	OK_EQUAL(alen, BUFSIZ * 2, "lst_allocstr(): strlen is correct");
	p3 = p2;
	while (*p3) {
		if (*p3 != 'a') {
			p3 = NULL; /* gncov */
			break; /* gncov */
		}
		p3++;
	}
	OK_NOTNULL(p3, "lst_allocstr(): Content of string is correct");

cleanup:
	free(p2);
	free(p);
}

/*
 * lst_chk_cs() - Used by test_lst_count_substr(). Verifies that the number of 
 * non-overlapping substrings `substr` inside string `s` is `count`. `desc` is 
 * the test description. Returns nothing.
 */

static void lst_chk_cs(const char *file, const int linenum, const char *s,
                       const char *substr, const size_t count,
                       const char *desc)
{
	size_t result;

	assert(file);
	assert(*file);
	assert(desc);
	assert(*desc);

	result = lst_count_substr(s, substr);
	OK_EQUAL_L(result, count, file, linenum,
	           "lst_count_substr(): %s", desc);
	print_gotexp_size_t(result, count);
}

/*
 * test_lst_count_substr() - Tests the lst_count_substr() function. Returns 
 * nothing.
 */

static void test_lst_count_substr(void)
{
	char *s;
	size_t bsize = 10000;

	lst_diag("Test lst_count_substr()");

#define lst_chk_cs(s, substr, count, desc)  \
        lst_chk_cs(__FILE__, __LINE__, (s), (substr), (count), (desc))

	lst_chk_cs("", "", 0, "s and substr are empty");
	lst_chk_cs("", "a", 0, "s is empty");
	lst_chk_cs("aaa", "", 0, "substr is empty");

	lst_chk_cs("", NULL, 0, "substr is NULL");
	lst_chk_cs(NULL, "abcdef", 0, "s is NULL");
	lst_chk_cs(NULL, NULL, 0, "s and substr is NULL");

	lst_chk_cs("Abc", "abc", 0, "Case sensitivity");
	lst_chk_cs("a", "aa", 0, "substr is longer than s");
	lst_chk_cs("aaa", "a", 3, "3 \"a\" in \"aaa\"");
	lst_chk_cs("aaa", "aa", 1, "Non-overlapping \"aa\" in \"aaa\"");
	lst_chk_cs("aaabaaa", "aaa", 2,
	           "Non-overlapping \"aaa\" split by \"b\"");
	lst_chk_cs("abababab", "ab", 4, "4 \"ab\" in s");
	lst_chk_cs("abc", "b", 1, "Single character substring");
	lst_chk_cs("abc", "d", 0, "Substring not found");
	lst_chk_cs("abcdeabc", "abc", 2, "Substring at start and end");
	lst_chk_cs("abcdef" "abcdef" "abcdef", "abc", 3, "3 \"abc\" in s");
	lst_chk_cs("abcdef", "abcdef", 1, "s and substr are identical");
	lst_chk_cs("zzzGHJ\nabc\nGHJ\nabcGHJ", "GHJ", 3, "s with newlines");
	lst_chk_cs("Ḡṹṛḡḷḗ", "ḡ", 1, "UTF-8, U+1Exx area");

	s = malloc(bsize + 1);
	if (!s) {
		failed_ok("malloc()"); /* gncov */
		return; /* gncov */
	}
	memset(s, '!', bsize);
	s[bsize] = '\0';
	lst_chk_cs(s, "!!!!!!!!!!", bsize / 10, "Large buffer");
	free(s);
#undef lst_chk_cs
}

/*
 * lst_chk_sr() - Used by test_lst_str_replace(). Verifies that all 
 * non-overlapping occurrences of substring `s1` are replaced with the string 
 * `s2` in the string `s`, resulting in the string `exp`. Returns nothing.
 */

static void lst_chk_sr(const char *file, const int linenum, const char *s,
                       const char *s1, const char *s2, const char *exp,
                       const char *desc)
{
	char *result;

	assert(file);
	assert(*file);
	assert(desc);
	assert(*desc);

	result = lst_str_replace(s, s1, s2);
	if (!result || !exp)
		OK_EQUAL_L(result, exp, file, linenum,
		           "lst_str_replace(): %s", desc);
	else
		OK_STRCMP_L(result, exp, file, linenum,
		            "lst_str_replace(): %s", desc);
	print_gotexp(result, exp);
	free(result);
}

/*
 * test_lst_str_replace() - Tests the lst_str_replace() function. Returns 
 * nothing.
 */

static void test_lst_str_replace(void)
{
	char *s;
	size_t bsize = 10000;

	lst_diag("Test lst_str_replace()");

#define lst_chk_sr(s, s1, s2, exp, desc)  \
        lst_chk_sr(__FILE__, __LINE__, (s), (s1), (s2), (exp), (desc))

	lst_chk_sr("", "", "", "", "s, s1, and s2 are empty");
	lst_chk_sr("abc", "", "b", "abc", "s1 is empty");
	lst_chk_sr("", "a", "b", "", "s is empty");
	lst_chk_sr("", "a", "", "", "s and s2 is empty");

	lst_chk_sr(NULL, "a", "b", NULL, "s is NULL");
	lst_chk_sr("abc", NULL, "b", NULL, "s1 is NULL");
	lst_chk_sr("abc", "a", NULL, NULL, "s2 is NULL");
	lst_chk_sr(NULL, NULL, NULL, NULL, "s, s1, and s2 is NULL");

	lst_chk_sr("test", "test", "test", "test",
	           "s, s1, and s2 are identical");
	lst_chk_sr("abc", "b", "DEF", "aDEFc", "abc, replace b with DEF");
	lst_chk_sr("abcabcabc", "b", "DEF", "aDEFcaDEFcaDEFc",
	           "abcabcabc, replace all b with DEF");
	lst_chk_sr("abcdefgh", "defg", "", "abch",
	           "Replace defg with nothing");
	lst_chk_sr("abcde", "bcd", "X", "aXe", "Replace bcd with X");
	lst_chk_sr("abc", "d", "X", "abc", "d not in abc");
	lst_chk_sr("ababab", "aba", "X", "Xbab", "Replace aba in ababab");
	lst_chk_sr("abc", "b", "", "ac", "Replace b with nothing");
	lst_chk_sr("abc", "", "X", "abc", "Replace empty with X");
	lst_chk_sr("Ḡṹṛḡḷḗ", "ḡ", "X", "ḠṹṛXḷḗ",
	           "Replace UTF-8 character with X");

	s = malloc(bsize + 1);
	if (!s) {
		failed_ok("malloc()"); /* gncov */
		return; /* gncov */
	}
	memset(s, '!', bsize);
	s[bsize] = '\0';
	lst_chk_sr(s, "!!!!!!!!!!", "", "",
	           "Replace all text in large buffer");
	free(s);

	s = malloc(bsize + 1);
	if (!s) {
		failed_ok("malloc()"); /* gncov */
		return; /* gncov */
	}
	memset(s, '!', bsize);
	s[1234] = 'y';
	s[bsize - 1] = 'z';
	s[bsize] = '\0';
	lst_chk_sr(s, "!!!!!!!!!!", "", "!!!!y!!!!z",
	           "Large buffer with y and z");
	free(s);
#undef lst_chk_sr
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
	OK_NOTNULL(outp, "diag_big: diag_output() returns ok");
	OK_EQUAL(strlen(outp), size + 2, "diag_big: String length is correct");
	print_gotexp_size_t(strlen(outp), size + 2);
	OK_STRNCMP(outp, "# aaabcaaa", 10, "diag_big: Beginning is ok");
	free(outp);
	free(p);
}

/*
 * test_diag() - Tests the diag_output() function. lst_diag() can't be tested 
 * directly because it would pollute the the test output. Returns nothing.
 */

static void test_diag(void)
{
	char *p, *s;
	const char *desc;

	lst_diag("Test lst_diag()");

	OK_EQUAL(lst_diag(NULL), 1, "lst_diag(NULL)");
	OK_NULL(diag_output(NULL), "diag_output() receives NULL");

	p = diag_output("Text with\nnewline");
	OK_NOTNULL(p, "diag_output() with newline didn't return NULL");
	s = "# Text with\n# newline";
	desc = "diag_output() with newline, output is ok";
	if (p)
		OK_STRCMP(p, s, desc);
	else
		OK_ERROR(desc); /* gncov */
	print_gotexp(p, s);
	free(p);

	p = diag_output("\n\n\n\n\n\n\n\n\n\n");
	OK_NOTNULL(p, "diag_output() with only newlines didn't return NULL");
	s = "# \n# \n# \n# \n# \n# \n# \n# \n# \n# \n# ";
	desc = "diag_output() with only newlines";
	if (p)
		OK_STRCMP(p, s, desc);
	else
		OK_ERROR(desc); /* gncov */
	print_gotexp(p, s);
	free(p);

	p = diag_output("%d = %s, %d = %s, %d = %s",
	                1, "one", 2, "two", 3, "three");
	OK_NOTNULL(p, "diag_output() with %%d and %%s didn't return NULL");
	s = "# 1 = one, 2 = two, 3 = three";
	desc = "diag_output() with %%d and %%s";
	if (p)
		OK_STRCMP(p, s, desc);
	else
		OK_ERROR(desc); /* gncov */
	print_gotexp(p, s);
	free(p);

	test_diag_big();
}

/*
 * lst_chk_go() - Used by test_gotexp_output(). Verifies that 
 * `gotexp_output(got, exp)` returns the correct output with the correct 
 * `exp_got` and `exp_exp` values, or an empty string if the strings are 
 * identical. Returns nothing.
 */

static void lst_chk_go(const char *file, const int linenum,
                       const char *got, const char *exp,
                       const char *exp_got, const char *exp_exp)
{
	char *s, *exp_str;

	assert(file);
	assert(*file);
	assert(exp_got);
	assert(exp_exp);

	s = gotexp_output(got, exp);
	if (!s) {
		failed_ok("gotexp_output()"); /* gncov */
		return; /* gncov */
	}
	if (exp_got == exp_exp
	    || (exp_got && exp_exp && !strcmp(exp_got, exp_exp))) {
		exp_str = strdup("");
	} else {
		exp_str = lst_allocstr("         got: '%s'\n"
		                       "    expected: '%s'", exp_got, exp_exp);
	}
	if (!exp_str) {
		failed_ok("strdup() or lst_allocstr()"); /* gncov */
		free(s); /* gncov */
		return; /* gncov */
	}
	OK_STRCMP_L(s, exp_str, file, linenum, "gotexp_output(\"%s\", \"%s\")",
	            no_null(got), no_null(exp));
	if (strcmp(s, exp_str))
		lst_diag("Got:\n%s\nExpected:\n%s", s, exp_str); /* gncov */
	free(exp_str);
	free(s);
}

/*
 * test_gotexp_output() - Tests the gotexp_output() function. print_gotexp() 
 * can't be tested directly because it would pollute stderr. Returns nothing.
 */

static void test_gotexp_output(void)
{
	lst_diag("Test gotexp_output()");

#define lst_chk_go(got, exp, exp_got, exp_exp)  \
        lst_chk_go(__FILE__, __LINE__, (got), (exp), (exp_got), (exp_exp))

	lst_chk_go("", "", "", "");
	lst_chk_go("a", "a", "", "");
	lst_chk_go("a", "b", "a", "b");
	lst_chk_go("got this", "expected this", "got this", "expected this");
	lst_chk_go("with\nnewline", "also with\nnewline",
	           "with\nnewline", "also with\nnewline");

	lst_chk_go("a", NULL, "a", "(null)");
	lst_chk_go(NULL, "b", "(null)", "b");
	lst_chk_go(NULL, NULL, "", "");
#undef lst_chk_go
}

/*
 * test_valgrind_lines() - Tests the behavior of valgrind_lines(). Returns 
 * nothing.
 */

static void test_valgrind_lines(void)
{
	int i;
	const char
	*has[] = {
		"==12345== Invalid read\n",
		"==123==",
		"\n==123==",
		"\n==154363456657465745674567456523==maybe",
		"\n==\n==123==maybe",
		"\n==\n==123==maybe==456==",
		"indeed\n==1==",
		NULL
	},
	*hasnot[] = {
		"",
		"=123==",
		"==jj==",
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
		"abc==123==def",
		"abc\n==",
		NULL
	};

	lst_diag("Test valgrind_lines()");

	i = 0;
	while (has[i]) {
		OK_TRUE(valgrind_lines(has[i]),
		        "valgrind_lines(): Has valgrind marker, string %d", i);
		i++;
	}

	i = 0;
	while (hasnot[i]) {
		OK_FALSE(valgrind_lines(hasnot[i]),
		         "valgrind_lines(): No valgrind marker, string %d", i);
		i++;
	}
}

/******************************************************************************
                   Function tests, use a temporary directory
******************************************************************************/

/*
 * test_lst_file_exists() - Tests the lst_file_exists() function. Returns 
 * nothing.
 */

static void test_lst_file_exists(void)
{
	FILE *fp;

	lst_diag("Test lst_file_exists()");

	OK_FALSE(lst_file_exists(LST_TMPDIR "/nonexisting"),
	         "%s/nonexisting does not exist", LST_TMPDIR);

	OK_NOTNULL(fp = fopen(LST_TMPDIR "/regfile", "w"),
	           "Create regular file %s/regfile", LST_TMPDIR);
	OK_SUCCESS(fclose(fp), "Close " LST_TMPDIR "/regfile");
	OK_TRUE(lst_file_exists(LST_TMPDIR "/regfile"),
	        "File %s/regfile does exist", LST_TMPDIR);

	OK_SUCCESS(mkdir(LST_TMPDIR "/dir", 0777), "mkdir %s/dir", LST_TMPDIR);
	OK_TRUE(lst_file_exists(LST_TMPDIR "/dir"),
	        "Directory %s/dir does exist", LST_TMPDIR);

	OK_SUCCESS(symlink("regfile", LST_TMPDIR "/symlink"),
	           "Create %s/symlink, points to regfile", LST_TMPDIR);
	OK_TRUE(lst_file_exists(LST_TMPDIR "/symlink"),
	        "%s/symlink does exist", LST_TMPDIR);

	OK_SUCCESS(symlink("dir", LST_TMPDIR "/dirlink"),
	           "Create %s/dirlink, points to dir", LST_TMPDIR);
	OK_TRUE(lst_file_exists(LST_TMPDIR "/dirlink"),
	        "%s/dirlink does exist", LST_TMPDIR);

	OK_SUCCESS(symlink("nonexisting", LST_TMPDIR "/broken"),
	           "Create broken symlink %s/broken", LST_TMPDIR);
	OK_TRUE(lst_file_exists(LST_TMPDIR "/broken"),
	        "Broken symlink %s/broken does exist", LST_TMPDIR);

	OK_SUCCESS(mkdir(LST_TMPDIR "/nopermdir", 0),
	           "mkdir %s/nopermdir with no permissions", LST_TMPDIR);
	OK_TRUE(lst_file_exists(LST_TMPDIR "/nopermdir"),
	        "Directory %s/nopermdir does exist", LST_TMPDIR);
	OK_FALSE(lst_file_exists(LST_TMPDIR "/nopermdir/inaccessible"),
	         "Tries to check %s/nopermdir/inaccessible", LST_TMPDIR);
	if (lst_is_root()) {
		OK_TRUE(1, "errno is EACCES (%s)" /* gncov */
		           " # SKIP Running as root", strerror(EACCES));
	} else {
		OK_EQUAL(errno, EACCES,
		         "errno is EACCES (%s)", strerror(EACCES));
	}
	errno = 0;

	OK_SUCCESS(remove(LST_TMPDIR "/regfile"),
	           "Delete %s/regfile", LST_TMPDIR);
	OK_SUCCESS(rmdir(LST_TMPDIR "/dir"), "rmdir %s/dir", LST_TMPDIR);
	OK_SUCCESS(remove(LST_TMPDIR "/symlink"),
	           "Delete %s/symlink", LST_TMPDIR);
	OK_SUCCESS(remove(LST_TMPDIR "/dirlink"),
	           "Delete %s/dirlink", LST_TMPDIR);
	OK_SUCCESS(remove(LST_TMPDIR "/broken"),
	           "Delete %s/broken", LST_TMPDIR);
	OK_SUCCESS(rmdir(LST_TMPDIR "/nopermdir"),
	           "rmdir %s/nopermdir", LST_TMPDIR);
}

/*
 * test_lst_read_from_file() - Tests the lst_read_from_file() function. Returns 
 * nothing.
 */

static void test_lst_read_from_file(void)
{
	char *p = NULL;
	int orig_errno;

	lst_diag("Test lst_read_from_file()");

	p = lst_read_from_file(LST_TMPDIR "/non-existing");
	orig_errno = errno;
	errno = 0;
	OK_NULL(p, "lst_read_from_file(): Non-existing file, returns NULL");
	OK_EQUAL(orig_errno, ENOENT, "lst_read_from_file(): errno is ENOENT");
	if (orig_errno != ENOENT) {
		lst_diag("errno was %d (%s)", /* gncov */
		         orig_errno, strerror(orig_errno));
	}
}

/*
 * lst_chk_rc() - Used by test_lst_re_check(). Verifies that 
 * `lst_re_check(pattern, s)` succeeds and returns 0.
 */

static void lst_chk_rc(const char *file, const int linenum,
                       const char *pattern, const char *s)
{
	assert(file);
	assert(*file);

	OK_SUCCESS_L(lst_re_check(pattern, s), file, linenum,
	             "lst_re_check(\"%s\", \"%s\")", pattern, s);
}

/*
 * test_lst_re_check() - Tests the lst_re_check() function. Returns nothing.
 */

static void test_lst_re_check(void)
{
	char *desc, *err_s = NULL, *exp = NULL;
	int res;

	lst_diag("Test lst_re_check()");

#define lst_chk_rc(pattern, s)  lst_chk_rc(__FILE__, __LINE__, (pattern), (s))
	lst_chk_rc("^$", "");
	lst_chk_rc("abc.*yz", "123abcdefghixyz321");
	lst_chk_rc("with\nlf", "123with\nlf321");
	lst_chk_rc("^a(b|c)d", "acdef");
	lst_chk_rc("^ab?cd", "acdef");
	lst_chk_rc("^a.*z$", "a\nb\ncdefz");
#undef lst_chk_rc

	desc = "lst_re_check(\"ab(c\", \"abc\")";
	if (lst_init_output_files()) {
		lst_restore_output_files(); /* gncov */
		failed_ok("lst_init_output_files()"); /* gncov */
		return; /* gncov */
	}
	res = lst_re_check("ab(c", "abc");
	lst_restore_output_files();
	OK_EQUAL(res, -1, "%s (retval)", desc);
	verify_output_files(desc, "", NULL);
	exp = lst_allocstr("lst_re_check(): regcomp() failed: ");
	if (!exp) {
		failed_ok("lst_allocstr()"); /* gncov */
		goto cleanup; /* gncov */
	}
	err_s = lst_read_from_file(stderr_file);
	if (!err_s) {
		failed_ok("lst_read_from_file()"); /* gncov */
		goto cleanup; /* gncov */
	}
	if (OK_STRNCMP(err_s, exp, strlen(exp), "%s (stderr)", desc))
		print_gotexp(err_s, exp); /* gncov */

cleanup:
	free(err_s);
	free(exp);
	lst_cleanup_tempdir(__FILE__, __LINE__);
}

/******************************************************************************
              Test the executable file with a temporary directory
******************************************************************************/

/*
 * get_execname() - Returns a pointer to an allocated string with the value 
 * `lst_execname` should be initialized with. `ename` is the value of the path 
 * to the executable (the value of `progname`). If `ename` doesn't contain a 
 * slash ('/'), no changes are made and a copy of `ename` is returned. If 
 * `ename` contains a slash, a string with full path to the executable is 
 * returned. If anything fails, it returns NULL.
 */

char *get_execname(const char *ename)
{
	char *s = NULL, *p, *path = NULL, *retval = NULL;

	assert(ename);
	assert(*ename);

	s = strdup(ename);
	if (!s) {
		failed_ok("strdup()"); /* gncov */
		return NULL; /* gncov */
	}
	if (!(p = strrchr(s, '/')))
		return s; /* gncov */
	*p = '\0';
	path = realpath(s, NULL);
	if (!path) {
		failed_ok("realpath()"); /* gncov */
		goto cleanup; /* gncov */
	}
	retval = lst_allocstr("%s/%s", path, ++p);
	if (!retval)
		failed_ok("lst_allocstr()"); /* gncov */

cleanup:
	free(path);
	free(s);

	return retval;
}

/******************************************************************************
                        Top-level selftest functions
******************************************************************************/

/*
 * functests_with_tempdir() - Tests functions that need a temporary directory 
 * to store the output from stderr and stdout. Returns nothing.
 */

static void functests_with_tempdir(void)
{
	int result;

	lst_diag("Test functions that need a temporary directory for stdout"
	         " and stderr");
	result = mkdir(LST_TMPDIR, 0755);
	OK_SUCCESS(result, "mkdir " LST_TMPDIR " for function tests");
	if (result) {
		lst_diag("Cannot create directory \"%s\", skipping" /* gncov */
		         " tests: %s",
		         LST_TMPDIR, strerror(errno)); /* gncov */
		errno = 0; /* gncov */
		return; /* gncov */
	}

	test_lst_file_exists();
	test_lst_read_from_file();
	test_lst_re_check();

	result = rmdir(LST_TMPDIR);
	OK_SUCCESS(result, "rmdir " LST_TMPDIR " after function tests");
	if (result) {
		diag_errno(); /* gncov */
		return; /* gncov */
	}
}

/*
 * test_lst_functions() - Tests various selftest functions directly. Returns 
 * nothing.
 */

void test_selftest_funcs(void)
{
	lst_diag("Test selftest routines");
	test_ok_macros();
	test_lst_binbuf_cpy();
	test_lst_allocstr();
	test_lst_count_substr();
	test_lst_str_replace();
	test_diag();
	test_gotexp_output();
	test_valgrind_lines();

	functests_with_tempdir();
}

/* vim: set ts=8 sw=8 sts=8 noet fo+=w tw=79 fenc=UTF-8 : */
