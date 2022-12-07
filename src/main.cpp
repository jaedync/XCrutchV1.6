#include <WiFi.h>
#include <HTTPClient.h>
//#include <Wire.h>
#include "HX711.h"
#include <TFT_eSPI.h>
#include <SPI.h>
#include <Pangodream_18650_CL.h>

const char* ssid     = "letnet-connect";
const char* password = "letnet-connect";
const char* serverName = "http://epm10.jaedynchilton.com/post-esp-data.php";
String apiKeyValue = "tPmAT5Ab3j7F9";
String sensorName = "HX711";
String sensorLocation = "TTGO_S3";

HX711 scale;
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);
TFT_eSprite chart = TFT_eSprite(&tft);
uint8_t dataPin = 18;
uint8_t clockPin = 12;
uint32_t start, stop;
volatile float f;
int reset = 0;
int lastState = 0;

float lb = 0;
unsigned long waitTime = 2147483646;
unsigned long displayStart = 2147483646;
String batterydot = ".";

float maxlb = 0;
float sumlb = 0;
float readingcount = 0;
float startTime = 0;
float circle = 0;
float activesum = 0;
unsigned long activereadingcount = 0;
unsigned long activeduration = 0;
float lastActive = 0;
int lastButtonState;    // the previous state of button14
int currentButtonState; // the current state of button14
int lcdButton = HIGH;     // the current state of LCD
int sentData = 0;
String WebData[14];

int espMode = 0;

unsigned long transmitDelay = 45; // delay before sending data and showing chart in seconds
unsigned long displayDelay = 12; // delay before closing chart in seconds
unsigned long wifiDelayOffset = 10; // seconds before transmit attempt that the device will connect to wifi

unsigned long previousMillis = 0;
unsigned long interval = 30000;

//#define gray 0x6B6D
#define gray 0x2A0F // blue-ish gray!
#define lcd_power 38
//#define bat_volt 04 pangodream lib handles battery
const int BUTTON_PIN = 14; // Arduino pin connected to right side button's pin

#define ADC_PIN 04
//#define CONV_FACTOR 1.68
#define CONV_FACTOR 1.73 // new esp, quirky adc
#define READS 20
Pangodream_18650_CL BL(ADC_PIN, CONV_FACTOR, READS);

int gw=204;
int gh=102;
int gx=6;
int gy=182;
int values[27]={0};
int values2[27]={0};
float LOG105 = 0.021189;
int batteryAvg[20]={0};
int batteryAvg2[20]={0};
int batteryPercent = 0;
int batteryPinVal = 0;

void setup() {
  setCpuFrequencyMhz(160);
  Serial.begin(115200);
  pinMode(15,OUTPUT);
  digitalWrite(15,1);
  tft.init();
  pinMode(lcd_power, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP); // set arduino pin to input pull-up mode
  //tft.setRotation(1);
  sprite.createSprite(170, 320);
  chart.createSprite(320, 170);
  chart.setRotation(1);
  scale.begin(dataPin, clockPin);
  scale.set_scale(24);
  scale.tare();
  tft.fillScreen(TFT_BLACK);
  //tft.setRotation(2);
  sprite.fillScreen(TFT_BLACK);
  chart.fillScreen(TFT_BLACK);
  sprite.fillRect(0, 0, 170, 320, TFT_WHITE);
  //ledcSetup( 4, 12000, 8 ); // ledc: 4  => Group: 0, Channel: 2, Timer: 1, led frequency, resolution  bits 
  //ledcAttachPin( lcd_power, 4 );   // gpio number and channel
  //ledcWrite( 4, 0 ); // write to channel number 4}
  pinMode(lcd_power, OUTPUT);          // set arduino pin to output mode
  sprite.pushSprite(0,0);
  
  //WiFi.begin(ssid, password);
  WiFi.begin(ssid, password);
  //Serial.println("Connecting");
  //while(WiFi.status() != WL_CONNECTED) { 
  //  delay(500);
    //Serial.print(".");
  //}
  //Serial.println("");
  //Serial.print("Connected to WiFi network with IP Address: ");
  //Serial.println(WiFi.localIP());
  
  tft.setPivot(85, 160); // Set the TFT pivot point that the needle will rotate around
  values[26]=gh/2;
  WiFi.disconnect();

}


