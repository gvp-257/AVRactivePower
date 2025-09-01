#include <Arduino.h>

#include "AVRactivePower.h"  //AVRchip object

// Macros to print variables and register contents.
#define printVar(name)    \
do {                      \
  Serial.print(F(#name)); \
  Serial.print(F(": "));  \
  Serial.println(name);     \
} while (0);


#define printReg(name)    \
do {                      \
  Serial.print(F(#name)); \
  Serial.print(F(": 0b"));\
  Serial.println(name, BIN);\
} while (0);

void setup()
{
  Serial.begin(9600);
  AVRchip.Allhw.powerOff();
  AVRchip.Timer0hw.powerOn();   // re-enable Timer0 for millis(), delay(), etc.
  AVRchip.Serialhw.powerOn();   // re-enable serial for debugging
  Serial.println(F("Re-enabled Timer 0 (PRR bit 5) and Serial (PRR bit 1) for debugging."));
  long thetime = micros();
  long themillis = millis();
  Serial.println(F("micros() and millis() and the power reduction register:-"));
  printVar(thetime);
  printVar(themillis);
  printReg(PRR);
  bool allhwisOn = AVRchip.Allhw.isOn();
  printVar(allhwisOn);
  // AVRchip.Allhw.powerOn();
  // printReg(PRR);
  // hardwarestate = AVRchip.Allhw.isOn();
  // Serial.println(hardwarestate);
  AVRchip.Timer0hw.powerOff();   // re-enable Timer0 for millis(), delay(), etc.
  Serial.println(F("Disabled Timer0 PRR bit 5."));
  thetime = micros();
  printVar(thetime);
  themillis = millis();
  printVar(themillis);
  Serial.println(F("Re-enabling Timer0 for millis() and delay()."));
  AVRchip.Timer0hw.powerOn();
  printReg(PRR);
  Serial.println("Re-enabling Analog Comparator.");
  AVRchip.AnalogComparatorhw.powerOn();
  printReg(PRR);
  Serial.println("Re-enabling ADC.");
  AVRchip.ADChw.powerOn();
  printReg(PRR);
  Serial.println("Re-enabling SPI.");
  AVRchip.SPIhw.powerOn();
  printReg(PRR);
  Serial.println("Re-enabling Wire.");
  AVRchip.Wirehw.powerOn();
  printReg(PRR);
  Serial.println("Re-enabling Timer2.");
  AVRchip.Timer2hw.powerOn();
  printReg(PRR);
  Serial.println("Re-enabling Timer1.");
  AVRchip.Timer1hw.powerOn();
  printReg(PRR);
  thetime = micros();
  themillis = millis();
  printVar(thetime);
  printVar(themillis);
}

void loop(){}
