#ifndef AVR_ACTIVEPOWER_LIB_H
#define AVR_ACTIVEPOWER_LIB_H
/*
 * Save a few hundred microamps while active, when you're not using
 * analogRead, and/or Serial, SPI or Wire, by disabling those modules in the
 * AVR8 chip.
 *
 * (C) GvP 2025.
 * MIT license.
 *
 * CHANGES
 * 2025-07 Initial version for ATmega168P/328P.
 * 2025-09 Extend to ATtiny44/84/45/85, ATmega1284P and ATmega2560.
 *         Add system clock divide functionality.
 * 2025-09 Implement RAII techniques.
 *
 *
 * ACTIVE MODE NOT SLEEP MODE
 * ==========================
 *
 * For low-current **sleep** (potentially 0.1 microamp), see
 * RocketScream's LowPower library or Peter Knight's Narcoleptic library.
 *
 * Most boards are in the multi-milliamp current consumption range, because of
 * LEDs, inefficient voltage regulators, USB communication chips, etc.
 *
 * Saving a few hundred microamps only makes sense for Pro Minis
 * with the power LED and voltage regulator removed, and breadboard Arduinos
 * that don't have such frills as power LEDs.
 *
 * USAGE
 * -----

 * #include "AVRactivePower.h"
 *  .....
 *
 * if (AVRchip.AnalogComparatorhw.isOn())  // shouldn't be on but maybe it is
 *      AVRchip.AnalogComparatorhw.powerOff();
 * AVRchip.ADChw.powerOff();   // not using ADC
 *  .....
 * // other periphs: Serialhw, SPIhw, Wirehw, Timer0hw, Timer1hw, Timer2hw
 * // For 2560: plus Serial1hw, Serial2hw, Serial3hw, Timer3hw, Timer4hw,
 * // Timer5hw.
 *
 * AVRchip.SystemClock.divideBy16();
 *  .....
 * AVRchip.SystemClock.fullSpeed();
 *
 * SYSTEM CLOCK DIVIDE WARNINGS
 * ----------------------------
 *
 * The AVRchip.SystemClock.divideBy() functions can save current if your chip
 * has to be active but isn't doing much. AVR chips use about a quarter as
 * much current at 1MHz as they do at 16 MHz. BUT they affect `millis()`,
 * `delay()`, etc - anything that uses a timer. The ADC may also become
 * inaccurate with a clock that is too slow. And of course Serial won't work.
 * SPI and TWI (Wire) will probably work, but slowly.
 *
 *
 * POWEROFF / POWERON NOTES
 * ------------------------
 *
 * Timer 0 is used extensively by the Arduino system.
 * (e.g. `millis()`, `micros()`, `delay()`, `delayMicroseconds()`).
 *
 * So power it back on after `AVRchip.Allhw.powerOff()` if you need `delay()`,
 * `millis()`, `micros()`, etc.
 *
 * Timer2 (and maybe Timer1) are used by `analogWrite()` and `tone()`.
 * The analog comparator is not used at all in Arduino base code.
 *
 * The big power users are the ADC (especially), the Serial interface, SPI, and
 * the 16-bit timers. *Hundreds* of microamps, I tell you!
 *
 * DETAILS
 * =======
 *
 * The clocks controlling peripherals have disable bits in the power reduction
 * register(s). There is one, called PRR, in the ATtiny chips and the
 * ATmega328P. ATmega1284P and ATmega2560 have two registers, PRR0 and PRR1.
 *
 * For the ATmega328P:-
 *
 * Bit  Periph bitname   Notes
 * ---  -----  --------  ---------------------------------------------------
 *  0   adc    PRADC     used in Arduino by analogRead()
 *  1   usart0 PRUSART0  used in Arduino by Serial
 *  2   spi    PRSPI     used in Arduino by SPI
 *  3   timer1 PRTIM1    timer1 may be used by tone() and/or analogWrite()
 *  4    --      --
 *  5   timer0 PRTIM0    timer0 used by Arduino for millis() and delay()
 *  6   timer2 PRTIM2    timer2 used by Arduino for tone() and analogWrite()
 *  7   twi    PRTWI     Arduino's Wire, a.k.a. I2C
 *
 *
 * Before stopping a peripheral's clock we need to zero its 'enable' bit(s)
 * in its control register.
 *
 * Timers, 0, 1, and 2 don't have 'enable' bits in their control registers.
 * Stopping their clocks in PRR will freeze them, stopping them generating
 * interrupts.
 *
 * The analog comparator doesn't have a clock and it has a 'disable' bit
 * rather than 'enable' bit(s).
 *
 * Peripheral        Register    Enable bits (device, interrupts) - set 0.
 * ----------        --------    -----------------------------------------
 * Analog comparator  ACSR       ACD (set to 1 to disable), ACIE
 * ADC                ADCSRA     ADEN,  ADIE
 * SPI                SPCR       SPE,   SPIE
 * TWI (I2C, Wire)    TWCR       TWEN,  TWIE
 * USART0 (Serial)    UCSR0B     RXEN0 and TXEN0, RXCIE0 TXCIE0: rx/tx complete
 *
 */

