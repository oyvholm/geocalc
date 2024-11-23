/*
 * geocalc.c
 * File ID: 9404faba-87f2-11ef-8907-83850402c3ce
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
 * Global variables
 */

char *progname;
struct Options opt;

/*
 * msg() - Print a message prefixed with "[progname]: " to stderr if the 
 * current verbose level is equal or higher than the first argument. The rest 
 * of the arguments are delivered to vfprintf().
 * Returns the number of characters written.
 */

int msg(const VerboseLevel verbose, const char *format, ...)
{
	int retval = 0;

	assert(format);
	assert(strlen(format));

	if (opt.verbose >= verbose) {
		va_list ap;

		va_start(ap, format);
		retval = fprintf(stderr, "%s: ", progname);
		retval += vfprintf(stderr, format, ap);
		retval += fprintf(stderr, "\n");
		va_end(ap);
	}

	return retval;
}

/*
 * std_strerror() - Replacement for `strerror()` that returns a predictable 
 * error message on every platform so the tests work everywhere.
 */

const char *std_strerror(const int errnum)
{
	switch (errnum) {
	case EACCES:
		return "Permission denied";
	case EINVAL:
		return "Invalid argument";
	case ERANGE:
		return "Numerical result out of range";
	default:
		/*
		 * Should never happen. If this line is executed, an `errno` 
		 * value is missing from `std_strerror()`, and tests may fail 
		 * on other platforms.
		 */
		fprintf(stderr,
		        "%s: %s(): Unknown errnum received: %d, \"%s\"\n",
		        progname, __func__, errnum, strerror(errnum));
		return strerror(errnum);
	}
}

/*
 * myerror() - Print an error message to stderr using this format:
 *
 *     a: b: c
 *
 * where `a` is the name of the program (the value of `progname`), `b` is the 
 * output from the printf-like string and optional arguments, and `c` is the 
 * error message from `errno`. If `errno` indicates no error, the ": c" part is 
 * not printed. Returns the number of characters written.
 */

int myerror(const char *format, ...)
{
	va_list ap;
	int retval = 0;
	const int orig_errno = errno;

	assert(format);
	assert(strlen(format));

	retval = fprintf(stderr, "%s: ", progname);
	va_start(ap, format);
	retval += vfprintf(stderr, format, ap);
	va_end(ap);
	if (orig_errno)
		retval += fprintf(stderr, ": %s", std_strerror(orig_errno));
	retval += fprintf(stderr, "\n");

	return retval;
}

/*
 * print_license() - Display the program license. Returns `EXIT_SUCCESS`.
 */

static int print_license(void)
{
	puts("(C)opyleft 2024- Øyvind A. Holm <sunny@sunbase.org>");
	puts("");
	puts("This program is free software; you can redistribute it"
	     " and/or modify it \n"
	     "under the terms of the GNU General Public License as"
	     " published by the \n"
	     "Free Software Foundation; either version 2 of the License,"
	     " or (at your \n"
	     "option) any later version.");
	puts("");
	puts("This program is distributed in the hope that it will be"
	     " useful, but \n"
	     "WITHOUT ANY WARRANTY; without even the implied warranty of \n"
	     "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.");
	puts("See the GNU General Public License for more details.");
	puts("");
	puts("You should have received a copy of"
	     " the GNU General Public License along \n"
	     "with this program. If not, see <http://www.gnu.org/licenses/>.");

	return EXIT_SUCCESS;
}

/*
 * print_version() - Print version information on stdout. If `-q` is used, only 
 * the version number is printed. Returns `EXIT_SUCCESS`.
 */

