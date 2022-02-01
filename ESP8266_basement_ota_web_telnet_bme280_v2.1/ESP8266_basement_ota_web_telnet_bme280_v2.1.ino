// I use for my toys simple OTA through a browser.
// When using port mapping on router, it works from anywhere.
// Here is the entire code that is necessary for that.
// The uploaded file must, of course, be in binary form as *.bin
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <PubSubClient.h>
#include <algorithm> // std::min

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme;
float temperature, humidity, pressure, altitude;

const char* ssid =     "xxxx";     // Set your router SSID
const char* password = "xxxx"; // Set your router password

// MQTT: ID, server IP, port, username and password
const PROGMEM char* MQTT_CLIENT_ID = "ESP8266_basement";
const PROGMEM char* MQTT_SERVER_IP = "192.168.xx.xx";
const PROGMEM uint16_t MQTT_SERVER_PORT = 1883;
const PROGMEM char* MQTT_USER = "xxx";
const PROGMEM char* MQTT_PASSWORD = "xxx";

#define SWITCH_TOPIC_1    "basement/led"
#define RELAY_TOPIC_1     "basement/relay1"
#define RELAY_TOPIC_2     "basement/relay2"
#define RELAY_TOPIC_3     "basement/relay3"
#define RELAY_TOPIC_4     "basement/relay4"
#define SENSOR_TEMP_TOPIC     "basement/temperature"
#define SENSOR_HUMID_TOPIC    "basement/humidity"
#define SENSOR_PRESS_TOPIC    "basement/pressure"
#define SENSOR_ALTI_TOPIC     "basement/altitude"

#define OTAUSER         "xxxx"    // Set OTA user
#define OTAPASSWORD     "xxxx"    // Set OTA password
#define OTAPATH         "/firmware"// Set path for update
#define SERVERPORT      80         // Server

//---------serial to telnet------------
#define SWAP_PINS 0
#define SERIAL_LOOPBACK 1
#define BAUD_SERIAL 115200
#define BAUD_LOGGER 115200
#define RXBUFFERSIZE 1024

#if SERIAL_LOOPBACK
#undef BAUD_SERIAL
#define BAUD_SERIAL 3000000
#include <esp8266_peri.h>
#endif

#if SWAP_PINS
#include <SoftwareSerial.h>
SoftwareSerial* logger = nullptr;
#else
#define logger (&Serial1)
#endif

#define STACK_PROTECTOR  512 // bytes

//how many clients should be able to telnet to this ESP8266
#define MAX_SRV_CLIENTS 2
const int port = 23;

// Initializes the espClient. You should change the espClient name if you have multiple ESPs running in your home automation system
WiFiClient espClient; //mqtt
PubSubClient client(espClient);

WiFiServer server(port); //telnet
WiFiClient serverClients[MAX_SRV_CLIENTS];

// Lamp - LED - GPIO 16 = buildin on ESP-12E NodeMCU board
const int lamp = 2;

// relay pins D5, D6, D7, D0
const int relay1 = 14;
const int relay2 = 12;
const int relay3 = 13;
const int relay4 = 16;

// Timers auxiliar variables
long now = millis();
long lastMeasure = 0;

ESP8266WebServer HttpServer(SERVERPORT);
ESP8266HTTPUpdateServer httpUpdater;

/* Send HTTP status 404 Not Found */
void handleNotFound() {
  HttpServer.send(404, "text/plain", "404: Not found");
}

void handleRoot() {
  HttpServer.send(200, "text/plain", MQTT_CLIENT_ID);
}

//-----------------------------------------------------------------

