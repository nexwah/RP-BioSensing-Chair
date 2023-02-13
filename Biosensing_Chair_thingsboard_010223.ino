#include <WiFi.h>
#include <Wire.h>
#include "DFRobot_Heartrate.h"
#include "ThingsBoard.h"
#include "MAX30100_PulseOximeter.h"

//MAX30100
#define REPORTING_PERIOD_MS     1000
PulseOximeter pox;
uint32_t tsLastReport = 0;

//GSR
const int GSR = 33;
int sensorValue = 0;
int gsr_average = 0;

//SEN0213
const int heartPin = 32;
int heartValue = 0;

//WiFi
#define WIFI_AP             "RP_Chair"
#define WIFI_PASSWORD       "rp@chair"

//ThingsBoard
#define TOKEN               "S02YJCI3uNGfADeC9GyK"
#define THINGSBOARD_SERVER  "demo.thingsboard.io"     

// Initialize ThingsBoard client
WiFiClient espClient;
// Initialize ThingsBoard instance
ThingsBoard tb(espClient);
// the Wifi radio's status
int status = WL_IDLE_STATUS;

int i;
int buf[250];
int buff[250];
int heartrate;
int SPO2;
int heartbeatFlag = 0;

//Declare Function
void InitWiFi();
void reconnect();
void SEN0213_readData();
void GSR_readData();

void onBeatDetected()
{
  heartbeatFlag =1;
  Serial.println("♥ Beat!");
  Serial.print("Heart rate:");
  heartrate = pox.getHeartRate();
  Serial.print(heartrate);
  Serial.print("bpm / SpO2:");
  SPO2 = pox.getSpO2();
  Serial.print(SPO2);
  Serial.println("%");
}

void MAX30100_Readdata(){
  if (!pox.begin()) {
        Serial.println("FAILED");
        for(;;);
    } else {
        Serial.println("SUCCESS Next");
  
    for(int i=0; i<100000; i++){
      pox.update();
      if (heartbeatFlag == 1){
        tb.sendTelemetryInt("Heart", heartrate);
        tb.sendTelemetryInt("SPO2", SPO2);
        Serial.println(SPO2);
        heartbeatFlag = 0;
        //tb.sendTelemetryInt("Heart", 0);
        //tb.sendTelemetryInt("SPO2", 0);
      } 
    }
    
    Serial.println("finish MAX30100");
   }
}

void setup() 
{
  /***** Initialize serial for debugging *****/
  Serial.begin(115200);

  /***** WiFi Setup *****/
  WiFi.begin(WIFI_AP, WIFI_PASSWORD);
  InitWiFi();
  Serial.print("Got IP: ");  
  Serial.println(WiFi.localIP());

  if (!pox.begin()) {
        Serial.println("FAILED");
        for(;;);
    } else {
        Serial.println("SUCCESS");
    }
  pox.setOnBeatDetectedCallback(onBeatDetected);
}

void loop() 
{
  delay(20);

  /***** WiFi Reconnect *****/
  if (WiFi.status() != WL_CONNECTED) {
    reconnect();
  }

  /***** ThingsBoard Connect *****/
  if (!tb.connected()) {
    // Connect to the ThingsBoard
    Serial.print("Connecting to: ");
    Serial.print(THINGSBOARD_SERVER);
    Serial.print(" with token ");
    Serial.println(TOKEN);
    if (!tb.connect(THINGSBOARD_SERVER, TOKEN)) {
      Serial.println("Failed to connect");
      return;
    }
  }

  /***** Sensor Loop *****/
  //GSR_readData();
  MAX30100_Readdata();
  SEN0213_readData();
  
  /***** ThingsBoard Send Data *****/
  
  for(i = 0; i < 250; i++) {
    tb.sendTelemetryInt("ECG",(buf[i]));
    tb.sendTelemetryInt("GSR", (buff[i]));
    //tb.sendTelemetryInt("Heart", pox.getHeartRate());
    //tb.sendTelemetryInt("SPO2", pox.getSpO2());
    //Serial.println(buf[i]);
    delay(500);
  }
  
  //delay(100);
  //tb.sendTelemetryInt("ECG", (buf[i]));
  //tb.sendTelemetryInt("GSR", gsr_average);

  //Serial.println("Sending data...");
  
  tb.loop();
  delay(100);
}

void InitWiFi()
{
  Serial.println("Connecting to WiFi ...");
  // attempt to connect to WiFi network

  WiFi.begin(WIFI_AP, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
}

void reconnect() 
{
  // Loop until we're reconnected
  status = WiFi.status();
  if ( status != WL_CONNECTED) {
    WiFi.begin(WIFI_AP, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("Connected to WiFi");
  }
}

void SEN0213_readData()
{
  Serial.println("ECG start");
  //Serial.print("ECG = ");
  //Serial.println(heartValue);
  //delay(500);
  for(i = 0; i < 250; i++) {
    heartValue = analogRead(heartPin);
    sensorValue=analogRead(GSR);
    buf[i] = heartValue;
    buff[i] = sensorValue;
    //Serial.println(buf[i]);
    delay(40);
    //tb.sendTelemetryInt("ECG",(buf[i]));
  }
  delay(100);

}

void GSR_readData()
{
  long sum=0;
  for(int i=0;i<10;i++)     //Average the 10 measurements to remove the glitch
  {
    sensorValue=analogRead(GSR);
    sum += sensorValue;
    delay(5);
  }
  Serial.println("GSR start");
  
  gsr_average = sum/10;
  Serial.print("GSR");
  Serial.println(gsr_average);
  delay(20);
}
