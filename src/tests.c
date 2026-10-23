/*
 * tests.c
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

#define TYPE_HELP_STR  EXECSTR ": Type \"" EXECSTR " --help\" for help" \
                       " screen. Returning with value 1.\n"
#define OPTION_ERROR_STR  EXECSTR ": Option error\n" TYPE_HELP_STR
#define Tc(cmd, exp_stdout, exp_stderr, ...) \
        tc_func(file, linenum, (cmd), (exp_stdout), (exp_stderr), __VA_ARGS__)

/******************************************************************************
                             --selftest functions
******************************************************************************/

/*
 * print_version_info() - Displays output from the --version command. Returns 0 
 * if ok, or 1 if lst_streams_exec() failed.
 */

static int print_version_info(void)
{
	struct lst_streams ss;
	int res;

	lst_streams_init(&ss);
	res = lst_streams_exec(&ss, chp{ lst_execname, "--version", NULL });
	if (res) {
		failed_ok("lst_streams_exec()"); /* gncov */
		if (ss.err.buf) /* gncov */
			lst_diag(ss.err.buf); /* gncov */
		return 1; /* gncov */
	}
	lst_diag("========== BEGIN version info ==========\n"
	         "%s"
	         "=========== END version info ===========",
	         no_null(ss.out.buf));
	lst_streams_free(&ss);

	return 0;
}

/******************************************************************************
                      geocalc-specific selftest functions
******************************************************************************/

/*
 * init_testvars() - Initializes file-static variables and environment 
 * variables to use the temporary directory defined in LST_TMPDIR. Returns 1 if 
 * anything fails, or 0 if OK.
 */

static int init_testvars(void)
{
	lst_diag("Initialize variables and the environment");

	lst_init_home();

	return 0;
}

/*
 * verify_constants() - Checks that certain constants are unmodified. Some of 
 * these constants are used in the tests themselves, so the tests will be 
 * automatically updated to match the new text or value. This function contains 
 * hardcoded versions of the expected output or return values. Returns nothing.
 */

static void verify_constants(void)
{
	const char *e;

	lst_diag("Verify constants");

	e = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	    "<gpx"
	    " xmlns=\"http://www.topografix.com/GPX/1/1\""
	    " version=\"1.1\""
	    " creator=\"Geocalc - https://gitlab.com/oyvholm/geocalc\""
	    ">\n";
	OK_STRCMP(GPX_HEADER, e, "GPX_HEADER is correct");
	print_gotexp(GPX_HEADER, e);

	e = "Geocalc";
	OK_STRCMP(PROJ_NAME, e, "PROJ_NAME is correct");
	print_gotexp(PROJ_NAME, e);

	e = "https://gitlab.com/oyvholm/geocalc";
	OK_STRCMP(PROJ_URL, e, "PROJ_URL is correct");
	print_gotexp(PROJ_URL, e);
}

/*
 * exec_output() - Executes the command stored in the NULL-terminated array 
 * `cmd` and stores stdout in `dest`. Returns the exit code from the program.
 */

static int exec_output(struct lst_binbuf *dest, char *cmd[])
{
	int retval;
	struct lst_streams st;

	assert(dest);
	assert(cmd);
	if (!dest || !cmd) {
		myerror("%s(): `dest` or `cmd` is NULL", __func__); /* gncov */
		return 1; /* gncov */
	}

	lst_streams_init(&st);
	retval = lst_streams_exec(&st, cmd);
	lst_binbuf_cpy(dest, &st.out);
	lst_streams_free(&st);

	return retval;
}

/*
 * cleanup_tempdir() - Delete all known filesystem entries in the temporary 
 * work directory defined in LST_TMPDIR. Expects `linenum` to be the `__LINE__` 
 * value from a parent function. Returns nothing.
 */

static void cleanup_tempdir(const char *file, const int linenum)
{
	lst_cleanup_tempdir(file, linenum);
}

/******************************************************************************
                 Function tests, no temporary directory needed
******************************************************************************/

                              /*** geocalc.c ***/

/*
 * test_std_strerror() - Tests the std_strerror() function. Returns nothing.
 */

static void test_std_strerror(void)
{
	lst_diag("Test std_strerror()");
	OK_STRCMP(std_strerror(EACCES), "Permission denied",
	          "std_strerror(EACCES) is as expected");
}

                               /*** cmds.c ***/

/*
 * chk_round() - Used by test_round_number(). Verifies that `round_number(num, 
 * decimals)` results in `exp_result`. Returns nothing.
 */

static void chk_round(const char *file, const int linenum, const double num,
                      const int decimals, const double exp_result)
{
	double res = num;

	round_number(&res, decimals);
	OK_EQUAL_L(res, exp_result, file, linenum,
	           "round_number(%.13f, %d)", num, decimals);
	if (res != exp_result) {
		lst_diag("got: %.13f", res); /* gncov */
		lst_diag("exp: %.13f", exp_result); /* gncov */
	}
}

/*
 * test_round_number() - Tests the round_number() function. Returns nothing.
 */

static void test_round_number(void)
{
	lst_diag("Test round_number()");

#define chk_round(num, decimals, exp_result)  \
	chk_round(__FILE__, __LINE__, (num), (decimals), (exp_result))

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

#undef chk_round
}

                              /*** geomath.c ***/

/*
 * chk_antip() - Used by test_are_antipodal(), checks that are_antipodal() 
 * returns the correct answer for the coordinates. The expected return value is 
 * stored in `exp`. Returns nothing.
 */

static void chk_antip(const char *file, const int linenum, const char *coor1,
                      const char *coor2, const int exp)
{
	int result;
	double lat1, lon1, lat2, lon2;

	if (!coor1 || !coor2) {
		OK_ERROR_L(file, linenum, /* gncov */
		           "%s(): coor1 or coor2 is NULL", __func__);
		return; /* gncov */
	}

	if (parse_coordinate(coor1, true, &lat1, &lon1)
	    || parse_coordinate(coor2, true, &lat2, &lon2)) {
		failed_ok("parse_coordinate()"); /* gncov */
		return; /* gncov */
	}

	result = are_antipodal(lat1, lon1, lat2, lon2);
	OK_EQUAL_L(result, exp, file, linenum,
	           "are_antipodal(): \"%s\" and \"%s\", expects %s",
	           coor1, coor2, exp ? "yes" : "no");
}

/*
 * test_are_antipodal() - Tests the are_antipodal() function. Returns nothing.
 */

static void test_are_antipodal(void)
{
#define chk_antip(coor1, coor2, exp)  \
        chk_antip(__FILE__, __LINE__, (coor1), (coor2), (exp))

	lst_diag("Test are_antipodal()");
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

#undef chk_antip
}

/*
 * chk_bpos() - Used by test_bearing_position(). Sends `lat`, `lon`, `bear`, 
 * and `dist` to bearing_position() and verifies that it returns `exp_ret` and 
 * that the generated location is equal to `exp_lat` and `exp_lon`. Returns 
 * nothing.
 */

static void chk_bpos(const char *file, const int linenum, const char *coor,
                     const double bear, const double dist, const int exp_ret,
                     const double exp_lat, const double exp_lon)
{
	int result, succ;
	double lat, lon, nlat = 0, nlon = 0;

	assert(coor);
	if (parse_coordinate(coor, false, &lat, &lon)) {
		failed_ok("parse_coordinate()"); /* gncov */
		lst_diag("%s(\"%s\", %f, %f, %d, %f, %f)", /* gncov */
		         __func__, coor, bear, dist, exp_ret, exp_lat,
		         exp_lon);
		return; /* gncov */
	}
	result = bearing_position(lat, lon, bear, dist, &nlat, &nlon);
	round_number(&nlat, 6);
	round_number(&nlon, 6);
	succ = (result == exp_ret) && (nlat == exp_lat) && (nlon == exp_lon);
	OK_TRUE_L(succ, file, linenum,
	          "bearing_position(%f, %f, %f, %f)", lat, lon, bear, dist);
	if (!succ) {
		lst_diag("retval = %d (exp %d), nlat = %f" /* gncov */
		         " (exp %f), nlon = %f (exp %f)",
		         result, exp_ret, nlat, exp_lat, nlon, exp_lon);
		lst_diag("errdist = %.8f m", /* gncov */
		     haversine(exp_lat, exp_lon, nlat, nlon));
	}
}

/*
 * test_bearing_position() - Tests the bearing_position() function. Returns 
 * nothing.
 */

static void test_bearing_position(void)
{
	lst_diag("Test bearing_position()");

#define chk_bpos(coor, bear, dist, exp_ret, exp_lat, exp_lon)  \
        chk_bpos(__FILE__, __LINE__, (coor), (bear), (dist), (exp_ret), (exp_lat), \
                 (exp_lon))

	/* "lat,lon", bear, dist, exp_ret, exp_lat, exp_lon */
	chk_bpos("-90.000001,15", 0, 10, 1, 0, 0);
	chk_bpos("0,0", 0, 2.5 * MAX_EARTH_DISTANCE, 0, 90, 0);
	chk_bpos("0,0", 123.4567, -2.0 * MAX_EARTH_DISTANCE, 0, 0, 0);
	chk_bpos("0,0", 123.4567, 200.0 * MAX_EARTH_DISTANCE, 0, 0, 0);
	chk_bpos("0,0", 37.5, 1000.0, 0, 0.007135, 0.005475);
	chk_bpos("0,0", 90, -0.5 * MAX_EARTH_DISTANCE, 0, 0, -90);
	chk_bpos("1,2", -0.000001, 10, 1, 0, 0);
	chk_bpos("1,2", 360.000001, 10, 1, 0, 0);
	chk_bpos("31,-181", 0, 10, 1, 0, 0);
	chk_bpos("31,180.00000001", 0, 10, 1, 0, 0);
	chk_bpos("90,0", 180, 2.5 * MAX_EARTH_DISTANCE, 0, 0, 0);
	chk_bpos("90,0", 90, -2.5 * MAX_EARTH_DISTANCE, 0, 0, -90);
	chk_bpos("91,15", 0, 10, 1, 0, 0);

	/*
	 * Created with
	 *
	 * awk 'BEGIN {
	 *        srand();
	 *        for (i = 0; i < 20; i++) {
	 *          printf "rlat=%f; rlon=%f; rbear=%f; rdist=%f;\n",
	 *                 -90 + 180 * rand(), -180 + 360 * rand(),
	 *                 360 * rand(), 20015086.796 * rand();
	 *          printf "coor=$rlat,$rlon; bear=$rbear; dist=$rdist;" \
	 *                 " ncoor=$(geocalc bpos $coor $bear $dist)\n";
	 *          printf "echo \"%cchk_bpos(\\\"$rlat,$rlon\\\"," \
	 *                 "$rbear,$rdist,0,$ncoor);\"\n", 9;
	 *        }
	 *      }' | sh | sed 's/,/, /g; s/, /,/;' | sort -rn -k1.12
	 */
	chk_bpos("89.326049,-36.529102", 215.441606, 903321.513553, 0, 81.318420, -69.408552);
	chk_bpos("75.327434,146.338141", 216.906138, 377036.451727, 0, 72.499872, 139.554979);
	chk_bpos("73.275683,95.043486", 309.444313, 10698125.450468, 0, 4.482065, -34.596773);
	chk_bpos("70.230561,126.642851", 219.696688, 19934871.928906, 0, -70.780321, -51.957347);
	chk_bpos("69.868535,101.799668", 141.805911, 13033758.395721, 0, -42.062838, 149.587411);
	chk_bpos("66.350025,145.429028", 231.188268, 10296540.270978, 0, -17.019889, 90.934368);
	chk_bpos("44.987243,150.444814", 149.919135, 6949371.313155, 0, -12.496578, 177.533295);
	chk_bpos("42.583599,124.096862", 329.283951, 6426063.994314, 0, 63.669800, 21.098705);
	chk_bpos("39.104040,-43.196379", 214.844812, 7419480.778805, 0, -19.621676, -77.059812);
	chk_bpos("6.618107,-159.730989", 77.446531, 10478279.178288, 0, 11.935203, -63.964068);
	chk_bpos("4.679518,-164.873324", 328.496291, 11740437.507322, 0, 52.807232, 71.499801);
	chk_bpos("-4.741458,8.055448", 25.393621, 18989357.664929, 0, 13.056509, -175.991205);
	chk_bpos("-11.366863,-46.182592", 345.151971, 10692324.170455, 0, 74.434381, -117.887177);
	chk_bpos("-28.029437,-114.413592", 299.897276, 3192753.375537, 0, -11.581600, -139.573482);
	chk_bpos("-36.313943,162.605259", 84.064269, 15893721.802184, 0, 31.519043, -62.081453);
	chk_bpos("-41.278839,94.072031", 275.882586, 15438973.459512, 0, 33.190354, -34.462196);
	chk_bpos("-48.580362,4.008114", 42.644186, 5577586.806635, 0, -6.127918, 35.552466);
	chk_bpos("-69.803126,109.901567", 147.974718, 18867585.828500, 0, 60.562749, -81.243398);
	chk_bpos("-76.150107,125.001814", 138.050038, 17098960.282799, 0, 52.401773, -83.956478);
	chk_bpos("-79.809787,-137.446550", 321.667061, 14446269.370292, 0, 47.562193, 177.728173);

#undef chk_bpos
}

/*
 * chk_karney() - Used by test_karney_distance(). Verifies that 
 * `karney_distance(coor1, coor2)` returns the value in `exp_result`. Returns 
 * nothing.
 */

