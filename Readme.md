# AVRactivePower.h - Reduce Power Use While Active

Reduce the power consumption of selected AVR chips while they are active:-

- reduce the system clock frequency in situations where you can't sleep the CPU, but are waiting around for a slow peripheral (a button or something.)

- power off parts of the controller chip such as the ADC, the built-in USART (Serial) and SPI hardware modules, the two-wire interface (Wire), timers, and the analog comparator. And power them back on when needed.

## COMPATIBILITY

`AVRactivePower.h` works with the AVR ATmega168P/PA/328P, ATtiny44/84, ATtiny45/85, ATmega1284P, and ATmega2560 chips.

Arduino boards that use the above AVR chips:  the Duemilanove, Uno, Nano, Pro Mini (both 5 volt and 3 volt versions), Mega 2560, and third-party boards based on the AVR chips listed above such as the Mighty 1284.

This library is intended mainly for **breadboard Arduinos** based on these chips, as most Arduino and third-party boards include power-hungry features like "power on" LEDs, voltage regulators, and USB interfaces. (It is relatively easy to convert a Pro Mini to a breadboard Arduino by removing or isolating its power-on LED and its voltage regulator.)

## USE-CASE

`AVRactivePower` may be useful for battery powered projects that want to eke out their batteries as much as possible.

### Use Sleep Mode First!
By far the greatest power savings come from having the chip spend as much time asleep as possible, using say RocketScream's "LowPower" library, or Peter Knight's "Narcoleptic" library.  And from removing any power LEDs and voltage regulators on your boards.

If you are still seeking power savings after implementing those things, then read on.

### System Clock Speed

8-bit AVR chips use roughly one-quarter as much power when running at 1 MHz compared to running at 16 MHz. If your project requires that the chip be awake, but it isn't doing much, then reducing the system clock speed can help save power.

#### Warning: `millis()`, `micros()`, `delay()`, and Communications Affected

The `AVRchip.SystemClock` functions change the speed of the system clock, which affects `millis()`, `micros()`, `delay()`, and `delayMicroseconds()`. It also affects `Serial`, `SPI`, and/or `Wire`.  It also affects functions that use timers under the covers: `analogWrite()` and `tone()`.  `analogRead()` will be slowed down as well, making it inaccurate.

Don't use those features after having used `AVRchip.SystemClock.divideBy()`. Use `XXX.end()` with Serial, SPI, and Wire before changing the clock speed. After returning to full speed (`AVRchip.SystemClock.fullSpeed()`), use `XXX.begin()`.


### Unused Chip Features

With this library you can power-off internal hardware modules in the chip while it is awake and processing (which should be a relatively small part of the time for battery powered projects).

The ADC (analog-to-digital converter) hardware uses about 300 microamps, and the serial (USART) hardware and SPI hardware each use 50 to 80 microamps when powered on.  Timer1 (16-bit timer) may use significant power as well.

## INSTALLATION

From the Arduino library manager: search for AVRactivePower in the Device Control category.

From Github:  click the green "Code" button and choose "download zip" Extract the zip-file into the "libraries" folder inside your Arduino sketchbook folder and rename the unzipped folder from "AVRactivePower-main" to "AVRactivePower".

## USAGE

### Hardware blocks (modules)

        #include "AVRactivePower.h"

        void setup() {
            AVRchip.Allhw.powerOff();
            AVRchip.Timer0hw.powerOn();    // re-enable Timer 0 for delay(), millis()
            AVRchip.Serialhw.powerOn();    // re-enable hardware serial module
            Serial.begin(9600);
            .... code using the serial port ...
            Serial.end();
            AVRchip.Serialhw.powerOff();
            .....
        }

Individual hardware modules are powered off or on using the `AVRchip` object and its sub-modules `AChw` (analog comparator module), `ADChw` (analog-to-digital converter), `Serialhw`, `SPIhw`, `Timer0hw`, `Timer1hw`, `Timer2hw` and `Wirehw`, or with the "umbrella" module `Allhw` to control them all at once.

Each hardware module has functions `powerOn()`, `powerOff()`, and `isOn()`:-

        if ( ! AVRchip.Wirehw.isOn())  AVRchip.Wirehw.powerOn();

        Wire.begin();  // Arduino's Wire object for the two-wire-interface
        .....
        .....
        Wire.end();

        AVRchip.Wirehw.powerOff();

The ATmega1284P and ATmega2560 have more than one serial hardware modules and extra timers.  The full set of modules for each chip is as follows:-

|ATtiny44/84/45/85 |ATmega168P/328P   |ATmega1284P       |ATmega2560        |
|------------------|------------------|------------------|------------------|
|SystemClock       |SystemClock       |SystemClock       |SystemClock       |
|AnalogComparatorhw|AnalogComparatorhw|AnalogComparatorhw|AnalogComparatorhw|
|ADChw             |ADChw             |ADChw             |ADChw             |
|Serialhw          |Serialhw          |Serialhw          |Serialhw          |
|Timer0hw          |SPIhw             |Serial1hw         |Serial1hw         |
|Timer1hw          |Timer0hw          |Serial2hw         |Serial2hw         |
|                  |Timer1hw          |SPIhw             |Serial3hw         |
|                  |Timer2hw          |Timer0hw          |SPIhw             |
|                  |Wirehw            |Timer1hw          |Timer0hw          |
|                  |                  |Timer2hw          |Timer1hw          |
|                  |                  |Timer3hw          |Timer2hw          |
|                  |                  |Wirehw            |Timer3hw          |
|                  |                  |                  |Timer4hw          |
|                  |                  |                  |Timer5hw          |
|                  |                  |                  |Wirehw            |


Each of these has `isOn()`, `powerOn()`, and `powerOff()` functions.

For each chip, the `Allhw` pseudo-module controls all modules except the `SystemClock`. (Powering off the system clock is done via `sleep()` operations. The easy way is to use RocketScream's *LowPower* library or Peter Knight's *Narcoleptic* library.) 

`Allhw.isOn()` returns false if *any* module is off.

### System Clock

As above, be careful when changing the system clock frequency if you want to use any of `Serial`, `Wire`, `SPI`, `millis()`, `micros()`, `delay()`, `delayMicroseconds()`, `tone()`, `analogWrite()`, or `analogRead()`.

For `Serial`, `Wire`, and `SPI`, use the `end()` function before slowing the system clock and use the `begin()` function after resetting it to full speed.

        #include "AVRactivePower.h"

        void setup() {
            AVRchip.Allhw.powerOff();
            .....
        }
        void loop() {
           AVRchip.SystemClock.divideBy(16);   // divide 16 MHz -> 1 MHz.
           .....
           AVRchip.SystemClock.fullSpeed();    // safe to use micros(), etc.
        }

The allowed values for `AVRchip.SystemClock.divideBy()` are 1, 2, 4, 8, 16, 32, 64, 128 and 256.  Consult the data sheet for the estimated operating current at each clock speed you get by using divideBy().

There are two "convenience" functions for `AVRchip.SystemClock`:

        AVRchip.SystemClock.divideBy8();    // for 8 MHz Pro Mini, to 1 MHz.
        AVRchip.SystemClock.divideBy16();   // for 16 MHz Uno / Nano / Pro Mini

`AVRchip.SystemClock.fullSpeed()` is the same as `AVRchip.SystemClock.divideBy(1)`.


## TO DO

GvP 2025-09-01.

### Maybe

Add to documentation.

### Unlikely

Add control of the brown-out detector and watchdog timer modules.

Extend to other AVR chips.

Extend to other chip families (STM32, ESP32, etc.)