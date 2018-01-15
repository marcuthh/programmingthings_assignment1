//StandardCplusplus library from GitHub
#include <StandardCplusplus.h>
#include <system_configuration.h>
#include <unwind-cxx.h>
#include <utility.h>
#include <vector>
//other libraries
#include <ZumoMotors.h>
#include <QTRSensors.h>
#include <ZumoReflectanceSensorArray.h>
using namespace std;

class Room {
  private:
    int roomId;
    bool hasObject;
    int corridorId;
    bool onLeftOfCorridor;

  public:
    Room();
    int getRoomId();
    void setRoomId(int id);
    bool containsObject();
    void setContainsObject(bool contains);
    int getCorridorId();
    void setCorridorId(int id);
    bool isOnLeftOfCorridor();
    void setSideOfCorridor(bool isLeft);
};
Room::Room() {}
int Room::getRoomId() {
  return roomId;
}
void Room::setRoomId(int id) {
  roomId = id;
}
bool Room::containsObject() {
  return hasObject;
}
void Room::setContainsObject(bool contains) {
  hasObject = contains;
}
int Room::getCorridorId() {
  return corridorId;
}
void Room::setCorridorId(int id) {
  corridorId = id;
}
bool Room::isOnLeftOfCorridor() {
  return onLeftOfCorridor;
}
void Room::setSideOfCorridor(bool isLeft) {
  onLeftOfCorridor = isLeft;
}

class Corridor {
  private:
    int corridorId;
    bool subCorridor;
    int connectingCorridorId;
    bool onLeftTurn;
    bool endReached;

  public:
    Corridor();
    int getCorridorId();
    void setCorridorId(int id);
    bool isSubCorridor();
    void setSubCorridor(bool sub);
    int getConnectingCorridorId();
    void setConnectingCorridorId(int id);
    bool isLeftTurn();
    void setTurnDirection(bool isLeft);
    bool endHasBeenReached();
    void setEndReached(bool reached);
};
Corridor::Corridor() {}
int Corridor::getCorridorId() {
  return corridorId;
}
void Corridor::setCorridorId(int id) {
  corridorId = id;
}
bool Corridor::isSubCorridor() {
  return subCorridor;
}
void Corridor::setSubCorridor(bool sub) {
  subCorridor = sub;
}
int Corridor::getConnectingCorridorId() {
  return corridorId;
}
void Corridor::setConnectingCorridorId(int id) {
  corridorId = id;
}
bool Corridor::isLeftTurn() {
  return onLeftTurn;
}
void Corridor::setTurnDirection(bool isLeft) {
  onLeftTurn = isLeft;
}
bool Corridor::endHasBeenReached() {
  return endReached;
}
void Corridor::setEndReached(bool reached) {
  endReached = reached;
}

// -- GLOBALS -- //
// -- CONSTANTS -- //
//pin values
#define LED_PIN 13
#define TRIGGER_PIN 2
#define ECHO_PIN 6

//possible commands received from GUI
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
#define _COMPLETE 'c'
#define _END 'e'


//constant strings to pass back to GUI
#define EDGE_REACHED "Edge of track detected - turn required"
#define EDGE_REACHED_LEFT_ONLY "Back to sub corridor entrance - turn left to continue"
#define EDGE_REACHED_RIGHT_ONLY "Back to sub corridor entrance - turn right to continue"
#define OBJECT_DETECTED "Object detected in room "
#define OBJECT_NOT_DETECTED "No objects detected in room "
#define INVALID_COMMAND "Invalid command! That command cannot be used at this time"

//sensitivity to border
#define QTR_THRESHOLD  300
//speeds
#define REVERSE_SPEED     100
#define TURN_SPEED        100
#define FORWARD_SPEED     100
#define REVERSE_DURATION  200
#define TURN_DURATION     300
//delay times
#define DELAY_CONTACT 300
#define DELAY_PAUSE 50
#define DELAY_COMMAND 2
//ultrasonic sensor values
#define MAX_DISTANCE 17 //17cm scan range for detecting objects in room

//number of sensors on Zumo
#define NUM_SENSORS 6
// -- CONSTANTS -- //

//initialise sensor object
ZumoMotors motors;
unsigned int sensor_values[NUM_SENSORS];
ZumoReflectanceSensorArray sensors(QTR_NO_EMITTER_PIN);

//globals to control turn behaviour between commands
bool turnRequired = false;
bool turnedLeft = false; //technically initialised to say that first corridor is off a right turn, but will never be used for this corridor
bool subCorridorEnded = false;