static void chk_karney(const char *file, const int linenum, const char *coor1,
                       const char *coor2, const double exp_result)
{
	double lat1, lon1, lat2, lon2;
	double exp_res = exp_result, result;
	char *res_s, *exp_s;

	assert(coor1);
	assert(coor2);

	if (parse_coordinate(coor1, false, &lat1, &lon1)
	    || parse_coordinate(coor2, false, &lat2, &lon2)) {
		OK_ERROR_L(file, linenum, /* gncov */
		           "%s() received invalid coordinate", __func__);
		return; /* gncov */
	}
	result = karney_distance(lat1, lon1, lat2, lon2);
	res_s = lst_allocstr("%.8f", result);
	exp_s = lst_allocstr("%.8f", exp_result);
	if (!res_s || !exp_s) {
		failed_ok("lst_allocstr()"); /* gncov */
		free(exp_s); /* gncov */
		free(res_s); /* gncov */
		return; /* gncov */
	}
	OK_STRCMP_L(res_s, exp_s, file, linenum,
	            "karney_distance(\"%s\", \"%s\")", coor1, coor2);
	if (strcmp(res_s, exp_s)) {
		print_gotexp(res_s, exp_s); /* gncov */
		lst_diag("        diff: %.15f", result - exp_res); /* gncov */
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
	lst_diag("Test karney_distance()");

#define chk_karney(coor1, coor2, exp_result)  \
        chk_karney(__FILE__, __LINE__, (coor1), (coor2), (exp_result))

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

	/* Coordinates out of range */

	chk_karney("-90.00000001,2", "3,4", -1.0);
	chk_karney("90.00000001,2", "3,4", -1.0);

	chk_karney("1,-180.0000001", "3,4", -1.0);
	chk_karney("1,180.0000000001", "3,4", -1.0);

	chk_karney("1,2", "-90.0000001,4", -1.0);
	chk_karney("1,2", "90.0000001,4", -1.0);

	chk_karney("1,2", "3,-180.0000000001", -1.0);
	chk_karney("1,2", "3,180.0000000001", -1.0);

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

#undef chk_karney
}

/*
 * chk_kb() - Used by test_karney_bearing(). Verifies that 
 * `karney_bearing(coor1, coor2)` returns the value in `exp_result`. Returns 
 * nothing.
 */

static void chk_kb(const char *file, const int linenum, const char *coor1,
                   const char *coor2, const double exp_result)
{
	double lat1, lon1, lat2, lon2;
	double result, exp_res_r = exp_result;

	assert(coor1);
	assert(*coor1);
	assert(coor2);
	assert(*coor2);

	if (parse_coordinate(coor1, false, &lat1, &lon1)
	    || parse_coordinate(coor2, false, &lat2, &lon2)) {
		return; /* gncov */
	}
	result = karney_bearing(lat1, lon1, lat2, lon2);
	round_number(&result, 8);
	round_number(&exp_res_r, 8);
	OK_EQUAL_L(result, exp_res_r, file, linenum,
	           "karney_bearing(\"%s\", \"%s\")", coor1, coor2);
	print_gotexp_double(result, exp_res_r);
}

/*
 * test_karney_bearing() - Tests the karney_bearing() function. Returns 
 * nothing.
 */

static void test_karney_bearing(void)
{
	lst_diag("Test karney_bearing()");

#define chk_kb(coor1, coor2, exp_result)  \
        chk_kb(__FILE__, __LINE__, (coor1), (coor2), (exp_result))

	chk_kb("0,0", "-10,0", 180.0);
	chk_kb("0,0", "0,-10", 270.0);
	chk_kb("0,0", "0,10", 90.0);
	chk_kb("0,0", "10,0", 0.0);

	chk_kb("-90,0", "90,0", -2.0);
	chk_kb("0,0", "0,180", -2.0);
	chk_kb("10,10", "-10,-170", -2.0);
	chk_kb("10,10", "10,10", -2.0);
	chk_kb("90,0", "-90,0", -2.0);
	chk_kb("90,145", "90,0", -2.0);
	chk_kb("90,145", "90,132", -2.0);

	chk_kb("-30,0", "30,180", -2.0);
	chk_kb("-90,45", "90,-135", -2.0);
	chk_kb("0,0", "0,179.999999999", 90.0);
	chk_kb("0,0", "0,180", -2.0);
	chk_kb("45,90", "-45,-90", -2.0);
	chk_kb("90,0", "89.999999999,180", 0.0);
	chk_kb("90,132", "-90,0", -2.0);

	chk_kb("-90.00000000000001,0", "0,0", -1.0);
	chk_kb("0,0", "-90.00000000000001,0", -1.0);
	chk_kb("0,0", "90.00000000000001,0", -1.0);
	chk_kb("90.00000000000001,0", "0,0", -1.0);

	chk_kb("0,-180.0000000000001", "0,0", -1.0);
	chk_kb("0,0", "0,-180.0000000000001", -1.0);
	chk_kb("0,0", "0,180.0000000000001", -1.0);
	chk_kb("0,180.0000000000001", "0,0", -1.0);

	/*** BEGIN gruesome ***/

	chk_kb("-89.99999999999,0", "-90,180", -2.0);
	chk_kb("-90,45", "-90,45.00000000000001", -2.0);
	chk_kb("0,0", "0,179.999999999", 90.0);
	chk_kb("0,0", "0.0000000001,0.0000000001", 45.10893383);
	chk_kb("0,180", "0,-180", -2.0);
	chk_kb("0,180.0000000000001", "0,0", -1.0);
	chk_kb("30,0", "30,179.99999999", 0.00000001);
	chk_kb("45,180", "45,-180", -2.0);
	chk_kb("89.99999999999,0", "-89.99999999999,180", -2.0);
	chk_kb("89.99999999999,45", "90,-135", -2.0);
	chk_kb("89.999999999999999,0", "-89.999999999999999,180", -2.0);
	chk_kb("90,145", "90,145.00000000000001", -2.0);
	chk_kb("90.00000000000001,0", "0,0", -1.0);

	chk_kb("-6.627869,-162.824", "6.265,17.7063", -2.0);
	chk_kb("0.0,0.0", "-0.000000004773980,179.999999994273736", -2.0);

#if 0
	/*
	 * Fails with Clang 20.1.8 in Termux:
	 *
	 * not ok 177 - 1303: karney_bearing("45,90", "-45,-89.5727192")
	 * #          got: '270.000000180000'
	 * #     expected: '270.000000000000'
	 * #         diff: '0.000000180000'
	 */
	chk_kb("45,90", "-45,-89.5727192", 270.0); /* FIXME: Large? -89.5727191 fails. */

	/*
	 * Fails with Clang 20.1.8 in Termux:
	 *
	 * not ok 184 - 1313: karney_bearing("10,10", "10,10.000000001")
	 * #          got: '90.000032500000'
	 * #     expected: '90.000000000000'
	 * #         diff: '0.000032500000'
	 */
	chk_kb("10,10", "10,10.000000001", 90.0);
#endif

	/*** END gruesome ***/

	/*
	 * The following values are generated by geographiclib-c v2.2.
	 */

	chk_kb("0,0", "25,25", 42.365753876027249);

	/*
	 * for f in $(seq 1 19); do
	 *     coor="$(geocalc --count 2 randpos)";
	 *     echo $coor | tr '\n,' ' ' | ./inverse \
	 *     | echo "$coor $(awk '{ printf("%s %s %s", $1, $2, $3); }')" \
	 *     | tr '\n' ' ' | cut -f -3 -d ' ';
	 * done | xsel -b
	 *
	 * # And then add 360 to all negative azimuths.
	 */

	chk_kb("-15.291072,33.733657", "-21.134958,-114.270580", 221.442071337961295);
	chk_kb("-16.250419,-57.408921", "44.229930,-9.241283", 33.754160798219282);
	chk_kb("-16.731835,77.019790", "-50.660897,43.273366", 210.997345671959039);
	chk_kb("-17.214471,-148.551499", "-14.143721,95.788014", 247.669001203815085);
	chk_kb("-42.956849,-174.523179", "23.787774,1.261647", 168.557789231506490);
	chk_kb("-43.590859,-40.094468", "68.812518,53.776295", 28.767854621807636);
	chk_kb("-44.231620,94.334243", "-32.584610,106.290944", 42.820915410330798);
	chk_kb("-6.993601,45.071224", "80.712957,-160.574281", 4.163264169181943);
	chk_kb("29.948191,-139.894272", "60.818154,127.296803", 327.586767888497235);
	chk_kb("30.482379,85.677017", "20.122221,74.782155", 226.178482975087832);
	chk_kb("31.019514,-48.751694", "-10.662815,22.267506", 109.015392228551036);
	chk_kb("32.649602,-92.037827", "13.168345,-135.276439", 254.159794360845481);
	chk_kb("5.601294,102.991470", "35.372309,137.799733", 42.503375006387230);
	chk_kb("6.065309,-31.437241", "2.856485,85.285084", 83.752900116126284);
	chk_kb("6.994571,59.705337", "82.625755,-19.744212", 352.649468341939357);
	chk_kb("64.267123,-22.780014", "-56.380612,116.793874", 86.889264535591380);
	chk_kb("65.351674,-157.208725", "39.658721,64.279225", 327.191722814535822);
	chk_kb("66.482941,68.362564", "6.266930,11.764577", 241.189261852706835);
	chk_kb("7.925686,150.847915", "-3.805892,-124.773509", 94.505683276820093);
	chk_kb("70.239019,25.076431", "31.466664,-145.779369", 352.044477780350643);

#undef chk_kb
}

/*
 * chk_rand_pos() - Used by test_rand_pos(). Executes rand_pos() with the 
 * values in `coor`, `maxdist` and `mindist` and checks that they're in the 
 * range defined by `exp_maxdist` and `exp_mindist`. Returns nothing.
 */

static void chk_rand_pos(const char *file, const int linenum, const char *coor,
                         const double maxdist, const double mindist,
                         const double exp_maxdist, const double exp_mindist)
{
	int errcount = 0, maxtests = 20;
	unsigned long l, numloop = 1e+4;
	double r_exp_max = round(exp_maxdist), r_exp_min = round(exp_mindist);

	for (l = 0; l < numloop; l++) {
		double clat = 1000.0, clon = 1000.0, rlat, rlon, dist, r_dist;
		if (coor && parse_coordinate(coor, true, &clat, &clon)) {
			failed_ok("parse_coordinate()"); /* gncov */
			lst_diag("test %d: coor = \"%s\"", /* gncov */
			         lst_testnum, coor);
			return; /* gncov */
		}
		rand_pos(&rlat, &rlon, clat, clon, maxdist, mindist);
		if (fabs(rlat) > 90.0) {
			OK_ERROR_L(file, linenum, /* gncov */
			           "rand_pos(): Coordinate %lu:"
			           " lat is outside [-90,90] range,"
			           " lat = %.15f", l, rlat);
			errcount++; /* gncov */
		}
		if (fabs(rlon) > 180.0) {
			OK_ERROR_L(file, linenum, /* gncov */
			           "rand_pos(): Coordinate %lu:"
			           " lon is outside [-180,180] range,"
			           " lon = %.15f", l, rlon);
			errcount++; /* gncov */
		}
		if (errcount >= maxtests) {
			lst_diag("Aborting rand_pos() range test" /* gncov */
			         " after %d errors", maxtests);
			break; /* gncov */
		}
		if (!coor)
			continue;
		dist = haversine(clat, clon, rlat, rlon);
		r_dist = round(dist);
		if (r_dist < r_exp_min || r_dist > r_exp_max) {
			errcount <= maxtests
			? OK_ERROR_L(file, linenum, /* gncov */
			             "randpos out of range (%.0f"
			             " to %.0f m), center = %f,%f randpos ="
			             " %f,%f dist = %f",
			             exp_mindist, exp_maxdist, clat, clon,
			             rlat, rlon, dist)
			: 1; /* gncov */
			errcount++; /* gncov */
		}
	}
	OK_EQUAL_L(errcount, 0, file, linenum,
	           "rand_pos(): All %lu coordinates %.0f-%.0f m are"
	           " in range, failed = %lu (%f%%)",
	           numloop, exp_mindist, exp_maxdist, errcount,
	           100.0 * (double)errcount / (double)numloop);
}

/*
 * test_rand_pos() - Tests the rand_pos() function. Returns nothing.
 */

static void test_rand_pos(void)
{
	double MED = MAX_EARTH_DISTANCE;

	lst_diag("Test rand_pos()");

#define chk_rand_pos(coor, maxdist, mindist, exp_maxdist, exp_mindist)  \
        chk_rand_pos(__FILE__, __LINE__, (coor), (maxdist), (mindist), \
                     (exp_maxdist), (exp_mindist))

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

#undef chk_rand_pos
}

                                /*** gpx.c ***/

/*
 * chk_xmlesc() - Used by test_xml_escape_string(). Verifies that 
 * xml_escape_string() converts the string `s` to `exp`. Returns nothing.
 */

static void chk_xmlesc(const char *file, const int linenum, const char *s,
                       const char *exp)
{
	char *got;

	assert(s);
	assert(exp);
	if (!s || !exp) {
		OK_ERROR_L(file, linenum, "%s() received NULL", /* gncov */
		                    __func__);
		return; /* gncov */
	}

	got = xml_escape_string(s);
	if (!got) {
		failed_ok("xml_escape_string()"); /* gncov */
		return; /* gncov */
	}
	OK_STRCMP_L(got, exp, file, linenum, "xml escape: \"%s\"", s);
	free(got);
}

/*
 * test_xml_escape_string() - Tests the xml_escape_string() function. Returns 
 * nothing.
 */

static void test_xml_escape_string(void)
{
	lst_diag("Test xml_escape_string()");

#define chk_xmlesc(s, exp)  chk_xmlesc(__FILE__, __LINE__, (s), (exp))
	chk_xmlesc("", "");
	chk_xmlesc("&", "&amp;");
	chk_xmlesc("<", "&lt;");
	chk_xmlesc(">", "&gt;");
	chk_xmlesc("\\", "\\");
	chk_xmlesc("a&c", "a&amp;c");
	chk_xmlesc("a<c", "a&lt;c");
	chk_xmlesc("a>c", "a&gt;c");
	chk_xmlesc("abc", "abc");
#undef chk_xmlesc
	OK_NULL(xml_escape_string(NULL), "xml_escape_string(NULL)");
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

	lst_diag("Test gpx_wpt()");

	e = "  <wpt lat=\"12.34\" lon=\"56.78\">\n"
	    "    <name>abc def</name>\n"
	    "    <cmt>ghi jkl MN</cmt>\n"
	    "  </wpt>\n";
	s = gpx_wpt(12.34, 56.78, "abc def", "ghi jkl MN");
	OK_STRCMP(no_null(s), e, "gpx_wpt() without special chars");
	print_gotexp(s, e);
	free(s);

	e = "  <wpt lat=\"12.34\" lon=\"56.78\">\n"
	    "    <name>&amp;</name>\n"
	    "    <cmt>&amp;</cmt>\n"
	    "  </wpt>\n";
	s = gpx_wpt(12.34, 56.78, "&", "&");
	OK_STRCMP(no_null(s), e, "gpx_wpt() with ampersand");
	print_gotexp(s, e);
	free(s);

	e = "  <wpt lat=\"12.34\" lon=\"56.78\">\n"
	    "    <name>&lt;</name>\n"
	    "    <cmt>&lt;</cmt>\n"
	    "  </wpt>\n";
	s = gpx_wpt(12.34, 56.78, "<", "<");
	OK_STRCMP(no_null(s), e, "gpx_wpt() with lt");
	print_gotexp(s, e);
	free(s);

	e = "  <wpt lat=\"12.34\" lon=\"56.78\">\n"
	    "    <name>&gt;</name>\n"
	    "    <cmt>&gt;</cmt>\n"
	    "  </wpt>\n";
	s = gpx_wpt(12.34, 56.78, ">", ">");
	OK_STRCMP(no_null(s), e, "gpx_wpt() with gt");
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
	e = lst_allocstr("  <wpt lat=\"12.34\" lon=\"56.78\">\n"
	                 "    <name>%s</name>\n"
	                 "    <cmt>%s</cmt>\n"
	                 "  </wpt>\n", c, c);
	if (!e) {
		failed_ok("lst_allocstr()"); /* gncov */
		return; /* gncov */
	}
	s = gpx_wpt(12.34, 56.78, p, p);
	OK_STRCMP(no_null(s), e, "gpx_wpt() with amp, gt, lt, and more");
	print_gotexp(s, e);
	free(s);
	free(e);

	p = NULL;
	e = NULL;
	s = gpx_wpt(12.34, 56.78, p, p);
	OK_NULL(s, "gpx_wpt() with NULL in name and cmt");
	print_gotexp(s, e);
	if (s) {
		OK_ERROR("%s(): `s` was allocated, should not" /* gncov */
		         " happen", __func__);
		free(s); /* gncov */
	}

	p = NULL;
	e = NULL;
	s = gpx_wpt(12.34, 56.78, p, "def");
	OK_NULL(s, "gpx_wpt() with NULL in name");
	print_gotexp(s, e);
	if (s) {
		OK_ERROR("%s(): `s` was allocated," /* gncov */
		         " should not happen", __func__);
		free(s); /* gncov */
	}

	s = gpx_wpt(12.34, 56.78, "abc", NULL);
	OK_NOTNULL(s, "gpx_wpt() with NULL in cmt");
	free(s);
}

                              /*** strings.c ***/

/*
 * chk_tz() - Used by test_trim_zeros(). Verifies that the number in the string 
 * `got` equals `exp` after processing by trim_zeros(). Returns nothing.
 */

static void chk_tz(const char *file, const int linenum, const char *got,
                   const char *exp)
{
	const char *res;
	char *s;

	assert(got);
	assert(exp);

	s = mystrdup(got);
	if (!s) {
		failed_ok("mystrdup()"); /* gncov */
		return; /* gncov */
	}
	res = trim_zeros(s);
	OK_STRCMP_L(res, exp, file, linenum,
	            "trim_zeros(\"%s\"), expects \"%s\"", got, exp);
	print_gotexp(res, exp);
	free(s);
}

/*
 * test_trim_zeros() - Tests the trim_zeros() function. Returns nothing.
 */

static void test_trim_zeros(void)
{
	lst_diag("Test trim_zeros()");

#define chk_tz(got, exp)  chk_tz(__FILE__, __LINE__, (got), (exp))

	chk_tz("", "");
	chk_tz("+", "+");
	chk_tz("++++++", "++++++");
	chk_tz("++1.0", "++1.0");
	chk_tz("+0.0000", "+0.0");
	chk_tz("+0.29750000", "+0.2975");
	chk_tz("+5.0", "+5.0");
	chk_tz("+5.000", "+5.0");
	chk_tz("-", "-");
	chk_tz("-+0.0", "-+0.0");
	chk_tz("---", "---");
	chk_tz("-0.0000", "-0.0");
	chk_tz("-7.9650", "-7.965");
	chk_tz(".", ".");
	chk_tz(".3000", ".3");
	chk_tz("0", "0");
	chk_tz("00000", "00000");
	chk_tz("1", "1");
	chk_tz("1.0.0", "1.0.0");
	chk_tz("1.234 abc", "1.234 abc");
	chk_tz("5.", "5.");
	chk_tz("5.0", "5.0");
	chk_tz("5.00000", "5.0");
	chk_tz("5.1", "5.1");
	chk_tz("5.100000", "5.1");
	chk_tz("5.10001", "5.10001");
	chk_tz("9.234abc", "9.234abc");
	chk_tz("abc", "abc");

#undef chk_tz
}

/*
 * test_mystrdup() - Tests the mystrdup() function. Returns nothing.
 */

static void test_mystrdup(void)
{
	const char *txt = "Test string";
	char *s;

	lst_diag("Test mystrdup()");
	OK_NULL(mystrdup(NULL), "mystrdup(NULL) == NULL");

	s = mystrdup(txt);
	if (!s) {
		failed_ok("mystrdup()"); /* gncov */
		return; /* gncov */
	}
	OK_STRCMP(s, txt, "mystrdup(): Strings are identical");
	free(s);
}

/*
 * test_allocstr() - Tests the allocstr() function. Returns nothing.
 */

static void test_allocstr(void)
{
	const size_t bufsize = BUFSIZ * 2 + 1;
	char *p, *p2, *p3;
	size_t alen;

	lst_diag("Test allocstr()");
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
		goto cleanup; /* gncov */
	}
	alen = strlen(p2);
	OK_EQUAL(alen, BUFSIZ * 2, "allocstr(): strlen is correct");
	p3 = p2;
	while (*p3) {
		if (*p3 != 'a') {
			p3 = NULL; /* gncov */
			break; /* gncov */
		}
		p3++;
	}
	OK_NOTNULL(p3, "allocstr(): Content of string is correct");

cleanup:
	free(p2);
	free(p);
}

