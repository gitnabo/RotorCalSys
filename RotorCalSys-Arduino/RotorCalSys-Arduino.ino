#include <Wire.h>


/* This program runs on the SparkFun ProMicro 5V/16MHz arduino board. 
 *  
 *  Just plug-in the board, and windows should automatically install the right USB to Serial
 *  kernel-mode-driver. Make sure that when you plug in your board that it pops up on a COM port in device manager.
 * 
 * The board support package for Arduino for this board can be found here:
 *  https://learn.sparkfun.com/tutorials/pro-micro--fio-v3-hookup-guide/installing-windows
 *  https://github.com/sparkfun/Arduino_Boards/archive/master.zip
 *  
 * The instructions are confusing, they are talking at first mainly about the kernel mode windows driver.
 * 
 * The instructions for getting the board support package setup:
 * 
 * 1. In Arduino IDE, go to Preferences and add this URL to the "Additional Board Manager URLs" entry
 * 2. Open the "Board Manager" from the Tools menu
 * 3. Type "sparkfun", it should filter down to a few entries
 * 4. Install the SparkFun AVR package
 * 5. Again in the Tools menu, select the "SparkFun Pro Micro"
 * 6. Select the 5V, 16MHz processor option
 * 7. Set the COM port to match the correct device manager entry
 * 8. Run Examples/Communication/SerialCallResponse, you should see AAA coming out of the Serial Monitor
 * 
 */


//for servo control
long servo1Value = 50;
long servo2Value = 50;


//for reading load cell

#define DOUT  10
#define CLK  16
float scaleCalFactor1 = 0.0000072418f; //.0000072418f; on 200426//0.00003f / lbs per count or 0.000007629f / v  / let spreadsheet do this
float scaleCalFactor2 = 1.0f;  // less gain for second differential input
float scaleOffset1 = 0.38f;          //lbs
float scaleOffset2 = 0.0f;
float scaleValue1 = 0.0f;           //lbs
float scaleValue2 = 0.0f;
boolean scale1 = false;


// for ADC breakout board, ADS1115
//Adafruit_ADS1115 ads1115;
//[0] is servo, [1] is esc / battery
float adcCalFactor[] = {0.003f, 0.002f};//previosuly .002f; //2mV for ads1015 and .125mV for ads1115 when gain is one
float adcOffset[] = {0.0f, 0.0f};          //lbs
float adcCAmps[] = {0.0f, 0.0f};
float adcCVolts[] = {0.0f, 0.0f};

float voltageGain[] = {1.104f, 15.71f}; // v  note there is a voltage divider to drop by factor of 10ish
float currentGain[] = {1000.0f, 1000.0f}; //ma note there is a gain in the INA169




//for serial com
byte inData[6];  //data read
byte inByte = 0;  // byte read
byte counter;
String inputString = "";
boolean stringComplete = false;
long printInterval = 0;
long timeSinceLastPrint = 0;


void setup() {


  Serial.begin(57600);  //baud doesnt matter for pro micro
  //analogReference(INTERNAL);

  //wait for serial port to open
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  inputString.reserve(200);
  delay(1);

}//setup




void loop() {

  //wait for serial commands

  //delay(300);
  //Serial.println(millis());






  //required for reading
  checkSerial();
  if (stringComplete) {
    //Serial.println(inputString);
    // clear the string:
    inputString = "";
    stringComplete = false;
  }


  //print values
  long t = millis();
  if ((printInterval > 0) && (t > timeSinceLastPrint + printInterval)) {

    printVals();

  }

}//loop

void readVals(int x){
     // read adc;
  float f = 0.0f;
  float f2 = 0.0f;

  //get x point average
  int i = 0;
  f = 0;
  f2 = 0;
  for (i = 0; i < 100; i++) {
    delay(1);
    
  }


    
  f /= i;
  f2 /= i;

////find max
//  int i = 0;
//  long temp = 0;
//  f = 0;
//  f2 = 0;
//  for (i = 0; i < 10; i++) {
//    delay(1);
//    temp =  ads1115.readADC_Differential_0_1();
//    if (temp < f){f=temp;}
//    delay(1);
//    temp = ads1115.readADC_Differential_2_3();  
//    if (temp > f2){f2=temp;}
//    
//  }

  
  }

void readVals(){
   
  }


 //read serial port for commands
 //if not any of the commands, toss a value, then repeat
