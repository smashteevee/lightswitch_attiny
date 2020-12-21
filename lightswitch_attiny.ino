#define F_CPU 8000000
#define __AVR_ATtiny85__
#include <Servo_ATTinyCore.h>
#define DEBUG
#include "DebugMacros.h"

#define IRMP_USE_COMPLETE_CALLBACK       1 // Enable callback functionality
#define IRMP_ENABLE_PIN_CHANGE_INTERRUPT   // Enable interrupt functionality
#define IRMP_INPUT_PIN   PIN_PB2
#define IRMP_SUPPORT_NEC_PROTOCOL        1
/*
   After setting the definitions we can include the IRMP code and compile it.
*/
#include <irmp.c.h>
IRMP_DATA irmp_data;
void handleReceivedIRData();

Servo myservo;
volatile byte gotCode = 0;

bool switchState = false;
const int STARTING_ANGLE = 45;
int pos = 0;    // variable to store the servo position
const int ANGLE_DELTA = 90;

// ATTIny85 Pins as defined by https://github.com/SpenceKonde/ATTinyCore/blob/master/avr/extras/ATtiny_x5.md
const int SERVO_PIN = PIN_B4;

const long int LIGHT_SWITCH_BUTTON_CODE = 0xF728D7;


void setup() {
  Serial.begin(9600); // Begin serial communication
  ACSR &= ~(1 << ACIE); // Disable RX , only use Tx
  ACSR |= ~(1 << ACD);

  irmp_init(); // Starts the IR receiver
  irmp_register_complete_callback_function(&handleReceivedIRData);

  myservo.attach(SERVO_PIN);  // attaches the servo to the servo object


}

void turnOn() {
  switchState = true;
  DEBUG_PRINT("Turnon: "); DEBUG_PRINTLN(pos);
  for (pos = STARTING_ANGLE; pos <= STARTING_ANGLE + ANGLE_DELTA; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
}

void turnOff() {
  switchState = false;
  DEBUG_PRINT("Turnoff: "); DEBUG_PRINTLN(pos);
  for (pos = pos; pos >= STARTING_ANGLE; pos -= 1) { // goes from 180 degrees to 0 degrees
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
}

void handleReceivedIRData()

{
  irmp_get_data(&irmp_data);
  interrupts();  // enable interrupts now that we got our data
  if (!(irmp_data.flags & IRMP_FLAG_REPETITION)) // filter out repeated codes // (irmp_data.command) == LIGHT_SWITCH_BUTTON_CODE)
  {
    DEBUG_PRINTLN("code.");
    //irmp_result_print(&irmp_data);
    gotCode = 1;
  }
}

void loop() {
  if (gotCode) {
    gotCode = 0;
    if (switchState == false) {
      turnOn();
    } else {
      turnOff();
    }

  }
}
