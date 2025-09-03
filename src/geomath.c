/*
 * geomath.c
 * File ID: 3a546558-895c-11ef-9911-83850402c3ce
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

const double EARTH_RADIUS = 6371000; /* Meters */
const double MAX_EARTH_DISTANCE = 20015086.79602057114243507385; /* Meters */

static const double DEG_TO_RAD = M_PI / 180.0;
static const double RAD_TO_DEG = 180.0 / M_PI;
#define deg2rad(a)  ((a) * DEG_TO_RAD)
#define rad2deg(a)  ((a) * RAD_TO_DEG)

/*
 * are_antipodal() - Checks if two points are antipodal, i.e. on exactly 
 * opposite positions of a spherical planet. To account for rounding errors, a 
 * small margin (≈0.01 mm) is allowed. Returns 1 if they're antipodal, 
 * otherwise 0.
 */

int are_antipodal(const double lat1, const double lon1,
                  const double lat2, const double lon2)
{
	const double eps = 1e-10;

	/* Check if points are at opposite poles */
	if (fabs(lat1 - 90.0) < eps && fabs(lat2 + 90.0) < eps)
		return 1;
	if (fabs(lat1 + 90.0) < eps && fabs(lat2 - 90.0) < eps)
		return 1;

	/* Check other antipodal points */
	if (fabs(lat1 + lat2) < eps && fabs(fabs(lon1 - lon2) - 180.0) < eps)
		return 1;

	return 0;
}

/*
 * normalize_longitude() - Normalizes a longitude value to the range 
 * [-180,180], adjusts the given longitude to make sure it falls within -180 to 
 * 180 degrees. If the input is already within this range, no change is made.
 *
 * Parameters:
 * - lon: Pointer to the longitude value to be normalized (in degrees).
 */

static void normalize_longitude(double *lon)
{
	assert(lon);
	assert(isfinite(*lon) && "Invalid longitude");

	if (fabs(*lon) <= 180.0)
		return;
	*lon = fmod(*lon, 360.0);
	if (*lon > 180.0)
		*lon -= 360.0;
	else if (*lon <= -180.0)
		*lon += 360.0;
}

/*
 * set_antipode() - Sets the coordinate `dlat,dlon` to the antipodal position 
 * on Earth. By repeating the process the original values are back. Returns 
 * nothing.
 */

void set_antipode(double *dlat, double *dlon)
{
	assert(dlat);
	assert(dlon);

	*dlat *= -1.0;
	if (fabs(*dlat) == 90.0) {
		*dlon = 0.0;
	} else {
		*dlon += 180.0;
		normalize_longitude(dlon);
	}
}

/*
 * bearing_position() - Calculates the new geographic position after moving 
 * `dist_m` meters from the position `lat,lon` in the direction `bearing_deg` 
 * (where north is 0, south is 180). The new coordinate is stored at memory 
 * locations pointed to by `new_lat` and `new_lon`.
 *
 * Negative values for `dist_m` are allowed, to calculate positions in the 
 * opposite direction of `bearing_deg`.
 *
 * For exact pole positions (lat = ±90°), the latitude is adjusted by ≈1 cm to 
 * avoid computational instability.
 *
 * If the provided values are outside the valid coordinate range, it returns 1. 
 * Otherwise, it returns 0.
 */

int bearing_position(const double lat, const double lon,
                     const double bearing_deg, const double dist_m,
                     double *new_lat, double *new_lon)
{
	double lat_a = lat;

	assert(new_lat);
	assert(new_lon);

	if (fabs(lat) > 90.0 || fabs(lon) > 180.0
	    || bearing_deg < 0.0 || bearing_deg > 360.0) {
		return 1;
	}

	if (fabs(lat_a) == 90.0)
		lat_a *= 1.0 - 1e-9;

	const double lat_rad = deg2rad(lat_a);
	const double lon_rad = deg2rad(lon);
	const double bearing_rad = deg2rad(bearing_deg);
	const double ang_dist = dist_m / EARTH_RADIUS;

	const double sin_lat = sin(lat_rad);
	const double cos_lat = cos(lat_rad);
	const double sin_ang_dist = sin(ang_dist);
	const double cos_ang_dist = cos(ang_dist);

	const double lat2_rad = asin(sin_lat * cos_ang_dist
	                             + cos_lat * sin_ang_dist
	                               * cos(bearing_rad));

	const double lon2_rad = lon_rad
	                        + atan2(sin(bearing_rad) * sin_ang_dist
	                                * cos_lat,
	                                cos_ang_dist
	                                - sin_lat * sin(lat2_rad));

	*new_lat = rad2deg(lat2_rad);
	*new_lon = rad2deg(lon2_rad);
	normalize_longitude(new_lon);

	return 0;
}

