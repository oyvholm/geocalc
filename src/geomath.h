/*
 * geomath.h
 * File ID: 3e516a7a-895c-11ef-a5f4-83850402c3ce
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

#ifndef _GEOMATH_H
#define _GEOMATH_H

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define HAVERSINE_DECIMALS  6
#define KARNEY_DECIMALS  8

typedef enum {
	FRM_HAVERSINE,
	FRM_KARNEY
} DistFormula;

extern const double MAX_EARTH_DISTANCE;

int are_antipodal(const double lat1, const double lon1,
                  const double lat2, const double lon2);
int bearing_position(const double lat, const double lon,
                     const double bearing_deg, const double dist_m,
                     double *new_lat, double *new_lon);
double haversine(const double lat1, const double lon1,
                 const double lat2, const double lon2);
double karney_distance(double lat1, double lon1, double lat2, double lon2);
double karney_bearing(const double lat1, const double lon1,
                      const double lat2, const double lon2);
double distance(const DistFormula formula,
                const double lat1, const double lon1,
                const double lat2, const double lon2);
double initial_bearing(const double lat1, const double lon1,
                       const double lat2, const double lon2);
double bearing(const DistFormula formula,
               const double lat1, const double lon1,
               const double lat2, const double lon2);
void rand_pos(double *dlat, double *dlon,
              const double c_lat, const double c_lon,
              const double maxdist, const double mindist);
int routepoint(const double lat1, const double lon1,
               const double lat2, const double lon2,
               const double fracdist,
               double *next_lat, double *next_lon);

#endif /* ifndef _GEOMATH_H */

/* vim: set ts=8 sw=8 sts=8 noet fo+=w tw=79 fenc=UTF-8 : */
