#pragma once

#include <stdint.h>

typedef uint32_t Time;

// The % operator has strange behavior with negative inputs
// This mod function compensates for negatives
int32_t mod(int32_t a, int32_t b);

// Get the smallest difference (normalized to -half_period and +half_period)
// between a source and target angle in modular arithmetic
int32_t angle_diff(int32_t source, int32_t target, int32_t period);

// Return 1 if input is positive
// 0 if input is zero
// -1 if input is negative
int32_t unit_direction(int32_t);