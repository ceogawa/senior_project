#ifndef UTIL_H
#define UTIL_H

#include <cmath>
#include <cstdlib>
#include <limits>
#include <memory>

using std::shared_ptr;
using std::make_shared;
using std::sqrt;

//constants
const double pi = 3.141592653897932385;
const double infinity = std::numeric_limits<double>::infinity();
const double epsilon = 0.001;

inline double degToRad(double degrees) {
	return degrees * pi/180.0;
}

inline double niceRand() {
 return rand() / (RAND_MAX+1.0);
}

inline double nicerRand(double min, double max) {
 return min + (max-min)*niceRand();
}

inline double clamp(double x, double min, double max) {
  	if (x < min) return min;
  	if (x > max) return max;
	return x;
}

#endif
