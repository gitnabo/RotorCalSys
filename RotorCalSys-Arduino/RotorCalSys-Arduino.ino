
#include <Wire.h>
#include <Servo.h>

// For simulating on an Adafruit feather (because that's what Steve has)
//#define FEATHER


#ifndef FEATHER
#include "HX711.h"
#include <Adafruit_ADS1015.h>

#define DOUT  10
#define CLK  16

HX711 g_scale;

// ADCs
Adafruit_ADS1015 g_ads1115[] = {(0x48),(0x49)};  

#endif


Servo g_servoMotor; // RPM used to be called '2'
Servo g_servoPitch;
int g_iServoMotorVal = 0;
int g_iServoPitchVal = 0;





// Top-level protocol handlnig
void ProcessRequest(const String& sRequest);
String ProcessRequest(const String& sOp, const String& sParams);
String ServiceSerial();

// Request processing for individual commands
String GetData();
void SetPitchPwm(int iPitchPWM);
void SetRpmPwm(int iRpmPwm);
float readADC(int iADC);


void Debug(const char* pszMsg)
{
  //Serial.print(pszMsg);
}


void setup() {
  Serial.begin(57600);  

  // Set pins for servos
  g_servoPitch.attach(5);
  g_servoMotor.attach(6);
return;

#ifndef FEATHER
  //set pins for scale
  g_scale.begin(DOUT, CLK);
  g_scale.set_scale();
  g_scale.tare(); //Reset the scale to 0
#endif

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
  String sResponseData = ProcessRequest(sOperation, sParams);

  String sResponse = sID;
  sResponse += ": ";
  sResponse += sResponseData;
  sResponse += "\n";
  
  Serial.print(sResponse.c_str());
}



String ProcessRequest(const String& sOp, const String& sParams)
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
  else if(sOp == "setpitchpwm") {
    int iPWM = sParams.toInt();
    SetPitchPwm(iPWM);
    return "";
  }
  else if(sOp == "setrpmpwm") {
    int iPWM = sParams.toInt();
    SetRpmPwm(iPWM);
    return "";
  }
  else if(sOp == "echo")
    return sParams;
  else
    return "";
}


/************ Command Implementation ************/




void SetPitchPwm(int iPitchPWM)
{
  g_iServoMotorVal = iPitchPWM;
  g_servoMotor.writeMicroseconds(iPitchPWM);
}



void SetRpmPwm(int iRpmPwm)
{  
  g_iServoMotorVal = iRpmPwm;
  g_servoPitch.writeMicroseconds(iRpmPwm);
}


// 0=amp0, 1=volt0, 2=amp1, 3=volt1
float readADC(int iADC)
{
#ifdef FEATHER
  return 1.0f;
#else
  float fSum = 0.0f;

  // get x point average
  int iCount = 100;
  for (int i = 0; i < iCount; ++i) {
    switch(iADC)
    {
      case 0:
        fSum += 0.003f * 1000.0f * g_ads1115[0].readADC_Differential_0_1();
        break;
      case 1:
        fSum += 0.003f * 1.104f * g_ads1115[0].readADC_Differential_2_3();
        break;
      case 2:
        fSum += 0.002f * 1000.0f * g_ads1115[1].readADC_Differential_0_1();
        break;
      case 3:
        fSum += 0.002f * 15.71f * g_ads1115[1].readADC_Differential_2_3();
        break;
    }
    delay(1);   
  }
    
  float fVal = fSum / (float)iCount;

  return fVal;
#endif
}


String GetData()
{  
#ifdef FEATHER
  // Fake data
  String sData = "1.0,2.0,3.0,4.0,5.0,6.0,7.0";
  return sData;  
#else
  // Read the real data from the hardware

  float fScaleRaw = (float)g_scale.read();
  float fScale = fScaleRaw * 0.0000072418f + 0.38f;

  float fAmp0 = readADC(0);
  float fVolt0 = readADC(1);
  float fAmp1 = readADC(2);
  float fVolt1 = readADC(3);
  
  Serial.print(fScale);
  Serial.print(",");
  Serial.print(fAmp0);  //servo
  Serial.print(",");
  Serial.print(fVolt0);  //servo
  Serial.print(",");
  Serial.print(fAmp1);  //batt/esc
  Serial.print(",");
  Serial.print(fVolt1);//batt/esc
  Serial.print(",");
  Serial.print(g_iServoMotorVal);  //servo
  Serial.print(",");
  Serial.println(g_iServoMotorVal); //batt/esc
#endif
}
