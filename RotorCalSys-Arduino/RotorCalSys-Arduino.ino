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
 * 1. In Arduino IDE, go to Preferences and add this URL to the "Additional Board Manager URLs" entry,
 *          https://raw.githubusercontent.com/sparkfun/Arduino_Boards/master/IDE_Board_Manager/package_sparkfun_index.json
 * 2. Open the "Board Manager" from the Tools menu
 * 3. Type "sparkfun", it should filter down to a few entries
 * 4. Install the SparkFun AVR package
 * 5. Again in the Tools menu, select the "SparkFun Pro Micro"
 * 6. Select the 5V, 16MHz processor option
 * 7. Set the COM port to match the correct device manager entry
 * 8. Run Examples/Communication/SerialCallResponse, you should see AAA coming out of the Serial Monitor
 *
 * Next, we need to install the libraries for the HX711 load sensor and the Adafruit ADS1015 ADC. Follow these
 * instructions:
 * 1. Menu "Sketch/Include Library/Manage Libraries"
 * 2. Enter HX711 in the search pane
 * 3. Select the HX711 library by Bogdan Necula
 * 4. Install it
 *
 * Do the same for these libraries:
 *    Adafruit BusIO
 *    Adafruit ADS1x15
 */

#include <Wire.h>
#include <Servo.h>


#include <HX711.h>
#include <Adafruit_ADS1X15.h>

#define SCALE_PIN_DOUT  10
#define SCALE_PIN_CLK  16



HX711 g_scale;

// ADCs
Adafruit_ADS1015 g_ads1115[2]; 


Servo g_servo_MotorRpm; // RPM used to be called '2'
Servo g_servo_Pitch;
int g_iServoMotorRpm = 0;
int g_iServoPitchPwm = 0;


// Top-level protocol handlnig
void ProcessRequest(const String& sRequest);
String ExecuteRequest(const String& sOp, const String& sParams);
String ServiceSerial();

// Request processing for individual commands
String GetData();
String GetScale();
void SetPitchPwm(int iPitchPWM);
void SetRpmPwm(int iRpmPwm);
float readRawADC(int iADC);
float ReadPitchServoV();
float ReadPitchServoA();
float ReadMotorV();
float ReadMotorA();

String Version()
{
	return "0.5";
}


void Debug(const char* pszMsg)
{
  //Serial.print(pszMsg);
}


void setup() {
  Serial.begin(57600);  

  // Set pins for servos
  g_servo_Pitch.attach(5);
  g_servo_MotorRpm.attach(6);

  g_ads1115[0].begin(0x48);
  g_ads1115[1].begin(0x49);
  SetRpmPwm(1000);

  //set pins for scale
  g_scale.begin(SCALE_PIN_DOUT, SCALE_PIN_CLK);
  g_scale.set_scale();
  g_scale.tare(); //Reset the scale to 0
}//setup





void loop() {  
  // Service the serial port
  String sRequest = ServiceSerial();

  if(sRequest.length() > 0) {
    // We have a valid request, process it
    ProcessRequest(sRequest);
  }
 
  // Not sure we need this, but breathe for a moment
  delay(1);
}//loop


String ServiceSerial() {

  // Do we have anything to read?
  int iBytesAvail = Serial.available();
  if(!iBytesAvail) 
    return "";

  // Read what there is into a temporary buffer
  char szBuffer[64];  
  memset(&szBuffer, 0, sizeof(szBuffer));
  Serial.readBytes(szBuffer, iBytesAvail);

  // Append to the global string buffer
  static String gs_sInputBuffer = "";  // Static/Global variable
  gs_sInputBuffer += szBuffer;
  if(!gs_sInputBuffer.endsWith("\r") && !gs_sInputBuffer.endsWith("\n"))
    return "";  // We don't have a complete request yet

  // We have a complete request, get it and reset the input buffer
  String sRequest = gs_sInputBuffer;
  sRequest.trim();
  gs_sInputBuffer = String();  // Reset the buffer

  return sRequest;
}



void ProcessRequest(const String& sRequest){
  Debug("Received request: ");
  Debug(sRequest.c_str());
  Debug("\r\n");

  // Parse the request. It must be in the format of <id>: <op> <data>
  String sLine = sRequest;

  // Extract the ID
  int iColonPos = sLine.indexOf(":");
  if(-1 == iColonPos)
    return; // Invalid command
  String sID = sLine.substring(0, iColonPos);
  sLine = sLine.substring(iColonPos + 1); // Prune out the id
  sLine.trim();

  if(sLine.length() == 0)
    return; // Invalid command

  // Extract the Operation word
  String sOperation;
  String sParams = "";
  int iSpacePos = sLine.indexOf(" ");
  if(-1 == iSpacePos) {
    // No additional data, last word is the operation
    sOperation = sLine;
    sOperation.trim();
  }
  else {
    // There are additional parameters
    sOperation = sLine.substring(0, iSpacePos);
    sLine = sLine.substring(iSpacePos + 1); // Prune out the op
    sLine.trim();
    sParams = sLine;
  }
  sOperation.toLowerCase();

  // Process the operation
  String sResponseData = ExecuteRequest(sOperation, sParams);

  String sResponse = sID;
  sResponse += ": ";
  sResponse += sResponseData;
  sResponse += "\n";
  
  Serial.print(sResponse.c_str());
}



