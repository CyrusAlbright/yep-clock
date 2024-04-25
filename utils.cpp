#include "utils.h"

int32_t mod(int32_t a, int32_t b) {
	return (a % b + b) % b;
}

int32_t angle_diff(int32_t source, int32_t target, int32_t period) {
	
	int32_t half_period = period / 2;
	return mod(target - source + half_period, period) - half_period;
}

int32_t unit_direction(int32_t input) {
	return (input > 0) ? 1 : ((input < 0) ? -1 : 0);
}