static int print_version(void)
{
#ifdef FAKE_MEMLEAK
	char *p;

	p = malloc(100);
	if (p) { }
#endif

	if (opt.verbose <= VERBOSE_QUIET) {
		puts(EXEC_VERSION);
		return EXIT_SUCCESS;
	}
	printf("%s %s (%s)\n", progname, EXEC_VERSION, EXEC_DATE);
#ifdef FAKE_MEMLEAK
	printf("has FAKE_MEMLEAK\n");
#endif
#ifdef GCOV
	printf("has GCOV\n");
#endif
#ifdef NDEBUG
	printf("has NDEBUG\n");
#endif
#ifdef PROF
	printf("has PROF\n");
#endif
#ifdef USE_NEW
	printf("has USE_NEW\n");
#endif

	return EXIT_SUCCESS;
}

/*
 * usage() - Prints a help screen. Returns `retval`.
 */

static int usage(const int retval)
{
	if (retval != EXIT_SUCCESS) {
		myerror("Type \"%s --help\" for help screen."
		        " Returning with value %d.", progname, retval);
		return retval;
	}
	puts("");
	if (opt.verbose >= 1) {
		print_version();
		puts("");
	}
	printf("Usage: %s [options] <command> [args]\n", progname);
	printf("\n");
	printf("Commands:\n");
	printf("\n");
	printf("Some arguments are specified as a coordinate. One format is"
	       " allowed, \n"
	       "decimal degrees:\n"
	       "\n"
	       "  lat,lon\n"
	       "\n"
	       "where `lat` and `lon` is a number in the range"
	       " -90..90 and -180..180, \n"
	       "separated by a comma. The decimal comma must be a period,"
	       " '.'.\n");
	printf("\n");
	printf("  bear <coor1> <coor2>\n"
	       "    Print initial compass bearing (0-360) between"
	       " two points.\n");
	printf("  bpos <coor> <bearing> <length>\n"
	       "    Find the new geographic position after moving a certain"
	       " amount of \n"
	       "    meters from the start position in a specific direction."
	       " Negative \n"
	       "    values for the length are allowed, to make it possible"
	       " to calculate \n"
	       "    positions in the opposite direction of the bearing.\n");
	printf("  course <coor1> <coor2> <numpoints>\n"
	       "    Generate a list of intermediate points on a direct line"
	       " between two \n"
	       "    locations.\n"
	       "");
	printf("  dist <coor1> <coor2>\n"
	       "    Calculate the distance between two points.\n");
	printf("  lpos <coor1> <coor2> <fracdist>\n"
	       "    Prints the position of a point on a straight line between"
	       " the \n"
	       "    positions, where `fracdist` is a fraction that specifies"
	       " how far \n"
	       "    along the line the point is. 0 = start position,"
	       " 1 = end position. \n"
	       "    `fracdist` can also take values below 0 or above 1"
	       " to calculate \n"
	       "    positions beyond `coor2` or in the opposite direction"
	       " from `coor1`.\n");
	printf("\n");
	printf("Options:\n");
	printf("\n");
	printf("  -F <format>, --format <format>\n"
	       "    Output in a specific format. Available formats:"
	       " default, gpx.\n");
	printf("  -h, --help\n"
	       "    Show this help.\n");
	printf("  --km\n"
	       "    Use kilometers instead of meters for input and output.\n");
	printf("  --license\n"
	       "    Print the software license.\n");
	printf("  -q, --quiet\n"
	       "    Be more quiet. Can be repeated to increase silence.\n");
	printf("  -v, --verbose\n"
	       "    Increase level of verbosity. Can be repeated.\n");
	printf("  --selftest [arg]\n"
	       "    Run the built-in test suite. If specified, the argument"
	       " can contain \n"
	       "    one or more of these strings: \"exec\" (the tests use the"
	       " executable \n"
	       "    file), \"func\" (runs function tests), or \"all\"."
	       " Multiple strings \n"
	       "    should be separated by commas. If no argument is"
	       " specified, default \n"
	       "    is \"all\".\n");
	printf("  --valgrind [arg]\n"
	       "    Run the built-in test suite with Valgrind memory checking."
	       " Accepts \n"
	       "    the same optional argument as --selftest, with the same"
	       " defaults.\n");
	printf("  --version\n"
	       "    Print version information.\n");
	printf("\n");

	return retval;
}

