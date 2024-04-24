#pragma once

#include <stdint.h>

typedef uint32_t Time;

int32_t mod(int32_t a, int32_t b);

int32_t angle_diff(int32_t source, int32_t target, int32_t period);

int32_t unit_direction(int32_t);