/******************************************************************************
                   Function tests, use a temporary directory
******************************************************************************/

                              /*** geocalc.c ***/

/*
 * test_myerror() - Tests the myerror() function. Returns nothing.
 */

static void test_myerror(void)
{
	int res, len_stderr;
	const char *desc;
	char *orig_progname = progname, *exp_stderr;

	lst_diag("Test myerror()");

	desc = "myerror() with float";
	if (lst_init_output_files()) {
		lst_restore_output_files(); /* gncov */
		failed_ok("lst_init_output_files()"); /* gncov */
		return; /* gncov */
	}
	errno = EACCES;
	progname = lst_execname;
	res = myerror("Test with float: %.5f", 3.14159);
	progname = orig_progname;
	lst_restore_output_files();
	OK_EQUAL(errno, 0, "%s (errno)", desc);
	print_gotexp_int(errno, 0);
	exp_stderr = lst_allocstr("%s: Test with float: 3.14159: Permission"
	                          " denied\n", lst_execname);
	if (!exp_stderr) {
		failed_ok("lst_allocstr()"); /* gncov */
		return; /* gncov */
	}
	len_stderr = (int)strlen(exp_stderr);
	verify_output_files(desc, "", exp_stderr);
	OK_EQUAL(res, len_stderr, "%s (retval)", desc);
	print_gotexp_int(res, len_stderr);
	cleanup_tempdir(__FILE__, __LINE__);
	free(exp_stderr);
}

                                /* strings.c */

/*
 * chk_coor() - Tries to parse the coordinate in `s` with `parse_coordinate()` 
 * and tests that `parse_coordinate()` returns the expected values.
 * Returns nothing.
 */

static void chk_coor(const char *file, const int linenum, const char *s,
                     const int exp_ret, const double exp_lat,
                     const double exp_lon)
{
	double lat, lon;
	int result;

	result = parse_coordinate(s, true, &lat, &lon);
	OK_EQUAL_L(result, exp_ret, file, linenum,
	           "parse_coordinate(\"%s\"), expected to %s",
	           s, exp_ret ? "fail" : "succeed");

	if (result)
		return;

	OK_EQUAL_L(lat, exp_lat, file, linenum,
	           "parse_coordinate(\"%s\"): lat is ok", s);
	print_gotexp_double(lat, exp_lat);
	OK_EQUAL_L(lon, exp_lon, file, linenum,
	           "parse_coordinate(\"%s\"): lon is ok", s);
	print_gotexp_double(lon, exp_lon);
}

/*
 * test_parse_coordinate() - Various tests for `parse_coordinate()`. Returns 
 * nothing.
 */

static void test_parse_coordinate(void)
{
	lst_diag("Test parse_coordinate()");

#define chk_coor(s, exp_ret, exp_lat, exp_lon)  \
	chk_coor(__FILE__, __LINE__, (s), (exp_ret), (exp_lat), (exp_lon))

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

#undef chk_coor
}

/******************************************************************************
            Test the executable file, no temporary directory needed
******************************************************************************/

                         /****** Option tests ******/

                             /*** --valgrind ***/

/*
 * test_valgrind_option() - Tests the --valgrind command line option. Returns 
 * nothing.
 */

static void test_valgrind_option(void)
{
	struct Options o = opt_struct();
	struct lst_streams ss;

	lst_diag("Test --valgrind");

	if (o.valgrind) {
		lst_use_valgrind = false; /* gncov */
		lst_streams_init(&ss); /* gncov */
		lst_streams_exec(&ss, chp{ "valgrind", /* gncov */
		                           "--version", NULL });
		if (!strstr(ss.out.buf, "valgrind-")) { /* gncov */
			OK_ERROR("Valgrind is not installed," /* gncov */
			         " disabling Valgrind checks");
			lst_use_valgrind = false; /* gncov */
		} else {
			OK_SUCCESS(0, "Valgrind is installed"); /* gncov */
			lst_use_valgrind = true; /* gncov */
		}
		lst_streams_free(&ss); /* gncov */
	}

	sc((chp{ lst_execname, "--valgrind", "-h", NULL }),
	   "Show this",
	   "",
	   EXIT_SUCCESS,
	   "--valgrind -h");
}

                          /*** Standard options ***/

/*
 * test_standard_options() - Tests the various generic options available in 
 * most programs. Returns nothing.
 */

static void test_standard_options(void)
{
	char *s;

	lst_diag("Test standard options");

	lst_diag("Test -h/--help");
	sc((chp{ lst_execname, "-h", NULL }),
	   "  Show this help",
	   "",
	   EXIT_SUCCESS,
	   "-h");
	sc((chp{ lst_execname, "--help", NULL }),
	   "  Show this help",
	   "",
	   EXIT_SUCCESS,
	   "--help");

	lst_diag("Test -v/--verbose");
	sc((chp{ lst_execname, "-h", "--verbose", NULL }),
	   "  Show this help",
	   "",
	   EXIT_SUCCESS,
	   "-hv: Help text is displayed");
	sc((chp{ lst_execname, "-hv", NULL }),
	   EXEC_VERSION,
	   "",
	   EXIT_SUCCESS,
	   "-hv: Version number is printed along with the help text");
	sc((chp{ lst_execname, "-vvv", "--verbose", "--help", NULL }),
	   "  Show this help",
	   EXECSTR ": main(): Using verbose level 4\n",
	   EXIT_SUCCESS,
	   "-vvv --verbose: Using correct verbose level");
	sc((chp{ lst_execname, "-vvvvq", "-v", "--verbose", "--help", NULL }),
	   "  Show this help",
	   EXECSTR ": main(): Using verbose level 5\n",
	   EXIT_SUCCESS,
	   "--verbose: One -q reduces the verbosity level");

	lst_diag("Test --version");
	s = lst_allocstr("%s %s (%s)\n",
	                 lst_execname, EXEC_VERSION, EXEC_DATE);
	if (s) {
		sc((chp{ lst_execname, "--version", NULL }),
		   s,
		   "",
		   EXIT_SUCCESS,
		   "--version");
		free(s);
	} else {
		failed_ok("lst_allocstr()"); /* gncov */
	}
	tc((chp{ lst_execname, "--version", "-q", NULL }),
	   EXEC_VERSION "\n",
	   "",
	   EXIT_SUCCESS,
	   "--version with -q shows only the version number");

	lst_diag("Test --license");
	rc((chp{ lst_execname, "--license", NULL }),
	   "GNU General Public License.*either version 2 of the License",
	   "^$",
	   EXIT_SUCCESS,
	   "--license: It's version 2 of the GPL");

	lst_diag("Unknown option");
	sc((chp{ lst_execname, "--gurgle", NULL }),
	   "",
	   OPTION_ERROR_STR,
	   EXIT_FAILURE,
	   "Unknown option: \"Option error\" message is printed");
}

                             /*** -F/--format ***/

/*
 * test_format_option() - Tests the -F/--format option. Returns nothing.
 */

static void test_format_option(void)
{
	lst_diag("Test -F/--format");
	sc((chp{ lst_execname, "-vvvv", "-F", "FoRmAt", NULL }),
	   "",
	   EXECSTR ": setup_options(): o.format = \"FoRmAt\"\n",
	   EXIT_FAILURE,
	   "-vvvv -F FoRmAt: o.format is correct");
	sc((chp{ lst_execname, "-vvvv", "--format", "FoRmAt", NULL }),
	   "",
	   EXECSTR ": setup_options(): o.format = \"FoRmAt\"\n",
	   EXIT_FAILURE,
	   "-vvvv --format FoRmAt: o.format is correct");
	sc((chp{ lst_execname, "-vvvv", "-F", "FoRmAt", NULL }),
	   NULL,
	   EXECSTR ": FoRmAt: Unknown output format\n",
	   EXIT_FAILURE,
	   "-vvvv -F FoRmAt: It says it's unknown");
	tc((chp{ lst_execname, "-vvvv", "-F", "", "lpos", "54,7", "12,22",
	         "0.23", NULL }),
	   "44.531328,12.14587\n",
	   NULL,
	   EXIT_SUCCESS,
	   "-F with an empty argument");
}

                           /*** -H/--haversine ***/