String ExecuteRequest(const String& sOp, const String& sParams)
{
  Debug("Processing request: \r\n");
  Debug("Op    : ");
  Debug(sOp.c_str());
  Debug("\r\n");
  Debug("Params: ");
  Debug(sParams.c_str());
  Debug("\r\n");

  // The mother-of-all switch statements
  if(sOp == "getdata")
    return GetData();
  else if(sOp == "getscale")
    return GetScale();
  else if (sOp == "setpitchpwm") {// Rotor Servo
	  int iPWM = sParams.toInt();
	  SetPitchPwm(iPWM);
	  return "";
  }
  else if (sOp == "setpitchpwmramp") {
	  // For testing
	  for (int iPWM = 800; iPWM < 2100; iPWM = iPWM + 100) {
		  char sz[32];
		  sprintf(sz, "Pitch=%d\r\n", iPWM);
		  Debug(sz);
		  SetPitchPwm(iPWM);
		  delay(500);
	  }
	  return "";
  }  
  else if (sOp == "setrpmpwmramp") {
	  // For testing
	  for (int iPWM = 1000; iPWM < 1960; iPWM = iPWM + 100) {
		  char sz[32];
		  sprintf(sz, "Rpm=%d\r\n", iPWM);
		  Debug(sz);
		  SetRpmPwm(iPWM);
		  delay(500);
	  }
	  return "";
  }
  else if(sOp == "setrpmpwm") { // Motor RPM
    int iPWM = sParams.toInt();
    SetRpmPwm(iPWM);
    return "";
  }
  else if (sOp == "echo")
	  return sParams;
  else if (sOp == "tarescale") {
	  g_scale.tare();
	  return "";
  }
  else if (sOp == "getversion")
	  return Version();
  else
    return "";
}


/************ Command Implementation ************/




void SetPitchPwm(int iPitchPWM)
{
  g_iServoPitchPwm = iPitchPWM;
  g_servo_Pitch.writeMicroseconds(iPitchPWM);
}



void SetRpmPwm(int iRpmPwm)
{  
  g_iServoMotorRpm = iRpmPwm;
  g_servo_MotorRpm.writeMicroseconds(iRpmPwm);
}


float readRawADC(int iADC)
{
	float fSum = 0.0f;

	// get x point average
	int iCount = 100;
	for (int i = 0; i < iCount; ++i) {
		switch (iADC)
		{
		case 0:
			fSum += g_ads1115[0].readADC_Differential_0_1();  // servo PitchA
			break;
		case 1:
			fSum += g_ads1115[0].readADC_Differential_2_3();  // servo PitchV
			break;
		case 2:
			fSum += g_ads1115[1].readADC_Differential_0_1();  // MotorA
			break;
		case 3:
			fSum += g_ads1115[1].readADC_Differential_2_3();  // MotorV
			break;
		}
		delay(1);
	}

	float fVal = fSum / (float)iCount;
	Debug("read ACD: ");
	Debug(iADC);
	Debug("\r\n");

	return fVal;
}





float ReadPitchServoA()
{
	return readRawADC(1) * 0.003f * 1.104f;
}

float ReadPitchServoV()
{
	return readRawADC(0) * 0.003f * 1000.0f;
}

float ReadMotorA()
{
	return readRawADC(3) * 0.002f * 1000;
}

float ReadMotorV()
{
	return readRawADC(2) * 0.002f * 15.71f;
}



String GetData()
{ 
  // Read the real data from the hardware
	
  String sResp;
  sResp += "ms=" + String(millis()) + ",";
  sResp += "kg=" + String(GetScale()) + ",";
  sResp += "pitch_servo_amp=" + String(ReadPitchServoA()) + ",";
  sResp += "pitch_servo_volt=" + String(ReadPitchServoV()) + ",";
  sResp += "motor_volt=" + String(ReadMotorV()) + ",";
  sResp += "motor_amp=" + String(ReadMotorA()) + ",";
  sResp += "servo_pitch=" + String(g_iServoPitchPwm) + ",";  
  sResp += "motor_rpm=" + String(g_iServoMotorRpm);  
 
  return sResp;
}



String GetScale()
{ 
  // Read the real data from the hardware

  // This code is meant to help deal with the scale sensor taking so long.
  // This code is not yet tested.
  
  // Create a timeout feature bc the 
  float fScaleRaw = (float)g_scale.read();    
  float fScale = fScaleRaw * 0.0000072418f + 0.38f;
  return String(fScale);
}
