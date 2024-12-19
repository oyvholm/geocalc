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

/*
 * are_antipodal() - Check if two points are antipodal, i.e. on exactly 
 * opposite positions of the planet. To account for rounding errors, a small 
 * margin (~1.1 mm) is allowed. Returns 1 if they're antipodal, otherwise 0.
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
 * deg2rad() - Convert degrees to radians.
 */

static inline double deg2rad(const double deg)
{
	return deg * M_PI / 180.0;
}

/*
 * rad2deg() - Convert radians to degrees.
 */

static inline double rad2deg(const double rad)
{
	return rad * 180.0 / M_PI;
}

/*
 * bearing_position() - Calculates the new geographic position after moving 
 * `dist_m` meters from the position `lat, lon` in the direction `bearing_deg` 
 * (where north is 0, south is 180). The new coordinate is stored at memory 
 * locations pointed to by `new_lat` and `new_lon`.
 *
 * Negative values for `dist_m` are allowed, to calculate positions in the 
 * opposite direction of `bearing_deg`.
 *
 * For exact pole positions (lat = ±90°), the latitude is adjusted by ~1 cm to 
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

	if (fabs(lat) > 90.0 || fabs(lon) > 180.0
	    || bearing_deg < 0.0 || bearing_deg > 360.0)
		return 1;

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

	return 0;
}

/*
 * haversine() - Calculate great-circle distance between two geographic 
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
	const double distance = EARTH_RADIUS * arc; /* Distance in meters */

	/* If `distance` is NaN, the positions are antipodal. */
	return isnan(distance) ? MAX_EARTH_DISTANCE : distance;
}

/*
 * initial_bearing() - Calculate the initial bearing from point `lat1, lon1` to 
 * point `lat2, lon2`. Returns bearing in degrees: 0 = north, 90 = east, 180 = 
 * south, 270 = west.
 *
 * Returns:
 * - -1.0: if values outside the valid coordinate range are provided
 * - -2.0: if points are antipodal, answer is undefined
 */
double initial_bearing(const double lat1, const double lon1,
                       const double lat2, const double lon2)
{
	if (fabs(lat1) > 90.0 || fabs(lat2) > 90.0
	    || fabs(lon1) > 180.0 || fabs(lon2) > 180.0)
		return -1.0;

	if (are_antipodal(lat1, lon1, lat2, lon2))
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
	return bearing_position(lat1, lon1,
	                        initial_bearing(lat1, lon1, lat2, lon2),
	                        haversine(lat1, lon1, lat2, lon2) * fracdist,
	                        next_lat, next_lon);
}

/* vim: set ts=8 sw=8 sts=8 noet fo+=w tw=79 fenc=UTF-8 : */
