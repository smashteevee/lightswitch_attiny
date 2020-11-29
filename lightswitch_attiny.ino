#define DEBUG
#define F_CPU 8000000
#define __AVR_ATtiny85__
#include <tiny_IRremote.h>
#include <tiny_IRremoteInt.h>
#include <Servo_ATTinyCore.h>
#include "DebugMacros.h"



Servo myservo;  // create servo object to control a servo

bool switchState = false;
const int STARTING_ANGLE = 45;
int pos = 0;    // variable to store the servo position
const int ANGLE_DELTA = 90;

// ATTIny85 Pins as defined by https://github.com/SpenceKonde/ATTinyCore/blob/master/avr/extras/ATtiny_x5.md
//const int PUSH_BUTTON_PIN = PIN_B2;
const int IR_PIN = PIN_B3;
const int SERVO_PIN = PIN_B4;

int pushButtonVal = 0;
const long int LIGHT_SWITCH_BUTTON_CODE = 0xF728D7;
unsigned long lastCommandMs = 0 ;   // last command received in Ms
const int ignoreCommandWindow = 3000; // 3000 ms window between valid commands

IRrecv irrecv(IR_PIN);
decode_results results;

void setup() {
  Serial.begin(9600); // Begin serial communication
  ACSR &= ~(1 << ACIE); // Disable RX , only use Tx
  ACSR |= ~(1 << ACD);

  myservo.attach(SERVO_PIN);  // attaches the servo to the servo object
  irrecv.enableIRIn(); // Starts the IR receiver

  // pinMode(PUSH_BUTTON_PIN, INPUT);    // declare pushbutton as input
}

void turnOn() {
  //delay(1000);
  switchState = true;
  DEBUG_PRINT("Turnon: starting from pos: "); DEBUG_PRINTLN(pos);
  for (pos = STARTING_ANGLE; pos <= STARTING_ANGLE + ANGLE_DELTA; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
}

void turnOff() {
  //delay(1000);
  switchState = false;
  DEBUG_PRINT("Turnoff: starting from pos: "); DEBUG_PRINTLN(pos);
  for (pos = pos; pos >= STARTING_ANGLE; pos -= 1) { // goes from 180 degrees to 0 degrees
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
}


#ifdef DEBUG
void dump(decode_results *results) {
  int count = results->rawlen;
  if (results->decode_type == UNKNOWN) {
    DEBUG_PRINT("Could not decode message");
  }
  else {
    if (results->decode_type == NEC) {
      DEBUG_PRINT("Decoded NEC: ");
    }
    else if (results->decode_type == SONY) {
      DEBUG_PRINT("Decoded SONY: ");
    }
    else if (results->decode_type == RC5) {
      DEBUG_PRINT("Decoded RC5: ");
    }
    else if (results->decode_type == RC6) {
      DEBUG_PRINT("Decoded RC6: ");
    }
   DEBUG_PRINT(results->value, HEX);
    DEBUG_PRINT(" (");
    DEBUG_PRINT(results->bits, DEC);
    DEBUG_PRINTLN(" bits)");
  }
  /* Serial.print("Raw (");
    Serial.print(count, DEC);
    Serial.print("): ");

    for (int i = 0; i < count; i++) {
     if ((i % 2) == 1) {
       Serial.print(results->rawbuf*USECPERTICK, DEC);
     }
     else {
       Serial.print(-(int)results->rawbuf*USECPERTICK, DEC);
     }
     Serial.print(" ");
    }*/
  DEBUG_PRINTLN("");
}
#endif

void loop() {
  //  pushButtonVal = digitalRead(PUSH_BUTTON_PIN);  // read input value
  // if (pushButtonVal == HIGH) {         // check if the input is HIGH (button released)

  //decodes the infrared input
  if (irrecv.decode(&results)) {
    DEBUG_PRINTLN("rx code");
    DEBUG_PRINTLN(results.value, HEX);

#ifdef DEBUG
    dump(&results);
#endif

    // Only process command if elapsed time since last command outside of ignore window
    if (millis() - lastCommandMs > ignoreCommandWindow) {


      if ((results.value) == LIGHT_SWITCH_BUTTON_CODE) {   // If I got a light switch code
        DEBUG_PRINTLN("Time to switch light!");
        if (switchState == false) {
          turnOn();
        } else {
          turnOff();
        }
      }
      // Update last command processed time
      lastCommandMs = millis();
    } else {
      DEBUG_PRINTLN("ignore");
    }

    irrecv.resume(); // Receives the next value from the button you press

  }
  /*else {
        if (switchState == false) {
       turnOn();
    } else {
      turnOff();
    }
    }*/

  delay(200);
}
