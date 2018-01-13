#include <StandardCplusplus.h>
#include <system_configuration.h>
#include <unwind-cxx.h>
#include <utility.h>
#include <NewPing.h>
#include <ZumoMotors.h>
#include <QTRSensors.h>
#include <ZumoReflectanceSensorArray.h>
#include <vector>
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

//constant strings to pass back to GUI
#define EDGE_REACHED "Edge of track detected - turn required"
#define EDGE_REACHED_LEFT_ONLY "Sub corridor exited - turn left to continue"
#define EDGE_REACHED_RIGHT_ONLY "Sub corridor exited - turn right to continue"
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
#define MAX_DISTANCE 1000 //equals about 17cm

//number of sensors on Zumo
#define NUM_SENSORS 6
// -- CONSTANTS -- //

//initialise sensor object
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
ZumoMotors motors;
unsigned int sensor_values[NUM_SENSORS];
ZumoReflectanceSensorArray sensors(QTR_NO_EMITTER_PIN);

//globals to control turn behaviour between commands
bool turnRequired = false;
bool turnedLeft = false; //technically initialised to say that first corridor is off a right turn, but will never be used for this corridor
bool nextCorridorSelected = false;

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
}
void establishContact() {
  while (Serial.available() <= 0) {
    Serial.println("requestcontact");
    delay(DELAY_CONTACT);
  }
}
// -- setup function and methods used only during setup -- //

// -- loop for all input and output between arduino and user with GUI -- //
void loop()
{
  //some message sent through from GUI
  if (Serial.available() > 0) {
    //read in command
    char command = (char) Serial.read();
    Corridor currentCorridor;

    switch (command) {
      //Zumo will attempt to go forward on either command
      case _FORWARD: case _COMPLETE:
        //'complete' case willflag that turn is finished and update the current corridor
        if (command == _COMPLETE) {
          updateCurrentCorridor(currentCorridor, !turnRequired, turnedLeft);
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
            break;
          }

          //read in sensor values again before next iteration of loop
          sensors.read(sensor_values);
        }

        //exited loop
        //stop Zumo & wait for next command
        if (!currentCorridor.isSubCorridor()) {
          stopMotors(EDGE_REACHED);
        } else {
          if (currentCorridor.isLeftTurn()) {
            stopMotors(EDGE_REACHED_LEFT_ONLY);
          } else {
            stopMotors(EDGE_REACHED_RIGHT_ONLY);
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
      case _SCAN:
        scanRoomForObject(rooms.back()); break;
      default:
        Serial.println(INVALID_COMMAND); break;
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

    //update current corridor data if it hasn't already been handled through GUI
    updateCurrentCorridorOnTurn(currentCorridor);

    delay(DELAY_COMMAND);
  }
}

void turnRight(Corridor currentCorridor) {
  //don't allow right turn if sub corridor was on left of main corridor
  if (!isLeftTurnOnly(currentCorridor)) {
    digitalWrite(LED_PIN, HIGH);
    motors.setSpeeds(TURN_SPEED, -TURN_SPEED);
    turnedLeft = false;

    //update current corridor data if it hasn't already been handled through GUI
    updateCurrentCorridorOnTurn(currentCorridor);

    delay(DELAY_COMMAND);
  }
}

void retreat() {
  digitalWrite(LED_PIN, HIGH);
  motors.setSpeeds(REVERSE_SPEED, REVERSE_SPEED);
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
  return currentCorridor.isSubCorridor() && currentCorridor.isLeftTurn();
}

bool isRightTurnOnly(Corridor currentCorridor) {
  //zumo turned right into a sub corridor so can only turn right out
  return currentCorridor.isSubCorridor() && !currentCorridor.isLeftTurn();
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
  //use for loop to perform sweep motion
  //so sonar can see the whole room
  for (int i = 0; i < 80; i++)
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

      delay(DELAY_PAUSE);
    }

    //sweep completed
    //output a message to confirm that room is empty
    if (!room.containsObject()) {
      Serial.println(OBJECT_NOT_DETECTED + String(room.getRoomId()) + " (corridor " + String(room.getCorridorId()) + ")");
    }
  }
}

void checkForObject(Room room) {
  delay(DELAY_PAUSE);
  //object found within max range
  if (objectDetected()) {
    room.setContainsObject(true);
    Serial.println(OBJECT_DETECTED + String(room.getRoomId()) + " (corridor " + String(room.getCorridorId()) + ")");
  }
}

bool objectDetected() {
  return sonar.ping_cm() > 0;
}
// -- object detection methods -- //

// -- room and corridor detection and collection management -- //
void detectNewRoom(Corridor currentCorridor) {
  Room room;
  //set values to empty object
  room.setRoomId(roomCount++);
  room.setCorridorId(currentCorridor.getCorridorId());

  //request and read in user input
  Serial.println("Room " + String(room.getRoomId()) + " discovered on corridor " + String(room.getCorridorId()) +
                 "; is room " + String(room.getRoomId()) + " on the left-hand or right-hand side of the corridor?");
  

  room.setSideOfCorridor(isOnLeftHandSide());
  //hasObject value will be assigned during '_SCAN' routine

  //added as last item in collection
  //'SCAN' routine will 
  rooms.push_back(room);
}

void detectNewCorridor(Corridor currentCorridor) {
  //request and read in user input
  Serial.println("Is the entrance to the new corridor on the left-hand or right-hand side of corridor "
                 + String(currentCorridor.getCorridorId()) + "?");
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
  //flag corridor as being on side indicated by user
  //left is true, right is false
  turnedLeft = isOnLeftHandSide();
  updateCurrentCorridor(currentCorridor, !turnRequired, turnedLeft);
  //set flag to true, and avoid duplicate corridor object being added to collection during turn method
  nextCorridorSelected = true;
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
  currentCorridor.setCorridorId(corridorCount++);
  currentCorridor.setSubCorridor(isSubCorridor);
  currentCorridor.setTurnDirection(isOnLeft);

  //add corridor to collection
  corridors.push_back(currentCorridor);
}

void updateCurrentCorridorOnTurn(Corridor currentCorridor) {
  //add new corridor data to list if it hasn't already been entered through GUI buttons (case _CORRIDOR + _ON_LEFT/_ON_RIGHT)
  if (!nextCorridorSelected) {
    //Zumo has not come out of a sub-corridor
    //is entering a new main corridor
    if (!currentCorridor.isSubCorridor()) {
      updateCurrentCorridor(currentCorridor, turnedLeft, !turnRequired);
    }
    //Zumo has exited sub corridor
    //re-joining the connected main corridor
    else {
      //set corridor that is being re-entered as current corridor
      currentCorridor = getCorridorById(currentCorridor.getConnectingCorridorId());
    }
  }
  else {
    //turn to false so does not affect logging of subsequent turns
    nextCorridorSelected = false;
  }
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
// -- room and corridor detection and collection management -- //
