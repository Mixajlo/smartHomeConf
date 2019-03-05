/*
  Connect relay to GPIO2 and push button to GPIO0
  Use this for sonoff to connect to mqtt and have a working push button
  Use the same code to control a relay with push button or message sent to mqtt
  Use the same code to control two relays with one ESP8266-01 module
*/

#define ENABLE_MQTT
#define SETUP_HTTP_SERVER
#define ONE_BUTTON
#define ENABLE_EEPROM

#ifdef ONE_BUTTON
#define RELAY_1_PIN 2
#define BUTTON_PIN 0
#define LED 13
#else
#define RELAY_1_PIN 2
#define RELAY_2_PIN 0
#endif


/*
  It connects to an MQTT server then:
  - on 0 switches off relay
  - on 1 switches on relay
  - on 2 switches the state of the relay

  - sends 0 on off relay
  - sends 1 on on relay

  It will reconnect to the server if the connection is lost using a blocking
  reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
  achieve the same result without blocking the main loop.

  The current state is stored in EEPROM and restored on bootup
*/
#ifdef ONE_BUTTON
#include <Bounce2.h>
#endif

#ifdef ENABLE_EEPROM
#include <EEPROM.h>
#endif

#include <ESP8266mDNS.h>

#ifdef SETUP_HTTP_SERVER
#include <ESP8266WebServer.h>
#endif

#ifdef ENABLE_MQTT
#include <PubSubClient.h>
#endif

/*
   Setup SSID and password for wifi access point
*/
const char* ssid = "SSID"; // CHANGE THIS
const char* password = "PASS"; // CHANGE THIS

/*
   Setup mqtt broker IP address
   Setup the "in" and "out" topics for mqtt
*/
#ifdef ENABLE_MQTT
const char* inTopic = "topic/in";
const char* outTopic = "topic/out";
const char* mqtt_server = "192.168.xxx.xxx"; // CHANGE THIS
#endif

/*
   Variables
*/
#ifdef SETUP_HTTP_SERVER
MDNSResponder mdns;
ESP8266WebServer server(80);
String webPage = "";
#endif

#ifdef ENABLE_MQTT
WiFiClient espClient;
PubSubClient mqttClient(espClient);
#endif

#ifdef ONE_BUTTON
// Instantiate a Bounce object :
Bounce debouncer = Bounce();
#endif

bool relay1State = LOW;
bool relay2State = LOW;

void setupPins() {
#ifdef ONE_BUTTON
  pinMode(BUTTON_PIN, INPUT);     // Initialize the button pin as an input
  pinMode(RELAY_1_PIN, OUTPUT);     // Initialize the relay 1 pin as an output
  pinMode(LED, OUTPUT);
  debouncer.attach(BUTTON_PIN);   // Use the bounce2 library to debounce the built in button
  debouncer.interval(50);         // Input must be low for 50 ms
#else
  pinMode(RELAY_1_PIN, OUTPUT);     // Initialize the relay 1 pin as an output
  pinMode(RELAY_2_PIN, OUTPUT);     // Initialize the relay 2 pin as an output
#endif
}

void publishMsg(const char* topic, char* msg) {
#ifdef ENABLE_MQTT
  mqttClient.publish(topic, msg);
#endif
}

