/*
    Digital Clock
    Jordi Ginjaume
    Version 20190118
    Code started 20190110

    Arduino code for digital clock, 3D printed inspired by www.otvinta.com, using the following hardware:
    1 Arduino
    2 PCA9685 servo drivers
    RTC DS3231 real time clock
    28 SG90 servos

    Enjoy!

*/

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h> // PCA9685 library
#include <RTClib.h> //Real Time Clock library


Adafruit_PWMServoDriver servoHours = Adafruit_PWMServoDriver(0x40); //first PCA9685 address
Adafruit_PWMServoDriver servoMinutes = Adafruit_PWMServoDriver(0x41); //second PCA9685 address
RTC_DS3231 rtc; //RTC address

const int enablepin = 12, setmodepin = 4, setpluspin = 2, setminuspin = 3;
const byte mapchar [11][7] = { //for each number, position of every segment
  {1, 1, 1, 0, 1, 1, 1}, // zero - segment number from top lo low, left to right
  {0, 0, 1, 0, 0, 1, 0}, // one
  {1, 0, 1, 1, 1, 0, 1}, // two
  {1, 0, 1, 1, 0, 1, 1}, // three
  {0, 1, 1, 1, 0, 1, 0}, // four
  {1, 1, 0, 1, 0, 1, 1}, // five
  {1, 1, 0, 1, 1, 1, 1}, // six
  {1, 0, 1, 0, 0, 1, 0}, // seven
  {1, 1, 1, 1, 1, 1, 1}, // eight
  {1, 1, 1, 1, 0, 1, 1}, // nine
  {0, 0, 0, 0, 0, 0, 0} // null (all down)
};
const int servopulse [2] {210, 450}; // pulse of servos at low positions and high position
const byte servoreverse [4] [7] = { // one identifies wservos that work reverse, zero is normal direction
  {0, 1, 0, 0, 1, 0, 0},
  {1, 0, 0, 1, 0, 1, 1},
  {1, 1, 0, 1, 0, 1, 1},
  {1, 1, 0, 1, 0, 1, 1}
};
const int servofinetune [4] [7] = { // pulse to add to servopulse for fine tunning of each individual servo
  {0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0},
  {-70, -20, 60, -50, -40, 110, 20}
};
byte moment [4] = {8, 8, 8, 8}; // actual time
byte momentnull [4] = {10, 10, 10, 10}; // all digits in null for startup
byte momentdisplay [4]; //time shown in display

void setup() {

  Serial.begin(9600);
  rtc.begin();
  pinMode(enablepin, OUTPUT); // this is the pin for OE in the PCA (enable/disable servos LOW/HIGH)
  pinMode(setmodepin, INPUT); // this is the pin for set time button
  pinMode(setpluspin, INPUT); // this is the pin for set plus button
  pinMode(setminuspin, INPUT); // this is the pin for set minus button
  servoHours.begin();
  servoHours.setPWMFreq(50); // set frequncy for first PCA
  servoMinutes.begin();
  servoMinutes.setPWMFreq(50); // set frequency for second PCA
  showtime(moment, 0);
  showtime(momentnull, 50);
}

void loop() {
  capturetime(); // captura el momento!
  showtime(moment, 0); // to the main routine that shows the time in our digital clock
  while (momentdisplay[3] == moment[3]) { // wait while minutes doesnt change...
    if (digitalRead(setmodepin) == HIGH) setmode(momentdisplay);
    capturetime();
  }
}

void capturetime() {
  DateTime now = rtc.now(); // save in variable now the actual time;
  moment[0] = now.hour() / 10; // find out first digit from hour
  moment[1] = now.hour() - moment[0] * 10; // find out second digit from hour
  moment[2] = now.minute() / 10; // find our first digit from minute
  moment[3] = now.minute() - moment[2] * 10; // find out second digit from minute
}

