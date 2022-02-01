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

const char* ssid =     "xxxxx";     // Set your router SSID
const char* password = "xxxxxxc"; // Set your router password

// MQTT: ID, server IP, port, username and password
const PROGMEM char* MQTT_CLIENT_ID = "ESP8266_alarm";
const PROGMEM char* MQTT_SERVER_IP = "192.168.xxx.xxx";
const PROGMEM uint16_t MQTT_SERVER_PORT = 1883;
const PROGMEM char* MQTT_USER = "xxxx";
const PROGMEM char* MQTT_PASSWORD = "xxxxx";

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

#define OTAUSER         "xxxx"    // Set OTA user
#define OTAPASSWORD     "xxxxx"    // Set OTA password
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

  } //sensor reading loop

 } //Wifi connection looop
  
}// END OF LOOP VOID FUNCTIONFUNCTION