#ifdef SETUP_HTTP_SERVER
void setupHttpServer() {
  webPage += "<h1>ESP8266 Web Server</h1>";
  webPage += "<p>Relay #1 <a href=\"relay1On\"><button>ON</button></a>&nbsp;<a href=\"relay1Off\"><button>OFF</button></a></p>";
#ifndef ONE_BUTTON
  webPage += "<p>Relay #2 <a href=\"relay2On\"><button>ON</button></a>&nbsp;<a href=\"relay2Off\"><button>OFF</button></a></p>";
#endif
  if (mdns.begin("relay", WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }

  server.on("/", []() {
    server.send(200, "text/html", webPage);
  });

  server.on("/relay1On", []() {
    server.send(200, "text/html", webPage);
    digitalWrite(RELAY_1_PIN, HIGH);
    digitalWrite(LED, LOW);
    publishMsg(outTopic, "1");
    delay(1000);
  });
  server.on("/relay1Off", []() {
    server.send(200, "text/html", webPage);
    digitalWrite(RELAY_1_PIN, LOW);
    digitalWrite(LED, HIGH);
    publishMsg(outTopic, "0");
    delay(1000);
  });
#ifndef ONE_BUTTON
  server.on("/relay2On", []() {
    server.send(200, "text/html", webPage);
    digitalWrite(RELAY_2_PIN, HIGH);
    publishMsg(outTopic, "1");
    delay(1000);
  });
  server.on("/relay2Off", []() {
    server.send(200, "text/html", webPage);
    digitalWrite(RELAY_2_PIN, LOW);
    publishMsg(outTopic, "0");
    delay(1000);
  });
#endif
  server.begin();
  Serial.println("HTTP server set up");
}
#endif

#ifdef ENABLE_MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Only "0", "1" and "2" are valid data
  if ((char)payload[0] == '0') {
    digitalWrite(RELAY_1_PIN, LOW);
    Serial.println("RELAY_1_PIN -> LOW");
    relay1State = LOW;
    writeToEEPROM(0, relay1State);
  } else if ((char)payload[0] == '1') {
    digitalWrite(RELAY_1_PIN, HIGH);
    Serial.println("RELAY_1_PIN -> HIGH");
    relay1State = HIGH;
    writeToEEPROM(0, relay1State);
  } else if ((char)payload[0] == '2') {
    relay1State = !relay1State;
    digitalWrite(RELAY_1_PIN, relay1State);
    Serial.print("RELAY_1_PIN -> switched to ");
    Serial.println(relay1State);
    writeToEEPROM(0, relay1State);
  }
}
#endif

#ifdef ONE_BUTTON
void extButton() {
  debouncer.update();

  // Call code if Bounce fell (transition from HIGH to LOW) :
  if ( debouncer.fell() ) {
    Serial.println("Debouncer fell");
    // Toggle relay state :
    relay1State = !relay1State;
    digitalWrite(RELAY_1_PIN, relay1State);
    digitalWrite(LED, !relay1State);
    writeToEEPROM(0, relay1State);    // Write state to EEPROM
#ifdef ENABLE_MQTT
    if (relay1State == 1) {
      mqttClient.publish(outTopic, "1");
    }
    else if (relay1State == 0) {
      mqttClient.publish(outTopic, "0");
    }
#endif
  }
}
#endif

#ifdef ENABLE_MQTT
void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    // Attempt to connect
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqttClient.publish(outTopic, "ESP8266 booted");
      // ... and resubscribe
      mqttClient.subscribe(inTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      for (int i = 0; i < 5000; i++) {
        extButton();
        delay(1);
      }
    }
  }
}
#endif

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    for (int i = 0; i < 500; i++) {
#ifdef ONE_BUTTON
      extButton();
#endif
      delay(1);
    }
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
#ifdef ENABLE_EEPROM
  EEPROM.begin(512);              // Begin eeprom to store on/off state
  relay1State = EEPROM.read(0);
#endif
  digitalWrite(RELAY_1_PIN, relay1State);
  setupPins();
  Serial.begin(115200);
#ifdef SETUP_HTTP_SERVER
  setupHttpServer();
#endif
  setup_wifi();                   // Connect to wifi
#ifdef ENABLE_MQTT
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(callback);
#endif
}

void loop() {
#ifdef ENABLE_MQTT
  if (!mqttClient.connected()) {
    reconnect();
  }
  delay(100);
  mqttClient.loop();
#endif
#ifdef ONE_BUTTON
  extButton();
#endif
#ifdef SETUP_HTTP_SERVER
  server.handleClient();
#endif
}

void writeToEEPROM(int addr, bool value) {
#ifdef ENABLE_EEPROM
  EEPROM.write(addr, value);
  EEPROM.commit();
#endif
}

