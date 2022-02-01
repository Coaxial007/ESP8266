// I use for my toys simple OTA through a browser.
// When using port mapping on router, it works from anywhere.
// Here is the entire code that is necessary for that.
// The uploaded file must, of course, be in binary form as *.bin
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <PubSubClient.h>
#include <algorithm> // std::min

#include <OneWire.h>
#include <DallasTemperature.h>

#include <ModbusMaster232.h> 
#include <SoftwareSerial.h>  // Modbus RTU pins   D7(13),D8(15)   RX,TX
// Instantiate ModbusMaster object as slave ID 1
ModbusMaster232 node(1);

const char* ssid =     "SSID_NAME";     // Set your router SSID
const char* password = "PASSWORD"; // Set your router password

// MQTT: ID, server IP, port, username and password
const PROGMEM char* MQTT_CLIENT_ID = "ESP8266_alarm_power";
const PROGMEM char* MQTT_SERVER_IP = "192.168.xx.xx";
const PROGMEM uint16_t MQTT_SERVER_PORT = 1883;
const PROGMEM char* MQTT_USER = "MQTT_USER";
const PROGMEM char* MQTT_PASSWORD = "MQTT_PASSWORD";

#define SWITCH_TOPIC_1     "alarm/led"
#define RELAY_TOPIC_1     "alarm/relay1"
#define RELAY_TOPIC_2     "alarm/relay2"
#define RELAY_TOPIC_3     "alarm/relay3"
#define SENSOR1_TEMP_TOPIC     "alarm/ulica"
#define SENSOR2_TEMP_TOPIC     "alarm/kuchnia1p"
#define SENSOR3_TEMP_TOPIC     "alarm/centrala"
#define SENSOR4_TEMP_TOPIC     "alarm/kuchnia2p"
#define SENSOR5_TEMP_TOPIC     "alarm/salon2p"
#define SENSOR6_TEMP_TOPIC     "alarm/sypialnia1p"
#define SENSOR7_TEMP_TOPIC     "alarm/salon1p"
#define SENSOR8_TEMP_TOPIC     "alarm/piwnica0p"

#define POWER1_TOPIC           "power/total_energy"
#define POWER2_TOPIC           "power/frequency"
#define POWER3_TOPIC           "power/voltage_phaseA"
#define POWER4_TOPIC           "power/voltage_phaseB"
#define POWER5_TOPIC           "power/voltage_phaseC"
#define POWER6_TOPIC           "power/current_phaseA"
#define POWER7_TOPIC           "power/current_phaseB"
#define POWER8_TOPIC           "power/current_phaseC"
#define POWER9_TOPIC           "power/total_power"
#define POWER10_TOPIC           "power/power_phaseA"
#define POWER11_TOPIC           "power/power_phaseB"
#define POWER12_TOPIC           "power/power_phaseC"
#define POWER13_TOPIC           "power/total_reactive_power"
#define POWER14_TOPIC           "power/reactive_power_phaseA"
#define POWER15_TOPIC           "power/reactive_power_phaseB"
#define POWER16_TOPIC           "power/reactive_power_phaseC"
#define POWER17_TOPIC           "power/total_apparent_power"
#define POWER18_TOPIC           "power/apparent_power_phaseA"
#define POWER19_TOPIC           "power/apparent_power_phaseB"
#define POWER20_TOPIC           "power/apparent_power_phaseC"
#define POWER21_TOPIC           "power/total_power_factor"
#define POWER22_TOPIC           "power/power_factor_phaseA"
#define POWER23_TOPIC           "power/power_factor_phaseB"
#define POWER24_TOPIC           "power/power_factor_phaseC"

#define OTAUSER         "ota_user_name"    // Set OTA user
#define OTAPASSWORD     "ota_user_password"    // Set OTA password
#define OTAPATH         "/firmware"// Set path for update
#define SERVERPORT      80         // Server

// Data wire is plugged TO GPIO 4
#define ONE_WIRE_BUS 4

// Initializes the espClient. You should change the espClient name if you have multiple ESPs running in your home automation system
WiFiClient espClient; //mqtt
PubSubClient client(espClient);