/*
 * test_haversine_option() - Tests the -H/--haversine option. Returns nothing.
 */

static void test_haversine_option(void)
{
	lst_diag("Test -H/--haversine");

	tc((chp{ lst_execname, "-H", "dist", "13.389820,-71.453489",
	         "-24.171099,-162.897613", NULL }),
	   "10755873.395009\n",
	   "",
	   EXIT_SUCCESS,
	   "-H dist 13.389820,-71.453489 -24.171099,-162.897613");

	tc((chp{ lst_execname, "--haversine", "dist", "-51.548124,19.706076",
	         "-35.721304,13.064358", NULL }),
	   "1837351.151434\n",
	   "",
	   EXIT_SUCCESS,
	   "--haversine dist -51.548124,19.706076 -35.721304,13.064358");
}

                             /*** -K/--karney ***/

/*
 * test_karney_option() - Tests the -K/--karney option. Returns nothing.
 */

static void test_karney_option(void)
{
	lst_diag("Test -K/--karney");

                                /*** bear ***/

	tc((chp{ lst_execname, "-K", "bear", "13.389820,-71.453489",
	         "-24.171099,-162.897613", NULL }),
	   "246.80807199\n",
	   "",
	   EXIT_SUCCESS,
	   "-K bear 13.389820,-71.453489 -24.171099,-162.897613");

	tc((chp{ lst_execname, "--karney", "bear", "-51.548124,19.706076",
	         "-35.721304,13.064358", NULL }),
	   "340.66682175\n",
	   "",
	   EXIT_SUCCESS,
	   "--karney bear -51.548124,19.706076 -35.721304,13.064358");

	tc((chp{ lst_execname, "-K", "bear", "12.34,56.789", "12.34,56.789",
	         NULL }),
	   "",
	   EXECSTR ": Antipodal or coincident points, answer is undefined\n",
	   EXIT_FAILURE,
	   "-K bear: Coincident points");

	tc((chp{ lst_execname, "-K", "bear", "37,7", "-37,-173", NULL }),
	   "",
	   EXECSTR ": Antipodal or coincident points, answer is undefined\n",
	   EXIT_FAILURE,
	   "--karney bear: Antipodal points");

                                /*** dist ***/

	tc((chp{ lst_execname, "-K", "dist", "13.389820,-71.453489",
	         "-24.171099,-162.897613", NULL }),
	   "10759030.9440929\n",
	   "",
	   EXIT_SUCCESS,
	   "-K dist 13.389820,-71.453489 -24.171099,-162.897613");

	tc((chp{ lst_execname, "--karney", "dist", "-51.548124,19.706076",
	         "-35.721304,13.064358", NULL }),
	   "1836406.16934653\n",
	   "",
	   EXIT_SUCCESS,
	   "--karney dist -51.548124,19.706076 -35.721304,13.064358");

	tc((chp{ lst_execname, "-K", "dist", "12.34,56.789", "12.34,56.789",
	         NULL }),
	   "0.0\n",
	   "",
	   EXIT_SUCCESS,
	   "-K dist: Coincident points");

	tc((chp{ lst_execname, "-K", "dist", "37,7", "-37,-173", NULL }),
	   "",
	   EXECSTR ": Formula did not converge, antipodal points\n",
	   EXIT_FAILURE,
	   "--karney dist: Antipodal points");

	tc((chp{ lst_execname, "--karney", "dist", "90.00001,0", "10,0",
	         NULL }),
	   "",
	   EXECSTR ": 90.00001,0: Invalid coordinate\n",
	   EXIT_FAILURE,
	   "--karney dist: lat1 out of range (90.00001)");

	tc((chp{ lst_execname, "--karney", "dist", "-90.0000001,0", "10,0",
	         NULL }),
	   "",
	   EXECSTR ": -90.0000001,0: Invalid coordinate\n",
	   EXIT_FAILURE,
	   "--karney dist: lat1 out of range (-90.0000001)");

	tc((chp{ lst_execname, "--karney", "dist", "10,180.000000001", "10,0",
	         NULL }),
	   "",
	   EXECSTR ": 10,180.000000001: Invalid coordinate\n",
	   EXIT_FAILURE,
	   "--karney dist: lon1 out of range (180.000000001)");

	tc((chp{ lst_execname, "--karney", "dist", "10,-180.000000001", "10,0",
	         NULL }),
	   "",
	   EXECSTR ": 10,-180.000000001: Invalid coordinate\n",
	   EXIT_FAILURE,
	   "--karney dist: lon1 out of range (-180.000000001)");

	tc((chp{ lst_execname, "--karney", "dist", "10,10.123", "h,0",
	         NULL }),
	   "",
	   EXECSTR ": h,0: Invalid coordinate: Invalid argument\n",
	   EXIT_FAILURE,
	   "--karney dist: lat2 is not a number");

	tc((chp{ lst_execname, "--karney", "dist", "10,10.123", "9,1..2",
	         NULL }),
	   "",
	   EXECSTR ": 9,1..2: Invalid coordinate: Invalid argument\n",
	   EXIT_FAILURE,
	   "--karney dist: lat2 has 2 periods");
}

                               /*** --seed ***/

/*
 * test_seed_option() - Tests the --seed option. Returns nothing.
 */

static void test_seed_option(void)
{
	struct lst_binbuf bb1, bb2, bb3;

	lst_diag("Test --seed");

	lst_binbuf_init(&bb1);
	lst_binbuf_init(&bb2);
	lst_binbuf_init(&bb3);
	exec_output(&bb1, (chp{ lst_execname, "--seed", "64738",
	                        "--count", "20", "randpos", NULL }));
	exec_output(&bb2, (chp{ lst_execname, "--seed", "64739",
	                        "--count", "20", "randpos", NULL }));
	exec_output(&bb3, (chp{ lst_execname, "--seed", "64738",
	                        "--count", "20", "randpos", NULL }));
	OK_NOTNULL(strcmp(bb1.buf, bb2.buf),
	           "randpos with seed 64738 and 64739 are different");
	OK_STRCMP(bb3.buf, bb1.buf,
	          "randpos with seed 64738 is identical to the first");
	lst_binbuf_free(&bb3);
	lst_binbuf_free(&bb2);
	lst_binbuf_free(&bb1);

	tc((chp{ lst_execname, "--seed", "-29271", "--count", "10", "randpos",
	         NULL }),
	   "56.398026,65.672317\n"
	   "-62.545731,39.973376\n"
	   "-12.953591,-109.219631\n"
	   "71.625404,10.907732\n"
	   "-49.720325,-97.898594\n"
	   "-9.863466,161.067034\n"
	   "-21.125676,-179.454884\n"
	   "-20.152272,-58.45828\n"
	   "11.179247,79.685331\n"
	   "-47.058531,149.678499\n",
	   "",
	   EXIT_SUCCESS,
	   "--seed -29271 --count 10 randpos");

	tc((chp{ lst_execname, "--seed", "29271", "--count", "10", "randpos",
	         NULL }),
	   "-8.603169,114.257108\n"
	   "-46.646685,-133.238413\n"
	   "32.233828,-45.004903\n"
	   "-10.383696,-105.396018\n"
	   "14.981908,-85.632935\n"
	   "-2.55139,93.617273\n"
	   "-45.194786,6.814725\n"
	   "-4.49135,-102.252955\n"
	   "-54.462817,-25.117826\n"
	   "-23.940993,-89.915442\n",
	   "",
	   EXIT_SUCCESS,
	   "--seed 29271 --count 10 randpos");

	tc((chp{ lst_execname, "--format", "gpx", "--seed", "19999",
	         "--count", "4", "randpos", NULL }),
	   "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	   "<gpx xmlns=\"http://www.topografix.com/GPX/1/1\""
	   " version=\"1.1\" creator=\"Geocalc -"
	   " https://gitlab.com/oyvholm/geocalc\">\n"
	   "  <wpt lat=\"-17.91477\" lon=\"127.6547\">\n"
	   "    <name>Random 1, seed 19999</name>\n"
	   "  </wpt>\n"
	   "  <wpt lat=\"-27.493377\" lon=\"115.562443\">\n"
	   "    <name>Random 2, seed 19999</name>\n"
	   "  </wpt>\n"
	   "  <wpt lat=\"-22.699248\" lon=\"152.244953\">\n"
	   "    <name>Random 3, seed 19999</name>\n"
	   "  </wpt>\n"
	   "  <wpt lat=\"-52.359745\" lon=\"-142.81243\">\n"
	   "    <name>Random 4, seed 19999</name>\n"
	   "  </wpt>\n"
	   "</gpx>\n",
	   "",
	   EXIT_SUCCESS,
	   "--format gpx --seed 19999 --count 4 randpos");

	tc((chp{ lst_execname, "--seed", "", "randpos", NULL }),
	   "",
	   EXECSTR ": : Invalid --seed argument\n"
	   OPTION_ERROR_STR,
	   EXIT_FAILURE,
	   "Empty argument to --seed");

	tc((chp{ lst_execname, "--seed", "9.14", "randpos", NULL }),
	   "",
	   EXECSTR ": 9.14: Invalid --seed argument\n"
	   OPTION_ERROR_STR,
	   EXIT_FAILURE,
	   "--seed 9.14 randpos");
}

                         /****** Command tests ******/

                                /*** anti ***/

/*
 * chk_ca() - Used by test_cmd_anti(). Verifies that the coordinate `coor` 
 * results in the output `exp_stdout` on stdout. If `exp_stdout` is empty, an 
 * "Invalid coordinate" error message is expected on stderr, and an exit status 
 * of EXIT_FAILURE. If `exp_stdout` is "inv", it expects an "Invalid 
 * coordinate" error message followed by "Invalid argument". Returns nothing.
 */

static void chk_ca(const char *file, const int linenum, char *coor,
                   const char *exp_stdout)
{
	char *e_stderr;
	const char *e_stdout = exp_stdout;
	int e_ret;

	assert(coor);
	assert(exp_stdout);

	if (!*exp_stdout) {
		e_stderr = lst_allocstr("%s: %s: Invalid coordinate\n",
		                    lst_execname, coor);
		e_ret = EXIT_FAILURE;
	} else if (!strcmp(exp_stdout, "inv")) {
		e_stderr = lst_allocstr("%s: %s: Invalid coordinate:"
		                    " Invalid argument\n", lst_execname, coor);
		e_stdout = "";
		e_ret = EXIT_FAILURE;
	} else {
		e_stderr = mystrdup("");
		e_ret = EXIT_SUCCESS;
	}
	if (!e_stderr) {
		failed_ok("lst_allocstr() or mystrdup()"); /* gncov */
		return; /* gncov */
	}

	Tc((chp{ lst_execname, "anti", coor, NULL }),
	   e_stdout,
	   e_stderr,
	   e_ret,
	   "anti %s", coor);

	free(e_stderr);
}

/*
 * test_cmd_anti() - Tests the `anti` command. Returns nothing.
 */

static void test_cmd_anti(void)
{
	lst_diag("Test anti command");

#define chk_ca(coor, exp_stdout)  \
        chk_ca(__FILE__, __LINE__, (coor), (exp_stdout))

	chk_ca("90,0", "-90.0,0.0\n");
	chk_ca("-90,0", "90.0,0.0\n");
	chk_ca("0,0", "0.0,180.0\n");
	chk_ca("0,180", "0.0,0.0\n");
	chk_ca("0,-180", "0.0,0.0\n");
	chk_ca("90,78", "-90.0,0.0\n");
	chk_ca("-90,-32.4", "90.0,0.0\n");
	chk_ca("10,10", "-10.0,-170.0\n");
	chk_ca("12.345678,-87.654321", "-12.345678,92.345679\n");
	chk_ca("90.000001,0", "");
	chk_ca("-90.000001,0", "");
	chk_ca("0,180.000001", "");
	chk_ca("0,-180.000001", "");
	chk_ca("a,0", "inv");
	chk_ca("0,b", "inv");
	chk_ca("0,,1", "inv");
	chk_ca("1.2,3..4", "inv");
#undef chk_ca

	tc((chp{ lst_execname, "--format", "sql", "anti", "-14.861893,-69.128489",
	         NULL }),
	   "BEGIN;\n"
	   "CREATE TABLE IF NOT EXISTS anti (lat REAL, lon REAL, a_lat REAL, a_lon REAL);\n"
	   "INSERT INTO anti VALUES (-14.861893, -69.128489, 14.861893, 110.871511);\n"
	   "COMMIT;\n",
	   "",
	   EXIT_SUCCESS,
	   "--format sql anti -14.861893,-69.128489");

	tc((chp{ lst_execname, "-F", "sql", "anti", "80,70", NULL }),
	   "BEGIN;\n"
	   "CREATE TABLE IF NOT EXISTS anti (lat REAL, lon REAL, a_lat REAL, a_lon REAL);\n"
	   "INSERT INTO anti VALUES (80.0, 70.0, -80.0, -110.0);\n"
	   "COMMIT;\n",
	   "",
	   EXIT_SUCCESS,
	   "-F sql anti 80,70");

	tc((chp{ lst_execname, "-F", "gpx", "anti", "-61.859515,84.219729",
	         NULL }),
	   GPX_HEADER
	   "  <wpt lat=\"61.859515\" lon=\"-95.780271\">\n"
	   "    <name>anti</name>\n"
	   "    <cmt>anti -61.859515,84.219729</cmt>\n"
	   "  </wpt>\n"
	   "</gpx>\n",
	   "",
	   EXIT_SUCCESS,
	   "-F gpx anti -61.859515,84.219729");

	tc((chp{ lst_execname, "anti", NULL }),
	   "",
	   EXECSTR ": Missing arguments\n",
	   EXIT_FAILURE,
	   "anti, missing coor argument");

	tc((chp{ lst_execname, "anti", "1,2", "3,4", NULL }),
	   "",
	   EXECSTR ": Too many arguments\n",
	   EXIT_FAILURE,
	   "anti, 1 extra argument");

	tc((chp{ lst_execname, "--karney", "anti", "1,2", NULL }),
	   "",
	   EXECSTR ": -K/--karney is not supported by the anti command\n",
	   EXIT_FAILURE,
	   "--karney anti");
}

                                /*** bench ***/

