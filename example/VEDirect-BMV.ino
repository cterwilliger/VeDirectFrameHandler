/******************************************************************
 VEDirect-BMV

 Uses VeDirectFrameHandler library

 Board: NodeMCU 1.0 (ESP-12E Module)
 Upload Speed: 115200
 CPU Freq: 80Mhz
 Flash Size: 4M (1M SPIFFS)
 IwIP Variant: v2 Lower Memory
 VTables: Flash
 Port: /dev/ttyUSB0
 Programmer: AVR ISP

 BMV700 is 3.3V, MPPT is 5V, so level shifter required for that one

 VEDirect:
   pin 1 - gnd
   pin 2 - RX
   pin 3 - TX
   pin 4 - power

 History:
  2020.04.10 - 0.0 - initial
  2020.04.17 - 0.1 - add public array to library
  2020.04.21 - 0.2 - add MQTT
  2020.05.02 - 0.3 - changed from SoftwareSerial to Serial, too many issues
******************************************************************/

#include "Arduino.h"
#include "framehandler.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

VeDirectFrameHandler myve;

uint8_t rc;

bool newData = false;

WiFiClient espClient;                                           // WiFi object
PubSubClient client(espClient);                                 // MQTT object
// PubSubClient.h : #define MQTT_MAX_PACKET_SIZE 1024

// Network settings
const char* ssid = "lola_wireless";
const char* password = "a67ad3ea";
const char* mqtt_server = "coachproxyos";   // lola

void setup() {
	Serial.begin(19200);		// VE Device
    Serial.swap();              // HW serial for pgm load and then switch to GPIO13 for reading from device

    // Initialize WiFi and MQTT
    setup_wifi();
    client.setServer(mqtt_server, 1883);

    Serial.flush();
} // setup


void loop() {
    //ReadVEData();
    
    
    // connect to mqtt server
    if ( !client.connected() ) {
        reconnect();
    }
    client.loop();
    //delay(100);
    
    //ClientLoop();
    
    ReadVEData();
    
    EverySecond();
    //PublishData(); // seems to work without delay, but is it necessary?  
} // loop


void ClientLoop() {
    static unsigned long client_prev_millis;
    if (millis() - client_prev_millis > 100) {

        // connect to mqtt server
        if ( !client.connected() ) {
            reconnect();
        }
        client.loop();

        client_prev_millis = millis();
    }
} // ClientLoop


void ReadVEData() {
    while ( Serial.available() ) {
        myve.rxData(Serial.read());
        yield();
    }
} // ReadVEData


void EverySecond() {
    static unsigned long prev_millis;
    if (millis() - prev_millis > 1000) {
        //PrintData();
        PublishData();
        prev_millis = millis();
    }
} // EverySecond


void PrintData() {
    for (int i = 0; i < myve.veEnd; i++ ) {
        Serial.print(myve.veName[i]);
        Serial.print("= ");
        Serial.println(myve.veValue[i]);    
    }
} // PrintData


void PublishData() {
    // JSONize it for coachproxy
    StaticJsonDocument<800> docI;   // <- bytes in the heap, 732 in testing    
    char tmpI[500];                 // 372 in testing
    
    for ( int i = 0; i < myve.veEnd; i++) {
        docI[myve.veName[i]] = myve.veValue[i];
    }

    // tried docI["instance"] = 1; but didn't seem reliable. this makes instance the same type as myve.veName
    char inst[1][9] = {"instance"};
    docI[inst[0]] = 1; 
    
    int tmpsize = serializeJson(docI, tmpI);
      
    client.publish("VE/BMV/1", tmpI);
    //delay(100);
}


void setup_wifi()
{
  delay(10);
  //Serial.println();
  //Serial.print("Connecting to ");
  //Serial.println(ssid);

  WiFi.mode(WIFI_STA);        // STA only, no AP
  WiFi.begin(ssid, password);   // connect to a WiFi network

  while ( WiFi.status() != WL_CONNECTED ) 
  {
    delay(500);
    //Serial.print(".");
  }

  //Serial.println("");
  //Serial.println("WiFi connected");
  //Serial.println("IP address: ");
  //Serial.println(WiFi.localIP());
} // setup_wifi


void reconnect() {
    // Loop until we're reconnected
    while ( !client.connected() ) {
        //Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if ( client.connect("ESP8266VictronBMV") ) {
            //Serial.println("connected");
            // Once connected, publish an announcement...
            //client.publish("VE/BMV/1", "STARTUP");
        } 
        else {
            //Serial.print("failed, rc=");
            //Serial.print(client.state());
            //Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
} // reconnect
