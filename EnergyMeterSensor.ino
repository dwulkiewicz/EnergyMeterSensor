
/************************************************************************/
/*                                                                      */
/*            Napięcie,Prąd, Moc i Energia na MQTT                      */
/*              Hardware: ESP8266 (ESP12E), PZEM004T                    */
/*                                                                      */
/*              Author: Dariusz Wulkiewicz                              */
/*                      d.wulkiewicz@gmail.com                          */
/*                                                                      */
/*              Date: 01.2019                                           */
/************************************************************************/

//https://github.com/olehs/PZEM004T

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <PZEM004T.h> 
#include "Constants.h"

String wifiSSID;
String wifiPassword;
String mqttServer;

WiFiClient espClient;
PubSubClient client(espClient);

PZEM004T pzem(SOFT_UART1_RX,SOFT_UART1_TX);  // (RX,TX) connect to TX,RX of PZEM
IPAddress ip(192,168,1,1);

char msg[50];

// Set Hostname.
String esp2866_hostname(HOSTNAME_PREFIX);
//----------------------------------------------------------------------------------------
bool loadConfig() {
  File file = SPIFFS.open("/config.json", "r");
  if (!file) {
    Serial.println("Failed to open config file");
    return false;
  }

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, file);
  if (error)
    Serial.println(F("Failed to read file, using default configuration"));
  else
  {
    JsonObject root = doc.as<JsonObject>();  
    wifiSSID = root["ssid"].as<String>();
    wifiPassword = root["password"].as<String>();
    mqttServer = root["mqtt_server"].as<String>();  
  }
  file.close();
 
  Serial.printf("Loaded wifiSSID: %s\r\n", wifiSSID.c_str());
  Serial.printf("Loaded wifiPassword: %s\r\n", wifiPassword.c_str());
  Serial.printf("Loaded mqttServer: %s\r\n", mqttServer.c_str());
  return true;
}
//----------------------------------------------------------------------------------------
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.printf("\r\nConnecting to %s\r\n", wifiSSID.c_str());

  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(BUILT_LED, LED_ON);
    delay(250);
    digitalWrite(BUILT_LED, LED_OFF);
    delay(250);
    Serial.print(".");
  }

  // Set Hostname.
  esp2866_hostname += String(ESP.getChipId(), HEX);
  WiFi.hostname(esp2866_hostname);

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.printf("IP address: %s Hostname: %s\r\n", WiFi.localIP().toString().c_str(), esp2866_hostname.c_str());
}
//----------------------------------------------------------------------------------------
void reconnect() {
  digitalWrite(BUILT_LED, LED_OFF);
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    // Attempt to connect
    char buf[20];
    sprintf(buf, "%s_%08X", prefixClientID, ESP.getChipId());
    if (client.connect(&buf[0])) {
      digitalWrite(BUILT_LED, LED_ON);
      Serial.printf("connected as %s\r\n", &buf[0]);

      // ... and resubscribe
      // można subskrybować wiele topiców
      client.subscribe(sensorCommandTopic);
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
//----------------------------------------------------------------------------------------
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Conver the incoming byte array to a string
  payload[length] = '\0'; // Null terminator used to terminate the char array
  String mqttTopic = topic;
  String mqttMessage = (char*)payload;

  Serial.printf("MQTT received topic:[%s], msg: %s\r\n", topic, mqttMessage.c_str());

  /*PZM004T*/
  if (mqttTopic.equals(sensorCommandTopic) && mqttMessage.equals(sensorCommandVoltage)) {    
    float v = pzem.voltage(ip);
    v = v >= 0.0 ? v : 0.0; 
    Serial.printf("Voltage %.2fV\r\n",v);    
    dtostrf(v, 2, 1, msg);
    Serial.printf("MQTT send topic:[%s], msg: %s\r\n", sensorVoltageTopic, msg);
    client.publish(sensorVoltageTopic, msg);
  }
  else if (mqttTopic.equals(sensorCommandTopic) && mqttMessage.equals(sensorCommandCurrent)) {
    float i = pzem.current(ip);
    i = i >= 0.0 ? i : 0.0; 
    Serial.printf("Current %.2fA\r\n",i);     
    dtostrf(i, 2, 2, msg);
    Serial.printf("MQTT send topic:[%s], msg: %s\r\n", sensorCurrentTopic, msg);
    client.publish(sensorCurrentTopic, msg);
  }
  else if (mqttTopic.equals(sensorCommandTopic) && mqttMessage.equals(sensorCommandPower)) {
    float p = pzem.power(ip);
    p = p >= 0.0 ? p : 0.0; 
    Serial.printf("Power %.2fW\r\n",p);   
    dtostrf(p, 1, 0, msg);
    Serial.printf("MQTT send topic:[%s], msg: %s\r\n", sensorPowerTopic, msg);
    client.publish(sensorPowerTopic, msg);
  }
  else if (mqttTopic.equals(sensorCommandTopic) && mqttMessage.equals(sensorCommandEnergy)) {
    float e = pzem.energy(ip);
    e = e >= 0.0 ? e : 0.0; 
    Serial.printf("Energy %.2fWh\r\n",e);   
    dtostrf(e, 1, 0, msg);
    Serial.printf("MQTT send topic:[%s], msg: %s\r\n", sensorEnergyTopic, msg);
    client.publish(sensorEnergyTopic, msg);
  }  
}
//----------------------------------------------------------------------------------------
void setup() {
  pinMode(BUILT_LED, OUTPUT);
  Serial.begin(115200);

  digitalWrite(BUILT_LED, LED_ON); // Turn the LED on
  delay(1000);
  digitalWrite(BUILT_LED, LED_OFF); // Turn the LED off by making the voltage HIGH
  Serial.println("");
    
  Serial.println("****************************");
  Serial.println("* PZM004T by MQTT          *");
  Serial.println("****************************");

  while (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    for (uint8_t i=0; i<=5; i++){
      digitalWrite(BUILT_LED, LED_ON); // Turn the LED on
      delay(100);
      digitalWrite(BUILT_LED, LED_OFF); // Turn the LED off by making the voltage HIGH
      delay(100);
    }
    delay(5000);    
  }

  while (!loadConfig()) {
    Serial.println("Failed load config");
    for (uint8_t i=0; i<=5; i++){
      digitalWrite(BUILT_LED, LED_ON); // Turn the LED on
      delay(100);
      digitalWrite(BUILT_LED, LED_OFF); // Turn the LED off by making the voltage HIGH
      delay(100);
    }   
    delay(5000);
  }
  
  //PZM004T initialize
  while (true) {
    Serial.println("Connecting to PZEM...");
    if(pzem.setAddress(ip))
      break;
    delay(1000);
  }

  setup_wifi();     // Connect to wifi   

  client.setServer(mqttServer.c_str(), 1883);
  client.setCallback(mqttCallback);

  // Start OTA server.
  ArduinoOTA.setHostname((const char *)esp2866_hostname.c_str());
  ArduinoOTA.begin();   
}
//----------------------------------------------------------------------------------------
void loop() {
  if (!client.connected()) {
    reconnect();
  }

  // Handle OTA server.
  ArduinoOTA.handle();

  client.loop();
}
