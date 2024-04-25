// Further comments are in the included header files

#include "utils.h"
#include "state_machine.h"

// The amount of steps per revolution for the 28byj-48 stepper motor
constexpr uint32_t TOTAL_STEPS = 2048;

// Controls for the shift register
uint32_t data_pin = 4;
uint32_t clock_pin = 5;
uint32_t latch_pin = 6;

// All the information needed for a single stepper motor
typedef struct {
	int32_t desired_step;
	int32_t current_step;
	// on startup, the program has no way of knowing the current angle of the stepper motor
	// offset is a variable that can be manually adjusted so that the program's 0 can match the physical 0
	int32_t offset;
	StateMachine state_machine;
} StepperConfiguration;

// The "clock face" display
typedef struct {
	uint8_t minutes_tens;
	uint8_t minutes_ones;
	uint8_t seconds_tens;
	uint8_t seconds_ones;
} Display;

// All the motors to be used
StepperConfiguration steppers[] = {
	{0, 0, 0, {Idle, 0, 5, 2}},
	{0, 0, 0, {Idle, 0, 5, 2}},
	{0, 0, 0, {Idle, 0, 5, 2}},
	{0, 0, 0, {Idle, 0, 3, 1}}
};

Display current_display = {0, 0, 0, 0};
Display previous_display = current_display;

void setup() {
	pinMode(data_pin, OUTPUT);
	pinMode(clock_pin, OUTPUT);
	pinMode(latch_pin, OUTPUT);

	// Input buttons for manually adjusting the offset
	for (uint8_t i = 0; i < 4; i++) {
		pinMode(8 + i, INPUT_PULLUP);
	}
}

void loop() {
	uint32_t now = millis();

	current_display = display_from_time(now);
	
	// Only do potentially costly floating point operations once whenever the display face needs to change
	// Probably not a necessary optimization but it's low hanging fruit
	if (!display_equals(previous_display, current_display)) {
		auto d = current_display;
		steppers[0].desired_step = (float)(d.minutes_tens) * (float)(TOTAL_STEPS) / 6.0f;
		steppers[1].desired_step = (float)(d.minutes_ones) * (float)(TOTAL_STEPS) / 10.0f;
		steppers[2].desired_step = (float)(d.seconds_tens) * (float)(TOTAL_STEPS) / 6.0f;
		steppers[3].desired_step = ((float)(10 * (d.seconds_tens % 2)) + (float)(d.seconds_ones)) * (float)(TOTAL_STEPS) / 20.0f;
	}

	uint32_t coil_activations = 0;
	for (uint8_t i = 0; i < 4; i++) {
		auto stepper = steppers + i;

		// Get the normalized difference between the current and desired position
		// angle_diff is needed so that if the current position is at 359 degrees and it wants to go to 0 degrees it only needs to move 1 step instead of 359 steps
		// Putting angle_diff through unit_dir tells us a single step in which direction, or none at all
		auto angle = angle_diff(stepper->current_step, stepper->desired_step + stepper->offset, TOTAL_STEPS);
		auto unit_dir = unit_direction(angle);
		auto output = state_machine_tick(&stepper->state_machine, {now, unit_dir});
		stepper->current_step += output.steps_increment;
		auto this_stepper_coils = output.activate_coils ? step_to_coils(stepper->current_step) : 0;
		
		// Concatenate all the output bits into a single "bit array"
		coil_activations |= this_stepper_coils << (4 * (3 - i));
	}

	// Send the bit array to the shift register
	sendOut(coil_activations);

	// Manual control of offsets using the buttons
	bool buttons[4];
	for (uint8_t i = 0; i < 4; i++) {
		buttons[i] = !digitalRead(8 + i);
		steppers[i].offset += buttons[i];
	}

	previous_display = current_display;

	delayMicroseconds(10);
}

bool display_equals(Display a, Display b) {
	return
		a.minutes_tens == b.minutes_tens &&
		a.minutes_ones == b.minutes_ones &&
		a.seconds_tens == b.seconds_tens &&
		a.seconds_ones == b.seconds_ones;
}

Display display_from_time(Time now) {
	uint32_t s = now / 1000;
	uint8_t minutes = s / 60;
	uint8_t seconds = s % 60;

	uint8_t minutes_tens = minutes / 10;
	uint8_t minutes_ones = minutes % 10;
	uint8_t seconds_tens = seconds / 10;
	uint8_t seconds_ones = seconds % 10;

	return { minutes_tens, minutes_ones, seconds_tens, seconds_ones };
}

// The order of coil activations matters in terms of what direction the stepper will move
uint8_t step_to_coils(int32_t step) {
	switch (mod(step, 4)) {
		case 0:
			return 0b1001;
		case 1:
			return 0b1100;
		case 2:
			return 0b0110;
		case 3:
			return 0b0011;
	}
}

// For controlling the shift register
void sendOut(uint16_t output) {
	digitalWrite(latch_pin, LOW);
	shiftOut(data_pin, clock_pin, MSBFIRST, (uint8_t) (output >> 8));
	shiftOut(data_pin, clock_pin, MSBFIRST, (uint8_t) (output));
	digitalWrite(latch_pin, HIGH);
}