/*
 * haversine() - Calculates great-circle distance between two geographic 
 * coordinates.
 *
 * Parameters:
 * - lat1, lon1 - First position's latitude and longitude in decimal degrees
 * - lat2, lon2 - Second position's latitude and longitude in decimal degrees
 *
 * Returns:
 * - Distance in meters between the points.
 * - MAX_EARTH_DISTANCE if points are antipodal.
 * - -1.0 if coordinates are outside valid ranges (-90° to 90° for latitude,
 *   -180° to 180° for longitude).
 */

double haversine(const double lat1, const double lon1,
                 const double lat2, const double lon2)
{
	if (fabs(lat1) > 90.0 || fabs(lat2) > 90.0
	    || fabs(lon1) > 180.0 || fabs(lon2) > 180.0)
		return -1.0;

	const double lat1_rad = deg2rad(lat1);
	const double lat2_rad = deg2rad(lat2);
	const double delta_phi = deg2rad(lat2 - lat1);
	const double delta_lambda = deg2rad(lon2 - lon1);

	const double sin_delta_phi = sin(delta_phi / 2.0);
	const double sin_delta_lambda = sin(delta_lambda / 2.0);

	const double hav = sin_delta_phi * sin_delta_phi
	                   + cos(lat1_rad) * cos(lat2_rad)
	                   * sin_delta_lambda * sin_delta_lambda;

	const double arc = 2.0 * atan2(sqrt(hav), sqrt(1.0 - hav));
	if (isnan(arc)) {
		/* Antipodal positions */
		errno = 0;
		return MAX_EARTH_DISTANCE;
	}

	return EARTH_RADIUS * arc; /* Distance in meters */
}

/*
 * karney_distance() - Calculates the distance between 2 locations, using the 
 * Karney formula. This formula models the Earth as an ellipsoid and provides 
 * significantly higher accuracy than the default Haversine formula, which 
 * assumes a spherical Earth. It achieves an accuracy of 10-15 nanometers for 
 * distance calculations, making it suitable for high-precision applications.
 * Returns the distance in meters.
 */

