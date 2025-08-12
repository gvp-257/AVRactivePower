#ifndef HARDWARECONTROL_H
#define HARDWARECONTROL_H
/*
 * Save a few hundred microamps while active, when you're not using
 * analogRead, and/or Serial, SPI or Wire, by disabling those modules in the
 * ATmega328P chip.
 *
 *
 * ACTIVE MODE NOT SLEEP MODE
 * ==========================
 *
 * For low power **sleep** (potentially 0.1 microamp), see
 * RocketScream's LowPower library or the Narcoleptic library.
 *
 * Saving a few hundred microamps while active only makes sense for Pro Minis
 * with the power LED and voltage regulator removed, or breadboard Arduinos.
 *
 * Most boards are in the multi-milliamp current consumption range, because of
 * LEDs, inefficient voltage regulators, USB communication chips, etc.
 *
 * NOTE: Timer 0 is used extensively by the Arduino code
 * (e.g. `millis()`, `delay()`, `delayMicroseconds()`).
 *
 * So turn it back on after `StopAll()` if you need `delay()`, `millis()`, etc.
 *
 * The big power users are the ADC, the Serial interface, and SPI.
 *
 * DETAILS
 * =======
 *
 * Peripheral clocks have disable bits in the power reduction register, PRR:-
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
 * Before disabling a peripheral we need to zero its 'enable' bit(s)
 * in its control register.
 *
 * Timers, 0, 1, and 2 don't have 'enable' bits in their control registers.
 * Disabling their clocks in PRR will freeze them, stopping them generating
 * interrupts.
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
// usage: ATmega.ADC.powerOn(), ATmega.ADC.powerOff(), ATmega.ADC.isOn()

struct AVRChipHardwareModules
{
    static struct Allhw
    {
        bool isOn()
        {
            if (!AnalogComparatorhw.isOn()) return false;
            if (!ADChw.isOn()) return false;
            if (!Serialhw.isOn()) return false;
            if (!Wirehw.isOn()) return false;
            if (!SPIhw.isOn()) return false;
            if (!Timer2hw.isOn()) return false;
            if (!Timer1hw.isOn()) return false;
            if (!Timer0hw.isOn()) return false;
            return true;
        }
        void powerOn()
        {
            AnalogComparatorhw.powerOn();
            ADChw.powerOn();
            SPIhw.powerOn();
            Wirehw.powerOn();
            Serialhw.powerOn();
            Timer1hw.powerOn();
            Timer2hw.powerOn();
            Timer0hw.powerOn();
        }
        void powerOff()
        {
            AnalogComparatorhw.powerOff();
            ADChw.powerOff();
            SPIhw.powerOff();
            Wirehw.powerOff();
            Serialhw.powerOff();
            Timer1hw.powerOff();
            Timer2hw.powerOff();
            Timer0hw.powerOff();
        }
    } Allhw;

    static struct AChw
    {
        bool isOn()    {return !(ACSR & (1<<ACD));} // On if disable bit is 0
        void powerOn() {ACSR &= ~(1<<ACD);}
        void powerOff(){ACSR |=  (1<<ACD);}
    } AnalogComparatorhw;

    static struct adchw
    {
        bool isOn()      {return (!(PRR & (1<PRADC)) && (ADCSRA & (1<<ADEN)));}
        void powerOn()   {PRR    &= ~(1<<PRADC); ADCSRA |= (1<<ADEN);}
        void powerOff()  {ADCSRA &= ~(1<<ADEN);  PRR    |= (1<<PRADC);}
    } ADChw;

    static struct usart0hw
    {
        bool isOn() {return (!(PRR & (1<PRUSART0)) && (UCSR0B & (1<<TXEN0)));}
        void powerOn()
            {PRR &= ~(1<<PRUSART0); UCSR0B |= (1<<TXEN0) | (1<<RXEN0);}
        void powerOff()
            {UCSR0B &= ~((1<<TXEN0) | (1<<RXEN0));  PRR |= (1<<PRUSART0);}
    } Serialhw;

    static struct spihw
    {
        bool isOn() {return (!(PRR & (1<PRSPI)) && (SPCR & (1<<SPE))) ;}
        void powerOn()   {PRR  &= ~(1<<PRSPI); SPCR |= (1<<SPE);}
        void powerOff()  {SPCR &= ~(1<<SPE);   PRR  |= (1<<PRSPI);}
    } SPIhw;

    static struct twihw
    {
        bool isOn() {return (!(PRR & (1<PRTWI)) && (TWCR & (1<<TWEN))) ;}
        void powerOn()   {PRR  &= ~(1<<PRTWI);  TWCR |= (1<<TWEN);}
        void powerOff()  {TWCR &= ~(1<<TWEN);   PRR  |= (1<<PRTWI);}
    } Wirehw;

    static struct timer2hw
    {
        bool isOn()      {return (!(PRR & (1<PRTIM2)));}
        void powerOn()   {PRR &= ~(1<<PRTIM2);}
        void powerOff()  {PRR |= (1<<PRTIM2);}
    } Timer2hw;

    static struct timer1hw
    {
        bool isOn()      {return (!(PRR & (1<PRTIM1)));}
        void powerOn()   {PRR &= ~(1<<PRTIM1);}
        void powerOff()  {PRR |= (1<<PRTIM1);}
    } Timer1hw;

    static struct timer0hw
    {
        bool isOn()      {return (!(PRR & (1<PRTIM0)));}
        void powerOn()   {PRR &= ~(1<<PRTIM0);}
        void powerOff()  {PRR |= (1<<PRTIM0);}
    } Timer0hw;
};

struct AVRChipHardwareModules Hardware;

#endif

