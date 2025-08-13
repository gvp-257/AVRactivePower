# Hardware.h

Control the internal hardware modules of the Atmega328P chip: ADC, SPI, two-wire interface (Wire), timers, analog comparator, hardware serial.  Check if on, power off and on.

        #include "Hardware.h"

        void setup() {
            Hardware.Allhw.powerOff();
            Hardware.Timer0hw.powerOn();    // re-enable Timer 0 for delay(), millis()
            Hardware.Serialhw.powerOn();    // re-enable hardware serial module
            Serial.begin(9600);
            .... code using the serial port ...
            Serial.end();
            Hardware.Serialhw.powerOff();
            .....
        }


## USE-CASE

Battery powered projects that want to eke out their batteries as much as possible.

Hardware.h controls the hardware modules in the chip while it is awake and processing, which should be a relatively small part of the time for such projects. They should spend most of their time asleep, using, for example, RocketScream's "LowPower" library, or brabl2's "Narcoleptic" library.

The ADC (analog-to-digital converter) hardware uses about 300 microamps, and the serial hardware and SPI hardware each use 50 to 80 microamps when powered on.

## USAGE

Via the `Hardware` object, and its sub-modules `AChw` (analog comparator module), `ADChw` (analog-to-digital converter), `Serialhw`, `SPIhw`, `Timer0hw`, `Timer1hw`, `Timer2hw` and `Wirehw`, or the "umbrella" module `Allhw` to control them all at once.

Each module has functions `powerOn()`, `powerOff()`, and `isOn()`:-

        if ( ! Hardware.Wirehw.isOn())  Hardware.Wirehw.powerOn();

        Wire.begin();  // Arduino's Wire object for the two-wire-interface
        .....
        .....
        Wire.end();

        Hardware.Wirehw.powerOff();


## TO DO

GvP 2025-08-14.

### Maybe

Extend to other AVR-8 chips: ATtiny84, ATmega2560, ATmega1284P.

Add to documentation.

### Unlikely

Add control of brown-out detector and watchdog timer modules.