double karney_distance(const double lat1, const double lon1,
                       const double lat2, const double lon2)
{
	if (fabs(lat1) > 90.0 || fabs(lat2) > 90.0
	    || fabs(lon1) > 180.0 || fabs(lon2) > 180.0)
		return -1.0;

	const double a = 6378137.0;
	const double f = 1.0 / 298.257223563;
	const double b = (a * (1.0 - f));
	const double L = deg2rad(lon2) - deg2rad(lon1);
	const double U1 = atan((1.0 - f) * tan(deg2rad(lat1)));
	const double U2 = atan((1.0 - f) * tan(deg2rad(lat2)));
	const double sinU1 = sin(U1), cosU1 = cos(U1);
	const double sinU2 = sin(U2), cosU2 = cos(U2);

	double lambda = L, lambdaP;
	double sin_lambda, cos_lambda, sin_sigma, cos_sigma, sigma, sin_alpha,
	       cos_sq_alpha, cos2_sigma_m;
	int iter_limit = 100;

	do {
		double C;

		sin_lambda = sin(lambda);
		cos_lambda = cos(lambda);
		sin_sigma = sqrt((cosU2 * sin_lambda) * (cosU2 * sin_lambda)
		                 + (cosU1 * sinU2 - sinU1 * cosU2 * cos_lambda)
		                   * (cosU1 * sinU2
		                      - sinU1 * cosU2 * cos_lambda));

		if (sin_sigma == 0.0)
			return 0; /* Coincident points */

		cos_sigma = sinU1 * sinU2 + cosU1 * cosU2 * cos_lambda;
		sigma = atan2(sin_sigma, cos_sigma);
		sin_alpha = cosU1 * cosU2 * sin_lambda / sin_sigma;
		cos_sq_alpha = 1.0 - sin_alpha * sin_alpha;
		cos2_sigma_m = cos_sigma - 2.0 * sinU1 * sinU2 / cos_sq_alpha;

		if (isnan(cos2_sigma_m))
			cos2_sigma_m = 0.0; /* Equatorial lines */

		C = f / 16.0 * cos_sq_alpha
		    * (4.0 + f * (4.0 - 3.0 * cos_sq_alpha));
		lambdaP = lambda;
		lambda = L + (1.0 - C) * f * sin_alpha
		             * (sigma
		                + C * sin_sigma
		                  * (cos2_sigma_m
		                     + C * cos_sigma
		                       * (-1.0 + 2.0 * cos2_sigma_m
		                                 * cos2_sigma_m)));
	} while (fabs(lambda - lambdaP) > 1e-12 && --iter_limit > 0.0);

	if (iter_limit == 0)
		return nan(""); /* The formula did not converge */

	const double u_sq = cos_sq_alpha * (a * a - b * b) / (b * b);
	const double A = 1.0 + u_sq / 16384.0
	                       * (4096.0
	                          + u_sq * (-768.0 + u_sq * (320.0
	                                                     - 175.0 * u_sq)));
	const double B = u_sq / 1024.0
	                 * (256.0 + u_sq * (-128.0 + u_sq * (74.0
	                                                     - 47.0 * u_sq)));
	const double delta_sigma = B * sin_sigma
	                           * (cos2_sigma_m
	                              + B / 4.0 * (cos_sigma
	                                           * (-1.0 + 2.0 * cos2_sigma_m
	                                                     * cos2_sigma_m)
	                                           - B / 6.0 * cos2_sigma_m
	                                             * (-3.0
	                                                + 4.0 * sin_sigma
	                                                  * sin_sigma)
	                                             * (-3.0
	                                                + 4.0 * cos2_sigma_m
	                                                  * cos2_sigma_m)));

	return b * A * (sigma - delta_sigma);
}

/*
 * karney_bearing() - Calculates the initial bearing from point (lat1, lon1) to 
 * point (lat2, lon2) using Karney's method on the WGS84 ellipsoid.
 * Returns bearing in degrees, 0 to 360.
 *
 * Antipodal points, including pole-to-pole (e.g., 90,0 to -90,0), return -2.0 
 * because the initial bearing is undefined due to multiple valid paths (all 
 * meridians). This is also the case with coincident points. It's hard to tell 
 * which direction to go when you're already there.
 *
 * Parameters:
 * - lat1, lon1 - First position's latitude and longitude in decimal degrees
 * - lat2, lon2 - Second position's latitude and longitude in decimal degrees
 *
 * Returns:
 * - Initial bearing in degrees
 * - -1.0 if values outside the valid coordinate range are provided
 * - -2.0 if points are antipodal, coincident, or calculation fails
 */

