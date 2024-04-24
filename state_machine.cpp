#include "state_machine.h"

StateMachineOutput state_machine_tick(StateMachine* state_machine, StateMachineInput input) {
	auto now = input.now;
	auto unit_difference = input.unit_difference;

	StateMachineOutput output = {0, false};

	switch (state_machine->state) {
		case Idle:
			if (unit_difference != 0) {
				state_machine->state = PulseOn;
				state_machine->next_tick = now + state_machine->on_time;
			}
			break;
		case PulseOn:
			if (state_machine->next_tick <= now) {
				state_machine->state = PulseOff;
				state_machine->next_tick = now + state_machine->off_time;
				output.steps_increment = unit_difference;
			}

			output.activate_coils = true;
			break;
		case PulseOff:
			if (unit_difference == 0) {
				state_machine->state = Idle;
				state_machine->next_tick = 0;
			} else if (state_machine->next_tick <= now) {
				state_machine->state = PulseOn;
				state_machine->next_tick = now + state_machine->on_time;
			}
			break;
	}

	return output;
}