// Functions: on, off, isOn
// usage: AVRchip.ADC.powerOn(), AVRchip.ADC.powerOff(), AVRchip.ADC.isOn()

#if !defined(_AVR_IOXXX_H_)
#   error This library is only for AVR ATtinyx4/ ATtinyx5/ ATmega168/328 /1284 / 2560 chips.
#endif

#if defined(PRUSART3)
#   define AVRCHIP 2560
#   define PRR PRR0
#endif

# if !defined(AVRCHIP) && defined(PRR1)
#   define AVRCHIP 1284
#   define PRR PRR0
#endif

#if  !defined(AVRCHIP) && defined (PRUSI)
#   define AVRCHIP 44         // ATtiny44, 84, 45, 85
#endif

# if  !defined(AVRCHIP) && defined (PRUSART0)
#   define AVRCHIP 328     // includes 168PA, etc.
#endif

#if !defined(AVRCHIP)
#   error Unrecognised chip type.
#endif

// Interrupt guard. RAII.
class AVRInterruptGuard
{
private:
    uint8_t oldsreg;
public:
     AVRInterruptGuard() : oldsreg(SREG) {cli();}
    ~AVRInterruptGuard()  {SREG = oldsreg;}
};

// IN-COMMON blocks / peripherals

// CLKPR: clock prescale register. Slow the system clock.
struct _clockreg
{
    void divideBy(const int ratio)
    {
        // ratio must be 1, 2, 4, 8, 16, 32, 64, 128, 256.
        uint8_t bits = 0;
        switch (ratio)
        {
            case   1: bits = 0; break;
            case   2: bits = 1; break;
            case   4: bits = 2; break;
            case   8: bits = 3; break;
            case  16: bits = 4; break;
            case  32: bits = 5; break;
            case  64: bits = 6; break;
            case 128: bits = 7; break;
            case 256: bits = 8; break;
            default:  return;
        }
        AVRInterruptGuard g;    // save SREG and cli
        CLKPR = 1<<CLKPCE;   // clock prescale change enable
        CLKPR = bits;        // must set prescale within 4 clock cycles
    }   /// scope end for g, call ~g.
    void divideBy8()
    {
        AVRInterruptGuard g;
        CLKPR = 1<<CLKPCE;
        CLKPR = 0x03;
    }
    void divideBy16()
    {
        AVRInterruptGuard g;
        CLKPR = 1<<CLKPCE;
        CLKPR = 0x04;
    }
    void fullSpeed()
    {
        AVRInterruptGuard g;
        CLKPR = 1<<CLKPCE;
        CLKPR = 0;
    }
};

struct _AChw
{
    bool isOn()    {return !(ACSR & (1<<ACD));} // On if disable bit is 0
    void powerOn() {ACSR &= ~(1<<ACD);}
    void powerOff(){ACSR |=  (1<<ACD);}
};

struct _adchw
{
    bool isOn()      {return (!(PRR & (1<PRADC)) && (ADCSRA & (1<<ADEN)));}
    void powerOn()   {PRR    &= ~(1<<PRADC); ADCSRA |= (1<<ADEN);}
    void powerOff()  {ADCSRA &= ~(1<<ADEN);  PRR    |= (1<<PRADC);}
};

struct _timer0hw
{
    bool isOn()      {return (!(PRR & (1<PRTIM0)));}
    void powerOn()   {PRR &= ~(1<<PRTIM0);}
    void powerOff()  {PRR |= (1<<PRTIM0);}
};

struct _timer1hw
{
    bool isOn()      {return (!(PRR & (1<PRTIM1)));}
    void powerOn()   {PRR &= ~(1<<PRTIM1);}
    void powerOff()  {PRR |= (1<<PRTIM1);}
};



// VARIANT BLOCKS


#if AVRCHIP == 44   // ATtiny 44 / 84 / 45 / 85

