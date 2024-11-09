/*
 * io.c
 * File ID: afb87148-98c7-11ef-85f1-83850402c3ce
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
 * read_from_fp() - Read data from fp into an allocated buffer and return a 
 * pointer to the allocated memory or NULL if something failed.
 */

char *read_from_fp(FILE *fp, struct binbuf *dest)
{
	struct binbuf buf;
	size_t bufsize = BUFSIZ;

	assert(fp);

	binbuf_init(&buf);

	do {
		char *p = NULL;
		char *new_mem = realloc(buf.buf, bufsize + buf.len);
		size_t bytes_read;

		if (!new_mem) {
			myerror("%s(): Cannot allocate" /* gncov */
			        " memory for stream buffer", __func__);
			binbuf_free(&buf); /* gncov */
			return NULL; /* gncov */
		}
		buf.alloc = bufsize + buf.len;
		buf.buf = new_mem;
		p = buf.buf + buf.len;
		bytes_read = fread(p, 1, bufsize - 1, fp);
		buf.len += bytes_read;
		p[bytes_read] = '\0';
		if (ferror(fp)) {
			myerror("%s(): Read error", __func__); /* gncov */
			binbuf_free(&buf); /* gncov */
			return NULL; /* gncov */
		}
	} while (!feof(fp));

	if (dest)
		*dest = buf;

	return buf.buf;
}

/*
 * streams_exec() - Execute a command and store stdout, stderr and the return 
 * value into `dest`. `cmd` is an array of arguments, and the last element must 
 * be NULL. The return value is somewhat undefined at this point in time.
 */

int streams_exec(struct streams *dest, char *cmd[])
{
	int retval = 1;
	int infd[2];
	int outfd[2];
	int errfd[2];
	pid_t pid;

	assert(cmd);
	if (opt.verbose >= 10) {
		int i = -1; /* gncov */

		fprintf(stderr, "# %s(", __func__); /* gncov */
		while (cmd[++i]) /* gncov */
			fprintf(stderr, "%s\"%s\"", /* gncov */
			                i ? ", " : "", cmd[i]); /* gncov */
		fprintf(stderr, ")\n"); /* gncov */
	}

	if (pipe(infd) == -1
	    || pipe(outfd) == -1
	    || pipe(errfd) == -1) {
		myerror("%s(): pipe() failed", __func__); /* gncov */
		goto out; /* gncov */
	}
	if ((pid = fork()) == -1) {
		myerror("%s(): fork() failed", __func__); /* gncov */
		goto out; /* gncov */
	}
	if (!pid) {
		/* child */
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		dup2(infd[0], STDIN_FILENO);
		dup2(outfd[1], STDOUT_FILENO);
		dup2(errfd[1], STDERR_FILENO);
		close(infd[0]);
		close(infd[1]);
		close(outfd[0]);
		close(outfd[1]);
		close(errfd[0]);
		close(errfd[1]);

		execvp(cmd[0], cmd); /* gncov */
		myerror("%s(): execvp() failed", __func__); /* gncov */

		return 1; /* gncov */
	} else {
		/* parent */
		FILE *infp, *outfp, *errfp;

		close(infd[0]);
		close(errfd[1]);
		close(outfd[1]);
		if (!dest) {
			wait(&retval); /* gncov */
			goto out; /* gncov */
		}
		infp = fdopen(infd[1], "w");
		outfp = fdopen(outfd[0], "r");
		errfp = fdopen(errfd[0], "r");
		if (infp && outfp && errfp) {
			if (dest->in.buf && dest->in.len)
				fwrite(dest->in.buf, 1, /* gncov */
				       dest->in.len, infp);
			read_from_fp(errfp, &dest->err);
			read_from_fp(outfp, &dest->out);
			msg(10, "%s(): %d: dest->out.buf = \"%s\"",
			        __func__, __LINE__, dest->out.buf);
			msg(10, "%s(): %d: dest->err.buf = \"%s\"",
			        __func__, __LINE__, dest->err.buf);
		} else {
			myerror("%s(): fdopen() failed", __func__); /* gncov */
		}
		fclose(errfp);
		fclose(outfp);
		fclose(infp);
		wait(&dest->ret);
		dest->ret = dest->ret >> 8;
		retval = dest->ret;
	}

out:
	return retval;
}

/* vim: set ts=8 sw=8 sts=8 noet fo+=w tw=79 fenc=UTF-8 : */