/*
 * test_cmd_bench() - Tests the `bench` command. Returns nothing.
 */

static void test_cmd_bench(void)
{
	lst_diag("Test bench command");
	sc((chp{ lst_execname, "bench", "0", NULL }),
	   " haversine\n",
	   "\nLooping haversine() for ",
	   EXIT_SUCCESS,
	   "bench 0");
	tc((chp{ lst_execname, "bench", "0", "0", NULL }),
	   "",
	   EXECSTR ": Too many arguments\n",
	   EXIT_FAILURE,
	   "bench has 1 extra argument");
	sc((chp{ lst_execname, "--format", "sql", "bench", "0", NULL }),
	   "INSERT INTO bench VALUES ",
	   "Looping haversine() for ",
	   EXIT_SUCCESS,
	   "--format sql bench");
	tc((chp{ lst_execname, "--format", "gpx", "bench", "0", NULL }),
	   "",
	   EXECSTR ": GPX output is not supported by the bench command\n",
	   EXIT_FAILURE,
	   "--format gpx bench");
}

                                /*** bpos ***/

/*
 * test_cmd_bpos() - Tests the `bpos` command. Returns nothing.
 */

static void test_cmd_bpos(void)
{
	lst_diag("Test bpos command");
	tc((chp{ lst_execname, "bpos", "45,0", "45", "1000", NULL }),
	   "45.006359,0.008994\n",
	   "",
	   EXIT_SUCCESS,
	   "bpos 45,0 45 1000");
	tc((chp{ lst_execname, "--km", "bpos", "45,0", "45", "1", NULL }),
	   "45.006359,0.008994\n",
	   "",
	   EXIT_SUCCESS,
	   "--km bpos 45,0 45 1");
	tc((chp{ lst_execname, "bpos", "0,0", "90.0000001", "1", NULL }),
	   "0.0,0.000009\n",
	   "",
	   EXIT_SUCCESS,
	   "bpos: No negative zero in lat");
	tc((chp{ lst_execname, "bpos", "0,0", "359.9999999", "1", NULL }),
	   "0.000009,0.0\n",
	   "",
	   EXIT_SUCCESS,
	   "bpos: No negative zero in lon");
	tc((chp{ lst_execname, "--km", "bpos", "-34,179", "2", "19716", NULL }),
	   "36.688059,-1.117018\n",
	   "",
	   EXIT_SUCCESS,
	   "bpos: Longitude is normalized");
	tc((chp{ lst_execname, "--km", "bpos", "90,0", "180", "20000", NULL }),
	   "-89.864321,0.0\n",
	   "",
	   EXIT_SUCCESS,
	   "bpos: North Pole, point is moved 1 cm");
	tc((chp{ lst_execname, "bpos", "90,97.97", "180", "1234567.89", NULL }),
	   "78.897264,97.97\n",
	   "",
	   EXIT_SUCCESS,
	   "bpos: North Pole, longitude is kept");
	tc((chp{ lst_execname, "--km", "bpos", "-90,0", "359", "7000", NULL }),
	   "-27.047487,-1.0\n",
	   "",
	   EXIT_SUCCESS,
	   "bpos: South Pole, bearing 359");
	tc((chp{ lst_execname, "bpos", "1,2", "360.0000000001", "1000", NULL }),
	   "",
	   EXECSTR ": 360.0000000001: Bearing out of range\n",
	   EXIT_FAILURE,
	   "bpos: bearing is larger than 360 degrees");
	tc((chp{ lst_execname, "bpos", "1,2", "-0.0000000001", "1000", NULL }),
	   "",
	   EXECSTR ": -0.0000000001: Bearing out of range\n",
	   EXIT_FAILURE,
	   "bpos: bearing is negative");
	tc((chp{ lst_execname, "bpos", "1,2", "r", "1000", NULL }),
	   "",
	   EXECSTR ": r: Invalid bearing: Invalid argument\n",
	   EXIT_FAILURE,
	   "bpos: bearing is not a number");
	tc((chp{ lst_execname, "bpos", "1,2w", "3", "4", NULL }),
	   "",
	   EXECSTR ": 1,2w: Invalid coordinate: Invalid argument\n",
	   EXIT_FAILURE,
	   "bpos: lon has trailing letter");
	tc((chp{ lst_execname, "bpos", "90.0000000001,2", "3", "4", NULL }),
	   "",
	   EXECSTR ": 90.0000000001,2: Invalid coordinate\n",
	   EXIT_FAILURE,
	   "bpos: lat is out of range");
	tc((chp{ lst_execname, "bpos", "1,2", "b", "4", NULL }),
	   "",
	   EXECSTR ": b: Invalid bearing: Invalid argument\n",
	   EXIT_FAILURE,
	   "bpos: Invalid bearing");
	tc((chp{ lst_execname, "bpos", "1,2", "3", "d", NULL }),
	   "",
	   EXECSTR ": d: Invalid distance: Invalid argument\n",
	   EXIT_FAILURE,
	   "bpos: Invalid distance");
	tc((chp{ lst_execname, "bpos", "1,2", "3", "4", "5", NULL }),
	   "",
	   EXECSTR ": Too many arguments\n",
	   EXIT_FAILURE,
	   "bpos: 1 extra argument");
	tc((chp{ lst_execname, "-F", "default", "bpos", "40.80542,-73.96546",
	         "188.7", "4817.84", NULL }),
	   "40.76259,-73.974113\n",
	   "",
	   EXIT_SUCCESS,
	   "-F default bpos");
	tc((chp{ lst_execname, "--format", "gpx", "bpos", "40.80542,-73.96546",
	         "188.7", "4817.84", NULL }),
	         GPX_HEADER
	         "  <wpt lat=\"40.76259\" lon=\"-73.974113\">\n"
	         "    <name>bpos</name>\n"
	         "    <cmt>bpos 40.80542,-73.96546 188.7 4817.84</cmt>\n"
	         "  </wpt>\n"
	         "</gpx>\n",
	   "",
	   EXIT_SUCCESS,
	   "--format gpx bpos");
	tc((chp{ lst_execname, "-K", "bpos", "1,2", "3", "4", NULL }),
	   "",
	   EXECSTR ": -K/--karney is not supported by the bpos command\n",
	   EXIT_FAILURE,
	   "-K bpos");

	lst_diag("--format sql bpos");
	tc((chp{ lst_execname, "-F", "sql", "bpos", "0,0", "90", "1000", NULL }),
	   "BEGIN;\n"
	   "CREATE TABLE IF NOT EXISTS bpos (lat1 REAL, lon1 REAL, lat2 REAL, lon2 REAL, bear REAL, dist REAL);\n"
	   "INSERT INTO bpos VALUES (0.0, 0.0, 0.0, 0.008993, 90.0, 1000.0);\n"
	   "COMMIT;\n",
	   "",
	   EXIT_SUCCESS,
	   "-F sql bpos 0,0 90 1000");
	tc((chp{ lst_execname, "-F", "sql", "--km", "bpos", "0,0", "90", "1",
	         NULL }),
	   "BEGIN;\n"
	   "CREATE TABLE IF NOT EXISTS bpos (lat1 REAL, lon1 REAL, lat2 REAL, lon2 REAL, bear REAL, dist REAL);\n"
	   "INSERT INTO bpos VALUES (0.0, 0.0, 0.0, 0.008993, 90.0, 1000.0);\n"
	   "COMMIT;\n",
	   "",
	   EXIT_SUCCESS,
	   "-F sql --km bpos 0,0 90 1");
	tc((chp{ lst_execname, "-F", "sql", "--km", "bpos",
	         "76.2379187,-134.9876543", "43.99999", "15000", NULL }),
	   "BEGIN;\n"
	   "CREATE TABLE IF NOT EXISTS bpos (lat1 REAL, lon1 REAL, lat2 REAL, lon2 REAL, bear REAL, dist REAL);\n"
	   "INSERT INTO bpos VALUES (76.237919, -134.987654, -34.358442, 8.42343, 43.99999, 15000000.0);\n"
	   "COMMIT;\n",
	   "",
	   EXIT_SUCCESS,
	   "-F sql --km bpos: dist = 15000 km");
}

                               /*** course ***/

/*
 * test_cmd_course() - Tests the `course` command. Returns nothing.
 */

static void test_cmd_course(void)
{
	char *exp_stdout;

	lst_diag("Test course command");
	tc((chp{ lst_execname, "course", "45,0", "45,180", "1", NULL }),
	   "45.0,0.0\n"
	   "90.0,0.0\n"
	   "45.0,180.0\n",
	   "",
	   EXIT_SUCCESS,
	   "course: Across the North Pole");
	tc((chp{ lst_execname, "course", "0,0", "0,180", "7", NULL }),
	   "",
	   EXECSTR ": Antipodal points, answer is undefined\n",
	   EXIT_FAILURE,
	   "course 0,0 0,180 7");
	tc((chp{ lst_execname, "course", "90,0", "0,0", "3", NULL }),
	   "90.0,0.0\n"
	   "67.5,0.0\n"
	   "45.0,0.0\n"
	   "22.5,0.0\n"
	   "0.0,0.0\n",
	   "",
	   EXIT_SUCCESS,
	   "course 90,0 0,0 3");
	exp_stdout = "60.39299,5.32415\n"
	             "66.169926,16.700678\n"
	             "70.664233,33.818071\n"
	             "72.834329,57.579125\n"
	             "71.826607,82.903321\n"
	             "68.075305,102.664288\n"
	             "62.689678,115.951619\n"
	             "56.44951,124.8845\n"
	             "49.752575,131.21524\n"
	             "42.795549,135.979229\n"
	             "35.681389,139.766944\n";
	tc((chp{ lst_execname, "course", "60.39299,5.32415",
	         "35.681389,139.766944", "9", NULL }),
	   exp_stdout,
	   "",
	   EXIT_SUCCESS,
	   "course: From Bergen to Tokyo");
	tc((chp{ lst_execname, "--km", "course", "60.39299,5.32415",
	         "35.681389,139.766944", "9", NULL }),
	   exp_stdout,
	   "",
	   EXIT_SUCCESS,
	   "--km course: From Bergen to Tokyo");
	tc((chp{ lst_execname, "course", "1,2", "3,4", NULL }),
	   "",
	   EXECSTR ": Missing arguments\n",
	   EXIT_FAILURE,
	   "course: Missing 1 argument");
	tc((chp{ lst_execname, "course", "1,2", "3,4", "5", "6", NULL }),
	   "",
	   EXECSTR ": Too many arguments\n",
	   EXIT_FAILURE,
	   "course: 1 argument too much");
	tc((chp{ lst_execname, "course", "90.00001,0", "12,34", "1", NULL }),
	   "",
	   EXECSTR ": 90.00001,0: Invalid coordinate\n",
	   EXIT_FAILURE,
	   "course: lat1 is outside range");
	tc((chp{ lst_execname, "course", "17,0", "12,34", "-1", NULL }),
	   "",
	   EXECSTR ": -1: Number of intermediate points cannot be negative\n",
	   EXIT_FAILURE,
	   "course: numpoints is -1");
	tc((chp{ lst_execname, "course", "17,6", "12,34", "-0.5", NULL }),
	   "",
	   EXECSTR ": -0.5: Number of intermediate points cannot be negative\n",
	   EXIT_FAILURE,
	   "course: numpoints is -0.5");
	tc((chp{ lst_execname, "course", "22,33", "44,55", "0", NULL }),
	   "22.0,33.0\n"
	   "44.0,55.0\n",
	   "",
	   EXIT_SUCCESS,
	   "course: numpoints is 0");
	tc((chp{ lst_execname, "course", "17,6%", "12,34", "-1", NULL }),
	   "",
	   EXECSTR ": 17,6%: Invalid coordinate: Invalid argument\n",
	   EXIT_FAILURE,
	   "course: lon1 is invalid number");
	tc((chp{ lst_execname, "course", "1,2", "xxx", "4", NULL }),
	   "",
	   EXECSTR ": xxx: Invalid coordinate\n",
	   EXIT_FAILURE,
	   "course: coor2 is invalid");
	tc((chp{ lst_execname, "course", "1,2", "3,4", "r", NULL }),
	   "",
	   EXECSTR ": r: Invalid number of points: Invalid argument\n",
	   EXIT_FAILURE,
	   "course: Invalid value in number of points");
	tc((chp{ lst_execname, "-F", "default", "course",
	         "52.3731,4.891", "35.681,139.767", "5", NULL }),
	   "52.3731,4.891\n"
	   "62.68586,22.57978\n"
	   "68.869393,53.549146\n"
	   "67.245712,91.173953\n"
	   "59.021383,116.487394\n"
	   "47.913547,130.771879\n"
	   "35.681,139.767\n",
	   "",
	   EXIT_SUCCESS,
	   "-F default course, Amsterdam to Tokyo");
	tc((chp{ lst_execname, "-F", "gpx", "course",
	         "52.3731,4.891", "35.681,139.767", "5", NULL }),
	   GPX_HEADER
	   "  <rte>\n"
	   "    <rtept lat=\"52.3731\" lon=\"4.891\">\n"
	   "    </rtept>\n"
	   "    <rtept lat=\"62.68586\" lon=\"22.57978\">\n"
	   "    </rtept>\n"
	   "    <rtept lat=\"68.869393\" lon=\"53.549146\">\n"
	   "    </rtept>\n"
	   "    <rtept lat=\"67.245712\" lon=\"91.173953\">\n"
	   "    </rtept>\n"
	   "    <rtept lat=\"59.021383\" lon=\"116.487394\">\n"
	   "    </rtept>\n"
	   "    <rtept lat=\"47.913547\" lon=\"130.771879\">\n"
	   "    </rtept>\n"
	   "    <rtept lat=\"35.681\" lon=\"139.767\">\n"
	   "    </rtept>\n"
	   "  </rte>\n"
	   "</gpx>\n",
	   "",
	   EXIT_SUCCESS,
	   "-F gpx course, Amsterdam to Tokyo");
	tc((chp{ lst_execname, "--karney", "course", "1,2", "3,4", "5", NULL }),
	   "",
	   EXECSTR ": -K/--karney is not supported by the course command\n",
	   EXIT_FAILURE,
	   "--karney course");

	lst_diag("--format sql course");
	tc((chp{ lst_execname, "-F", "sql", "course", "-45,-123", "45,-123", "5",
	         NULL }),
	   "BEGIN;\n"
	   "CREATE TABLE IF NOT EXISTS course (num INTEGER, lat REAL, lon REAL, dist REAL, frac REAL, bear REAL);\n"
	   "INSERT INTO course VALUES (0, -45.0, -123.0, 0.0, 0.0, 0.0);\n"
	   "INSERT INTO course VALUES (1, -30.0, -123.0, 1667923.899668, 0.166667, 0.0);\n"
	   "INSERT INTO course VALUES (2, -15.0, -123.0, 3335847.799337, 0.333333, 0.0);\n"
	   "INSERT INTO course VALUES (3, 0.0, -123.0, 5003771.699005, 0.5, 0.0);\n"
	   "INSERT INTO course VALUES (4, 15.0, -123.0, 6671695.598674, 0.666667, 0.0);\n"
	   "INSERT INTO course VALUES (5, 30.0, -123.0, 8339619.498342, 0.833333, 0.0);\n"
	   "INSERT INTO course VALUES (6, 45.0, -123.0, 10007543.39801, 1.0, NULL);\n"
	   "COMMIT;\n",
	   "",
	   EXIT_SUCCESS,
	   "-F sql course -45,-123 45,-123 5");
	tc((chp{ lst_execname, "-F", "sql", "course", "60,5", "-35,135", "5",
	         NULL }),
	   "BEGIN;\n"
	   "CREATE TABLE IF NOT EXISTS course (num INTEGER, lat REAL, lon REAL, dist REAL, frac REAL, bear REAL);\n"
	   "INSERT INTO course VALUES (0, 60.0, 5.0, 0.0, 0.0, 74.908926);\n"
	   "INSERT INTO course VALUES (1, 57.898298, 50.808536, 2584622.088993, 0.166667, 114.711921);\n"
	   "INSERT INTO course VALUES (2, 43.683265, 80.527411, 5169244.094446, 0.333333, 138.121194);\n"
	   "INSERT INTO course VALUES (3, 24.968228, 97.421916, 7753866.163712, 0.5, 147.823758);\n"
	   "INSERT INTO course VALUES (4, 4.878069, 109.598447, 10338488.288345, 0.666667, 151.019525);\n"
	   "INSERT INTO course VALUES (5, -15.417397, 121.038906, 12923110.365395, 0.833333, 149.948567);\n"
	   "INSERT INTO course VALUES (6, -35.0, 135.0, 15507732.39972, 1.0, NULL);\n"
	   "COMMIT;\n",
	   "",
	   EXIT_SUCCESS,
	   "-F sql course 60,5 -35,135 5");
}

                                /*** lpos ***/