String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void loop() {
  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  unsigned long currentMillis = millis();
  // if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
  //   WiFi.disconnect();
  //   WiFi.reconnect();
  //   previousMillis = currentMillis;
  // }
  
  if(waitTime + (transmitDelay-wifiDelayOffset)*1000 < millis()){
    if(WiFi.status() != WL_CONNECTED) WiFi.reconnect();
  } else if(espMode == 0 && WiFi.status() == WL_CONNECTED) WiFi.disconnect();


  f = scale.get_units();
  if (f < 0) f = 0.00;
  if (f > 100000) f = 10000.00;
  lb = f*.002205;

  batteryPinVal = BL.pinRead();
  batteryPercent = BL.getBatteryChargeLevel();

  switch (espMode) {

    case 0: //diagnostic mode
      
      sprite.fillRect(0, 0, 170, 185, TFT_BLACK);
      sprite.fillRect(0, 0, 4, 320, TFT_BLACK);
      sprite.fillRect(166, 0, 4, 320, TFT_BLACK);
      sprite.fillRect(0, 316, 170, 4, TFT_BLACK);
      sprite.fillRoundRect(0, -20, 170, 85, 16, gray);

      sprite.fillRect(4, 270, 162, 47, TFT_BLACK);
      sprite.fillRoundRect(21, 270, 130, 90, 16, gray);
      sprite.setTextDatum(1);
      sprite.setTextColor(TFT_WHITE, gray);
      sprite.drawString("X-Crutch", 85+3, 276, 4);
      sprite.drawString("EPM10", 85, 306, 1);

      sprite.setTextDatum(2);
      sprite.setTextColor(TFT_WHITE, gray);
      sprite.drawString(String(lb, 1), 165, 8, 7);
      
      for(int i=0;i<27;i++) values2[i]=values[i];
      for(int i=26;i>0;i--) values[i-1]=values2[i];
      if(lb > 1) {
        values[26]=log10(lb)/LOG105;
      } else {
        values[26] = 0;
      }
      
      // for(int i=0;i<20;i++) batteryAvg2[i]=batteryAvg[i];
      // for(int i=19;i>0;i--) batteryAvg[i-1]=batteryAvg2[i];
      // batteryAvg[19] = analogRead(04);
      // batteryPercent = 0;
      // for(int i=0;i<20;i++) batteryPercent = batteryPercent + batteryAvg[i];
      // batteryPercent = batteryPercent / 20;

      for(int i = 0; i < 13; i++) {
        int height = gy-(log10(i*4)/.0155);
        sprite.drawLine(gx, height, gx+156, height, gray);
      }

      sprite.drawLine(gx, gy-111, gx+156, gy-111, TFT_BLUE);

      for(int i=0;i<27;i++) sprite.drawLine(gx+(i*6),gy,gx+(i*6),gy-110,gray);

      //sprite.drawString(String(values[26], 8), 150, 200, 4);

      for(int i=0;i<26;i++){
        sprite.drawLine(gx+(i*6),gy-values[i],gx+((i+1)*6),gy-values[i+1],TFT_SKYBLUE);
      }
      
      sprite.drawLine(gx, gy, gx+156, gy, TFT_WHITE);
      sprite.drawLine(gx, gy-111, gx+156, gy-111, TFT_WHITE);
      sprite.drawLine(gx, gy, gx, gy-111, TFT_WHITE);
      sprite.drawLine(gx+156, gy, gx+156, gy-111, TFT_WHITE);

      if(startTime > 0 && lastState == 0) {
        sprite.drawLine(84, 68, map((millis() - waitTime), 0, transmitDelay*1000, 84, 0), 68, TFT_SKYBLUE);
        sprite.drawLine(85, 68, map((millis() - waitTime), 0, transmitDelay*1000, 85, 169), 68, TFT_SKYBLUE);
      }
          
      if ((WiFi.status() == WL_CONNECTED)) {
        sprite.fillRect(4, 184, 162, 86, TFT_BLACK);
        sprite.setTextDatum(0);
        sprite.setTextColor(gray, TFT_BLACK);
        sprite.drawString(String(ssid), 8, 191, 1);
        sprite.drawString(WiFi.localIP().toString(), 8, 203, 1);
        sprite.drawString("Connected", 8, 215, 1);
      }
      else {
        sprite.fillRect(4, 184, 162, 86, TFT_BLACK);
        sprite.setTextDatum(0);
        sprite.setTextColor(gray, TFT_BLACK);
        sprite.drawString(String(ssid), 8, 191, 1);
        sprite.drawString("DISCONNECTED", 8, 203, 1);
        // sprite.drawString("Reconnect in: "+String((interval - (currentMillis - previousMillis))/1000.0), 8, 215, 1);
      }

      if(batteryPinVal > 2700){
        sprite.setTextColor(TFT_GREEN, TFT_BLACK);
        sprite.drawString("Charging " + batterydot, 8, 239, 1);

        if(batterydot == ".") batterydot = "..";
        else if(batterydot == "..") batterydot = "...";
        else if(batterydot == "...") batterydot = "";
        else if(batterydot == "") batterydot = ".";
      }
      else if(batteryPercent < 15) {
        sprite.setTextColor(TFT_RED, TFT_BLACK);
        sprite.drawString(String(batteryPercent) + "%", 8, 239, 1);
      } else {
        sprite.setTextColor(gray, TFT_BLACK);
        sprite.drawString(String(batteryPercent) + "%", 8, 239, 1);
      }

      if(startTime != 0) {
        sprite.setTextDatum(2);
        sprite.setTextColor(gray, TFT_BLACK);
        sprite.drawString("Measuring", 162, 191, 1);
        sprite.drawString(String(activesum/activereadingcount) + "lbs", 162, 203, 1);
        sprite.drawString(String(activeduration/1000.0, 2) + "s", 162, 215, 1);
        //sprite.drawString(String(activereadingcount) + " readings", 162, 215, 1);
      }

      sprite.pushSprite(0,0);

      break;


    case 5: // Display pause
      digitalWrite(lcd_power, LOW); 
    break;

    case 1: // CURRENT WEEK CHART
      // already drawn chart, need to not overwrite
      // only needs countdown timer and visual to show it
      if(displayStart + displayDelay*1000 < millis()) {
        espMode = 0;
      } else {

        chart.drawLine(1, 84, 1, map((millis() - displayStart), 0, displayDelay*1000, 84, 0), TFT_SKYBLUE);
        chart.drawLine(1, 85, 1, map((millis() - displayStart), 0, displayDelay*1000, 85, 169), TFT_SKYBLUE);

        chart.pushRotated(90);
        //sprite.pushSprite(0,0);

      }
    break;
    
    case 9: // get week chart failed, display error message for 30% of the normal duration
      if(displayStart + displayDelay*300 < millis()) {
        espMode = 0;
      } else {
        sprite.drawLine(84, 68, map((millis() - displayStart), 0, displayDelay*300, 84, 0), 68, TFT_SKYBLUE);
        sprite.drawLine(85, 68, map((millis() - displayStart), 0, displayDelay*300, 85, 169), 68, TFT_SKYBLUE);
        sprite.setTextDatum(1);
        sprite.setTextColor(TFT_WHITE, gray);
        sprite.drawString("SERVER DID NOT RESPOND", 10, 100, 4);

        
        sprite.pushSprite(0,0);
      }
      
    break;
  
  }



  if(lb > 10) {
    waitTime = millis();
    if(startTime == 0) startTime = millis();
    if(maxlb < lb) maxlb = lb;
    //sprite.drawLine(0, 100, 170, 100, TFT_BLACK);
    activesum = activesum + lb;
    activereadingcount = activereadingcount + 1;
    if(lastState = 1) activeduration = activeduration + (millis() - lastActive);
    //Serial.println(activeduration);
    lastActive = millis();
    lastState = 1;
  }
  else {
    lastState = 0;
    lastActive = millis();
  }

  if(startTime > 0 && lastState == 0) {
    sumlb = sumlb + lb;
    readingcount = readingcount + 1;
  }


  lastButtonState    = currentButtonState;      // save the last state
  currentButtonState = digitalRead(BUTTON_PIN); // read new state
  if(lastButtonState == HIGH && currentButtonState == LOW) {
    //Serial.println("The button is pressed");
    
    // toggle state of LED
    lcdButton = !lcdButton;

    // control LED arccoding to the toggled state
    //digitalWrite(lcd_power, lcdButton); 
  }
  // if(startTime == 0) {
  //   ledcWrite( 4, 64 ); // write to channel number 4}
  // } else {
  //   ledcWrite( 4, 255 ); // write to channel number 4}
  // }



  //Serial.println(waitTime);

  //Check WiFi connection status
  if(waitTime + transmitDelay*1000 < millis()){
    if(WiFi.status()== WL_CONNECTED) {
      float readDuration = (millis() - startTime)/1000;
      float avglb = sumlb/readingcount;
      float activeaverage = activesum/activereadingcount;
      HTTPClient http;
      http.begin(serverName); // Your Domain name with URL path or IP address with path
      http.addHeader("Content-Type", "application/x-www-form-urlencoded"); // Specify content-type header
      String httpRequestData = "api_key=" + apiKeyValue + "&sensor=" + sensorName + "&location=" + sensorLocation + "&value1=" + String(avglb*readDuration) + "&value2=" + String(activeaverage) + "&value3=" + String(activeduration) + ""; // Prepare your HTTP POST request data
      int httpResponseCode = http.POST(httpRequestData);
      if (httpResponseCode == 200) {
        sprite.fillRect(0, 100, 170, 220, TFT_GREEN);
        startTime = 0;      //reset counters
        waitTime = 2147483646;
        maxlb = 0;
        sumlb = 0;
        readingcount = 0;
        activesum = 0;
        activereadingcount = 0;
        activeduration = 0;
        lastActive = 2147483646;
        lastState = 0;
        sentData = 1;
      }
      else {
        //sprite.fillRect(0, 140, 320, 170, TFT_RED);
        sprite.fillRect(0, 100, 170, 220, TFT_RED);
      }
      // Free resources
      http.end();
    }
    else{
      sprite.fillRect(0, 100, 170, 10, TFT_SKYBLUE);
      waitTime = millis();
    }
  }
  else {
    //Serial.println("WiFi Disconnected");
    //sprite.fillRect(0, 0, 320, 170, TFT_BLUE);
  }

  if(WiFi.status()== WL_CONNECTED && sentData == 1){
    HTTPClient http;
    // Your Domain name with URL path or IP address with path
    http.begin(serverName);
    // Send HTTP GET request
    int httpResponseCode = http.GET();
    
    if (httpResponseCode == 200) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      chart.fillRect(0, 0, 360, 200, TFT_BLACK);
      Serial.println(payload);
      int chartx = 0;
      sprite.setTextColor(TFT_WHITE, TFT_BLACK);
      int testcount = 0;
      int payloadlength = 0;
      String chartDate[7] = {"--", "--", "--", "--", "--", "--", "--"};
      float chartValue[7] = {0.00, 0.00, 0.00, 0.00, 0.00, 0.00, 0.00};
      float maxchartvalue = 0.00;

      while(payloadlength < payload.length()) { //deconstruct data from server into useful arrays
        String nowChartDate = getValue(payload, ' ', testcount).substring(5);
        int dayOfWeek = getValue(payload, ' ', (testcount+1)).toInt();
        float nowChartValue = getValue(payload, ' ', (testcount+2)).toInt();
        chartDate[dayOfWeek-1] = nowChartDate;
        chartValue[dayOfWeek-1] = nowChartValue;
        if(nowChartValue > maxchartvalue) maxchartvalue = nowChartValue;
        testcount = testcount + 3;
        payloadlength = payloadlength + 19;
      }

      int max = (maxchartvalue+5.00)/10.00;
      for(float i = 0; i <= max; i++) {
        int y = map(i, 0, max, 150, 20);
        chart.drawLine(0, y, 320, y, gray);
      }

      sprite.setTextDatum(1);
      for (int i = 0; i < 7; i++) {
        int x = 40+i*40;
        chart.drawString(chartDate[i], x, 160);
        chart.fillRect(x, map(long(chartValue[i]), 0, maxchartvalue+5.00, 150, 20), 20, map(long(chartValue[i]), 0, maxchartvalue+5.00, 0, 130), TFT_SKYBLUE);
        chart.drawString(String(chartValue[i]), x, 3);
        chart.drawString(chartDate[i], x, 160);
        chart.drawLine(x-10, 170, x-10, 0, gray);
        chart.drawLine(x+30, 170, x+30, 0, gray);
      }

      

      chart.drawLine(0, 150, 320, 150, TFT_WHITE);
      chart.drawLine(0, 20, 320, 20, TFT_WHITE);

      espMode = 1;
    }
    else {
      espMode = 9;
    }
    http.end(); // Free resources
    
    sentData = 0;
    displayStart = millis();
    //WiFi.disconnect();
  }





  //sprite.drawString(String(batteryPercent), 168, 3, 1);
  //sprite.drawString(String(BL.getBatteryVolts()) + "V", 168, 13, 1);

  // if(lcdButton != HIGH && espMode == 0) {
  //   digitalWrite(lcd_power, LOW);
  // } else {
  //   digitalWrite(lcd_power, HIGH);
  // }

  //sprite.pushSprite(0,0);
}