double karney_bearing(const double lat1, const double lon1,
                      const double lat2, const double lon2)
{
	double lat1_rad, lat2_rad;
	const double f = 1.0 / 298.257223563, /* WGS84 flattening */
	             eps_deg = 1e-10, eq_eps = 1e-13;
	double lambda, lambda_prev, sin_lambda, cos_lambda, sin_sigma,
	       cos_sigma, sigma, sin_alpha, cos_sq_alpha, cos2_sigma_m;
	int iter_limit;

	if (fabs(lat1) > 90.0 || fabs(lat2) > 90.0 || fabs(lon1) > 180.0
	    || fabs(lon2) > 180.0)
		return -1.0;

	lat1_rad = deg2rad(lat1);
	lat2_rad = deg2rad(lat2);

	double dlon_deg = fmod((lon2 - lon1) + 540.0, 360.0) - 180.0;
	const double L = deg2rad(dlon_deg);

	/* Coincident points */
	if (fabs(lat1 - lat2) < eps_deg && fabs(dlon_deg) < eps_deg)
		return -2.0;

	if ((fabs(lat1 - 90.0) < eps_deg && fabs(lat2 - 90.0) < eps_deg)
	    || (fabs(lat1 + 90.0) < eps_deg && fabs(lat2 + 90.0) < eps_deg)) {
		return -2.0; /* Same pole */
	}

	/* Antipodal points */
	if (are_antipodal(lat1, lon1, lat2, lon2))
		return -2.0;

	if (fabs(lat1) < eq_eps && fabs(lat2) < eq_eps)
		return (dlon_deg > 0.0) ? 90.0 : 270.0;

	/* Vincenty/Karney inspired iteration (robust for non-antipodal) */
	const double U1 = atan((1.0 - f) * tan(lat1_rad));
	const double U2 = atan((1.0 - f) * tan(lat2_rad));
	const double sinU1 = sin(U1), cosU1 = cos(U1);
	const double sinU2 = sin(U2), cosU2 = cos(U2);

	lambda = L;
	iter_limit = 100;

	do {
		double C;

		sin_lambda = sin(lambda);
		cos_lambda = cos(lambda);

		const double t1 = cosU2 * sin_lambda;
		const double t2 = cosU1 * sinU2 - sinU1 * cosU2 * cos_lambda;

		sin_sigma = sqrt(t1 * t1 + t2 * t2);

		cos_sigma = sinU1 * sinU2 + cosU1 * cosU2 * cos_lambda;
		sigma = atan2(sin_sigma, cos_sigma);
		sin_alpha = cosU1 * cosU2 * sin_lambda / sin_sigma;
		cos_sq_alpha = 1.0 - sin_alpha * sin_alpha;

		if (cos_sq_alpha != 0.0) {
			cos2_sigma_m = cos_sigma
			               - 2.0 * sinU1 * sinU2 / cos_sq_alpha;
		} else {
			cos2_sigma_m = 0.0;
		}

		C = f / 16.0 * cos_sq_alpha
		    * (4.0 + f * (4.0 - 3.0 * cos_sq_alpha));
		lambda_prev = lambda;
		lambda = L + (1.0 - C) * f * sin_alpha
		             * (sigma + C * sin_sigma
		                        * (cos2_sigma_m
		                           + C * cos_sigma
		                             * (-1.0 + 2.0 * cos2_sigma_m
		                                       * cos2_sigma_m)));
	} while (fabs(lambda - lambda_prev) > 1e-11 && --iter_limit > 0);

	if (iter_limit == 0)
		return -2.0; /* Formula did not converge */

	const double alpha1_rad = atan2(cosU2 * sin_lambda,
	                                cosU1 * sinU2
	                                - sinU1 * cosU2 * cos_lambda);

	return fmod(rad2deg(alpha1_rad) + 360.0, 360.0);
}

/*
 * distance() - Calculates the distance between 2 locations with the formula 
 * specified in `formula`. Returns the distance in meters.
 */

double distance(const DistFormula formula,
                const double lat1, const double lon1,
                const double lat2, const double lon2)
{
	switch (formula) {
	case FRM_HAVERSINE:
		return haversine(lat1, lon1, lat2, lon2);
	case FRM_KARNEY:
		return karney_distance(lat1, lon1, lat2, lon2);
	default: /* gncov */
		myerror("%s() received unknown formula %d", /* gncov */
		        __func__, formula);
		return nan(""); /* gncov */
	}
}

/*
 * initial_bearing() - Calculates the initial bearing from point `lat1, lon1` 
 * to point `lat2, lon2`. Returns bearing in degrees: 0 = north, 90 = east, 180 
 * = south, 270 = west.
 *
 * Returns:
 * - -1.0 if values outside the valid coordinate range are provided
 * - -2.0 if points are antipodal or coincident, answer is undefined
 */

double initial_bearing(const double lat1, const double lon1,
                       const double lat2, const double lon2)
{
	if (fabs(lat1) > 90.0 || fabs(lat2) > 90.0
	    || fabs(lon1) > 180.0 || fabs(lon2) > 180.0)
		return -1.0;

	if (are_antipodal(lat1, lon1, lat2, lon2)
	    || (lat1 == lat2 && lon1 == lon2))
		return -2.0;

	const double lat1_rad = deg2rad(lat1);
	const double lat2_rad = deg2rad(lat2);
	const double delta_lon = deg2rad(lon2 - lon1);

	const double cos_lat1 = cos(lat1_rad);
	const double cos_lat2 = cos(lat2_rad);

	const double y = sin(delta_lon) * cos_lat2;
	const double x = cos_lat1 * sin(lat2_rad)
	                 - sin(lat1_rad) * cos_lat2 * cos(delta_lon);

	return fmod(rad2deg(atan2(y, x)) + 360.0, 360.0);
}