void checkSerial() {

  //check for char in buffer
  while (Serial.available()) {
    delay(1);

    //create new string, check if its end of a command such as new ling, or total length is greater than 200
   
    char inChar = (char)Serial.read();
    if ((inChar == ' ') || (inChar == '\n') || (inChar == '\r') || (inputString.length() > 200))
    { stringComplete = true;                                  //if string is complete, move to enxt
      //Serial.println("string Completed");
    }
    else {
      inputString += inChar;                                //if not complete, check next char
      //Serial.println(inChar);
      //Serial.println(inputString);
    }//while to concatonate string

    //test case
    if (inputString.compareTo("hello") == 0) {

      stringComplete = true;
      goto readSomething;
    }// if hello

    //Set Servo position
    if (inputString.compareTo("setServoOnePos") == 0) {
      long temp = 0;
      //process 4 bytes for first output  this is servo pos in us
      for (int i = 0; i < 4; i++) {
        temp  = temp * 10 + (Serial.read() - 48);
      }
      
      stringComplete = true;
      //Serial.print("Set Servo One to ");
      //Serial.println(temp);
      goto readSomething;
    }// if setServoOne

    if (inputString.compareTo("setServoTwoPos") == 0) {
      long temp = 0;
      //process 3 bytes for first output
      for (int i = 0; i < 4; i++) {
        temp  = temp * 10 + (Serial.read() - 48);
      }

      stringComplete = true;
      //Serial.print("Set Servo Two to ");
      //Serial.println(temp);
      goto readSomething;
    }// if setServoTwo

    if (inputString.compareTo("zeroScale") == 0) {

      //process 3 bytes for first output
      scaleOffset1 += (-1 * scaleValue1);

      goto readSomething;
    }// if setServoTwo


    if (inputString.compareTo("printVals") == 0) {
      printVals();
      stringComplete = true;
      goto readSomething;
    }// if printVals


    if (inputString.compareTo("printContinuous") == 0) {
      long temp = 0;
      //process 3 bytes for first output
      for (int i = 0; i < 6; i++) {
        temp  = temp * 10 + (Serial.read() - 48);
      }
      printInterval = temp;
      printVals();
      stringComplete = true;
      goto readSomething;
    }// if printContinueous

      if (inputString.compareTo("printStop") == 0) {
      printInterval = 0;
      stringComplete = true;
      goto readSomething;
    }// if printVals


    goto readSomething;   //ignore some early debug 

    //peak at first byte.first byte not removed.
    inByte = Serial.peek();
    //Serial.println(inByte);


    if (inByte == 'r') {
      Serial.read();  //dump the 'r
      delay(1);
      Serial.print(millis());
      Serial.print(",");
      Serial.print(analogRead(0));
      Serial.print(",");
      Serial.print(analogRead(1));
      Serial.print(",");
      Serial.print(servo1Value);
      Serial.print(",");
      Serial.println(servo2Value);

      goto readSomething;
    }//if read 'r'

    if (inByte == 's') {
      //read 6 bytes
      Serial.read(); // dump the 's
      Serial.readBytes(inData, 6);
      long temp = 0;

      //process 3 bytes for first output
      for (int i = 0; i < 3; i++) {
        temp  = temp * 10 + (inData[i] - 48);
      }

      servo1Value = temp;

      //repeat for servo 2
      temp = 0;

      //process 3 bytes for second output
      for (int i = 3; i < 6; i++) {
        temp  = temp * 10 + (inData[i] - 48);
      }
      servo2Value = temp;

      goto readSomething;
    }//if read 's'


  }//if statement
  Serial.read();//dump a char to look for next one

readSomething:
  delay(1);
}//check serial


void printVals() {

  readVals(0);
  readVals(1);
  timeSinceLastPrint = millis();
  Serial.print("Read:,");
  Serial.print(timeSinceLastPrint);
  Serial.print(",");
  Serial.print(scaleValue1);
  Serial.print(",");
  Serial.print(adcCAmps[0] * currentGain[0]);  //servo
  Serial.print(",");
  Serial.print(adcCVolts[0] * voltageGain[0]);  //servo
  Serial.print(",");
  Serial.print(adcCAmps[1] * currentGain[1]);  //batt/esc
  Serial.print(",");
  Serial.print(adcCVolts[1] * voltageGain[1]);//batt/esc
  Serial.print(",");
  Serial.print(servo1Value);  //servo
  Serial.print(",");
  Serial.println(servo2Value); //batt/esc


}


/**********************************
   todo
   add ADC via ADS1115
   add setting gain and offset values
   add setting min and max us for servo
   check out servo driver board and consider using it.
   add periodic reads ability
   refactor writes to reduce clutter
   add servo power measurement
   create schematic
   post on github
*/


/****************************************
  revision history
  12 Feb 2017
  revised input to take strings for more flexibility
  added compatibility for scale input via HX711 chip
  revised servo input to be in us and not degrees

  15 Feb 2017
  added compatibility for differential input via adafruit ADS1115
  outputs values in mv
*********************************/
