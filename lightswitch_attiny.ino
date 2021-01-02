#define F_CPU 8000000
#define __AVR_ATtiny85__
#include <Servo_ATTinyCore.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#define adc_disable() (ADCSRA &= ~(1<<ADEN)) // disable ADC (before power-off)

//#define DEBUG
#include "DebugMacros.h"

// IRMP-specific defines
#define IRMP_USE_COMPLETE_CALLBACK       1 // Enable callback functionality
#define IRMP_ENABLE_PIN_CHANGE_INTERRUPT   // Enable interrupt functionality
#define IRMP_INPUT_PIN   PIN_PB2
#define IRMP_SUPPORT_NEC_PROTOCOL        1
// Now IRMP defines are done, compile it
#include <irmp.c.h>
IRMP_DATA irmp_data;
void handleReceivedIRData();

Servo myservo;
volatile byte gotCode = 0;
volatile bool turnSwitchOn = false;

const int STARTING_ANGLE = 90;
const int ANGLE_DELTA = 45;
// ATTIny85 Pins per https://github.com/SpenceKonde/ATTinyCore/blob/master/avr/extras/ATtiny_x5.md
const int SERVO_PIN = PIN_B4;
const int MOSFET_GATE_PIN = PIN_B3;

const long int LIGHT_SWITCH_BUTTON_ON_CODE = 0x3;// On BUTTON ON REMOTE
const long int LIGHT_SWITCH_BUTTON_OFF_CODE = 0x2; // Off Button

unsigned long lastCommandMs = 0 ;   // last command received in Ms
const int validCommandWindow = 5000; // 5000 ms window of no commands before sleeping

void setup() {
#ifdef DEBUG
  Serial.begin(9600); // Begin serial communication
  ACSR &= ~(1 << ACIE); // Disable RX , only use Tx
  ACSR |= ~(1 << ACD);
#endif

  irmp_init(); // Starts the IR receiver
  irmp_register_complete_callback_function(&handleReceivedIRData);

  DDRB |= (1 << MOSFET_GATE_PIN);   // Set Gate Pin as Output
  PORTB |= (1 << MOSFET_GATE_PIN); // Set Mosfet Gate Pin HIGH to turn on Servo Power Supply
  delay(3000);
  myservo.attach(SERVO_PIN);  // attaches the servo to the servo object

}

/*
 * Stateless toggle switch function
 */
void toggleSwitch(bool turnOn) {
  int pos = 0;
  
  if (turnOn) { // Turn on light
    for (pos = STARTING_ANGLE; pos >= ANGLE_DELTA; pos -= 1) { // goes from pos to Off rocker press
      // in steps of 1 degree
      myservo.write(pos);              // tell servo to go to position in variable 'pos'
      delay(15);                       // waits 15ms for the servo to reach the position
    }
  } else { // Turn off light
      for (pos = STARTING_ANGLE; pos <= STARTING_ANGLE + ANGLE_DELTA; pos += 1) { // goes from pos to On rocker press

      myservo.write(pos);              // tell servo to go to position in variable 'pos'
      delay(15);                       // waits 15ms for the servo to reach the position
    }
  }
  
  DEBUG_PRINTLN(pos);
  // Reset to neutral position as switch may be toggled manually by people
  myservo.write(STARTING_ANGLE);

}

/*
   Callback function for IRMP received data - triggered by Pin change interrupt
*/
void handleReceivedIRData()

{
  irmp_get_data(&irmp_data);
  interrupts();  // enable interrupts now that we got our data
  if (!(irmp_data.flags & IRMP_FLAG_REPETITION)) {
    switch (irmp_data.command) {
      case LIGHT_SWITCH_BUTTON_ON_CODE:
        turnSwitchOn = true; // Turn on switch
        gotCode = 1;
        break;
      case LIGHT_SWITCH_BUTTON_OFF_CODE:
        turnSwitchOn = false; // Turn off switch
        gotCode = 1;
        break;
    }
  }
  //DEBUG_PRINTLN(irmp_data.command, HEX);

}

/*
   Function to sleep and wake - per 3.3V Alakaline AAs should cut down from 5mA consumption to `239uA
*/
void sleepNow() {
  // TODO: Disable interrupts from happening here?
  PORTB &= ~(1 << MOSFET_GATE_PIN);              // Set Mosfet Gate Pin LOW to turn off Supply
  myservo.detach();                                        // Detach Servo control

  adc_disable();                          // ADC disable saves another 200+ uA
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // Set sleep mode
  sleep_enable();                          // Enables the sleep bit in the mcucr register so sleep is possible

  sleep_cpu();                          // Zzzzzzzzzz...

  sleep_disable();                       // first thing after waking from sleep: clear SE bit
  PORTB |= (1 << MOSFET_GATE_PIN);       // Turn Mosfet Gate Pin HIGH to turn on Servo Supply
  myservo.attach(SERVO_PIN);             // re-attach servo control TODO: Move into function
}

/*
   Main loop
*/
void loop() {

  // Only sleep if elapsed time since last command is outside of recent window
  if (millis() - lastCommandMs > validCommandWindow) {
    sleepNow();
    lastCommandMs = millis();                  // Reset timestamp of last command as we wake up

  } else {

    if (gotCode) {  // awake and doing servo stuff
      gotCode = 0;  // reset flag

      toggleSwitch(turnSwitchOn); // toggle light switch

      // Update last command processed time
      lastCommandMs = millis();

    }
  }
}