// Lamp - LED - GPIO 16 = buildin on ESP-12E NodeMCU board
const int lamp = 2;

// relay pins D5, D6, D0
const int relay1 = 14;
const int relay2 = 12;
const int relay3 = 16;


// Timers auxiliar variables
double now = millis();
double lastMeasure = 0;
double lastMeasure2 = 0;

ESP8266WebServer HttpServer(SERVERPORT);
ESP8266HTTPUpdateServer httpUpdater;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// Number of temperature devices found
int numberOfDevices;

// We'll use this variable to store a found device address
DeviceAddress tempDeviceAddress;

/*-----( Declare Variables )-----*/
// Assign the addresses of your 1-Wire temp sensors.
// See the tutorial on how to obtain these addresses:
// http://www.hacktronics.com/Tutorials/arduino-1-wire-address-finder.html
//Ulica
DeviceAddress Probe01 = { 0x10, 0xF9, 0x3F, 0xD6, 0x02, 0x08, 0x00, 0xC5 };
//Kuchnia1P
DeviceAddress Probe02 = { 0x10, 0xB2, 0x62, 0xD6, 0x02, 0x08, 0x00, 0x31 };
//Centrala
DeviceAddress Probe03 = { 0x10, 0xFA, 0xE3, 0xDA, 0x02, 0x08, 0x00, 0xF1 };
//Kuchnia2P
DeviceAddress Probe04 = { 0x10, 0xFD, 0xF1, 0xD5, 0x02, 0x08, 0x00, 0x11 };
//Salon2P
DeviceAddress Probe05 = { 0x10, 0x61, 0x24, 0xD6, 0x02, 0x08, 0x00, 0xD9 };
//Sypialnia1P
DeviceAddress Probe06 = { 0x10, 0x66, 0x11, 0xDC, 0x02, 0x08, 0x00, 0x11 };
//Salon1P
DeviceAddress Probe07 = { 0x28, 0xFF, 0x66, 0xEB, 0xC2, 0x16, 0x03, 0xA0 };
//Piwnica0P
DeviceAddress Probe08 = { 0x28, 0xFF, 0xAA, 0x6D, 0xD0, 0x16, 0x05, 0xAB };

/* Send HTTP status 404 Not Found */
void handleNotFound() {
  HttpServer.send(404, "text/plain", "404: Not found");
}

void handleRoot() {
  HttpServer.send(200, "text/plain", MQTT_CLIENT_ID);
}


//-----------------------------------------------------------------
void setup(void) {
  
  ESP.wdtEnable(120000); // watch dog enable 120s = 2 min
  Serial.println("[INFO] WDT start 120 sec");
  
  pinMode(lamp, OUTPUT);

  // Start up the library ds18b20
  sensors.begin();

  // Grab a count of devices on the wire
  numberOfDevices = sensors.getDeviceCount();
  
  Serial.begin(115200);
  node.begin(9600);  // Modbus RTU
  
  Serial.println("INFO: Modbus RTU Master started");

  // print device info
  Serial.println();
  Serial.println("Boot info");
  Serial.print("Device name: ");
  Serial.println(MQTT_CLIENT_ID);
  Serial.print("MQTT Brocker IP address: ");
  Serial.println(MQTT_SERVER_IP);
  Serial.println("MQTT topics: ");
  Serial.println(SWITCH_TOPIC_1);
  Serial.println(RELAY_TOPIC_1);
  Serial.println(RELAY_TOPIC_2);
  Serial.println(RELAY_TOPIC_3);
  Serial.println(SENSOR1_TEMP_TOPIC);
  Serial.println(SENSOR2_TEMP_TOPIC);
  Serial.println(SENSOR3_TEMP_TOPIC);
  Serial.println(SENSOR4_TEMP_TOPIC);
  Serial.println(SENSOR5_TEMP_TOPIC);
  Serial.println(SENSOR6_TEMP_TOPIC);
  Serial.println(SENSOR7_TEMP_TOPIC);
  Serial.println(SENSOR8_TEMP_TOPIC);
  Serial.println("INFO: sensor ds18b20");
  
    // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to WiFi AP: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  /* wait for WiFi connect */
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Connected (OK)");
  Serial.print("Device IP address: ");
  Serial.println(WiFi.localIP());
 
  httpUpdater.setup(&HttpServer, OTAPATH, OTAUSER, OTAPASSWORD);

  //main page
  HttpServer.on("/", handleRoot);
  HttpServer.on("/info", [](){
    HttpServer.send(200, "text/plain", MQTT_CLIENT_ID);
  });
  
  HttpServer.onNotFound(handleNotFound);
  HttpServer.begin();
  
 client.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
 client.setCallback(callback);

  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);
  digitalWrite(relay3, HIGH);

  // locate devices on the bus
  Serial.print("Locating DS18B20 devices...");
  Serial.print("Found ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" devices.");
  
  // Loop through each device, print out address
  for(int i=0;i<numberOfDevices; i++){
    // Search the wire for address
    if(sensors.getAddress(tempDeviceAddress, i)){
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: ");
      printAddress(tempDeviceAddress);
      Serial.println();
    } else {
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }
  }


 
}  //end setup function

