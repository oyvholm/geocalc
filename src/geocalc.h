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
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if 1
#  define DEBL  msg(2, "DEBL: %s, line %u in %s()", \
                       __FILE__, __LINE__, __func__)
#else
#  define DEBL  ;
#endif

#define stddebug  stderr

struct Options {
	bool help;
	bool license;
	bool selftest;
	int verbose;
	bool version;
};

/*
 * Public function prototypes
 */

/* geocalc.c */
int msg(const int verbose, const char *format, ...);
int myerror(const char *format, ...);

/* selftest.c */
int selftest(void);

/*
 * Global variables
 */

extern const char *progname;
extern struct Options opt;

#endif /* ifndef _GEOCALC_H */

/* vim: set ts=8 sw=8 sts=8 noet fo+=w tw=79 fenc=UTF-8 : */
