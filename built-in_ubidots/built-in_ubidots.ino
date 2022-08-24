#include <PubSubClient.h>
#include <rpcWiFi.h>
#include <TFT_eSPI.h>
#include"LIS3DHTR.h"

LIS3DHTR<TwoWire> lis;

//Required Information
#define WIFISSID "<YOUR-WIFISSD>" // Put your WifiSSID here
#define PASSWORD "<YOUR-WIFI-PASSWORD" // Put your wifi password here
#define TOKEN "<YOUR-UBIDOTS-TOKEN>" // Put your Ubidots' TOKEN
#define VARIABLE_LABEL1 "light" // Assign the variable label
#define VARIABLE_LABEL2 "IMUx"
#define VARIABLE_LABEL3 "IMUy"
#define VARIABLE_LABEL4 "IMUz"
#define VARIABLE_LABEL5 "sound"
#define DEVICE_LABEL "wio-terminal" // Assign the device label
#define MQTT_CLIENT_NAME "r6y1ax7mq8" // MQTT client Name

const long interval = 100;
unsigned long previousMillis = 0;

TFT_eSPI tft;

char mqttBroker[] = "industrial.api.ubidots.com";

WiFiClient wifiClient;
PubSubClient client(wifiClient);

//sensor values
static int lightValue = 0;
static float imuxValue = 0;
static float imuyValue = 0;
static float imuzValue = 0;
static int soundValue = 0;

// Space to store values to send
static char str_light[6];
static char str_imux[6];
static char str_imuy[6];
static char str_imuz[6];
static char str_sound[6];
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

//Reading built-in sensor values
void read_builtin()
{
  lightValue = analogRead(WIO_LIGHT);
  Serial.print("Light = ");
  Serial.println(lightValue);
  
  imuxValue = lis.getAccelerationX();
  Serial.print("IMU_x = ");
  Serial.println(imuxValue);
  imuyValue = lis.getAccelerationY();
  Serial.print("IMU_y = ");
  Serial.println(imuyValue);
  imuzValue = lis.getAccelerationZ();
  Serial.print("IMU_z = ");
  Serial.println(imuzValue);
  
  soundValue = analogRead(WIO_MIC);
  Serial.print("Sound = ");
  Serial.println(soundValue);
}

//Sending data to Ubidots
void send_data()
{
  dtostrf(lightValue, 4, 0, str_light);
  dtostrf(imuxValue, 4, 3, str_imux);
  dtostrf(imuyValue, 4, 3, str_imuy);
  dtostrf(imuzValue, 4, 3, str_imuz);
  dtostrf(soundValue, 4, 0, str_sound);

  if (!client.connected()) {
    reconnect();
  }
  
  // Builds the topic
  sprintf(topic, "%s", ""); // Cleans the topic content
  sprintf(topic, "%s%s", "/v2.0/devices/", DEVICE_LABEL);

  //Builds the payload
  sprintf(payload, "%s", ""); // Cleans the payload
  sprintf(payload, "{\"%s\":", VARIABLE_LABEL1); // Adds the variable label
  sprintf(payload, "%s%s", payload, str_light); // Adds the value
  sprintf(payload, "%s}", payload); // Closes the dictionary brackets
  client.publish(topic, payload);
  delay(500);

  sprintf(payload, "%s", ""); // Cleans the payload
  sprintf(payload, "{\"%s\":", VARIABLE_LABEL2); // Adds the variable label
  sprintf(payload, "%s%s", payload, str_imux); // Adds the value
  sprintf(payload, "%s}", payload); // Closes the dictionary brackets
  client.publish(topic, payload);
  delay(500);

  sprintf(payload, "%s", ""); // Cleans the payload
  sprintf(payload, "{\"%s\":", VARIABLE_LABEL3); // Adds the variable label
  sprintf(payload, "%s%s", payload, str_imuy); // Adds the value
  sprintf(payload, "%s}", payload); // Closes the dictionary brackets
  client.publish(topic, payload);
  delay(500);

  sprintf(payload, "%s", ""); // Cleans the payload
  sprintf(payload, "{\"%s\":", VARIABLE_LABEL4); // Adds the variable label
  sprintf(payload, "%s%s", payload, str_imuz); // Adds the value
  sprintf(payload, "%s}", payload); // Closes the dictionary brackets
  client.publish(topic, payload);
  delay(500);

  sprintf(payload, "%s", ""); // Cleans the payload
  sprintf(payload, "{\"%s\":", VARIABLE_LABEL5); // Adds the variable label
  sprintf(payload, "%s%s", payload, str_sound); // Adds the value
  sprintf(payload, "%s}", payload); // Closes the dictionary brackets
  client.publish(topic, payload);
  delay(500);

  client.loop();
}

void setup() {
  Serial.begin(115200);
  lis.begin(Wire1);
  pinMode(WIO_MIC, INPUT);
  pinMode(WIO_LIGHT, INPUT);

  tft.begin();
  tft.setRotation(3);
  tft.setTextSize(2);
  tft.fillScreen(TFT_BLACK);

  lis.setOutputDataRate(LIS3DHTR_DATARATE_25HZ); //Data output rate
  lis.setFullScaleRange(LIS3DHTR_RANGE_2G);
  
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
  read_builtin();   //Reading buile-in sensor values
  send_data();   //Sending data to Ubidots
  delay(5000);
}