// This functions is executed when some device publishes a message to a topic that your ESP8266 is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that 
// your ESP8266 is subscribed you can actually do something
void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic room/lamp, you check if the message is either on or off. Turns the lamp GPIO according to the message
  if(topic==SWITCH_TOPIC_1){
      Serial.print("Changing led value to ");
      if(messageTemp == "on"){
        digitalWrite(lamp, HIGH);
        Serial.print("on");
      }
      else if(messageTemp == "off"){
        digitalWrite(lamp, LOW);
        Serial.print("off");
      }
  }
  Serial.println();

  if(topic==RELAY_TOPIC_1){
      Serial.print("Changing relay #1 value to ");
      if(messageTemp == "on"){
        digitalWrite(relay1, HIGH);
        Serial.print("on");
      }
      else if(messageTemp == "off"){
        digitalWrite(relay1, LOW);
        Serial.print("off");
      }
  }
  Serial.println();

  if(topic==RELAY_TOPIC_2){
      Serial.print("Changing relay #2 value to ");
      if(messageTemp == "on"){
        digitalWrite(relay2, HIGH);
        Serial.print("on");
      }
      else if(messageTemp == "off"){
        digitalWrite(relay2, LOW);
        Serial.print("off");
      }
  }
  Serial.println();

  if(topic==RELAY_TOPIC_3){
      Serial.print("Changing relay #3 value to ");
      if(messageTemp == "on"){
        digitalWrite(relay3, HIGH);
        Serial.print("on");
      }
      else if(messageTemp == "off"){
        digitalWrite(relay3, LOW);
        Serial.print("off");
      }
  }
  Serial.println();
  
}

// This functions reconnects your ESP8266 to your MQTT broker
// Change the function below if you want to subscribe to more topics with your ESP8266 
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("INFO: Attempting MQTT connection...");
    // Attempt to connect
      if (client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("INFO: Connected to MQTT brocker (OK)");  
      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics (to control more LEDs in this example)
      client.subscribe(SWITCH_TOPIC_1);
      client.subscribe(RELAY_TOPIC_1);
      client.subscribe(RELAY_TOPIC_2);
      client.subscribe(RELAY_TOPIC_3);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


// function to print a device address
void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++){
    if (deviceAddress[i] < 16) Serial.print("0");
      Serial.print(deviceAddress[i], HEX);
  }
}