struct _usihw
{
    bool isOn() {return (!(PRR & (1<PRUSI)));}
    void powerOn()
    {PRR &= ~(1<<PRUSI);}
    void powerOff()
    {PRR |= (1<<PRUSI);}
};
#endif

#if AVRCHIP == 328 || AVRCHIP == 1284 || AVRCHIP == 2560

struct _spihw
{
    bool isOn() {return (!(PRR & (1<PRSPI)) && (SPCR & (1<<SPE))) ;}
    void powerOn()   {PRR  &= ~(1<<PRSPI); SPCR |= (1<<SPE);}
    void powerOff()  {SPCR &= ~(1<<SPE);   PRR  |= (1<<PRSPI);}
};

struct _timer2hw
{
    bool isOn()      {return (!(PRR & (1<PRTIM2)));}
    void powerOn()   {PRR &= ~(1<<PRTIM2);}
    void powerOff()  {PRR |= (1<<PRTIM2);}
};

struct _twihw
{
    bool isOn() {return (!(PRR & (1<PRTWI)) && (TWCR & (1<<TWEN))) ;}
    void powerOn()   {PRR  &= ~(1<<PRTWI);  TWCR |= (1<<TWEN);}
    void powerOff()  {TWCR &= ~(1<<TWEN);   PRR  |= (1<<PRTWI);}
};

struct _usart0hw
{
    bool isOn() {return (!(PRR & (1<PRUSART0)) && (UCSR0B & (1<<TXEN0)));}
    void powerOn()
    {PRR &= ~(1<<PRUSART0); UCSR0B |= (1<<TXEN0) | (1<<RXEN0);}
    void powerOff()
    {UCSR0B &= ~((1<<TXEN0) | (1<<RXEN0));  PRR |= (1<<PRUSART0);}
};

#endif

#if AVRCHIP == 1284
// ATmega1284P has PRUSART1, PRTIM3, PRR0, PRR1
// PRUSART1 is in PRR0, only PRTIM3 in PRR1.

struct _usart1hw
{
    bool isOn() {return (!(PRR0 & (1<PRUSART1)) && (UCSR1B & (1<<TXEN1)));}
    void powerOn()
    {PRR0 &= ~(1<<PRUSART1); UCSR1B |= (1<<TXEN1) | (1<<RXEN1);}
    void powerOff()
    {UCSR1B &= ~((1<<TXEN1) | (1<<RXEN1));  PRR0 |= (1<<PRUSART1);}
};

struct _timer3hw
{
    bool isOn()      {return (!(PRR1 & (1<PRTIM3)));}
    void powerOn()   {PRR1 &= ~(1<<PRTIM3);}
    void powerOff()  {PRR1 |= (1<<PRTIM3);}
};
#endif

#if AVRCHIP == 2560

// PRUSART1 is in PRR1, which also has PRTIM4, PRTIM5, USART2, USART3.

struct _timer3hw
{
    bool isOn()      {return (!(PRR1 & (1<PRTIM3)));}
    void powerOn()   {PRR1 &= ~(1<<PRTIM3);}
    void powerOff()  {PRR1 |= (1<<PRTIM3);}
};

struct _timer4hw
{
    bool isOn()      {return (!(PRR1 & (1<PRTIM4)));}
    void powerOn()   {PRR1 &= ~(1<<PRTIM4);}
    void powerOff()  {PRR1 |= (1<<PRTIM4);}
};

struct _timer5hw
{
    bool isOn()      {return (!(PRR1 & (1<PRTIM5)));}
    void powerOn()   {PRR1 &= ~(1<<PRTIM5);}
    void powerOff()  {PRR1 |= (1<<PRTIM5);}
};

struct _usart1hw
{
    bool isOn() {return (!(PRR1 & (1<PRUSART1)) && (UCSR1B & (1<<TXEN1)));}
    void powerOn()
    {PRR1 &= ~(1<<PRUSART1); UCSR1B |= (1<<TXEN1) | (1<<RXEN1);}
    void powerOff()
    {UCSR1B &= ~((1<<TXEN1) | (1<<RXEN1));  PRR1 |= (1<<PRUSART1);}
};

struct _usart2hw
{
    bool isOn() {return (!(PRR1 & (1<PRUSART2)) && (UCSR2B & (1<<TXEN2)));}
    void powerOn()
    {PRR1 &= ~(1<<PRUSART2); UCSR2B |= (1<<TXEN2) | (1<<RXEN2);}
    void powerOff()
    {UCSR2B &= ~((1<<TXEN2) | (1<<RXEN2));  PRR1 |= (1<<PRUSART2);}
};

