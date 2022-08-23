#include <PubSubClient.h>
#include <rpcWiFi.h>
#include <TFT_eSPI.h>
#include"LIS3DHTR.h"
#include "Seeed_Arduino_GroveAI.h"

//Required Information
#define WIFISSID "<YOUR-WIFISSD>" // Put your WifiSSID here
#define PASSWORD "<YOUR-WIFI-PASSWORD" // Put your wifi password here
#define TOKEN "<YOUR-UBIDOTS-TOKEN>" // Put your Ubidots' TOKEN
#define VARIABLE_LABEL1 "num" // Assign the variable label
#define VARIABLE_LABEL2 "confidence"
#define DEVICE_LABEL "wio-terminal" // Assign the device label
#define MQTT_CLIENT_NAME "r6y1ax7mq8" // MQTT client Name

const long interval = 100;
unsigned long previousMillis = 0;

char mqttBroker[] = "industrial.api.ubidots.com";

WiFiClient wifiClient;
PubSubClient client(wifiClient);

GroveAI ai(Wire);
uint8_t state = 0;
TFT_eSPI tft = TFT_eSPI();

static int num = 0;
static int conf = 0;

// Space to store values to send
char str_num[6];
char str_conf[6];
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

//Vision AI init
void VisionAI_Init()
{
  Serial.println("begin");
  if (ai.begin(ALGO_OBJECT_DETECTION, MODEL_EXT_INDEX_1)) // Object detection and pre-trained model 1
  {
    state = 1;
  }
  else
  {
    Serial.println("Algo begin failed.");
  }
}

//Read VisionAI values: number of characters recognized, confidence level for each person
void read_VisionAI()  
{
  if (state == 1)
  {
    uint32_t tick = millis();
    if (ai.invoke()) // begin invoke
    {
      while (1) // wait for invoking finished
      {
        CMD_STATE_T ret = ai.state(); 
        if (ret == CMD_STATE_IDLE)
        {
          break;
        }
        delay(20);
      }
      uint8_t len = ai.get_result_len(); // receive how many people detect
      if(len)
      {
         Serial.print("Number of people: ");

         num = len;
         Serial.println(num);

         object_detection_t data;       //get data
         for (int i = 0; i < len; i++)
         {
            ai.get_result(i, (uint8_t*)&data, sizeof(object_detection_t)); //get result
            Serial.print("confidence:");
            
            conf = data.confidence;
            Serial.println(conf);
          }
      }
     else
     {
       Serial.println("No identification");
       num = 0;
       conf = 0;
     }
   }
    else
    {
      Serial.println("Invoke Failed.");
      num = 0;
      conf = 0;
      delay(1000);
    }
  }
}

void send_data()
{
  dtostrf(num, 4, 0, str_num);
  dtostrf(conf, 4, 0, str_conf);
  
  if (!client.connected()) {
    reconnect();
  }
  
  // Builds the topic
  sprintf(topic, "%s", ""); // Cleans the topic content
  sprintf(topic, "%s%s", "/v2.0/devices/", DEVICE_LABEL);

  //Builds the payload
  sprintf(payload, "%s", ""); // Cleans the payload
  sprintf(payload, "{\"%s\":", VARIABLE_LABEL1); // Adds the variable label
  sprintf(payload, "%s%s", payload, str_num); // Adds the value
  sprintf(payload, "%s}", payload); // Closes the dictionary brackets
  client.publish(topic, payload);
  delay(500);

  sprintf(payload, "%s", ""); // Cleans the payload
  sprintf(payload, "{\"%s\":", VARIABLE_LABEL2); // Adds the variable label
  sprintf(payload, "%s%s", payload, str_conf); // Adds the value
  sprintf(payload, "%s}", payload); // Closes the dictionary brackets
  client.publish(topic, payload);
  delay(500);

  client.loop();
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  tft.begin();
  tft.setRotation(3);
  tft.setTextSize(2);
  tft.fillScreen(TFT_BLACK);
  VisionAI_Init();
  
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
  read_VisionAI(); //Reading visionai sensor values
  send_data();     //Sending data to Ubidots
  delay(5000);
}