void loop(void) {

if (WiFi.status() != WL_CONNECTED) {
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  return;
 }

if (WiFi.status() == WL_CONNECTED) {
 if (!client.connected()) {
  if (client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
    }
  }

 if (client.connected())
 client.loop();

 HttpServer.handleClient();       // Listen for HTTP requests from clients
    
  now = millis();
  
  ESP.wdtFeed(); //reset watchdog

  // Publishes new temperature and humidity every 10 seconds
  if (now - lastMeasure > 30000) {
    lastMeasure = now;
    sensors.requestTemperatures(); // Send the command to get temperatures
    
    // Loop through each device, print out temperature data
   float tempC1 = sensors.getTempC(Probe01);
   if (tempC1 < -60.00 || tempC1 > 85.00)
   {
   } 
   else 
   {
    // Convert the value to a char array
    char tempString[8];
    dtostrf(tempC1, 1, 2, tempString);
    // Publishes Temperature values
    client.publish(SENSOR1_TEMP_TOPIC, tempString);
   }
    
    float tempC2 = sensors.getTempC(Probe02);
   if (tempC2 < -60.00 || tempC2 > 85.00)
   {
   } 
   else 
   {
    // Convert the value to a char array
    char tempString[8];
    dtostrf(tempC2, 1, 2, tempString);
    // Publishes Temperature values
    client.publish(SENSOR2_TEMP_TOPIC, tempString);
   }
    
    float tempC3 = sensors.getTempC(Probe03);
   if (tempC3 < -60.00 || tempC3 > 85.00)
   {
   } 
   else 
   {
    // Convert the value to a char array
    char tempString[8];
    dtostrf(tempC3, 1, 2, tempString);
    // Publishes Temperature values
    client.publish(SENSOR3_TEMP_TOPIC, tempString);
   }
    
    float tempC4 = sensors.getTempC(Probe04);
   if (tempC4 < -60.00 || tempC4 > 85.00)
   {
   } 
   else 
   {
    // Convert the value to a char array
    char tempString[8];
    dtostrf(tempC4, 1, 2, tempString);
    // Publishes Temperature values
    client.publish(SENSOR4_TEMP_TOPIC, tempString);
   }
    
    float tempC5 = sensors.getTempC(Probe05);
   if (tempC5 < -60.00 || tempC5 > 60.00) 
   {
   } 
   else 
   {
    // Convert the value to a char array
    char tempString[8];
    dtostrf(tempC5, 1, 2, tempString);
    // Publishes Temperature values
    client.publish(SENSOR5_TEMP_TOPIC, tempString);
   }
    
    float tempC6 = sensors.getTempC(Probe06);
   if (tempC6 < -60.00 || tempC6 > 85.00)
   {
   } 
   else 
   {
    // Convert the value to a char array
    char tempString[8];
    dtostrf(tempC6, 1, 2, tempString);
    // Publishes Temperature values
    client.publish(SENSOR6_TEMP_TOPIC, tempString);
   }
    
    float tempC7 = sensors.getTempC(Probe07);
   if (tempC7 < -60.00 || tempC7 > 85.00)
   {
   } 
   else 
   {
    // Convert the value to a char array
    char tempString[8];
    dtostrf(tempC7, 1, 2, tempString);
    // Publishes Temperature values
    client.publish(SENSOR7_TEMP_TOPIC, tempString);
   }
    
    float tempC8 = sensors.getTempC(Probe08);
   if (tempC8 < -60.00 || tempC8 > 85.00)
   {
   } 
   else 
   {
    // Convert the value to a char array
    char tempString[8];
    dtostrf(tempC8, 1, 2, tempString);
    // Publishes Temperature values
    client.publish(SENSOR8_TEMP_TOPIC, tempString);
   }  
      
      Serial.println("Temperature for device: ");
      Serial.print("C1 Ulica      : ");
      Serial.println(tempC1);
      Serial.print("C2 Kuchnia1P  : ");
      Serial.println(tempC2);
      Serial.print("C3 Centralia  : ");
      Serial.println(tempC3);
      Serial.print("C4 Kuchnia2P  : ");
      Serial.println(tempC4);
      Serial.print("C5 Salon2P    : ");
      Serial.println(tempC5);
      Serial.print("C6 Sypialnia1P: ");
      Serial.println(tempC6);
      Serial.print("C7 Salon1P    : ");
      Serial.println(tempC7);
      Serial.print("C8 Piwnica0P  : ");
      Serial.println(tempC8);
      Serial.println("-------------------------------------------");

  }

if (now - lastMeasure2 > 5000) {
    lastMeasure2 = now;
 
int Mdelay = 0; 

node.readHoldingRegisters(0, 1); 
Serial.print("[Total energy H 0x00] ");
Serial.print(node.getResponseBuffer(0));
float temp_powerH = node.getResponseBuffer(0);
node.clearResponseBuffer();
delay(Mdelay);

  
node.readHoldingRegisters(1, 1); 
Serial.print(" [Total energy L 0x01] ");
Serial.println(node.getResponseBuffer(0));
    float temp_power = temp_powerH * 65535 + node.getResponseBuffer(0);
    // Convert the value to a char array
    char tempStringPowerHL[16];
    dtostrf(temp_power, 1, 2, tempStringPowerHL);
    // Publishes Temperature values
    client.publish(POWER1_TOPIC, tempStringPowerHL);
node.clearResponseBuffer();
delay(Mdelay);
    
node.readHoldingRegisters(17, 1); 
Serial.print(" [Frequency Hz 0x11] ");
Serial.println(node.getResponseBuffer(0));
    temp_power = node.getResponseBuffer(0);
    // Convert the value to a char array
    char tempStringPower[8];
    dtostrf(temp_power, 1, 2, tempStringPower);
    // Publishes Temperature values
    client.publish(POWER2_TOPIC, tempStringPower);
node.clearResponseBuffer();
delay(Mdelay);
 
node.readHoldingRegisters(128, 1); 
Serial.print(" [Voltage phaseA 0x80] ");
Serial.println(node.getResponseBuffer(0));
    temp_power = node.getResponseBuffer(0);
    // Convert the value to a char array
    tempStringPower[8];
    dtostrf(temp_power, 1, 2, tempStringPower);
    // Publishes Temperature values
    client.publish(POWER3_TOPIC, tempStringPower);
node.clearResponseBuffer();
delay(Mdelay);
 
node.readHoldingRegisters(129, 1); 
Serial.print(" [Voltage phaseB 0x81] ");
Serial.println(node.getResponseBuffer(0));
    temp_power = node.getResponseBuffer(0);
    // Convert the value to a char array
    tempStringPower[8];
    dtostrf(temp_power, 1, 2, tempStringPower);
    // Publishes Temperature values
    client.publish(POWER4_TOPIC, tempStringPower);
node.clearResponseBuffer();
delay(Mdelay);

node.readHoldingRegisters(130, 1); 
Serial.print(" [Voltage phaseC 0x82] ");
Serial.println(node.getResponseBuffer(0));
    temp_power = node.getResponseBuffer(0);
    // Convert the value to a char array
    tempStringPower[8];
    dtostrf(temp_power, 1, 2, tempStringPower);
    // Publishes Temperature values
    client.publish(POWER5_TOPIC, tempStringPower);
node.clearResponseBuffer();
delay(Mdelay);

node.readHoldingRegisters(131, 1); 
Serial.print(" [Current phaseA 0x83] ");
Serial.println(node.getResponseBuffer(0));
    temp_power = node.getResponseBuffer(0);
    // Convert the value to a char array
    tempStringPower[8];
    dtostrf(temp_power, 1, 2, tempStringPower);
    // Publishes Temperature values
    client.publish(POWER6_TOPIC, tempStringPower);
node.clearResponseBuffer();
delay(Mdelay);
 
node.readHoldingRegisters(132, 1); 
Serial.print(" [Current phaseB 0x84] ");
Serial.println(node.getResponseBuffer(0));
    temp_power = node.getResponseBuffer(0);
    // Convert the value to a char array
    tempStringPower[8];
    dtostrf(temp_power, 1, 2, tempStringPower);
    // Publishes Temperature values
    client.publish(POWER7_TOPIC, tempStringPower);
node.clearResponseBuffer();
delay(Mdelay);

node.readHoldingRegisters(133, 1); 
Serial.print(" [Current phaseC 0x85] ");
Serial.println(node.getResponseBuffer(0));
    temp_power = node.getResponseBuffer(0);
    // Convert the value to a char array
    tempStringPower[8];
    dtostrf(temp_power, 1, 2, tempStringPower);
    // Publishes Temperature values
    client.publish(POWER8_TOPIC, tempStringPower);
node.clearResponseBuffer();
delay(Mdelay);

node.readHoldingRegisters(134, 1); 
Serial.print(" [Total Active Power H 0x86] ");
Serial.print(node.getResponseBuffer(0));
node.clearResponseBuffer();
delay(Mdelay);

node.readHoldingRegisters(135, 1);
Serial.print(" [Total Active Power L 0x87] ");
Serial.println(node.getResponseBuffer(0));
    temp_power = node.getResponseBuffer(0);
    // Convert the value to a char array
    tempStringPowerHL[8];
    dtostrf(temp_power, 1, 2, tempStringPower);
    // Publishes Temperature values
    client.publish(POWER9_TOPIC, tempStringPower);
node.clearResponseBuffer();
delay(Mdelay);

node.readHoldingRegisters(136, 1); 
Serial.print(" [Active Power phaseA 0x88] ");
Serial.println(node.getResponseBuffer(0));
    temp_power = node.getResponseBuffer(0);
    // Convert the value to a char array
    tempStringPower[8];
    dtostrf(temp_power, 1, 2, tempStringPower);
    // Publishes Temperature values
    client.publish(POWER10_TOPIC, tempStringPower);
node.clearResponseBuffer();
delay(Mdelay);

node.readHoldingRegisters(137, 1); 
Serial.print(" [Active Power phaseB 0x89] ");
Serial.println(node.getResponseBuffer(0));
    temp_power = node.getResponseBuffer(0);
    // Convert the value to a char array
    tempStringPower[8];
    dtostrf(temp_power, 1, 2, tempStringPower);
    // Publishes Temperature values
    client.publish(POWER11_TOPIC, tempStringPower);
node.clearResponseBuffer();
delay(Mdelay);

node.readHoldingRegisters(138, 1); 
Serial.print(" [Active Power phaseC 0x8A] ");
Serial.println(node.getResponseBuffer(0));
    temp_power = node.getResponseBuffer(0);
    // Convert the value to a char array
    tempStringPower[8];
    dtostrf(temp_power, 1, 2, tempStringPower);
    // Publishes Temperature values
    client.publish(POWER12_TOPIC, tempStringPower);
node.clearResponseBuffer();
delay(Mdelay);


node.readHoldingRegisters(139, 1); 
Serial.print(" [Total Reactive Power H 0x8B] ");
Serial.println(node.getResponseBuffer(0));
node.clearResponseBuffer();
delay(Mdelay);

node.readHoldingRegisters(140, 1); 
Serial.print(" [Total Reactive Power L 0x8C] ");
Serial.println(node.getResponseBuffer(0));
    temp_power = node.getResponseBuffer(0);
    // Convert the value to a char array
    tempStringPower[8];
    dtostrf(temp_power, 1, 2, tempStringPower);
    // Publishes Temperature values
    client.publish(POWER13_TOPIC, tempStringPower);
node.clearResponseBuffer();
delay(Mdelay);

node.readHoldingRegisters(141, 1); 
Serial.print(" [Reactive Power phaseA 0x8D] ");
Serial.println(node.getResponseBuffer(0));
    temp_power = node.getResponseBuffer(0);
    // Convert the value to a char array
    tempStringPower[8];
    dtostrf(temp_power, 1, 2, tempStringPower);
    // Publishes Temperature values
    client.publish(POWER14_TOPIC, tempStringPower);
node.clearResponseBuffer();
delay(Mdelay);

node.readHoldingRegisters(142, 1); 
Serial.print(" [Reactive Power phaseB 0x8E] ");
Serial.println(node.getResponseBuffer(0));
    temp_power = node.getResponseBuffer(0);
    // Convert the value to a char array
    tempStringPower[8];
    dtostrf(temp_power, 1, 2, tempStringPower);
    // Publishes Temperature values
    client.publish(POWER15_TOPIC, tempStringPower);
node.clearResponseBuffer();
delay(Mdelay);

node.readHoldingRegisters(143, 1); 
Serial.print(" [Reactive Power phaseC 0x8F] ");
Serial.println(node.getResponseBuffer(0));
    temp_power = node.getResponseBuffer(0);
    // Convert the value to a char array
    tempStringPower[8];
    dtostrf(temp_power, 1, 2, tempStringPower);
    // Publishes Temperature values
    client.publish(POWER16_TOPIC, tempStringPower);
node.clearResponseBuffer();
delay(Mdelay);

node.readHoldingRegisters(144, 1); 
Serial.print(" [Total Apparent Power H 0x90] ");
Serial.println(node.getResponseBuffer(0));
node.clearResponseBuffer();
delay(Mdelay);

node.readHoldingRegisters(145, 1); 
Serial.print(" [Total Apparent Power L 0x91] ");
Serial.println(node.getResponseBuffer(0));
    temp_power = node.getResponseBuffer(0);
    // Convert the value to a char array
    tempStringPower[8];
    dtostrf(temp_power, 1, 2, tempStringPower);
    // Publishes Temperature values
    client.publish(POWER17_TOPIC, tempStringPower);
node.clearResponseBuffer();
delay(Mdelay);

node.readHoldingRegisters(146, 1); 
Serial.print(" [Apparent Power phaseA 0x92] ");
Serial.println(node.getResponseBuffer(0));
    temp_power = node.getResponseBuffer(0);
    // Convert the value to a char array
    tempStringPower[8];
    dtostrf(temp_power, 1, 2, tempStringPower);
    // Publishes Temperature values
    client.publish(POWER18_TOPIC, tempStringPower);
node.clearResponseBuffer();
delay(Mdelay);

node.readHoldingRegisters(147, 1); 
Serial.print(" [Apparent Power phaseB 0x93] ");
Serial.println(node.getResponseBuffer(0));
    temp_power = node.getResponseBuffer(0);
    // Convert the value to a char array
    tempStringPower[8];
    dtostrf(temp_power, 1, 2, tempStringPower);
    // Publishes Temperature values
    client.publish(POWER19_TOPIC, tempStringPower);
node.clearResponseBuffer();
delay(Mdelay);

node.readHoldingRegisters(148, 1); 
Serial.print(" [Apparent Power phaseC 0x94] ");
Serial.println(node.getResponseBuffer(0));
    temp_power = node.getResponseBuffer(0);
    // Convert the value to a char array
    tempStringPower[8];
    dtostrf(temp_power, 1, 2, tempStringPower);
    // Publishes Temperature values
    client.publish(POWER20_TOPIC, tempStringPower);
node.clearResponseBuffer();
delay(Mdelay);




node.readHoldingRegisters(149, 1); 
Serial.print(" [Total Power factor 0x95] ");
Serial.println(node.getResponseBuffer(0));
    temp_power = node.getResponseBuffer(0);
    // Convert the value to a char array
    tempStringPower[8];
    dtostrf(temp_power, 1, 2, tempStringPower);
    // Publishes Temperature values
    client.publish(POWER21_TOPIC, tempStringPower);
node.clearResponseBuffer();
delay(Mdelay);

node.readHoldingRegisters(150, 1); 
Serial.print(" [Power factor phaseA 0x96] ");
Serial.println(node.getResponseBuffer(0));
    temp_power = node.getResponseBuffer(0);
    // Convert the value to a char array
    tempStringPower[8];
    dtostrf(temp_power, 1, 2, tempStringPower);
    // Publishes Temperature values
    client.publish(POWER22_TOPIC, tempStringPower);
node.clearResponseBuffer();
delay(Mdelay);

node.readHoldingRegisters(160, 1); 
Serial.print(" [Power factor phaseB 0x97] ");
Serial.println(node.getResponseBuffer(0));
    temp_power = node.getResponseBuffer(0);
    // Convert the value to a char array
    tempStringPower[8];
    dtostrf(temp_power, 1, 2, tempStringPower);
    // Publishes Temperature values
    client.publish(POWER23_TOPIC, tempStringPower);
node.clearResponseBuffer();
delay(Mdelay);

node.readHoldingRegisters(170, 1); 
Serial.print(" [Power factor phaseC 0x98] ");
Serial.println(node.getResponseBuffer(0));
    temp_power = node.getResponseBuffer(0);
    // Convert the value to a char array
    tempStringPower[8];
    dtostrf(temp_power, 1, 2, tempStringPower);
    // Publishes Temperature values
    client.publish(POWER24_TOPIC, tempStringPower);
node.clearResponseBuffer();
delay(Mdelay);
      
  } //RS485 loop

 } //Wifi connection looop 
  
}// END OF LOOP VOID FUNCTIONFUNCTION
