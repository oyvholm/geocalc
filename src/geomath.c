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
 * haversine() - Return distance in meters between two geographic coordinates.
 *
 * If values outside the valid coordinate range are provided, it returns -1.0.
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

	return EARTH_RADIUS * arc; /* Distance in meters */
}

/*
 * initial_bearing() - Calculate the initial bearing from point `lat1, lon1` to 
 * point `lat2, lon2`. Returns bearing in degrees: 0 = north, 90 = east, 180 = 
 * south, 270 = west.
 *
 * If values outside the valid coordinate range are provided, it returns -1.0.
 */

double initial_bearing(const double lat1, const double lon1,
                       const double lat2, const double lon2)
{
	if (fabs(lat1) > 90.0 || fabs(lat2) > 90.0
	    || fabs(lon1) > 180.0 || fabs(lon2) > 180.0)
		return -1.0;

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

/* vim: set ts=8 sw=8 sts=8 noet fo+=w tw=79 fenc=UTF-8 : */