/*
 * test_cmd_lpos() - Tests the `lpos` command. Returns nothing.
 */

static void test_cmd_lpos(void)
{
	lst_diag("Test lpos command");
	tc((chp{ lst_execname, "lpos", "45,0", "45,180", "0.5", NULL }),
	   "90.0,0.0\n",
	   "",
	   EXIT_SUCCESS,
	   "lpos: At the North Pole");
	tc((chp{ lst_execname, "--km", "lpos", "45,0", "45,180", "0.5", NULL }),
	   "90.0,0.0\n",
	   "",
	   EXIT_SUCCESS,
	   "--km lpos: At the North Pole");
	tc((chp{ lst_execname, "--format", "gpx", "lpos", "45,0", "45,180",
	         "0.5", NULL }),
	   GPX_HEADER
	   "  <wpt lat=\"90.0\" lon=\"0.0\">\n"
	   "    <name>lpos</name>\n"
	   "    <cmt>lpos 45,0 45,180 0.5</cmt>\n"
	   "  </wpt>\n"
	   "</gpx>\n",
	   "",
	   EXIT_SUCCESS,
	   "--format gpx lpos: At the North Pole");
	tc((chp{ lst_execname, "lpos", "0,0", "-0.0000001,0", "1", NULL }),
	   "0.0,0.0\n",
	   "",
	   EXIT_SUCCESS,
	   "lpos: No negative zero in lat");
	tc((chp{ lst_execname, "lpos", "0,0", "0,-0.0000001", "1", NULL }),
	   "0.0,0.0\n",
	   "",
	   EXIT_SUCCESS,
	   "lpos: No negative zero in lon");
	tc((chp{ lst_execname, "--km", "lpos", "-24.598059,178.290755",
	         "-12.500692,163.651473", "9.8", NULL }),
	   "24.508273,-1.833156\n",
	   "",
	   EXIT_SUCCESS,
	   "lpos: Longitude is normalized");
	tc((chp{ lst_execname, "lpos", "90,0", "0,0", "0.5", NULL }),
	   "45.0,0.0\n",
	   "",
	   EXIT_SUCCESS,
	   "lpos: Point is moved 1 cm");
	tc((chp{ lst_execname, "lpos", "-90,0", "90,0", "3", NULL }),
	   "",
	   EXECSTR ": Antipodal points, answer is undefined\n",
	   EXIT_FAILURE,
	   "lpos: South Pole to the North Pole");
	tc((chp{ lst_execname, "lpos", "1,2", "3,4", NULL }),
	   "",
	   EXECSTR ": Missing arguments\n",
	   EXIT_FAILURE,
	   "lpos: Missing 1 argument");
	tc((chp{ lst_execname, "lpos", "1,2", "3,4", "5", "6", NULL }),
	   "",
	   EXECSTR ": Too many arguments\n",
	   EXIT_FAILURE,
	   "lpos: 1 argument too much");
	tc((chp{ lst_execname, "lpos", "-90.00001,0", "12,34", "1", NULL }),
	   "",
	   EXECSTR ": -90.00001,0: Invalid coordinate\n",
	   EXIT_FAILURE,
	   "lpos: lat1 is outside range");
	tc((chp{ lst_execname, "lpos", "17,6%", "12,34", "-1", NULL }),
	   "",
	   EXECSTR ": 17,6%: Invalid coordinate: Invalid argument\n",
	   EXIT_FAILURE,
	   "lpos: lon1 is invalid number");
	tc((chp{ lst_execname, "lpos", "17,6", "12%,34", "-1", NULL }),
	   "",
	   EXECSTR ": 12%,34: Invalid coordinate: Invalid argument\n",
	   EXIT_FAILURE,
	   "lpos: lat in coor1 is invalid number");
	tc((chp{ lst_execname, "lpos", "1,2", "3,4", "0", NULL }),
	   "1.0,2.0\n",
	   "",
	   EXIT_SUCCESS,
	   "lpos: fracdist is 0");
	tc((chp{ lst_execname, "lpos", "11.231,-34.55", "29.97777,47.311001",
	         "1", NULL }),
	   "29.97777,47.311001\n",
	   "",
	   EXIT_SUCCESS,
	   "lpos: fracdist is 1");
	tc((chp{ lst_execname, "--km", "lpos", "11.231,-34.55",
	         "29.97777,47.311001", "1", NULL }),
	   "29.97777,47.311001\n",
	   "",
	   EXIT_SUCCESS,
	   "--km lpos: fracdist is 1");
	tc((chp{ lst_execname, "lpos", "1,2", "3,4", "INF", NULL }),
	   "",
	   EXECSTR ": INF: Invalid fraction: Numerical result out of range\n",
	   EXIT_FAILURE,
	   "lpos: fracdist is INF");
	tc((chp{ lst_execname, "lpos", "0,0", "0,180", "0.5", NULL }),
	   "",
	   EXECSTR ": Antipodal points, answer is undefined\n",
	   EXIT_FAILURE,
	   "lpos: Antipodal positions, 0,0 and 0,180");
	tc((chp{ lst_execname, "lpos", "90,0", "-90,0", "0.5", NULL }),
	   "",
	   EXECSTR ": Antipodal points, answer is undefined\n",
	   EXIT_FAILURE,
	   "lpos: Antipodal positions, 90,0 and -90,0");
	tc((chp{ lst_execname, "--karney", "lpos", "1,2", "3,4", "0.2", NULL }),
	   "",
	   EXECSTR ": -K/--karney is not supported by the lpos command\n",
	   EXIT_FAILURE,
	   "--karney lpos");
	tc((chp{ lst_execname, "-F", "sql", "lpos", "1,2", "87.188,-130.77", "0.2",
	         NULL }),
	   "BEGIN;\n"
	   "CREATE TABLE IF NOT EXISTS lpos (lat1 REAL, lon1 REAL, lat2 REAL, lon2 REAL, frac REAL, dlat REAL, dlon REAL, dist REAL, bear REAL);\n"
	   "INSERT INTO lpos VALUES (1.0, 2.0, 87.188, -130.77, 0.2, 19.169669, 1.31824, 2021740.468082, 357.935874);\n"
	   "COMMIT;\n",
	   "",
	   EXIT_SUCCESS,
	   "-F sql lpos");
}

                            /*** bear and dist ***/

/*
 * test_multiple() - Tests the `bear` or `dist` command. Returns nothing.
 */

static void test_multiple(const char *file, const int linenum, char *cmd)
{
	assert(cmd);
	assert(!strcmp(cmd, "bear") || !strcmp(cmd, "dist"));
	if (!cmd) {
		OK_ERROR_L(file, linenum, /* gncov */
		           "%s(): cmd is NULL", __func__);
		return; /* gncov */
	}

	lst_diag("Test %s command", !strcmp(cmd, "bear") ? "bear" : "dist");

	Tc((chp{ lst_execname, "-vv", cmd, NULL }),
	   "",
	   EXECSTR ": Missing arguments\n",
	   EXIT_FAILURE,
	   "%s with no arguments", cmd);
	Tc((chp{ lst_execname, cmd, "1,2", "3", NULL }),
	   "",
	   EXECSTR ": 3: Invalid coordinate\n",
	   EXIT_FAILURE,
	   "%s: Argument 2 is not a coordinate", cmd);
	Tc((chp{ lst_execname, cmd, "1,2", "3,4", NULL }),
	   !strcmp(cmd, "bear") ? "44.951998\n" : "314402.951024\n",
	   "",
	   EXIT_SUCCESS,
	   "%s 1,2 3,4", cmd);
	if (!strcmp(cmd, "bear")) {
		Tc((chp{ lst_execname, "bear", "12,34", "-12,-146", NULL }),
		   "",
		   EXECSTR ": Antipodal or coincident points, answer is undefined\n",
		   EXIT_FAILURE,
		   "bear 12,34 -12,-146 - antipodal points");
		Tc((chp{ lst_execname, "bear", "12,34", "12,34", NULL }),
		   "",
		   EXECSTR ": Antipodal or coincident points, answer is undefined\n",
		   EXIT_FAILURE,
		   "bear 12,34 -12,34 - coincident points");
	} else {
		Tc((chp{ lst_execname, "dist", "12,34", "-12,-146", NULL }),
		   "20015086.796021\n",
		   "",
		   EXIT_SUCCESS,
		   "dist 12,34 -12,-146 - antipodal points");
		Tc((chp{ lst_execname, "dist", "12,34", "12,34", NULL }),
		   "0.0\n",
		   "",
		   EXIT_SUCCESS,
		   "dist 12,34 12,34 - coincident points");
	}
	Tc((chp{ lst_execname, cmd, "1,2", "3,4", "5", NULL }),
	   "",
	   EXECSTR ": Too many arguments\n",
	   EXIT_FAILURE,
	   "%s with 1 argument too much", cmd);
	Tc((chp{ lst_execname, cmd, "1,2", "3,1e+900", NULL }),
	   "",
	   EXECSTR ": 3,1e+900: Invalid coordinate: Numerical result out of range\n",
	   EXIT_FAILURE,
	   "%s with 1 number too large", cmd);
	Tc((chp{ lst_execname, cmd, "1,2", "urgh,4", NULL }),
	   "",
	   EXECSTR ": urgh,4: Invalid coordinate: Invalid argument\n",
	   EXIT_FAILURE,
	   "%s with 1 non-number", cmd);
	Tc((chp{ lst_execname, cmd, "1,2.9y", "3,4", NULL }),
	   "",
	   EXECSTR ": 1,2.9y: Invalid coordinate: Invalid argument\n",
	   EXIT_FAILURE,
	   "%s with non-digit after number", cmd);
	Tc((chp{ lst_execname, cmd, "1,2 g", "3,4", NULL }),
	   "",
	   EXECSTR ": 1,2 g: Invalid coordinate: Invalid argument\n",
	   EXIT_FAILURE,
	   "%s with whitespace and non-digit after number", cmd);
	Tc((chp{ lst_execname, cmd, "10,2,", "3,4", NULL }),
	   !strcmp(cmd, "bear") ? "164.027619\n" : "809080.682265\n",
	   "",
	   EXIT_SUCCESS,
	   "%s with comma after number", cmd);
	Tc((chp{ lst_execname, cmd, "1,2", "3,NAN", NULL }),
	   "",
	   EXECSTR ": 3,NAN: Invalid coordinate: Invalid argument\n",
	   EXIT_FAILURE,
	   "%s with NAN", cmd);
	Tc((chp{ lst_execname, cmd, "1,2", "3,INF", NULL }),
	   "",
	   EXECSTR ": 3,INF: Invalid coordinate: Numerical result out of range\n",
	   EXIT_FAILURE,
	   "%s with INF", cmd);
	Tc((chp{ lst_execname, cmd, "1,2", "", NULL }),
	   "",
	   EXECSTR ": : Invalid coordinate\n",
	   EXIT_FAILURE,
	   "%s with empty argument", cmd);
	Tc((chp{ lst_execname, cmd, "1,180.001", "3,4", NULL }),
	   "",
	   EXECSTR ": 1,180.001: Invalid coordinate\n",
	   EXIT_FAILURE,
	   "%s: lon1 out of range", cmd);
	if (!strcmp(cmd, "bear")) {
		Tc((chp{ lst_execname, "bear", "90,0", "-90,0", NULL }),
		   "",
		   EXECSTR ": Antipodal or coincident points, answer is"
		   " undefined\n",
		   EXIT_FAILURE,
		   "bear 90,0 -90,0");
		Tc((chp{ lst_execname, "--km", cmd, "90,0", "-90,0", NULL }),
		   "",
		   EXECSTR ": Antipodal or coincident points, answer is"
		   " undefined\n",
		   EXIT_FAILURE,
		   "--km bear 90,0 -90,0");
	} else {
		Tc((chp{ lst_execname, "dist", "90,0", "-90,0", NULL }),
		   "20015086.796021\n",
		   "",
		   EXIT_SUCCESS,
		   "dist 90,0 -90,0");
		Tc((chp{ lst_execname, "--km", cmd, "90,0", "-90,0", NULL }),
		   "20015.086796\n",
		   "",
		   EXIT_SUCCESS,
		   "--km dist 90,0 -90,0");
	}
	Tc((chp{ lst_execname, "-F", "default", cmd, "34,56", "-78,9", NULL }),
	   !strcmp(cmd, "bear") ? "189.693136\n" : "12835310.777042\n",
	   "",
	   EXIT_SUCCESS,
	   "-F default %s", cmd);
	Tc((chp{ lst_execname, "--format", "gpx", cmd, "34,56", "-78,9", NULL }),
	   "",
	   !strcmp(cmd, "bear")
	       ? EXECSTR ": GPX output is not supported by the bear command\n"
	       : EXECSTR ": GPX output is not supported by the dist command\n",
	   EXIT_FAILURE,
	   "--format gpx %s", cmd);

	lst_diag("--format sql %s", cmd);
	if (!strcmp(cmd, "bear")) {
		Tc((chp{ lst_execname, "--format", "sql", cmd, "34,56", "-78,9",
		         NULL }),
		   "BEGIN;\n"
		   "CREATE TABLE IF NOT EXISTS bear (lat1 REAL, lon1 REAL, lat2 REAL, lon2 REAL, bear REAL, dist REAL);\n"
		   "INSERT INTO bear VALUES (34.0, 56.0, -78.0, 9.0, 189.693136, 12835310.777042);\n"
		   "COMMIT;\n",
		   "",
		   EXIT_SUCCESS,
		   "--format sql %s", cmd);
	} else {
		Tc((chp{ lst_execname, "--format", "sql", cmd, "34,56", "-78,9",
		         NULL }),
		   "BEGIN;\n"
		   "CREATE TABLE IF NOT EXISTS dist (lat1 REAL, lon1 REAL, lat2 REAL, lon2 REAL, dist REAL, bear REAL);\n"
		   "INSERT INTO dist VALUES (34.0, 56.0, -78.0, 9.0, 12835310.77704204, 189.69313615);\n"
		   "COMMIT;\n",
		   "",
		   EXIT_SUCCESS,
		   "--format sql %s", cmd);
	}
}

                               /*** randpos ***/

