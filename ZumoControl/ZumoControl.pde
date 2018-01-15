import processing.serial.*;

final char _FORWARD = 'w';
final char _LEFT = 'a';
final char _BACK = 's';
final char _RIGHT = 'd';
final char _STOP = 'q';
final char _ON_LEFT = 'l';
final char _ON_RIGHT = 'r';
final char _CORRIDOR = 'y';
final char _ROOM = 'z';
final char _SCAN = 'x';
final char _COMPLETE = 'c';
final char _END = 'e';

// Need G4P library
import g4p_controls.*;

//Serial port for communication to/from Arduino
private Serial com4;
String message;

boolean firstContact = false;

public void setup() {
  size(720, 480, JAVA2D);
  createGUI();
  customGUI();
  // Place your setup code here

  com4 = new Serial(this, "COM4", 9600);
  com4.bufferUntil('\n');
}

public void draw() {
  background(230);
}

void serialEvent(Serial myPort) {
  //read in data to string
  //new line command indicates end of message
  message = myPort.readString();
  //some value provided from Arduino
  if (message != null) {
    //remove whitespace and formatting
    message = trim(message);
    println(message); //TEST

    if (!firstContact) { //executes on first message received
      if (message.equals("requestcontact")) {
        myPort.clear();
        firstContact = true;
        myPort.write('t');
        txtOutput.setText("Connection to Arduino established!");
      }
    } else { //on all subsequent messages after contact established
      txtOutput.setText(message);
    }
  }
}

// Use this method to add additional statements
// to customise the GUI controls
public void customGUI() {
}