# Introduction

This is my final project for ECE 180 at BSU. It's way beyond the scope of the class, but the class is an intro class and I've already got plenty of experience working with electronics and programming, so I chose this mostly just to flex. The bulk of this project focuses on controlling several stepper motors simultaneously from an Arduino microcontroller. Essentially, this is a digital clock that, instead of using an LED or LCD display, uses physically rotating barrels, each of which displays one digit. Furthermore, I wanted to use the limitation that I would use no premade code, including libraries. Everything in this project was done by yours truly.

# Motivation

When brainstorming, one of the many ideas I came up with was inspired by my love for chess. In much of competitive and casual chess, a so-called "chess clock" is used, limiting the total amount of time a player can use to make their moves. But a normal digital clock is pretty easy to make and uninteresting, as it has been already done and mass-produced before in many different forms. In a similar vein to slot machines or the counters that track how many people leave an amusement park ride, I wanted to make a physical display that uses rotating barrels with digits on them.

# Design overview

When workshopping the idea, I had to figure out what all would be needed. At the start of the project, I figured that I would essentially only need 4 motors and some way to control them all. I settled on using stepper motors and shift registers.

## Component choice

### Motors

I settled upon the 28byj-48 stepper motor, a small cheap 4-phase stepper. Before starting this project, it seemed like the ideal choice because it can work on 5V (which the Arduino Uno uses), it is small and inexpensive, and I can precisely control the output angle. Typical DC motors just spin with an applied voltage, and so I would have to somehow measure how fast and far it moved and compensate, so DC motors didn't seem like a correct choice. Using servos would be a good idea, as those can achieve precise control and be fairly quick. However, I couldn't find a reasonable servo that could also do a "full continuous rotation" where there were no endstops.

If I revisit this project, finding a small, lightweight, and fast motor whose position can be precisely measured and controlled will be imperative. The steppers work somewhat, but with a faster rotational speed I would be able to implement the full goal of the project. This is explored more in the Limitations portion of the writeup down below.

