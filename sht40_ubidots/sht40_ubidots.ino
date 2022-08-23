#include <PubSubClient.h>
#include <rpcWiFi.h>
#include <TFT_eSPI.h>
#include <SensirionI2CSht4x.h>
#include <Wire.h>

#define WIFISSID "<YOUR-WIFISSD>" // Put your WifiSSID here
#define PASSWORD "<YOUR-WIFI-PASSWORD" // Put your wifi password here
#define TOKEN "<YOUR-UBIDOTS-TOKEN>" // Put your Ubidots' TOKEN
#define VARIABLE_LABEL1 "temperature" // Assign the variable label
#define VARIABLE_LABEL2 "humidity"
#define DEVICE_LABEL "wio-terminal" // Assign the device label
#define MQTT_CLIENT_NAME "r6y1ax7mq8" // MQTT client Name

const long interval = 100;
unsigned long previousMillis = 0;

char mqttBroker[] = "industrial.api.ubidots.com";

WiFiClient wifiClient;
PubSubClient client(wifiClient);

TFT_eSPI tft = TFT_eSPI();
SensirionI2CSht4x sht4x;

static float temp = 0;
static float humi = 0;

// Space to store values to send
char str_temp[6];
char str_humi[6];
char payload[700];
char topic[150];

void callback(char* topic, byte* payload, unsigned int length){
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");

  // Attempt to connect
  if (client.connect(MQTT_CLIENT_NAME, TOKEN,"")) {
    Serial.println("connected");
  }
  else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 2 seconds");
    // Wait 2 seconds before retrying
    delay(2000);
    }
  }
}

void read_sht40()
{
  uint16_t error;
  char errorMessage[256];
  error = sht4x.measureHighPrecision(temp, humi);
  if (error) {
      Serial.print("Error trying to execute measureHighPrecision(): ");
      errorToString(error, errorMessage, 256);
      Serial.println(errorMessage);
  } else {
      Serial.print("Temperature:");
      Serial.print(temp);
      Serial.print("\t");
      Serial.print("Humidity:");
      Serial.println(humi);
  }
}

void send_data()
{
  dtostrf(temp, 4, 2, str_temp);
  dtostrf(humi, 4, 2, str_humi);
  
  if (!client.connected()) {
    reconnect();
  }
  
  // Builds the topic
  sprintf(topic, "%s", ""); // Cleans the topic content
  sprintf(topic, "%s%s", "/v2.0/devices/", DEVICE_LABEL);

  //Builds the payload
  sprintf(payload, "%s", ""); // Cleans the payload
  sprintf(payload, "{\"%s\":", VARIABLE_LABEL1); // Adds the variable label
  sprintf(payload, "%s%s", payload, str_temp); // Adds the value
  sprintf(payload, "%s}", payload); // Closes the dictionary brackets
  client.publish(topic, payload);
  delay(500);

  sprintf(payload, "%s", ""); // Cleans the payload
  sprintf(payload, "{\"%s\":", VARIABLE_LABEL2); // Adds the variable label
  sprintf(payload, "%s%s", payload, str_humi); // Adds the value
  sprintf(payload, "%s}", payload); // Closes the dictionary brackets
  client.publish(topic, payload);
  delay(500);

  client.loop();
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  sht4x.begin(Wire);

  tft.begin();
  tft.setRotation(3);
  tft.setTextSize(2);
  tft.fillScreen(TFT_BLACK);
  
//  while(!Serial);

  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  tft.drawString("Connecting to WiFi...",20,120);
  WiFi.begin(WIFISSID, PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    WiFi.begin(WIFISSID, PASSWORD);
  }
  
  tft.fillScreen(TFT_BLACK);
  tft.drawString("Connected to the WiFi",20,120);

  delay(1000);
  client.setServer(mqttBroker, 1883);
  client.setCallback(callback);
}

void loop() {
  read_sht40();    //Reading sht40 sensor values
  send_data();     //Sending data to Ubidots
  delay(5000);
}