void showtime(byte m [4], int w) {
  int d = 275; // delay needed to stop the servos moving before disabling them
  // parameters: matrix with the digits to show
  if (memcmp(m, momentdisplay, 4) != 0) { // if time displayed is different to time expected to be shown
    digitalWrite(enablepin, LOW); // enable the servos
    for (int i = 0; i < 4; i++) { // for every single position, show only if different to actually shown
      if (m[i] != momentdisplay[i]) {
        showdigit(i, m[i], w);
        momentdisplay [i] = m[i]; // take note of the shown digit
      }
    }
    delay(d); // give time to the servos to stop moving before disabling them
    digitalWrite(enablepin, HIGH); // disable the servos
  }
}

void showdigit(byte i, byte digit, int w) {
  // parameters: digit position =01:23, digit to show
  int pulse;
  byte servonum, a, segmentposition;

  for (byte j = 0; j < 7; j++) { // show the 7 segments
    servonum = j + (8 * ((i == 1 || i == 3))); // add 8 if position is 1 or 3 because 1 ands 3 digits start on pin 8 of the PCA
    segmentposition = mapchar[digit] [j]; // segment should be low or high?
    if (servoreverse [i] [j] == 0) {
      pulse = servopulse[segmentposition]; // for a normal (not reverse) servo assign pulse low or high
    }
    else {
      pulse = servopulse[!segmentposition]; // if servo works reverse, assign contrary
    }
    pulse = pulse + servofinetune [i] [j]; // add the fine tunning to the servo

    switch (i) { // switch for both PCA controllers
      case 0: case 1:
        servoHours.setPWM(servonum, 0, pulse);
        break;
      case 2: case 3:
        servoMinutes.setPWM(servonum, 0, pulse);
        break;
    }
    delay(w); // delay in between movement of contiguous segments, so we can do waves oleeeeeeee. Normally is 0.
  }
}

void setmode(byte m[4]) {
  // parameter is the moment in display and the one we will set and finally save in the RTC
  int hourset = m[0] * 10 + m[1];
  int minuteset = m[2] * 10 + m[3];
  byte mset[4];
  int d = 200; // delay on button press

  // set hours
  mset[0] = m[0]; //mset is the array to show while setting time
  mset[1] = m[1];
  mset[2] = 10; // we start setting hours, so let's show minutes blank
  mset[3] = 10;
  showtime(mset, 0);
  delay(d); // the delays in this routine are the wait between button press

  while (digitalRead(setmodepin) != HIGH) { // wait in set hour mode until we press again the set button
    if (digitalRead(setpluspin) == HIGH) {
      hourset = (hourset + 1) % 24; // add one hour and do modulo 24
      mset[0] = hourset / 10; // assign to mset in order to be displayed
      mset[1] = hourset - mset[0] * 10;
      showtime(mset, 0); // display the set time
      delay(d);
    }
    if (digitalRead(setminuspin) == HIGH) {
      hourset = (hourset - 1); // substract one to hour and if neative assign 23 (next line)
      if (hourset < 0) hourset = 23;
      mset[0] = hourset / 10;
      mset[1] = hourset - mset[0] * 10;
      showtime(mset, 0);
      delay(d);
    }
  }

  // set minutes
  mset[0] = 10; //let's start setting minutes, so show blank hours
  mset[1] = 10;
  mset[2] = minuteset / 10;
  mset[3] = minuteset - mset[2] * 10;
  showtime(mset, 0);
  delay(d);
  while (digitalRead(setmodepin) != HIGH) { //setting minutes until we press again the set button
    if (digitalRead(setpluspin) == HIGH) {
      minuteset = (minuteset + 1) % 60; // same math as previously done to hours
      mset[2] = minuteset / 10;
      mset[3] = minuteset - mset[2] * 10;
      showtime(mset, 0);
      delay(d);
    }
    if (digitalRead(setminuspin) == HIGH) {
      minuteset = (minuteset - 1);
      if (minuteset < 0) minuteset = 59;
      mset[2] = minuteset / 10;
      mset[3] = minuteset - mset[2] * 10;
      showtime(mset, 0);
      delay(d);
    }
  }
  rtc.adjust(DateTime(2019, 1, 16, hourset, minuteset, 0)); //save time in RTC, date is useless so let's invent it
  capturetime(); // capturte the time in variable moment (is reduntant, but just to make sure jeje)
  showtime(moment, 0); // show the actual time
  delay(d * 2);
}
