#include <Arduino.h>

#include "hardware.h" // Hardware object

#define  BAUD 9600
#include "dtos.h"            // DebugSerial object, printReg and printVar macros

void setup()
{
  DebugSerial.begin();
  Hardware.Allhw.powerOff();
  Hardware.Serialhw.powerOn();  // re-enable serial for debugging
  DebugSerial.println("Re-enabled Serial for debugging.");
  long thetime = micros();
  long themillis = millis();
  DebugSerial.println("micros() and millis() and the power reduction register:-");
  printVar(thetime);
  printVar(themillis);
  printReg(PRR);
  bool allhwisOn = Hardware.Allhw.isOn();
  printVar(allhwisOn);
  // Hardware.Allhw.powerOn();
  // printReg(PRR);
  // hardwarestate = Hardware.Allhw.isOn();
  // DebugSerial.println(hardwarestate);
  thetime = micros();
  printVar(thetime);
  themillis = millis();
  printVar(themillis);
  DebugSerial.println("Re-enabling Timer0 for millis() and delay().");
  Hardware.Timer0hw.powerOn();
  printReg(PRR);
  DebugSerial.println("Re-enabling Analog Comparator.");
  Hardware.AnalogComparatorhw.powerOn();
  printReg(PRR);
  DebugSerial.println("Re-enabling ADC.");
  Hardware.ADChw.powerOn();
  printReg(PRR);
  DebugSerial.println("Re-enabling SPI.");
  Hardware.SPIhw.powerOn();
  printReg(PRR);
  DebugSerial.println("Re-enabling Wire.");
  Hardware.Wirehw.powerOn();
  printReg(PRR);
  DebugSerial.println("Re-enabling Timer2.");
  Hardware.Timer2hw.powerOn();
  printReg(PRR);
  DebugSerial.println("Re-enabling Timer1.");
  Hardware.Timer1hw.powerOn();
  printReg(PRR);
  thetime = micros();
  themillis = millis();
  printVar(thetime);
  printVar(themillis);
}

void loop(){}
