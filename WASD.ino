#include <ZumoBuzzer.h>
#include <ZumoMotors.h>
#include <Pushbutton.h>
#include <QTRSensors.h>
#include <ZumoReflectanceSensorArray.h>

#define LED_PIN 13
#define _FORWARD 'w'
#define _LEFT 'a'
#define _BACK 's'
#define _RIGHT 'd'
#define _STOP 'q'
#define _ON_LEFT 'l'
#define _ON_RIGHT 'r'
#define _CORRIDOR 'y'
#define _ROOM 'z'
#define _SCAN 'x'
#define _COMPLETE 'f'

// this might need to be tuned for different lighting conditions, surfaces, etc.
#define QTR_THRESHOLD  300 // microseconds

// these might need to be tuned for different motor types
#define REVERSE_SPEED     200 // 0 is stopped, 400 is full speed
#define TURN_SPEED        200
#define FORWARD_SPEED     200
#define REVERSE_DURATION  200 // ms
#define TURN_DURATION     300 // ms

ZumoBuzzer buzzer;
ZumoMotors motors;
Pushbutton button(ZUMO_BUTTON); // pushbutton on pin 12

#define NUM_SENSORS 6
unsigned int sensor_values[NUM_SENSORS];

ZumoReflectanceSensorArray sensors(QTR_NO_EMITTER_PIN);

void waitForButtonAndCountDown()
{
  digitalWrite(LED_PIN, HIGH);
  button.waitForButton();
  digitalWrite(LED_PIN, LOW);

  // play audible countdown
  for (int i = 0; i < 3; i++)
  {
    delay(1000);
    buzzer.playNote(NOTE_G(3), 200, 15);
  }
  delay(1000);
  buzzer.playNote(NOTE_G(4), 500, 15);
  delay(1000);
}

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);
  // uncomment one or both of the following lines if your motors' directions need to be flipped
  //motors.flipLeftMotor(true);
  //motors.flipRightMotor(true);
  establishContact();

  waitForButtonAndCountDown();
}

void loop()
{
  if (Serial.available() > 0) {
    char command = (char) Serial.read();
    switch (command) {
      case _FORWARD:
        while (!(sensor_values[0] > QTR_THRESHOLD && sensor_values[5] > QTR_THRESHOLD)) {
          if (Serial.read() != _STOP) {
            sensors.read(sensor_values);

            if (sensor_values[0] > QTR_THRESHOLD)
            {
              // if leftmost sensor detects line, reverse and turn to the right
              motors.setSpeeds(-REVERSE_SPEED, -REVERSE_SPEED);
              delay(REVERSE_DURATION);
              motors.setSpeeds(TURN_SPEED, -TURN_SPEED);
              delay(TURN_DURATION);
              motors.setSpeeds(FORWARD_SPEED, FORWARD_SPEED);
            }
            else if (sensor_values[5] > QTR_THRESHOLD)
            {
              // if rightmost sensor detects line, reverse and turn to the left
              motors.setSpeeds(-REVERSE_SPEED, -REVERSE_SPEED);
              delay(REVERSE_DURATION);
              motors.setSpeeds(-TURN_SPEED, TURN_SPEED);
              delay(TURN_DURATION);
              motors.setSpeeds(FORWARD_SPEED, FORWARD_SPEED);
            }
            else
            {
              // otherwise, go straight
              motors.setSpeeds(FORWARD_SPEED, FORWARD_SPEED);
            }
          }
          else {
            break;
          }
        }
        //dropped out of loop because line detected
        //stop motors
        motors.setSpeeds(0, 0);
        delay(2);
        break;
      //wait for next command
      case _LEFT:
        digitalWrite(LED_PIN, HIGH);
        motors.setLeftSpeed(-100);
        motors.setRightSpeed(100);
        delay(2);
        break;
      case _BACK:
        digitalWrite(LED_PIN, HIGH);
        motors.setLeftSpeed(-100);
        motors.setRightSpeed(-100);
        delay(2);
        break;
      case _RIGHT:
        digitalWrite(LED_PIN, HIGH);
        motors.setLeftSpeed(100);
        motors.setRightSpeed(-100);
        delay(2);
        break;
      case _STOP:
        digitalWrite(LED_PIN, HIGH);
        motors.setLeftSpeed(0);
        motors.setRightSpeed(0);
        delay(2);
        break;
    }
  }
}

void establishContact() {
  while (Serial.available() <= 0) {
    Serial.println("requestcontact");
    delay(300);
  }
}