/*
 * choose_opt_action() - Decide what to do when option `c` is found. Read 
 * definitions for long options from `opts`.
 * Returns 0 if ok, or 1 if `c` is unknown or anything fails.
 */

static int choose_opt_action(const int c, const struct option *opts)
{
	int retval = 0;

	assert(opts);

	switch (c) {
	case 0:
		if (!strcmp(opts->name, "km"))
			opt.km = true;
		else if (!strcmp(opts->name, "license"))
			opt.license = true;
		else if (!strcmp(opts->name, "selftest"))
			opt.selftest = true;
		else if (!strcmp(opts->name, "valgrind"))
			opt.valgrind = opt.selftest = true;
		else if (!strcmp(opts->name, "version"))
			opt.version = true;
		break;
	case 'F':
		opt.format = optarg;
		break;
	case 'h':
		opt.help = true;
		break;
	case 'q':
		opt.verbose--;
		break;
	case 'v':
		opt.verbose++;
		break;
	default:
		msg(VERBOSE_DEBUG,
		    "%s(): getopt_long() returned character code %d",
		    __func__, c);
		retval = 1;
		break;
	}

	return retval;
}

/*
 * parse_options() - Parse command line options.
 * Returns 0 if succesful, or 1 if an error occurs.
 */

static int parse_options(const int argc, char * const argv[])
{
	int retval = 0;

	assert(argv);

	opt.format = NULL;
	opt.help = false;
	opt.km = false;
	opt.license = false;
	opt.outpformat = OF_DEFAULT;
	opt.selftest = false;
	opt.testexec = false;
	opt.testfunc = false;
	opt.valgrind = false;
	opt.verbose = 0;
	opt.version = false;

	while (!retval) {
		int c;
		int option_index = 0;
		static const struct option long_options[] = {
			{"format", required_argument, NULL, 'F'},
			{"help", no_argument, NULL, 'h'},
			{"km", no_argument, NULL, 0},
			{"license", no_argument, NULL, 0},
			{"quiet", no_argument, NULL, 'q'},
			{"selftest", no_argument, NULL, 0},
			{"valgrind", no_argument, NULL, 0},
			{"verbose", no_argument, NULL, 'v'},
			{"version", no_argument, NULL, 0},
			{0, 0, 0, 0}
		};

		c = getopt_long(argc, argv,
		                "+"  /* Stop parsing after first non-option */
		                "F:" /* --format */
		                "h"  /* --help */
		                "q"  /* --quiet */
		                "v"  /* --verbose */
		                , long_options, &option_index);
		if (c == -1)
			break;
		retval = choose_opt_action(c, &long_options[option_index]);
	}

	return retval;
}

/*
 * wrong_argcount() - Check that the number of arguments to a command is 
 * correct. Returns 0 if the amount is correct, if it's not, it prints an error 
 * message and returns 1.
 */

static int wrong_argcount(const int exp, const int got)
{
	if (got != exp) {
		myerror("%s arguments", got < exp ? "Missing" : "Too many");
		return 1;
	}

	return 0;
}

/*
 * process_args() - Parses non-option arguments and executes the appropriate 
 * command with the provided arguments. Returns `EXIT_SUCCESS` if the command 
 * succeeds, otherwise it returns `EXIT_FAILURE`.
 */