/*
 * verify_coor_dist() - Verifies that the distance between `coor` and all 
 * "\n"-separated coordinates in `str` are in the range `mindist`-`maxdist` 
 * meters. Returns nothing.
 */

static void verify_coor_dist(const char *file, const int linenum,
                             const char *str, const char *coor,
                             const double mindist, const double maxdist)
{
	int errcount = 0;
	char *s, *p;
	double clat, clon,
	       mindist_r = round(mindist), maxdist_r = round(maxdist);
	unsigned long coorcount = 0;

	assert(str);
	assert(coor);
	if (!str || !coor) {
		OK_ERROR_L(file, linenum, /* gncov */
		           "%s(): `str` or `coor` is NULL", __func__);
		return; /* gncov */
	}
	if (!strchr(str, '\n')) {
		OK_ERROR_L(file, linenum, /* gncov */
		           "%s(): No '\\n' found in `str`", __func__);
		return; /* gncov */
	}
	if (parse_coordinate(coor, true, &clat, &clon)) {
		failed_ok("parse_coordinate()"); /* gncov */
		lst_diag("%s(): coor = \"%s\"", __func__, coor); /* gncov */
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
		if (parse_coordinate(p, true, &lat, &lon)) {
			failed_ok("parse_coordinate()"); /* gncov */
			lst_diag("%s(): p = \"%s\"", __func__, p); /* gncov */
			errcount++; /* gncov */
			goto next; /* gncov */
		}
		dist = haversine(clat, clon, lat, lon);
		if (dist == -1.0) {
			OK_ERROR_L(file, linenum, /* gncov */
			           "%s(): haversine() failed, values out of"
			           " range", __func__);
			lst_diag("%s(): coor = \"%s\", lat = %f," /* gncov */
			         " lon = %f", __func__, coor, lat, lon);
			errcount++; /* gncov */
			goto next; /* gncov */
		}
		dist = round(dist);
		if (dist < mindist_r || dist > maxdist_r) {
			errcount < 11
			? OK_ERROR_L(file, linenum, /* gncov */
			             "Coordinate out of range (%f"
			             " to %f m). center: %f,%f, coor = %f,%f,"
			             " dist = %f",
			             mindist, maxdist,
			             clat, clon, lat, lon, dist)
			: 1; /* gncov */
			errcount++; /* gncov */
		}
next:
		if (errcount >= 10) {
			OK_ERROR_L(file, linenum, /* gncov */
			           "Aborting this test after 10 errors");
			break; /* gncov */
		}
		p = strtok(NULL, "\n");
	}
	OK_EQUAL_L(errcount, 0, file, linenum,
	           "randpos: All %lu coordinates were inside range of"
	           " %.0f to %.0f meters", coorcount, mindist, maxdist);
	free(s);
}

/*
 * chk_coor_outp() - Verifies the output from the randpos command. Returns 1 if 
 * any error, 0 if everything is ok.
 */

static int chk_coor_outp(const char *file, const int linenum,
                         const OutputFormat format, const char *output,
                         const unsigned int num, const char *coor,
                         const double mindist, const double maxdist)
{
	int errcount = 0;
	regex_t regex;
	char *regstr, *pattern;
	static int count = 0;

	assert(output);

	count++;
	if (format == OF_DEFAULT && coor)
		verify_coor_dist(file, linenum, output, coor, mindist,
		                 maxdist);

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
		         "    <name>Random [0-9]+, seed [0-9]+</name>\n"
		         "  </wpt>\n"
		         "){%u}"
		         "</gpx>\n$";
		break;
	default: /* gncov */
		regstr = "NON-EXISTENT VALUE"; /* gncov */
		break; /* gncov */
	}

	pattern = lst_allocstr(regstr, num);
	if (!pattern) {
		failed_ok("lst_allocstr()"); /* gncov */
		return 1; /* gncov */
	}

	if (regcomp(&regex, pattern, REG_EXTENDED)) {
		failed_ok("regcomp()"); /* gncov */
		errcount++; /* gncov */
		goto cleanup; /* gncov */
	}

	OK_SUCCESS_L(regexec(&regex, output, 0, NULL, 0), file, linenum,
	             "Regexp matches, count = %d", count);
	regfree(&regex);

cleanup:
	free(pattern);

	return !!errcount;
}

/*
 * te_randpos() - Executes `cmd` and verifies that all coordinates are valid 
 * and inside the various ranges specified. Returns nothing.
 */

static void te_randpos(const char *file, const int linenum,
                       const OutputFormat format, char **cmd,
                       const unsigned int num, const char *coor,
                       const double mindist, const double maxdist,
                       const char *desc)
{
	struct lst_streams ss;

	assert(cmd);
	assert(*cmd);
	assert(desc);
	assert(*desc);

	lst_streams_init(&ss);
	lst_streams_exec(&ss, cmd);
	OK_SUCCESS_L(chk_coor_outp(file, linenum, format, ss.out.buf, num,
	                           coor, mindist, maxdist),
	             file, linenum, desc);
	lst_streams_free(&ss);
}

#define te_randpos(format, cmd, num, coor, mindist, maxdist, desc)  \
        te_randpos(__FILE__, __LINE__, (format), (cmd), (num), (coor), (mindist), \
                   (maxdist), (desc))

/*
 * test_randpos_dist_max() - Tests the randpos command with only the maximum 
 * distance. Returns nothing.
 */

static void test_randpos_dist_max(void)
{
	char **as;

	lst_diag("randpos with max_dist");

	as = (chp{ lst_execname, "--count", "50", "randpos", "1.234,5.6789", "100",
	           NULL });
	te_randpos(OF_DEFAULT, as, 50, "1.234,5.6789", 0.0, 100.0,
	           "randpos: 50 pos inside a radius of 100m");

	as = (chp{ lst_execname, "--count", "51", "randpos", "1.234,5.6789",
	           "100000000", NULL });
	te_randpos(OF_DEFAULT, as, 51, "1.234,5.6789", 0.0,
		   MAX_EARTH_DISTANCE,
		   "randpos: max_dist is larger than MAX_EARTH_DISTANCE");

	as = (chp{ lst_execname, "--count", "52", "randpos", "1.234,5.6789", "0",
	           "100000000", NULL });
	te_randpos(OF_DEFAULT, as, 52, "1.234,5.6789", 0.0,
		   MAX_EARTH_DISTANCE,
		   "randpos: min_dist is larger than MAX_EARTH_DISTANCE");

	as = (chp{ lst_execname, "--count", "53", "randpos", "1.234,5.67",
	           "100000000", "100000000", NULL });
	te_randpos(OF_DEFAULT, as, 53, "1.234,5.67", MAX_EARTH_DISTANCE,
	           MAX_EARTH_DISTANCE,
	           "randpos: min_dist and max_dist are larger than"
	           " MAX_EARTH_DISTANCE, stdout looks ok");

	as = (chp{ lst_execname, "-F", "gpx", "--count", "14", "randpos",
	           "19.63,-19.70", "25", NULL });
	te_randpos(OF_GPX, as, 14, NULL, 0, 0,
	           "-F gpx --count 14 randpos 19.63,-19.70 25");

	as = (chp{ lst_execname, "--km", "--count", "50", "randpos",
	           "1.234,5.6789", "100", NULL });
	te_randpos(OF_DEFAULT, as, 50,
	           "1.234,5.6789", 0.0, 100000.0,
	           "--km randpos: 50 pos inside a radius of 100km,"
	           " stdout looks ok");

	as = (chp{ lst_execname, "--km", "--count", "50", "randpos",
	           "1.234,5.6789", "100000", NULL });
	te_randpos(OF_DEFAULT, as, 50,
	           "1.234,5.6789", 0.0, MAX_EARTH_DISTANCE,
	           "--km randpos: max_dist is larger than MAX_EARTH_DISTANCE,"
	           " stdout looks ok");
}

/*
 * test_randpos_dist_minmax() - Tests the randpos command with both maximum and 
 * minimum distance. Returns nothing.
 */

static void test_randpos_dist_minmax(void)
{
	char **as;

	lst_diag("randpos with max_dist and min_dist");

	as = (chp{ lst_execname, "randpos", "12.34,56.78", "100", "200", NULL });
	te_randpos(OF_DEFAULT, as, 1, "12.34,56.78", 100.0, 200.0,
	           "randpos: min_dist is larger than max_dist");

	as = (chp{ lst_execname, "--count", "27", "randpos", "1.234,5.6789",
	           "2000", "2000", NULL });
	te_randpos(OF_DEFAULT, as, 27, "1.234,5.6789", 2000.0, 2000.0,
	           "randpos: max_dist is equal to min_dist, stdout looks ok");

	as = (chp{ lst_execname, "-F", "gpx", "--count", "21", "randpos",
	           "90,0", "7741", "7777", NULL });
	te_randpos(OF_GPX, as, 21, "90,0", 7741, 7777,
	           "randpos, North Pole, 25 pos as GPX, dists swapped");

	as = (chp{ lst_execname, "--count", "33", "randpos", "90,0", "10000",
	           NULL });
	te_randpos(OF_DEFAULT, as, 33, "90,0", 0.0, 10000.0,
	           "randpos: Exactly at the North Pole");

	as = (chp{ lst_execname, "--count", "34", "randpos", "-90,0", "10001",
	           NULL });
	te_randpos(OF_DEFAULT, as, 34, "-90,0", 0.0, 10001.0,
	           "randpos: Exactly at the South Pole");
}

/*
 * test_randpos_dist() - Tests the randpos command with maximum and minimum 
 * distance. Returns nothing.
 */

static void test_randpos_dist(void)
{
	test_randpos_dist_max();
	test_randpos_dist_minmax();
}

/*
 * test_cmd_randpos() - Tests the randpos command. Returns nothing.
 */