void setup(void) {
  pinMode(lamp, OUTPUT);
  
  bme.begin(0x76);  
  
  Serial.begin(115200);
  Serial.setRxBufferSize(RXBUFFERSIZE);

#if SWAP_PINS
  Serial.swap();
  // Hardware serial is now on RX:GPIO13 TX:GPIO15
  // use SoftwareSerial on regular RX(3)/TX(1) for logging
  logger = new SoftwareSerial(3, 1);
  logger->begin(BAUD_LOGGER);
  logger->enableIntTx(false);
#else
  logger->begin(BAUD_LOGGER);
#endif

#if SERIAL_LOOPBACK
  USC0(0) |= (1 << UCLBE); // incomplete HardwareSerial API
#endif
  
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
  Serial.println(RELAY_TOPIC_4);
  Serial.println(SENSOR_TEMP_TOPIC);
  Serial.println(SENSOR_HUMID_TOPIC);
  Serial.println(SENSOR_PRESS_TOPIC);
  Serial.println(SENSOR_ALTI_TOPIC);
  Serial.println("INFO: sensor BME280");

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
  Serial.print("INFO: Device IP address: ");
  Serial.println(WiFi.localIP());

  //start server
  server.begin();
  server.setNoDelay(true);
 
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
  pinMode(relay4, OUTPUT);
  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);
  digitalWrite(relay3, HIGH);
  digitalWrite(relay4, HIGH);
  
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

  if(topic==RELAY_TOPIC_4){
      Serial.print("Changing relay #4 value to ");
      if(messageTemp == "on"){
        digitalWrite(relay4, HIGH);
        Serial.print("on");
      }
      else if(messageTemp == "off"){
        digitalWrite(relay4, LOW);
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
      client.subscribe(RELAY_TOPIC_4);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop(void) {
  HttpServer.handleClient();       // Listen for HTTP requests from clients

 if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
    client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD);
    
  now = millis();
  // Publishes new temperature and humidity every 10 seconds
  if (now - lastMeasure > 10000) {
    lastMeasure = now;
    
  temperature = bme.readTemperature();
  humidity = bme.readHumidity();
  pressure = bme.readPressure() / 100.0F;
  altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
    
    // Check if any reads failed and exit early (to try again).
    if (isnan(temperature) || isnan(humidity) || isnan(pressure)) {
      Serial.println("Failed to read from BME280 sensor!");
      return;
    }

    static char temperatureTemp[7];
    dtostrf(temperature, 6, 2, temperatureTemp);
    // Publishes Temperature and Humidity values
    client.publish(SENSOR_TEMP_TOPIC, temperatureTemp);
    
    dtostrf(humidity, 6, 2, temperatureTemp);
    client.publish(SENSOR_HUMID_TOPIC, temperatureTemp);

    dtostrf(pressure, 6, 2, temperatureTemp);
    client.publish(SENSOR_PRESS_TOPIC, temperatureTemp);

    dtostrf(altitude, 6, 2, temperatureTemp);
    client.publish(SENSOR_ALTI_TOPIC, temperatureTemp);

 
    Serial.println("");
    Serial.println("Home>");
    Serial.println("---------------------------------------------");
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println("% RH ");
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" *C ");
    Serial.print("Pressure: ");
    Serial.print(pressure);
    Serial.println("hPa");
    Serial.print("Altitude: ");
    Serial.print(altitude);
    Serial.println(" m");
    
  }
    

/// telnet
 //check if there are any new clients
  if (server.hasClient()) {
    //find free/disconnected spot
    int i;
    for (i = 0; i < MAX_SRV_CLIENTS; i++)
      if (!serverClients[i]) { // equivalent to !serverClients[i].connected()
        serverClients[i] = server.available();
        logger->print("New client: index ");
        logger->print(i);
        break;
      }

    //no free/disconnected spot so reject
    if (i == MAX_SRV_CLIENTS) {
      server.available().println("busy");
      // hints: server.available() is a WiFiClient with short-term scope
      // when out of scope, a WiFiClient will
      // - flush() - all data will be sent
      // - stop() - automatically too
      logger->printf("server is busy with %d active connections\n", MAX_SRV_CLIENTS);
    }
  }

  //check TCP clients for data
#if 1
  // Incredibly, this code is faster than the bufferred one below - #4620 is needed
  // loopback/3000000baud average 348KB/s
  for (int i = 0; i < MAX_SRV_CLIENTS; i++)
    while (serverClients[i].available() && Serial.availableForWrite() > 0) {
      // working char by char is not very efficient
      Serial.write(serverClients[i].read());
    }
#else
  // loopback/3000000baud average: 312KB/s
  for (int i = 0; i < MAX_SRV_CLIENTS; i++)
    while (serverClients[i].available() && Serial.availableForWrite() > 0) {
      size_t maxToSerial = std::min(serverClients[i].available(), Serial.availableForWrite());
      maxToSerial = std::min(maxToSerial, (size_t)STACK_PROTECTOR);
      uint8_t buf[maxToSerial];
      size_t tcp_got = serverClients[i].read(buf, maxToSerial);
      size_t serial_sent = Serial.write(buf, tcp_got);
      if (serial_sent != maxToSerial) {
        logger->printf("len mismatch: available:%zd tcp-read:%zd serial-write:%zd\n", maxToSerial, tcp_got, serial_sent);
      }
    }
#endif

  // determine maximum output size "fair TCP use"
  // client.availableForWrite() returns 0 when !client.connected()
  size_t maxToTcp = 0;
  for (int i = 0; i < MAX_SRV_CLIENTS; i++)
    if (serverClients[i]) {
      size_t afw = serverClients[i].availableForWrite();
      if (afw) {
        if (!maxToTcp) {
          maxToTcp = afw;
        } else {
          maxToTcp = std::min(maxToTcp, afw);
        }
      } else {
        // warn but ignore congested clients
        logger->println("one client is congested");
      }
    }

  //check UART for data
  size_t len = std::min((size_t)Serial.available(), maxToTcp);
  len = std::min(len, (size_t)STACK_PROTECTOR);
  if (len) {
    uint8_t sbuf[len];
    size_t serial_got = Serial.readBytes(sbuf, len);
    // push UART data to all connected telnet clients
    for (int i = 0; i < MAX_SRV_CLIENTS; i++)
      // if client.availableForWrite() was 0 (congested)
      // and increased since then,
      // ensure write space is sufficient:
      if (serverClients[i].availableForWrite() >= serial_got) {
        size_t tcp_sent = serverClients[i].write(sbuf, serial_got);
        if (tcp_sent != len) {
          logger->printf("len mismatch: available:%zd serial-read:%zd tcp-write:%zd\n", len, serial_got, tcp_sent);
        }
      }
  }
  

}
// END OF LOOP VOID FUNCTION
