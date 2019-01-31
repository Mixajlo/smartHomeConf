/***************************************************
  Adafruit MQTT Library ESP8266 Example

  Must use ESP8266 Arduino from:
    https://github.com/esp8266/Arduino

  Works great with Adafruit's Huzzah ESP board:
  ----> https://www.adafruit.com/product/2471

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Tony DiCola for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
 
/*****************************************************
   AdafruitIO mqtt connectoin to esp8266 board(WeMos) to control a heater remote control.

   The code uses static ssid and password
   The feed on AdafruitIO is "/feed/onoff". Same feed is used for both ON and OFF buttons
*/
/*
   TODOs:
          Implement wifi client for easy change of ssid and pass
          Implement dynamic # of buttons for more buttons to be controlled, or maybe turn leds on/off
****************************************************/
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

/*********************** Buttons configuration *******************************/

#define BUTTON_ON  D6
#define BUTTON_OFF D7

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "MY_SSID" // CHANGE THIS
#define WLAN_PASS       "MY_WIFI_PASS" // CHANGE THIS

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "MY_ADA_USERNAME" // CHANGE THIS
#define AIO_KEY         "MY_ADA_KEY" // CHANGE THIS

/************ Global State (you don't need to change this!) ******************/
// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_USERNAME, AIO_KEY);
/****************************** Feeds ***************************************/
// Setup a feed called 'onoff' for subscribing to changes to the button
Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/onoff", MQTT_QOS_1);
/*************************** Sketch Code ************************************/
void onoffcallback(char *data, uint16_t len) {
  Serial.print("Hey we're in a onoff callback, the button value is: ");
  Serial.println(data);
  if (strcmp(data, "1") == 0) {
    Serial.println("Turning ON");
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(BUTTON_ON, HIGH);
    delay(1000);
    digitalWrite(BUTTON_ON, LOW);
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    Serial.println("Turning OFF");
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(BUTTON_OFF, HIGH);
    delay(1000);
    digitalWrite(BUTTON_OFF, LOW);
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

void setup() {
  Serial.begin(115200);
  delay(10);

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  onoffbutton.setCallback(onoffcallback);

  // Setup MQTT subscription for feed.
  mqtt.subscribe(&onoffbutton);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  pinMode(BUTTON_ON, OUTPUT);
  digitalWrite(BUTTON_ON, LOW);
  pinMode(BUTTON_OFF, OUTPUT);
  digitalWrite(BUTTON_OFF, LOW);
}

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // this is our 'wait for incoming subscription packets and callback em' busy subloop
  // try to spend your time here:
  mqtt.processPackets(10000);

  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds

  if (! mqtt.ping()) {
    mqtt.disconnect();
  }
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 10 seconds...");
    mqtt.disconnect();
    delay(10000);  // wait 10 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1);
    }
  }
  Serial.println("MQTT Connected!");
}