//corridor and room collections
vector<Room> rooms;
vector<Corridor> corridors;
//counter variables for unique id's of new corridors and rooms
int roomCount = 0;
int corridorCount = 0;
// -- GLOBALS -- //

// -- setup function and methods used only during setup -- //
void setup()
{
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);
  establishContact();

  //set up initial corridor
  Corridor firstCorridor;
  updateCurrentCorridor(firstCorridor, false, false);
}
void establishContact() {
  while (Serial.available() <= 0) {
    Serial.println("requestcontact");
    delay(DELAY_CONTACT);
  }
  Serial.read();
}
// -- setup function and methods used only during setup -- //

// -- loop for all input and output between arduino and user with GUI -- //
void loop()
{
  //some message sent through from GUI
  if (Serial.available() > 0) {
    //read in command
    char command = (char) Serial.read();
    //indicates a stop command through GUI rather than wall reached
    bool commandedStop = false;

    //local variable holds data on Zumo's current corridor
    Corridor currentCorridor;
    //if it is null at this point, revert to using last corridor added to collection
    if (currentCorridor.getCorridorId() <= 0) {
      currentCorridor = corridors.back();
    }

    switch (command) {
      //Zumo will attempt to go forward on either command
      case _FORWARD: case _COMPLETE:
        //'complete' case willflag that turn is finished and update the current corridor
        if (command == _COMPLETE) {
          //updateCurrentCorridor(currentCorridor, !turnRequired, turnedLeft);
          turnRequired = false;
        }
        //'forward' command will not work if a turn is still required
        sensors.read(sensor_values); //read in sensor values to check if path ahead is clear

        while (isClearPath()) { //'wall' not yet reached
          if (Serial.read() != _STOP) {
            navigateForwards();
          }
          else {
            //hit when 'QUIT' command arrives, drops out of inner loop
            //calls stopMotors() by dropping out of loop
            commandedStop = true;
            break;
          }

          //read in sensor values again before next iteration of loop
          sensors.read(sensor_values);
        }

        //exited loop
        //stop Zumo & wait for next command
        if (commandedStop) { //stopped through GUI command and not wall detection
          stopMotors("");
          commandedStop = false;
        }
        else { //stopped through wall detection
          if (!currentCorridor.isSubCorridor()) { //while in main corridor
            stopMotors(EDGE_REACHED);
          } else {
            //still allow full set of turns while inside subCorridor
            if (!subCorridorEnded) {
              //end not yet flagged but zumo has stopped
              //meaning this must be the end
              //zumo must turn and head back to entrance
              subCorridorEnded = true;
              turnRequired = false;

              stopMotors("End of corridor " + String(currentCorridor.getCorridorId()) + " reached. Please turn around and return to the main corridor.");
              char turnDirection;
              validateForTurnCommands(turnDirection);
              
              if (turnDirection == _LEFT) {
                turnLeftIntoRoom();
              } else {
                turnRightIntoRoom();
              }
            }
            else {
              //back to sub corridor entrance
              //can only turn to continue previous path
              if (currentCorridor.isLeftTurn()) {
                stopMotors(EDGE_REACHED_LEFT_ONLY);
              } else {
                stopMotors(EDGE_REACHED_RIGHT_ONLY);
              }
            }
          }
        }

        break;
      case _LEFT:
        turnLeft(currentCorridor); break;
      case _BACK:
        retreat(); break;
      case _RIGHT:
        turnRight(currentCorridor); break;
      case _STOP:
        stopMotors(""); break;
      case _ROOM:
        detectNewRoom(currentCorridor); break;
      case _CORRIDOR:
        detectNewCorridor(currentCorridor); break;
      case _END:
        outputFindings();
      default:
        Serial.println("Invalid command! The command '" + String(command) + "' cannot be used at this time."); break;
    }
  }
}
// -- loop for all input and output between arduino and user with GUI -- //

//WASD & Q - motor control functions -- //
void onwards() {
  digitalWrite(LED_PIN, HIGH);
  motors.setSpeeds(FORWARD_SPEED, FORWARD_SPEED);
  delay(DELAY_COMMAND);
}

void turnLeft(Corridor currentCorridor) {
  //don't allow left turn if sub corridor was on right of main corridor
  if (!isRightTurnOnly(currentCorridor)) {
    digitalWrite(LED_PIN, HIGH);
    motors.setSpeeds(-TURN_SPEED, TURN_SPEED);
    turnedLeft = true;

    //update state of corridor and current collection
    updateCurrentCorridor(currentCorridor, !turnRequired, turnedLeft);

    delay(DELAY_COMMAND);
  }
  else {
    Serial.println("You can only turn right out of this corridor!");
  }
}

