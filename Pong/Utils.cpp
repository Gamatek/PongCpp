#include "Utils.h"

double clampd(double v, double min, double max) {
    const double t = v < min ? min : v;
    return t > max ? max : t;
};