/*
 * bearing() - Calculates the initial bearing at position `lat1,lon1` towards 
 * position `lat2,lon2` using the distance formula in `formula`. Returns the 
 * compass direction as a value between 0 and 360 where north is 0.
 */

double bearing(const DistFormula formula,
               const double lat1, const double lon1,
               const double lat2, const double lon2)
{
	switch (formula) {
	case FRM_HAVERSINE:
		return initial_bearing(lat1, lon1, lat2, lon2);
	case FRM_KARNEY:
		return karney_bearing(lat1, lon1, lat2, lon2);
	default: /* gncov */
		myerror("%s() received unknown formula %d", /* gncov */
		        __func__, formula);
		return nan(""); /* gncov */
	}
}

/*
 * rand_pos() - Generates a random position on Earth with optional distance 
 * constraints.
 *
 * If `c_lat` is larger than 90, generate a random location somewhere on Earth. 
 * Otherwise, the position is between `mindist` and `maxdist` meters from 
 * `c_lat,c_lon`.
 *
 * Parameters:
 * - dlat:    Pointer to store resulting latitude (-90 to 90 degrees)
 * - dlon:    Pointer to store resulting longitude (-180 to 180 degrees)
 * - c_lat:   Center position latitude (-90 to 90 degrees)
 * - c_lon:   Center position longitude (-180 to 180 degrees)
 * - maxdist: Maximum distance in meters from center
 * - mindist: Minimum distance in meters from center
 *
 * Returns nothing.
 */

void rand_pos(double *dlat, double *dlon,
              const double c_lat_p, const double c_lon_p,
              const double maxdist_p, const double mindist_p)
{
	double c_lat = c_lat_p, c_lon = c_lon_p,
	       maxdist = maxdist_p, mindist = mindist_p,
	       rand_bear, rand_dist, result;

	assert(dlat);
	assert(dlon);

	if (c_lat > 90.0 || (maxdist == 0.0 && mindist == 0.0)) {
		/* No center coordinate or distances, use the whole world */
		*dlat = rad2deg(asin(2.0 * drand48() - 1.0));
		*dlon = rad2deg(drand48() * 2.0 * M_PI) - 180.0;
		return;
	}

	if (mindist != 0.0 && maxdist == 0.0) {
		set_antipode(&c_lat, &c_lon);
		maxdist = MAX_EARTH_DISTANCE - mindist;
		mindist = 0.0;
	}
	if (mindist > maxdist) {
		double d = mindist;
		mindist = maxdist;
		maxdist = d;
	}
	do {
		rand_bear = drand48() * 360.0;
		rand_dist = mindist + sqrt(drand48()) * (maxdist - mindist);
		if (rand_dist > MAX_EARTH_DISTANCE)
			rand_dist = MAX_EARTH_DISTANCE;

		bearing_position(c_lat, c_lon, rand_bear, rand_dist,
		                 dlat, dlon);
		result = haversine(c_lat, c_lon, *dlat, *dlon);
	} while (mindist != maxdist && (result < mindist || result > maxdist));
}

/*
 * routepoint() - Calculates the position of a point on a straight line between 
 * `lat1, lon1` and `lat2, lon2`, where `fracdist` is a fraction that specifies 
 * how far along the line the point is, with 0 = start position, 1 = end 
 * position. `fracdist` can also take values below 0 or above 1 to calculate 
 * positions beyond `lat2, lon2` or in the opposite direction from `lat1, 
 * lon1`.
 *
 * See `bearing_position()` for information on return values.
 */

int routepoint(const double lat1, const double lon1,
               const double lat2, const double lon2,
               const double fracdist,
               double *next_lat, double *next_lon)
{
	assert(next_lat);
	assert(next_lon);

	return bearing_position(lat1, lon1,
	                        initial_bearing(lat1, lon1, lat2, lon2),
	                        haversine(lat1, lon1, lat2, lon2) * fracdist,
	                        next_lat, next_lon);
}

#undef deg2rad
#undef rad2deg

/* vim: set ts=8 sw=8 sts=8 noet fo+=w tw=79 fenc=UTF-8 : */
