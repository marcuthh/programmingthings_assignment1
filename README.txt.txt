13-Dec-17: Started assignment - XBee shield and sensor array were replaced by Pete, everything else working

19-Dec-17: Ran Zumo through LineFollower, BorderDetect, ObjectDetect & WASD (using Arduino serial port input)

20-Dec-17: Built GUI in Processing, and linked this to the port in the Arduino so the two environments could communicate
	   Set up a series on constants used in both programs to represent all possible commands sent from GUI

21-Dec-17: Connection established and commands sent from GUI to Zumo. WASD functionality works, except for bug where Zumo starts by turning left until stopped.
	   Bug introduced somewhere along the line, first command is now received and executed, while subsequent ones are received and not executed by Zumo

22-Dec-17: Bug from yesterday not yet resolved. Focused on another area of work by creating classes for Room and Corridor.
	   Bug still not fixed but Zumo will take the first command to drive forwards, so code implementation begun to start corridor navigation.
	   Formula for the forwards navigation is a combination of the WASD code and the BorderDetect code...
	   Zumo will receive 'forward' command and will go forwards, running BorderDetect code to change direction every time a border is detected on the left or right.
           At this point, Zumo is staying within the lines of the track but is not stopping at the corner, doesn't seem to be detecting the line at the end.

//CHRISTMAS - BREAK TO FOCUS ON FYP

06-Jan-18: Starting back with a general tidy-up. Divvying-up of code into methods rather than huge chunks inside the core switch statement.
	   Attempted to hook up to version control using Git but run into a ton of issues with the project's library dependencies etc. being in different folders.
	   Wanted to have the repository set up by now but may have to take a back seat if it's going to be more difficult than expected.

08-Jan-18: Testing the Zumo's ability to detect the line by executing 'forward' command and physically holding the sensor array over the end line.
           Didn't detect at first, so QTR_THRESHOLD constant has been lowered to 300ms. After this, Zumo stops trying to move once held over the end line.
	   Still not stopping when put on the track to run. Googled the issue and found a post about how speed may be affecting the sensors' ability to detect the line.
	   Speed lowered. Still no detection of line.
	   Found a piece of code on an Arduino Forum post that has fixed the error. Now, whenever a line is detected on either side, Zumo pauses and gets latest sensor values.
           This has given it time to detect the line with both the far-left and far-right sensor. Code can be seen in method 'pauseToUpdateSensorValues()'
           Solution has made the Zumo's progress slightly jerky, as it now stops for 50ms on every line detection, but worth it at this point for carrying on development.

09-Jan-18: With navigating now largely handled, beginning work on collecting data of Room and Corridor objects that are found on the track.
	   Specification is vague in regard to what constitutes a new Corridor. Track contains main corridor and sub corridor, not sure if turn = new corridor or same one.
	   Assumption made that a turn leads to a new corridor, apart from in the situation described in Task 4C where the Zumo is returning to main corridor out of sub corridor.

10-Jan-18: Decision made to use a boolean value to indicate left and right for turns, rooms and corridors.
	   Only ever two options available, so nothing is earned by using a struct or enum.
	   As a rule for the whole project: true = left, false = right.