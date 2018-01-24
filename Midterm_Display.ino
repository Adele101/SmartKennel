/*Adele Parsons
 * 
 * Controls an ambient display. Neopixels change color based
 * on the readings from the sensors, and green LEDs act as a visual timer.
 * 
 * 
*/

// Libraries to include.
#include "Wire.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

#define wifi_ssid "" // WiFi name
#define wifi_password "" // WiFi password
#define mqtt_server "mediatedspaces.net" //MQTT server name
#define mqtt_user "hcdeiot" // username
#define mqtt_password "esp8266" //still using login/pass authorization for MQTT server
#define topic_name "Kennel" // Unique topic name to subscribe to
#define PIN 14

// Some integer definition
int tempInt;
int motionInt;
int speakerPin = 0;
int numTones = 4;
int tones[] = {261, 220, 246, 196};


// Initialize LEDs
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(60, PIN, NEO_GRB + NEO_KHZ800);

// Initializes the LCD and wifi
WiFiClient espClient;
PubSubClient client(espClient);

//some arrays to hold our parsed data
const char* temp;
const char* motion;
const char* timing;

char incoming[100]; //an array to hold the incoming message

// Connects to wifi
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

//here is our callback
void callback(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, "Kennel") == 0) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < 101; i++) {
      incoming[i] = (char)payload[i];  //payload is an array of bytes (characters) that we put into incoming[]
    }
    Serial.println(incoming); //just to debug, is it there?

    // Sets up a JSON buffer so the incoming message can be parsed
    DynamicJsonBuffer  jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(incoming);

    if (!root.success()) {
      Serial.println("parseObject() failed");
      return;
    }

    // Variables for the sensor data received in a JSON format.
    temp = root["temp"].asString();
    motion = root["motion"].asString();
    tempInt = atoi(temp);
    motionInt = atoi(motion);

    if (tempInt >= 80.00) { // If the temperature is too hot, turn pixels red
      for (int i = 0; i < 60; i++) {
        pixels.setPixelColor(i, pixels.Color(150, 0, 0));
        pixels.show();
        delay(50);
      }
      if (tempInt >= 90.00) { // If the temp is > 90, buzzer plays
        for (int thisNote = 0; thisNote < numTones; thisNote++) {
          tone(speakerPin, tones[thisNote]);
          delay(500);
          noTone(speakerPin);
        }
      }
    } else if (motionInt == 1) { // If motion detected, pixels turn blue
      for (int i = 0; i < 60; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 0, 150));
        pixels.show();
        delay(50);
      }
    } else { // Everything is ok (white pixels)
      for (int i = 0; i < 60; i++) {
        pixels.setPixelColor(i, pixels.Color(10, 10, 10));
        pixels.show();
        delay(50);
      }
    }
  }
  else if (strcmp(topic, "timeKennel") == 0) { // Green LEDs flash every 30 min spent in kennel
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < 101; i++) {
      incoming[i] = (char)payload[i];  //payload is an array of bytes (characters) that we put into incoming[]
    }
    Serial.println(incoming); //just to debug, is it there?

    // Sets up a JSON buffer so the incoming message can be parsed
    DynamicJsonBuffer  jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(incoming);

    if (!root.success()) {
      Serial.println("parseObject() failed");
      return;
    }
      digitalWrite(12, HIGH); // LED on
      digitalWrite(13, HIGH);
      delay(5000);
      digitalWrite(12, LOW); // LED off
      digitalWrite(13, LOW);
  }
}

// Function to reconnect the ESP8266 if it becomes disconnected.
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("AdeleParsons", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(topic_name);
      client.subscribe("timeKennel");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
    }
  }
}

// sets up the serial monitor, wifi, and MQTT server
void setup() {

  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  Serial.begin(115200);

  pixels.begin(); // This initializes the NeoPixel library.
  for (int i = 0; i < 60; i++) { // Start with white pixels
    pixels.setPixelColor(i, pixels.Color(10, 10, 10));
    pixels.show();
    delay(50);
  }

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

// LOOP
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

