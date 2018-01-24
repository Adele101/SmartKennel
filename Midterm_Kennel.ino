/*
 * 
 * Controls temperature sensor and accelerometer. Sensor readings are sent to
 * an MQTT server.
 * 
 */

// Libraries to include
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_TSL2561_U.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Si7021.h>

#define mqtt_server "mediatedspaces.net" //MQTT server name
#define mqtt_name "hcdeiot"
#define mqtt_pass "esp8266"
#define CLICKTHRESHHOLD 80 // Accelerometer setting

const char *ssid = ""; // WiFi name
const char *password = ""; // WiFi password

// Initialize Adafruit sensors
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
Adafruit_Si7021 sensor = Adafruit_Si7021();
Adafruit_LIS3DH lis = Adafruit_LIS3DH();

// Set up WiFi
WiFiClient espClient;
PubSubClient mqtt(espClient);

// Integer definitions
const int motionPin = 15;
int motionState = 1;
unsigned long previousMillis;
char espUUID[8] = "Kennel";
int isMoving = 0;

//function to configure the sensor prior to use
void configureSensor()
{
  tsl.enableAutoRange(true); //Auto-gain ... switches automatically between 1x and 16x
  //Changing the integration time gives you better sensor resolution (402ms = 16-bit data)
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS); //16-bit data but slowest conversions
}


//function to do something when a message arrives from mqtt server
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

//function to reconnect if we become disconnected from the server
void reconnect() {
  // Loop until we're reconnected
  while (!espClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    // Attempt to connect
    if (mqtt.connect(espUUID, mqtt_name, mqtt_pass)) { //the connction
      Serial.println("connected");
      // Once connected, publish an announcement...
      char announce[40];
      strcat(announce, espUUID);
      strcat(announce, "is connecting. <<<<<<<<<<<");
      mqtt.publish(espUUID, announce);
      // ... and resubscribe
      //      client.subscribe("");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() {
  Serial.begin(115200);

  // initialize SPI:
  SPI.begin();
  Serial.println("Adafruit LIS3DH Tap Test!");

  if (! lis.begin(0x18)) {   // change this to 0x19 for alternative i2c address
    Serial.println("Couldnt start");
    while (1);
  }
  Serial.println("LIS3DH found!");

  lis.setRange(LIS3DH_RANGE_2_G);   // 2, 4, 8 or 16 G!

  Serial.print("Range = "); Serial.print(2 << lis.getRange());
  Serial.println("G");

  lis.setClick(2, CLICKTHRESHHOLD); // Accelerometer configuration

  // WiFi setup
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected");
  Serial.println("");
  Serial.print("ip: ");
  Serial.println(WiFi.localIP());

  /* Initialize the sensor */
  if (!tsl.begin())
  {
    /* There was a problem detecting the TSL2561 ... check your connections */
    Serial.print("Ooops, no TSL2561 detected ... Check your wiring or I2C ADDR!");
    while (1);
  }

  configureSensor();

  //Setup MQTT server
  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(callback);

}

void loop() {
  unsigned long currentMillis = millis();

  mqtt.loop();
  if (!mqtt.connected()) {
    reconnect();
  }

  if (currentMillis - previousMillis > 5000) { //a periodic report, every 5 seconds
    // Gather sensor data
    sensors_event_t event;
    lis.getEvent(&event);
    float temp = sensor.readTemperature() * 1.8 + 32;
    isMoving = isClicked();

    // Sets up char variables for sensor readings
    char str_temp[6];
    char str_moving[6];
    char message[50];

    // Sends a message to the MQTT server.
    dtostrf(isMoving, 1, 0, str_moving);
    dtostrf(temp, 4, 2, str_temp);
    sprintf(message, "{\"uuid\": \"%s\", \"temp\": \"%s\", \"motion\": \"%s\"}", espUUID, str_temp, str_moving);
    mqtt.publish(espUUID, message);
    Serial.println("publishing");
    previousMillis = currentMillis;
    isMoving = 0;
  }
}

// Function to determine the accelerometer's state
bool isClicked() {
  uint8_t click = lis.getClick();
  if (click == 0) return 0;
  if (! (click & 0x30)) return 1;
  Serial.print("Click detected (0x"); Serial.print(click, HEX); Serial.print("): ");
  if (click & 0x10 || click & 0x20) {
    Serial.print(" single click");
    return 1;
  }
}