static void test_cmd_randpos(void)
{
	int res;
	struct lst_streams ss;
	double lat, lon;
	char **as;

	lst_diag("Test randpos command");

	tc((chp{ lst_execname, "randpos", "1,2", "100", "90", "5", NULL }),
	   "",
	   EXECSTR ": Too many arguments\n",
	   EXIT_FAILURE,
	   "randpos with 1 extra argument");

	lst_streams_init(&ss);
	lst_streams_exec(&ss, (chp{ lst_execname, "randpos", NULL }));
	lat = lon = 0;
	res = parse_coordinate(ss.out.buf, true, &lat, &lon);
	OK_SUCCESS(res, "randpos: Coordinate is valid");
	OK_TRUE(fabs(lat) <= 90, "randpos: lat is in range");
	OK_TRUE(fabs(lat) <= 180, "randpos: lon is in range");
	lst_streams_free(&ss);

	as = (chp{ lst_execname, "--count", "5", "randpos", NULL });
	te_randpos(OF_DEFAULT, as, 5, NULL, 0, 0,
	           "--count 5 randpos, stdout is ok");

	as = (chp{ lst_execname, "-F", "gpx", "--count", "9", "randpos", NULL });
	te_randpos(OF_GPX, as, 9, NULL, 0, 0,
	           "-F gpx --count 9 randpos, stdout is ok");

	lst_diag("--count with invalid argument");
	tc((chp{ lst_execname, "--count", "", "randpos", NULL }),
	   "",
	   EXECSTR ": : Invalid --count argument\n"
	   OPTION_ERROR_STR,
	   EXIT_FAILURE,
	   "Empty argument to --count");
	tc((chp{ lst_execname, "--count", "g", "randpos", NULL }),
	   "",
	   EXECSTR ": g: Invalid --count argument\n"
	   OPTION_ERROR_STR,
	   EXIT_FAILURE,
	   "--count receives non-number");
	tc((chp{ lst_execname, "--count", "11y", "randpos", NULL }),
	   "",
	   EXECSTR ": 11y: Invalid --count argument\n"
	   OPTION_ERROR_STR,
	   EXIT_FAILURE,
	   "--count 11y");
	tc((chp{ lst_execname, "--count", "11.3", "randpos", NULL }),
	   "",
	   EXECSTR ": 11.3: Invalid --count argument\n"
	   OPTION_ERROR_STR,
	   EXIT_FAILURE,
	   "--count 11.3");
	tc((chp{ lst_execname, "--count", "-2", "randpos", NULL }),
	   "",
	   EXECSTR ": -2: Invalid --count argument\n"
	   OPTION_ERROR_STR,
	   EXIT_FAILURE,
	   "--count -2");

	lst_diag("--count 0");
	tc((chp{ lst_execname, "--count", "0", "randpos", NULL }),
	   "",
	   "",
	   EXIT_SUCCESS,
	   "--count 0");
	tc((chp{ lst_execname, "-F", "gpx", "--count", "0", "randpos", NULL }),
	   GPX_HEADER "</gpx>\n",
	   "",
	   EXIT_SUCCESS,
	   "-F gpx --count 0");
	tc((chp{ lst_execname, "-F", "sql", "--count", "10", "--seed", "1000",
	         "randpos", "20,20", NULL }),
	   "BEGIN;\n"
	   "CREATE TABLE IF NOT EXISTS randpos (seed INTEGER, num INTEGER, lat REAL, lon REAL, dist REAL, bear REAL);\n"
	   "INSERT INTO randpos VALUES (1000, 1, 71.13329, -57.468159, 7458038.290706, 339.955948);\n"
	   "INSERT INTO randpos VALUES (1000, 2, 4.810956, -39.143396, 6605679.37255, 263.597432);\n"
	   "INSERT INTO randpos VALUES (1000, 3, 31.762576, 91.243012, 7126548.517582, 63.514344);\n"
	   "INSERT INTO randpos VALUES (1000, 4, -55.576794, -166.832537, 16018000.848656, 173.421528);\n"
	   "INSERT INTO randpos VALUES (1000, 5, -3.788815, 152.508528, 14567412.13215, 77.097274);\n"
	   "INSERT INTO randpos VALUES (1000, 6, 12.619138, 169.426644, 15080928.93906, 45.214445);\n"
	   "INSERT INTO randpos VALUES (1000, 7, 32.292105, -106.309325, 11866448.651468, 314.66247);\n"
	   "INSERT INTO randpos VALUES (1000, 8, 65.299565, 16.899777, 5042230.856845, 358.179425);\n"
	   "INSERT INTO randpos VALUES (1000, 9, -4.673171, -44.745272, 7581105.91409, 256.164898);\n"
	   "INSERT INTO randpos VALUES (1000, 10, 20.935854, 18.624757, 177068.647935, 306.231896);\n"
	   "COMMIT;\n",
	   "",
	   EXIT_SUCCESS,
	   "-F sql with coordinate and no distances");

	test_randpos_dist();

	lst_diag("randpos with max_dist, invalid arguments");

	tc((chp{ lst_execname, "randpos", "12.34,56.34y", "10", NULL }),
	   "",
	   EXECSTR ": 12.34,56.34y: Invalid coordinate: Invalid argument\n",
	   EXIT_FAILURE,
	   "randpos with error in coordinate");

	tc((chp{ lst_execname, "randpos", "12.34,56.34", "10y", NULL }),
	   "",
	   EXECSTR ": 10y: Invalid max_dist argument: Invalid argument\n",
	   EXIT_FAILURE,
	   "randpos with error in max_dist");

	tc((chp{ lst_execname, "randpos", "12.34,56.34", "-17.9", NULL }),
	   "",
	   EXECSTR ": Distance cannot be negative\n",
	   EXIT_FAILURE,
	   "randpos with negative max_dist");

	lst_diag("randpos with max_dist and min_dist, invalid arguments");

	tc((chp{ lst_execname, "randpos", "12.34,56.34", "10", "3y", NULL }),
	   "",
	   EXECSTR ": 3y: Invalid min_dist argument: Invalid argument\n",
	   EXIT_FAILURE,
	   "randpos with error in min_dist");

	tc((chp{ lst_execname, "randpos", "12.34,56.34", "9", "-2", NULL }),
	   "",
	   EXECSTR ": Distance cannot be negative\n",
	   EXIT_FAILURE,
	   "randpos with negative min_dist");

	tc((chp{ lst_execname, "--karney", "randpos", "1,2", "200", "100", NULL }),
	   "",
	   EXECSTR ": -K/--karney is not supported by the randpos command\n",
	   EXIT_FAILURE,
	   "--karney randpos");

	lst_diag("--format sql randpos");
	tc((chp{ lst_execname, "--format", "sql", "--seed", "19", "--count", "20",
	         "randpos", NULL }),
	   "BEGIN;\n"
	   "CREATE TABLE IF NOT EXISTS randpos (seed INTEGER, num INTEGER, lat REAL, lon REAL, dist REAL, bear REAL);\n"
	   "INSERT INTO randpos VALUES (19, 1, 25.603688, -130.636512, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 2, -48.27306, 77.529775, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 3, -17.1173, 140.106483, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 4, -52.240484, -115.036322, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 5, 3.344781, 17.095447, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 6, 16.755787, 9.521758, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 7, -19.490223, 12.660125, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 8, 69.69621, 156.752235, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 9, 52.694091, 90.355201, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 10, -6.44931, -117.03235, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 11, 13.210432, -169.948761, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 12, -22.225278, 129.713413, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 13, -1.595068, -106.917102, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 14, -22.137051, -55.379762, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 15, 0.672918, -35.768761, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 16, 8.184838, -1.136274, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 17, -57.901507, 155.489707, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 18, 14.890397, 131.859757, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 19, 31.066093, -2.976285, NULL, NULL);\n"
	   "INSERT INTO randpos VALUES (19, 20, 31.66207, 0.677547, NULL, NULL);\n"
	   "COMMIT;\n",
	   "",
	   EXIT_SUCCESS,
	   "--format sql randpos without coordinate or dists");
	tc((chp{ lst_execname, "-F", "sql", "--seed", "19", "--count", "20",
	         "randpos", "1,2", "200", "100", NULL }),
	   "BEGIN;\n"
	   "CREATE TABLE IF NOT EXISTS randpos (seed INTEGER, num INTEGER, lat REAL, lon REAL, dist REAL, bear REAL);\n"
	   "INSERT INTO randpos VALUES (19, 1, 0.999739, 1.998795, 137.029826, 257.785884);\n"
	   "INSERT INTO randpos VALUES (19, 2, 1.00116, 2.001187, 184.578986, 45.661444);\n"
	   "INSERT INTO randpos VALUES (19, 3, 0.998948, 2.001395, 194.296589, 127.020796);\n"
	   "INSERT INTO randpos VALUES (19, 4, 1.001014, 2.000784, 142.479955, 37.694182);\n"
	   "INSERT INTO randpos VALUES (19, 5, 0.998461, 1.999715, 173.992388, 190.501973);\n"
	   "INSERT INTO randpos VALUES (19, 6, 0.999042, 1.998779, 172.556828, 231.892738);\n"
	   "INSERT INTO randpos VALUES (19, 7, 0.999223, 2.00135, 173.15511, 119.94372);\n"
	   "INSERT INTO randpos VALUES (19, 8, 1.001736, 1.999657, 196.717262, 348.815877);\n"
	   "INSERT INTO randpos VALUES (19, 9, 1.001344, 1.998994, 186.659487, 323.173976);\n"
	   "INSERT INTO randpos VALUES (19, 10, 0.998803, 2.000441, 141.822259, 159.781653);\n"
	   "INSERT INTO randpos VALUES (19, 11, 0.999209, 1.999309, 116.709311, 221.135064);\n"
	   "INSERT INTO randpos VALUES (19, 12, 0.999353, 2.001608, 192.753169, 111.915138);\n"
	   "INSERT INTO randpos VALUES (19, 13, 0.9987, 2.000114, 145.056415, 174.989594);\n"
	   "INSERT INTO randpos VALUES (19, 14, 0.999461, 2.001323, 158.835986, 112.1718);\n"
	   "INSERT INTO randpos VALUES (19, 15, 0.998532, 1.999946, 163.296314, 182.113987);\n"
	   "INSERT INTO randpos VALUES (19, 16, 0.998618, 1.999337, 170.48714, 205.626063);\n"
	   "INSERT INTO randpos VALUES (19, 17, 1.001568, 2.000817, 196.535789, 27.515539);\n"
	   "INSERT INTO randpos VALUES (19, 18, 0.998799, 1.998745, 193.074008, 226.254748);\n"
	   "INSERT INTO randpos VALUES (19, 19, 1.000077, 1.998472, 170.123644, 272.884771);\n"
	   "INSERT INTO randpos VALUES (19, 20, 1.00012, 1.998468, 170.843636, 274.483494);\n"
	   "COMMIT;\n",
	   "",
	   EXIT_SUCCESS,
	   "-F sql randpos with maxdist and mindist");
}

#undef te_randpos

/******************************************************************************
              Test the executable file with a temporary directory
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

	/* geocalc.c */
	test_myerror();

	result = rmdir(LST_TMPDIR);
	OK_SUCCESS(result, "rmdir " LST_TMPDIR " after function tests");
	if (result) {
		diag_errno(); /* gncov */
		return; /* gncov */
	}
}

/*
 * tests_with_tempdir() - Executes tests on the executable with a temporary 
 * directory. The temporary directory uses a standard name, defined in 
 * LST_TMPDIR. If the directory already exists or it's unable to create it, it 
 * aborts. Returns nothing.
 */

static void tests_with_tempdir(void)
{
	int result;

	result = mkdir(LST_TMPDIR, 0777);
	OK_SUCCESS(result, "%s(): Create temporary directory %s",
	                   __func__, LST_TMPDIR);
	if (result) {
		lst_diag("Cannot create directory \"%s\", skipping" /* gncov */
		         " tests: %s",
		         LST_TMPDIR, strerror(errno)); /* gncov */
		errno = 0; /* gncov */
		return; /* gncov */
	}

	if (OK_SUCCESS(rmdir(LST_TMPDIR),
	               "Delete temporary directory %s", LST_TMPDIR))
		diag_errno(); /* gncov */
}

/******************************************************************************
                        Top-level --selftest functions
******************************************************************************/

/*
 * test_functions() - Tests various functions directly. Returns nothing.
 */

static void test_functions(const struct Options *o)
{
	assert(o);

	if (!o->testfunc)
		return; /* gncov */

	/* tests.c */
	verify_constants();

	lst_diag("Test various routines");

	/* geocalc.c */
	test_std_strerror();

	/* cmds.c */
	test_round_number();

	/* geomath.c */
	test_are_antipodal();
	test_bearing_position();
	test_karney_distance();
	test_karney_bearing();
	test_rand_pos();

	/* gpx.c */
	test_xml_escape_string();
	test_gpx_wpt();

	/* strings.c */
	test_trim_zeros();
	test_mystrdup();
	test_allocstr();
	test_parse_coordinate();

	functests_with_tempdir();
}

/*
 * test_executable() - Runs various tests with the executable and verifies that 
 * stdout, stderr and the return value are as expected. Returns nothing.
 */

static void test_executable(const struct Options *o)
{
	assert(o);
	if (!o->testexec)
		return; /* gncov */

	lst_diag("Test the executable");
	test_valgrind_option();
	print_version_info();
	tc((chp{ lst_execname, NULL }),
	   "",
	   EXECSTR ": No arguments specified\n" TYPE_HELP_STR,
	   EXIT_FAILURE,
	   "No arguments or options");
	tc((chp{ lst_execname, "abc", NULL }),
	   "",
	   EXECSTR ": Unknown command: abc\n",
	   EXIT_FAILURE,
	   "Unknown command");
	test_standard_options();
	test_format_option();
	test_haversine_option();
	test_karney_option();
	test_seed_option();
	test_cmd_anti();
	test_cmd_bench();
	test_cmd_bpos();
	test_cmd_course();
	test_cmd_lpos();
	test_multiple(__FILE__, __LINE__, "bear");
	test_multiple(__FILE__, __LINE__, "dist");
	test_cmd_randpos();
	tests_with_tempdir();
	print_version_info();
}

/*
 * opt_selftest() - Runs internal testing to check that it works on the current 
 * system. Executed if --selftest is used. Returns `EXIT_FAILURE` if any tests 
 * fail. Otherwise, it returns `EXIT_SUCCESS`.
 */

int opt_selftest(char *main_execname, const struct Options *o)
{
	assert(main_execname);
	assert(*main_execname);
	assert(o);

	lst_execname = get_execname(main_execname);
	lst_diag("Running tests for %s %s (%s)",
	         lst_execname, EXEC_VERSION, EXEC_DATE);

	if (init_testvars())
		return EXIT_FAILURE; /* gncov */
	test_selftest_funcs();
	test_functions(o);
	test_executable(o);
	print_selftest_footer();
	free(lst_execname);

	return selftest_failcount() ? EXIT_FAILURE : EXIT_SUCCESS;
}

/* vim: set ts=8 sw=8 sts=8 noet fo+=w tw=79 fenc=UTF-8 : */