void turnRight(Corridor currentCorridor) {
  //don't allow right turn if sub corridor was on left of main corridor
  if (!isLeftTurnOnly(currentCorridor)) {
    digitalWrite(LED_PIN, HIGH);
    motors.setSpeeds(TURN_SPEED, -TURN_SPEED);
    turnedLeft = false;

    //update state of corridor and current collection
    updateCurrentCorridor(currentCorridor, !turnRequired, turnedLeft);

    delay(DELAY_COMMAND);
  }
  else {
    Serial.println("You can only turn left out of this corridor!");
  }
}

//simple turns not affected by corridor state
void turnLeftIntoRoom() {
  digitalWrite(LED_PIN, HIGH);
  motors.setSpeeds(-TURN_SPEED, TURN_SPEED);
  delay(DELAY_COMMAND);
}
//simple turns not affected by corridor state
void turnRightIntoRoom() {
  digitalWrite(LED_PIN, HIGH);
  motors.setSpeeds(TURN_SPEED, -TURN_SPEED);
  delay(DELAY_COMMAND);
}

void retreat() {
  digitalWrite(LED_PIN, HIGH);
  motors.setSpeeds(-REVERSE_SPEED, -REVERSE_SPEED);
  delay(DELAY_COMMAND);
  //cannot be left true if gone backwards from wall - must be allowed to go forwards to reach wall again
  turnRequired = false;
}

void stopMotors(String output) {
  digitalWrite(LED_PIN, HIGH);
  motors.setSpeeds(0, 0);
  if (!output.equals("")) {
    Serial.println(output);
    turnRequired = true;
  } else {
    Serial.println("Zumo stopped on command.");
    turnRequired = false;
  }

  delay(DELAY_COMMAND);
}
// -- WASD & Q - motor control functions -- //

// -- navigation helper methods -- //
void navigateForwards() {
  //only navigate forwards if a wall has not been reached that forces a turn
  if (!turnRequired) {
    //left-hand sensor detects line
    if (sensor_values[0] > QTR_THRESHOLD)
    {
      //delay and get read in new sensor values
      pauseToUpdateSensorValues();

      //use updated values to check if opposite sensor also detected line
      //'false' parameter turns zumo right
      reverseAndTurn(sensor_values[0], sensor_values[5], false);
    }
    //right-hand sensor detects line
    else if (sensor_values[5] > QTR_THRESHOLD)
    {
      //delay and get read in new sensor values
      pauseToUpdateSensorValues();

      //use updated values to check if opposite sensor also detected line
      //'true' parameter turns zumo left
      reverseAndTurn(sensor_values[5], sensor_values[0], true);
    }
    //no edges detected
    else
    {
      //go straight ahead until next line
      onwards();
    }
  }
}

bool isClearPath() {
  //has not reached a 'wall' directly in front
  return !(sensor_values[0] > QTR_THRESHOLD && sensor_values[5] > QTR_THRESHOLD);
}

bool isLeftTurnOnly(Corridor currentCorridor) {
  //zumo turned left into a sub corridor so can only turn left out
  return currentCorridor.isSubCorridor() && currentCorridor.isLeftTurn() && subCorridorEnded;
}

bool isRightTurnOnly(Corridor currentCorridor) {
  //zumo turned right into a sub corridor so can only turn right out
  return currentCorridor.isSubCorridor() && !currentCorridor.isLeftTurn() && subCorridorEnded;
}

//idea taken from Arduino Forum post for similar issue
//called whenever one sensor reaches a line
//gives time to updates values for other sensors
void pauseToUpdateSensorValues() {
  delay(DELAY_PAUSE);
  motors.setSpeeds(0, 0);
  sensors.read(sensor_values);
}

void reverseAndTurn(int sensorValue, int oppositeSensorValue, bool goLeft) {
  if (sensorValue > QTR_THRESHOLD && !(oppositeSensorValue > QTR_THRESHOLD)) {
    motors.setSpeeds(-REVERSE_SPEED, -REVERSE_SPEED);
    delay(REVERSE_DURATION);
    if (goLeft) {
      motors.setSpeeds(-TURN_SPEED, TURN_SPEED);
    }
    else {
      motors.setSpeeds(TURN_SPEED, -TURN_SPEED);
    }
    delay(TURN_DURATION);
    motors.setSpeeds(FORWARD_SPEED, FORWARD_SPEED);
  }
}
// -- navigation helper methods -- //