struct _usart3hw
{
    bool isOn() {return (!(PRR1 & (1<PRUSART3)) && (UCSR3B & (1<<TXEN3)));}
    void powerOn()
    {PRR1 &= ~(1<<PRUSART3); UCSR3B |= (1<<TXEN3) | (1<<RXEN3);}
    void powerOff()
    {UCSR3B &= ~((1<<TXEN3) | (1<<RXEN3));  PRR1 |= (1<<PRUSART3);}
};

#endif  // AVRCHIP == 2560



//========================================================



struct _AVRChipHardwareModules
{
    static struct _clockreg SystemClock;

    static struct _AChw     AnalogComparatorhw;
    static struct _adchw    ADChw;
    static struct _timer0hw Timer0hw;
    static struct _timer1hw Timer1hw;

#if AVRCHIP == 44
    static struct _usihw    Serialhw;
#endif

#if AVRCHIP == 328 || AVRCHIP == 1284 || AVRCHIP == 2560
    static struct _usart0hw Serialhw;
    static struct _spihw    SPIhw;
    static struct _timer2hw Timer2hw;
    static struct _twihw    Wirehw;
#endif

#if AVRCHIP == 1284 || AVRCHIP == 2560
    static struct _usart1hw Serial1hw;
    static struct _timer3hw Timer3hw;
#endif

#if AVRCHIP == 2560
    static struct _usart2hw Serial2hw;
    static struct _usart3hw Serial3hw;
    static struct _timer4hw Timer4hw;
    static struct _timer5hw Timer5hw;
#endif

    struct _allhw
    {
        bool isOn()
        {
#if AVRCHIP == 1284 || AVRCHIP == 2560
            if (PRR0 != 0 || PRR1 != 0)    return false;
            if (!Serial1hw.isOn())         return false;
            if (!Timer3hw.isOn())          return false;
#endif

            if (AnalogComparatorhw.isOn()) return false;
            if (!ADChw.isOn())             return false;
            if (!Serialhw.isOn())          return false;
            if (!Timer1hw.isOn())          return false;
            if (!Timer0hw.isOn())          return false;

#if AVRCHIP == 328 || AVRCHIP == 1284 || AVRCHIP == 2560
            if (!Timer2hw.isOn())          return false;
            if (!Wirehw.isOn())            return false;
            if (!SPIhw.isOn())             return false;
#endif

#if AVRCHIP == 2560
            if (!Serial2hw.isOn())         return false;
            if (!Serial3hw.isOn())         return false;
            if (!Timer5hw.isOn())          return false;
            if (!Timer4hw.isOn())          return false;
#endif
            return true;
        }   // isOn()

        void powerOn()
        {
            Timer0hw.powerOn();
            Timer1hw.powerOn();
            ADChw.powerOn();
            AnalogComparatorhw.powerOn();
            Serialhw.powerOn();

#if AVRCHIP == 328 || AVRCHIP == 1284 || AVRCHIP == 2560
            SPIhw.powerOn();
            Timer2hw.powerOn();
            Wirehw.powerOn();
#endif

#if AVRCHIP == 1284 || AVRCHIP == 2560
            Serial1hw.powerOn();
            Timer3hw.powerOn();
#endif

#if AVRCHIP == 2560
            Serial2hw.powerOn();
            Serial3hw.powerOn();
            Timer4hw.powerOn();
            Timer5hw.powerOn();
#endif
        } // powerOn()

        void powerOff()
        {
            Timer0hw.powerOff();
            Timer1hw.powerOff();
            ADChw.powerOff();
            AnalogComparatorhw.powerOff();
            Serialhw.powerOff();

#if AVRCHIP == 328 || AVRCHIP == 1284 || AVRCHIP == 2560
            SPIhw.powerOff();
            Timer2hw.powerOff();
            Wirehw.powerOff();
#endif

#if AVRCHIP == 1284 || AVRCHIP == 2560
            Serial1hw.powerOff();
            Timer3hw.powerOff();
#endif

#if AVRCHIP == 2560
            Serial2hw.powerOff();
            Serial3hw.powerOff();
            Timer4hw.powerOff();
            Timer5hw.powerOff();
#endif
        } // powerOff()
    };

    static struct _allhw Allhw;
};

struct _AVRChipHardwareModules AVRchip;

#endif  // AVR_ACTIVEPOWER_LIB_H
