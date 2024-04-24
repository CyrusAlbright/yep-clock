#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "utils.h"

typedef enum {
	Idle,
	PulseOn,
	PulseOff
} State;

typedef struct {
	State state;
	Time next_tick;
	Time on_time;
	Time off_time;
} StateMachine;

typedef struct {
	Time now;
	int32_t unit_difference;
} StateMachineInput;

typedef struct {
	int32_t steps_increment;
	bool activate_coils;
} StateMachineOutput;

StateMachineOutput state_machine_tick(StateMachine* state_machine, StateMachineInput input);