// -- object detection methods -- //
void scanRoomForObject(Room room) {
  Serial.println("Scanning room " + String(room.getRoomId()) + " (corridor " + String(room.getCorridorId()) + ")...");
  //use for loop to perform sweep motion
  //so sonar can see the whole room
  for (int i = 0; i < 10; i++) //maximum of 10 sweeps before concluded that no object found
  {
    if (!room.containsObject()) {
      if ((i > 10 && i <= 30) || (i > 50 && i <= 70)) {
        motors.setSpeeds(-TURN_SPEED, TURN_SPEED);
        //perform object detect routine at every pause in turn
        checkForObject(room);
      }
      else {
        motors.setSpeeds(TURN_SPEED, -TURN_SPEED);
        //perform object detect routine at every pause in turn
        checkForObject(room);
      }
    }
    else {
      break; //exit for loop
    }
    stopMotors(OBJECT_DETECTED + String(room.getRoomId()) + " (corridor " + String(room.getCorridorId()) + ")");
    delay(DELAY_PAUSE);
  }

  //sweep completed
  //output a message to confirm that room is empty
  if (!room.containsObject()) {
    stopMotors(OBJECT_NOT_DETECTED + String(room.getRoomId()) + " (corridor " + String(room.getCorridorId()) + ")");
  }
  //pause to display message for long enough
  delay(2000);

  //turn zumo back into corridor
  if (room.isOnLeftOfCorridor()) {
    Serial.println("Scan of " + String(room.getRoomId()) + " completed. Please turn right back into the corridor.");
    validateForCommand(_RIGHT, "Right");
    turnRightIntoRoom(); //not the cleanest, but effectively undoes the simple turn into the room
  }
  else {
    Serial.println("Scan of " + String(room.getRoomId()) + " completed. Please turn left back into the corridor.");
    validateForCommand(_LEFT, "Left");
    turnLeftIntoRoom(); //not the cleanest, but effectively undoes the simple turn into the room
  }
  Serial.println("Click 'Stop' when you are facing back into corridor " + String(room.getCorridorId()));
  validateForCommand(_STOP, "Stop");
  stopMotors("You can now continue your search");
}

void checkForObject(Room room) {
  delay(DELAY_PAUSE);
  //object found within max range
  if (objectDetected()) {
    room.setContainsObject(true);
  }
}

bool objectDetected() {
  //attempt with NewPing library got nothing - constantly returned 0
  //reverted back to code from ObjectDetect example - where this code is borrowed from

  // establish variables for duration of the ping,
  // and the distance result in inches and centimeters:
  long duration, dist;

  // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  pinMode(TRIGGER_PIN, OUTPUT);
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);

  // Read the signal from the sensor: a HIGH pulse whose
  // duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  pinMode(ECHO_PIN, INPUT);
  duration = pulseIn(ECHO_PIN, HIGH);

  // convert the time into a distance
  dist = microsecondsToCentimeters(duration);

  return dist < MAX_DISTANCE;
}

long microsecondsToCentimeters(long microseconds) {
  //borrowed method from ObjectDetect example

  // The speed of sound is 340 m/s or 29 microseconds per centimeter.
  // The ping travels out and back, so to find the distance of the
  // object we take half of the distance travelled.
  return microseconds / 29 / 2;
}
// -- object detection methods -- //

// -- room and corridor detection and collection management -- //
void detectNewRoom(Corridor currentCorridor) {
  Room room;
  //set values to empty object
  room.setRoomId(roomCount += 1);
  room.setCorridorId(currentCorridor.getCorridorId());
  room.setContainsObject(false);

  //request and read in user input
  Serial.println("Room " + String(room.getRoomId()) + " discovered on corridor " + String(room.getCorridorId()) +
                 "; is room " + String(room.getRoomId()) + " on the left-hand or right-hand side of the corridor?");

  bool isLeft = isOnLeftHandSide();
  room.setSideOfCorridor(isLeft);

  if (isLeft) {
    Serial.println("Please turn left");
    validateForCommand(_LEFT, "Left");
    //valid input - exit loop
    turnLeftIntoRoom();

    Serial.println("Click 'Stop' when you are facing into room " + String(room.getRoomId()));
    validateForCommand(_STOP, "Stop");
    //valid input - exit loop
    //ask user to detect objects in room
    stopMotors("Click 'Scan room' to check room " + String(room.getRoomId()) + " for objects");

    validateForCommand(_SCAN, "Scan room");
  }
  else {
    Serial.println("Please turn right");
    validateForCommand(_RIGHT, "Right");
    //valid input - exit loop
    turnRightIntoRoom();

    Serial.println("Click 'Stop' when you are facing into room " + String(room.getRoomId()));
    validateForCommand(_STOP, "Stop");
    //valid input - exit loop
    //ask user to detect objects in room
    stopMotors("Click 'Scan room' to check room " + String(room.getRoomId()) + " for objects");

    validateForCommand(_SCAN, "Scan room");
  }
  //valid input - exit loop
  //perform scan of room
  scanRoomForObject(room);

  //added as last item in collection
  //'SCAN' routine will
  rooms.push_back(room);
}

