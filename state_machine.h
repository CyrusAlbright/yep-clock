/*
	A custom state machine implementation that handles scheduling and manual PWM.

	Inputs:
		now:
			Current timestamp
		unit_difference:
			-1 if it needs to move backwards, 1 if it needs to move forwards, 0 otherwise

	All state transitions:
		If it is in the Idle state and it needs to move:
			It will start moving by switching to the PulseOn state and scheduling the next state transition
		If it is in the PulseOn state and the next state transition is now:
			It will switch to PulseOff and schedule the next state transition, in addition to adjusting the current step
		If it is in the PulseOff state and it no longer needs to move:
			It will switch back to the Idle state
		If it is in the PulseOff state and its next state transition is now:
			It will switch back to the PulseOn state and schedule the next state transition

	Outputs:
		activate_coils:
			Whether or not to activate the coils this instant
		steps_increment:
			Adjusts the current step on a state transition from PulseOn to PulseOff
*/

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