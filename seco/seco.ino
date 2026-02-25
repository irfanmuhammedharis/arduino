
#include <ESP32WebServer.h>
#include <HTTPClient.h>
#include <SPI.h>
#include "BluetoothSerial.h"
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>  //add

WiFiClientSecure client;  //add

String datas;
#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

BluetoothSerial SerialBT;

#ifdef USE_NAME
String slaveName = "Tester";
#else
String MACadd = "7C:9E:BD:38:9C:4A";  // This only for printing
uint8_t address[6] = { 0x7C, 0x9E, 0xBD, 0x38, 0x9C, 0x4A };
#endif

String myName = "ESP32-BT-Master";

#define led_pin 33

int heartrate = 32;
int spo2 = 98;
float temp = 37;
int step_count = 90;
int Sec_id = 501;
int Device_id = 23;
int glu =13;
int pre1=110;
int pre2=10;

const char* ssid = "Acutrotechnologies Pvt Ltd";
const char* password = "Acutro@1234$$";


ESP32WebServer server(80);  //--> Server on port 80


void setup() {
  bool connected;
  pinMode(led_pin, OUTPUT);
  Serial.begin(115200);  //--> Initialize serial communications with the PC
  SPI.begin();           //--> Init SPI bus
  delay(500);

  WiFi.begin(ssid, password);  //--> Connect to your WiFi router
  Serial.println("");

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(250);
  }

  Serial.println("");
  Serial.print("Successfully connected to : ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  client.setInsecure();  //add
  //   Serial.println("");
  //   SerialBT.begin(myName, true);
  //   Serial.printf("The device \"%s\" started in master mode, make sure slave BT device is on!\n", myName.c_str());

  // #ifndef USE_NAME

  //   Serial.println("Using PIN");
  // #endif

  // #ifdef USE_NAME
  //   connected = SerialBT.connect(slaveName);
  //   Serial.printf("Connecting to slave BT device named \"%s\"\n", slaveName.c_str());
  // #else
  //   connected = SerialBT.connect(address);
  //   Serial.print("Connecting to slave BT device with MAC ");
  //   Serial.println(MACadd);
  // #endif

  //   if (connected) {
  //     Serial.println("Connected Successfully!");
  //   } else {
  //     Serial.println("Not Connected!");
  //   }
}

void processSensorData() {
  // void processSensorData(String jsonStr) {
  // Create a JSON document
  // StaticJsonDocument<200> jsonDoc;  // Adjust the capacity as needed

  // // Parse the JSON string
  // DeserializationError error = deserializeJson(jsonDoc, jsonStr);

  // // Check for parsing errors
  // if (error) {
  //   Serial.print("JSON parsing error: ");
  //   Serial.println(error.c_str());
  //   return;
  // }

  // // Extract individual sensor values
  // heartrate = jsonDoc["heartrate"];
  // spo2 = jsonDoc["spo2"];
  // temp = jsonDoc["temp"];
  // step_count = jsonDoc["step_count"];

  // Use the extracted values as needed
  // Serial.print("Heart Rate: ");
  // Serial.println(heartrate);
  // Serial.print("SpO2: ");
  // Serial.println(spo2);
  // Serial.print("Temperature: ");
  // Serial.println(temp);
  // Serial.print("Step Count: ");
  // Serial.println(step_count);
  // WiFiClient client; remove
  HTTPClient https;
  String postData;
  postData = "heart_rate=" + String(heartrate) + "&SpO2=" + String(spo2) + "&temperature=" + String(temp) + "&pedometer=" + String(step_count) + "&systolic_pressure=" + String(pre1) + "&diastolic_pressure=" + String(pre2) + "&glucose=" + String(glu) + "&device_id=" + String(Device_id) + "&Sec_deviceid=" + String(Sec_id);
  // postData = "heart_rate=" + String(heartrate) + "&SpO2=" + String(spo2) + "&temperature=" + String(temp) + "&pedometer=" + String(step_count) + "&systolic_pressure="0&diastolic_pressure=0&glucose=0&device_id=" + String(Device_id) + "&Sec_deviceid=" + String(Sec_id);



  Serial.println(postData);
  https.begin(client, "https://www.adithyainfosoft.com/timelymeds-api/api/devicedataupload.php");
  https.addHeader("Content-Type", "application/json");  //Specify content-type header
  // https.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpCode = https.POST(postData);  //Send the request
  String payload = https.getString();   //Get the response payload


  Serial.println(httpCode);  //Print HTTP return code
  Serial.println(payload);   //Print request response payload

  https.end();  //Close connection
  delay(1000);
}


void loop() {

  // if (SerialBT.available()) {
  //   char receivedChar = SerialBT.read();

  //   // Collect the received characters until a newline character is received
  //   static String receivedData = "";

  //   if (receivedChar != '\n') {
  //     receivedData += receivedChar;
  //   } else {
  //     processSensorData(receivedData);
  //     receivedData = "";  // Clear the received data buffer
  //   }
  // }
  processSensorData();
  digitalWrite(led_pin, HIGH);
  delay(3000);
  digitalWrite(led_pin, LOW);
  delay(1000);
}