void detectNewCorridor(Corridor currentCorridor) {
  //request and read in user input
  Serial.println("Is the entrance to the new corridor on the left-hand or right-hand side of corridor "
                 + String(currentCorridor.getCorridorId()) + "?");

  //flag corridor as being on side indicated by user
  //left is true, right is false
  turnedLeft = isOnLeftHandSide();
  if (turnedLeft) {
    Serial.println("Please turn left");
    validateForCommand(_LEFT, "Left");
    //valid input - exit loop
    //turn zumo left into corridor
    turnLeft(currentCorridor);
  }
  else {
    Serial.println("Please turn right");
    validateForCommand(_RIGHT, "Right");
    //valid input - exit loop
    //turn zumo right into corridor
    turnRight(currentCorridor);
  }
}

//generic validation that does not allow code to continue until the required button has been pressed
void validateForCommand(char requiredCommand, String buttonLabel) {
  char subCommand;
  //wait for and read in user input
  while (Serial.available() <= 0) {
    delay(DELAY_CONTACT);
  }
  if (Serial.available() > 0) {
    subCommand = Serial.read();
  }

  //cannot progress until valid option has been clicked
  while (!subCommand == requiredCommand) {
    Serial.println("Error! Please click '" + buttonLabel + "'");
    subCommand = (char) Serial.read();
  }
}

//hard-coded alternative to validateForCommands() that will allow a turn in either direction
//if any bigger, array of accepted commands could be used, but that gets a bit heavy for the purposes of this
void validateForTurnCommands(char turnDirection) {
  //wait for and read in user input
  while (Serial.available() <= 0) {
    delay(DELAY_CONTACT);
  }
  if (Serial.available() > 0) {
    turnDirection = Serial.read();
  }

  //cannot progress until valid option has been clicked
  while (!(turnDirection == _LEFT || turnDirection == _RIGHT)) {
    Serial.println("Error! Please click 'Left' or 'Right'");
    turnDirection = (char) Serial.read();
  }
}

bool isOnLeftHandSide() {
  char side;
  while (Serial.available() <= 0) {
    delay(DELAY_CONTACT);
  }
  if (Serial.available() > 0) {
    side = Serial.read();
  }

  //cannot progress until one of the two valid options has been clicked
  while (!(side == _ON_LEFT || side == _ON_RIGHT)) {
    Serial.println("Error! Please click 'On left' or 'On right'");
    side = (char) Serial.read();
  }
  //valid input - exit loop

  //return true if user flagged left
  //or false if right
  return (side == _ON_LEFT);
}

//set all values for new corridor entered and add it to the collection
void updateCurrentCorridor(Corridor currentCorridor, bool isSubCorridor, bool isOnLeft) {
  //set values using parameter values
  //apart from corridor id being set using unique global
  currentCorridor.setConnectingCorridorId(currentCorridor.getCorridorId());
  currentCorridor.setCorridorId(corridorCount += 1);
  currentCorridor.setSubCorridor(isSubCorridor);
  currentCorridor.setTurnDirection(isOnLeft);
  subCorridorEnded = false //false until wall detected

  //add corridor to collection
  corridors.push_back(currentCorridor);
}

Corridor getCorridorById(int id) {
  if (corridors.size() > 0) {
    for (int i = 0; i < corridors.size(); i++) {
      if (corridors.at(i).getCorridorId() == id) {
        return corridors.at(i);
      }
    }

    //return empty object
    //can have id checked for > 0
    return Corridor();
  }
}

void outputFindings() {
  String outputStr;
  outputStr = "Total rooms detected and scanned: " + String(rooms.size() + "\n");
  if (rooms.size() > 0) {
    int objectCount = 0;
    for (int i = 0; i < rooms.size(); i++) {
      outputStr += "Room " + String(rooms.at(i).getRoomId()) + " (corridor " + String(rooms.at(i).getCorridorId()) + "): ";
      if (rooms.at(i).containsObject()) {
        outputStr += "object found\n";
        objectCount++;
      }
      else {
        outputStr += "no object found\n";
      }
    }
    outputStr += "Number of rooms containing an object: " + String(objectCount);
  }
}
// -- room and corridor detection and collection management -- //
