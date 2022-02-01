# ESP8266

Hello,
this is my smart home "IoT devices" connected via WiFi to home network. 
They collect temperature, humidity and power meter sensors info and send it via MQTT protocol to MQTT brocker for futher visualization and automation. 
MQTT brocker is running on Home Assistant.
Home Assistant is used as main home controller.

There is short description what "IoT devices" do:
ESP8266_alarm_power_ota_web_ds18b20_v3 - WiFi module with temperature sensors DS18B20 and POWER meter DTS238-7 (via RS485) readings
ESP8266_basement_ota_web_telnet_bme280_v2.1 - WiFi module with temperature and humidity sensors BME280 readings
ESP8266_heater_ota_web_telnet_ds18b20_v2.2 - WiFi module with temperature sensors DS18B20 readings
ESP8266_power_ota_web_v4.0 - WiFi module with POWER meter (via RS485) readings
ESP8266_alarm_ota_web_ds18b20_v4.0 - WiFi module with temperature sensors DS18B20