These steppers came with driver modules, essentially just Darlington transistor arrays with flyback diodes to damp feedback spikes from switching the motor coils off. The Darlington transistors allow a much larger current (~100mA at 5V to run the coils) to be controlled by a small current (the Arduino output pins can't handle much load at all, max of around ~20mA). These driver modules have 2 power pins for 5V in and 4 control pins, one for each separate coil.

### Shift registers

As the motors I chose required 4 pins to control, if I were to drive them all directly from the Arduino, I would need 16 output pins, which would not work. Therefore, I needed a shift register, which would allow me to control several parallel output pins using only a few (3) serial inputs. I already had a 74HC595 shift register on hand, but that could only run 8 total output pins. I bought more and chained 2 together to give me a 16 pin shift register, allowing me to drive all the motors. This would require more code complexity and slightly limit what was possible, but would greatly reduce and simplify the wiring problem I would otherwise have.

# Programming

Programming everything was by far the hardest part of this project. I'll break it down by sequentially increasing difficulty.

## Driving a single stepper at constant continuous rotation

First, I got a proof of concept working, driving a single stepper motor from the Arduino's output pins. There are 4 pins for driving the motor, each corresponding to a single coil, and they need to be driven in a specific order to get proper stepping action. Otherwise, it would vibrate in place or do nothing. I used full-step mode, as that would be easiest to implement, where each coil is toggled between fully-on and fully-off. I won't go too much into the details of the coil activation sequences, as there are a variety of online guides that explain this in the proper depth with helpful illustrations.

## Expanding the available output ports using a shift register

After getting a single motor driven from the Arduino's output pins, I needed to figure out how to work the shift register. I started by breadboarding everything and connecting the output pins of the shift register to LEDs with series resistors to ground, then connecting the 5V and ground pins, then the serial input and clock and latch inputs to the Arduino's output pins, then writing some code that would write out certain combinations to the shift register by pulsing the serial pins with the data, pulsing the clock pin once per data bit, and toggling the latch pin at the beginning and end of a sequence.

### Resolving floating pins

To my surprise, everything worked the first time, mostly. However, I had a strange issue where if my hand got too close or too far away from the breadboard, it would shift between working and not working. There had to be a floating pin somewhere in the circuit, where an input pin is disconnected from anything and stray charges from the surrounding environment can cause it to appear as either on or off. I then checked the data sheet for the shift register again and found that the inverted output enable pin and the inverted clear pin in my circuit needed to be tied to ground and +5V respectively. This solved that issue. With that working, I was able to remove the LEDs from the circuit and hook up the output from the shift register to the steppers.

## Driving multiple motors at constant continuous rotation

Then I needed to resolve the issue of driving more than one stepper at the same time. At that time, my code was pretty much "pulse the proper coils for the current step, wait 5ms, adjust the current step, repeat." This represented a constant rotation in a single direction for a single motor. At first, I just modified this code to drive all the steppers to the same rotation, just to make sure everything worked. 

## Driving multiple motors with independent rotations using state machines

Then I added controls for turning the rotation on and off, then separate counters for current step and desired step, then a state machine-based implementation that handles scheduling, bidirectional drive, and "manual PWM" to reduce the current and avoid saturating the motor coils (which wastes energy and risks damaging the motors via heat). The final implementation is fairly elegant and idiomatic C if I say so myself. To change the position of the motors, all that needs to be done is to adjust the desired position of that motor, and the state machine handler essentially does the rest, calculating if it needs to move, in which direction it needs to move, and using the current time and given tick times to schedule an on-off PWM signal for each of the 4 coils per motor.

My state machine handler was specifically designed to be non-blocking (see https://en.wikipedia.org/wiki/Blocking_(computing)). In terms of what this means, essentially it allows other operations to be done during the same time period. In the initial implementation, the code was completely blocking, spending 5ms with the coils in one state, then 5ms with the coils in another state. If any other code needed to be run during that time frame, it would have to work around the fact that there would be 5ms between successive runs. In a non-blocking style, any number of non-blocking components can be combined without having to wait for these activations to be done. This is called cooperative multitasking, and is essentially a way to pretend to have multiple threads of execution on a single-threaded system.

My implementation is extensible enough that I would easily be able to drive several more motors if I wanted to. The computation time per loop spent on calculating which coils need to be activated is pretty negligible, so scaling upwards to e.g. controlling 20 steppers at a time pretty much boils down to properly setting up the StepperConfiguration array and then just adjusting the shift register handling code to shift out however many bits are needed. Power's not even that much of an issue, as hardly ever is more than one stepper motor active at a time, and they each draw around 500 mW when active. Most good USB power supplies nowadays can handle 5V at 1A easily, sometimes even to 3A. 

As always, I have gripes with using C/C++ because it's old, fairly unergonomic, and easy to shoot yourself in the foot with. I much prefer languages with a more flexible type system. For instance, using Rust, I could easily model the state machine as a sum type, thereby giving more type safety (e.g. avoiding invalid states) and simplifying the program logic. In practice, since this implementation is fairly straightforward, C/C++ is okay, but in a more complicated situation, with more states and more complicated logic, the state machine would be much harder to debug in C/C++. I am of course biased towards Rust, and I have previously experimented with running Rust on the Arduino platform, but I wanted to make a complete project using C/C++ so that I could better understand it.

# Limitations

I eventually realized that due to the limitations of the stepper motors I used, the concept of a chess clock using stepper motors was unviable due to many chess time controls using increment, where a certain amount of time is added to your clock per move. This would require a lot of fast rotation on the part of the ones digit of the seconds, and the steppers I chose can only handle a max of around 10 revolutions per minute, or ~0.16 revolutions per second. Given that many common chess time formats have 2 to 10 second increment per move, the stepper showing the ones digit of the seconds would not have enough time to catch up and so the display would lag noticeably behind which is extremely undesirable especially when a player's remaining time is running low.

I might be able to make it work with the steppers by using some kind of gearing system to increase rotational speed, but I'm not a mechanical engineer and am not particularly interested in going down that route anyway. Maybe I could use a magnetic hall effect sensor to measure angle in combination with a brushless DC motor with some kind of closed loop feedback system. It's something to look into anyway.

In practice, for this project, I'm just counting minutes and seconds upward from when it's plugged in. Not as interesting or useful, but it's a proof of concept that might be revisited.

# Conclusion

I learned a lot about motor control both in the pre-project research and during the course of implementing it. I think that there's a good market for inexpensive motor control boards (especially in the CNC and 3D printing world), and it's something I'd like to pursue further in the future, possibly looking into using other motor types as well. If or when I take an IC design course, encoding my state machine logic into a hardware implementation could be a good project, with possible improvements for handling half-stepping or microstepping.

Since in my initial motivation I wanted to make a chess clock and realized it wasn't possible due to the steppers being too slow, I would love to use a brushless DC motor and magnetic hall effect sensor in a closed loop system to get quick snappy movement that would make the project really shine and would be at home in a real chess tournament. With proper motor choice and design (both electrical and mechanical), I think it could be a stylish competitor to current digital chess clocks and could be much easier and more pleasant to read than the existing poor-contrast LCD chess clocks on the market, with the tradeoff of a much greater price point considering that good motors are not cheap. Alternately, a split-flap style design would probably be cheaper and easier to implement.

Another option is to drive the whole system using a single motor, and have a mechanical gearing and carry system, so that by controlling only the ones digit of the seconds, the entire display could update off of that. However, as mentioned before, I am not a mechanical engineer nor am I interested in that kind of work. It is possible that it could be simpler from a software perspective and less expensive due to using fewer motors, however it could possibly be less robust due to having more moving parts (if using 3D printed parts as I did for mine), and furthermore can only be used in "count up" or "count down" applications where there's only one digit that's truly independent and all the others can be driven by its movement.

This was ultimately a good project for learning about the control process and limitations of motors, and for learning about how to write robust C code. I wouldn't necessarily recommend it for beginners, as it's fairly easy to either destroy your Arduino board by drawing too much current from its pins or destroy the motors by leaving the same coils activated for too long and burning them, and furthermore my C code is fairly sophisticated and not easy to explain to somebody who hasn't used C before.