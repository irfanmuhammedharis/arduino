#include <SoftwareSerial.h>

//Serial Communication for the barcode scanner
SoftwareSerial ScannerSerial(11, 12); // RX, TX

//Serial Monitor
bool typeStringComplete = false; 
String typeString = "";

//ScannerSerial Barcode Scanner
String scannerString = "";  
bool scannerComplete = false; 
int strglength = 0;
int oldstrglength = 0;
long productcode;    //barcode must contain integers only


void setup() {
  ScannerSerial.begin(9600); 
  Serial.begin(9600);

  Serial.println("Start Scanning");
}


void loop() {


 ////Comanding the Barcode Scanner to scan/////
 //when the word "scan" is sent thru the serial monitor
  if (typeStringComplete) {
    if(typeString.startsWith("scan")){
      Serial.println("Scanning...");

 /// ****Problem is here*****//
 /// 7E 00 08 01 00 02 01 AB CD :code from data sheet
   // ScannerSerial.write("7E 00 08 01 00 02 01 AB CD");
     ScannerSerial.write(0x7E);   
 //    ScannerSerial.write(0x00);
 ScannerSerial.write((byte) 0x00);
     ScannerSerial.write(0x08);
     ScannerSerial.write(0x01);
    // ScannerSerial.write(0x00);
     ScannerSerial.write((byte) 0x00);
     ScannerSerial.write(0x02);
     ScannerSerial.write(0x01);
     ScannerSerial.write(0xAB);
    ScannerSerial.write(0xCD);
    }
 
  else { Serial.println("Invalid Command"); }
      typeString = "";  //clear the string
    typeStringComplete = false;
  }


///Recieving data from the Barcode Scanner///
   oldstrglength = scannerString.length(); 
  if (ScannerSerial.available()) {
  char c =  (char)ScannerSerial.read();
  scannerString  += c;
  }
  strglength = scannerString.length();
     
  if (strglength == oldstrglength && strglength != 0) {
  scannerComplete = true;
  }
  if (scannerComplete) {
    productcode = scannerString.toInt();
     Serial.print("Product Code: ");
    Serial.println(scannerString);
    scannerString = ""; // clear the string:
    scannerComplete = false;
  }
  delay(5);   
}


//Reading the Serial Monitor 
void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    typeString += inChar;
    if (inChar == '\n') {
      typeStringComplete = true;
    }
  }
}
