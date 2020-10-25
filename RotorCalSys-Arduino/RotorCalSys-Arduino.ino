
#include <Wire.h>




void ProcessRequest(const String& sRequest);
String ProcessRequest(const String& sOp, const String& sParams);
String ServiceSerial();



void setup() {
  Serial.begin(57600);  //baud doesnt matter for pro micro

  //wait for serial port to open
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

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



void ProcessRequest(const String& sRequest)
{
  Serial.print("Received request: ");
  Serial.print(sRequest);
  Serial.print("\r\n");

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

  // Process the operation
  String sResponseData = ProcessRequest(sOperation, sParams);

  String sResponse = sID;
  sResponse += ": ";
  sResponse += sResponseData;
  Serial.print("Response: ");
  Serial.print(sResponse.c_str());
  Serial.print("\r\n");
}



String ProcessRequest(const String& sOp, const String& sParams)
{
  Serial.print("Processing request: \r\n");
  Serial.print("Op    : ");
  Serial.print(sOp.c_str());
  Serial.print("\r\n");
  Serial.print("Params: ");
  Serial.print(sParams.c_str());
  Serial.print("\r\n");

  // The mother-of-all switch statements
  if(sOp == "echo") {
    return sParams;
  }
}