static int process_args(int argc, char *argv[])
{
	int retval;
	const int numargs = argc - optind;
	const char *cmd = argv[optind];

	msg(VERBOSE_DEBUG, "%s(): cmd = %s", __func__, cmd);

	if (!strcmp(cmd, "bear") || !strcmp(cmd, "dist")) {
		if (wrong_argcount(3, numargs))
			return EXIT_FAILURE;
		retval = cmd_bear_dist(cmd,
		                       argv[optind + 1], argv[optind + 2]);
	} else if (!strcmp(cmd, "bpos")) {
		if (wrong_argcount(4, numargs))
			return EXIT_FAILURE;
		retval = cmd_bpos(argv[optind + 1], argv[optind + 2],
		                  argv[optind + 3]);
	} else if (!strcmp(cmd, "course")) {
		if (wrong_argcount(4, numargs))
			return EXIT_FAILURE;
		retval = cmd_course(argv[optind + 1], argv[optind + 2],
		                    argv[optind + 3]);
	} else if (!strcmp(cmd, "lpos")) {
		if (wrong_argcount(4, numargs))
			return EXIT_FAILURE;
		retval = cmd_lpos(argv[optind + 1], argv[optind + 2],
		                  argv[optind + 3]);
	} else {
		myerror("Unknown command: %s", cmd);
		retval = EXIT_FAILURE;
	}

	return retval;
}

/*
 * setup_options() - Do necessary changes to `o` based on the user input.
 *
 * - Set `o->outpformat` to the corresponding integer value of the -F/--format 
 *   argument.
 * - Parse the optional argument to --selftest and set `o->testexec` and 
 *   `o->testfunc`.
 *
 * Returns 0 if everything is ok, otherwise it returns 1.
 */

static int setup_options(struct Options *o, const int argc, char *argv[])
{
	if (o->format) {
		msg(VERBOSE_DEBUG, "%s(): o.format = \"%s\"",
		                   __func__, o->format);
		if (!strlen(o->format) || !strcmp(o->format, "default")) {
			o->outpformat = OF_DEFAULT;
		} else if (!strcmp(o->format, "gpx")) {
			o->outpformat = OF_GPX;
		} else {
			myerror("%s: Unknown output format", o->format);
			return 1;
		}
	}
	if (o->selftest) {
		if (optind < argc) {
			const char *s = argv[optind];
			if (!s) {
				myerror("%s(): argv[optind] is" /* gncov */
				        " NULL", __func__);
				return 1; /* gncov */
			}
			if (strstr(s, "all"))
				o->testexec = o->testfunc = true; /* gncov */
			if (strstr(s, "exec"))
				o->testexec = true; /* gncov */
			if (strstr(s, "func"))
				o->testfunc = true; /* gncov */
		} else {
			o->testexec = o->testfunc = true;
		}
	}

	return 0;
}

/*
 * main()
 */

int main(int argc, char *argv[])
{
	int retval = EXIT_SUCCESS;

	progname = argv[0];
	errno = 0;

	if (parse_options(argc, argv)) {
		myerror("Option error");
		return usage(EXIT_FAILURE);
	}

	msg(VERBOSE_DEBUG, "%s(): Using verbose level %d",
	                   __func__, opt.verbose);
	msg(VERBOSE_DEBUG, "%s(): argc = %d, optind = %d",
	                   __func__, argc, optind);

	if (setup_options(&opt, argc, argv))
		return EXIT_FAILURE;

	if (opt.help)
		return usage(EXIT_SUCCESS);
	if (opt.selftest)
		return selftest();
	if (opt.version)
		return print_version();
	if (opt.license)
		return print_license();

	msg(VERBOSE_TRACE, "argc = %d, optind = %d, argv[optind] = %s\n",
	                   argc, optind, argv[optind]);
	if (optind < argc) {
		int t;

		for (t = optind; t < argc; t++) {
			msg(VERBOSE_DEBUG, "%s(): Non-option arg %d: %s",
			                   __func__, t, argv[t]);
		}
		retval = process_args(argc, argv);
	} else {
		myerror("No arguments specified");
		return usage(EXIT_FAILURE);
	}

	msg(VERBOSE_DEBUG, "Returning from %s() with value %d",
	                   __func__, retval);
	return retval;
}

/* vim: set ts=8 sw=8 sts=8 noet fo+=w tw=79 fenc=UTF-8 : */
