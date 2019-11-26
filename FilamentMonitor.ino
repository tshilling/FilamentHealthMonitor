//############################################################

#import "index.h"
#include <ArduinoJson.h>
#include "EEPROM.h" 

//############################################################
//################### D1 Mini WiFi Board Settings ##################

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#ifndef STASSID
    #define STASSID "Parrot"        // Network Name
    #define STAPSK  "306101105"     // Network Passcode
#endif
const char* ssid = STASSID;
const char* password = STAPSK;

#define name "RFXFilamentMonitor"   // Name of server
ESP8266WebServer server(80);

//############################################################
//###################### Display Settings ####################

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128        // OLED display width, in pixels
#define SCREEN_HEIGHT 32        // OLED display height, in pixels
#define CHAR_WIDTH 6            // How many pixels wide is a size 1 charater
#define CHAR_HEIGHT 8           // How many pixels wide is a size 2 character

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
bool displayPresent = false;    // A Boolean for storing if the front LCD is present

//############################################################
//################### Color Sensor Settings ##################

#include "Adafruit_TCS34725.h"
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);
bool colorPresent = false;

float rScale = 1;
float gScale = 1;
float bScale = 1;
float cScale = 1;


//############################################################
//################### Temp DHT-11 Sensor Settings ##################

#include "DHT.h"
#define DHTPIN D5     // Digital pin connected to the DHT sensor
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);

//############################################################
//################### HX711 Weight Sensor Settings ##################
#include "HX711.h"
// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = D7;
const int LOADCELL_SCK_PIN = D6;
HX711 scale;

//############################################################
//NOTE: These are not user defined, they hold the values that the monitor sets
byte flag = 0;                      // Used to determine if its the first time run
String IP = "000.000.000.000";      // Holds the current IP (Set in Setup)

int Humidity = 99;                  // Humidity updated in Loop
float rawTemp, rawHumidity;         

// Weight = (rawWeight + rawOffset) * scaleScale + offset

int Weight = 1000;                  // Weight updated in Loop, scaled and offset
long rawWeight = 0;                 // Raw Value from the Scale
long rawOffset = 0;                 // Raw Offset from scale value
int offset = 0;                     // Scaled Offset, used to store empty weight of spool
float scaleScale = 1;               // Scaling value

// The following are for color detection
uint16_t rawRed, rawGreen, rawBlue, rawClear;   // The unscaled / unoffset color value from sensor
uint16_t rbR, rbG,rbB;  // Offset of raw value to make the value zero
uint16_t rwR, rwG,rwB;  // Used for scaling, this stores the white value coorisponding to 255

// readSettings reads values from EEPROM and stores them to the coorisponding variable
void readSettings(){
    EEPROM.begin(50);
    int Add = 0;
    EEPROM.get(Add,flag);
    Add+=sizeof(flag);
    EEPROM.get(Add,rawOffset);
    Add+=sizeof(rawOffset);
    EEPROM.get(Add,offset);
    Add+=sizeof(offset);
    EEPROM.get(Add,scaleScale);
    Add+=sizeof(scaleScale);
    EEPROM.get(Add,rbR);
    Add+=sizeof(rbR);
    EEPROM.get(Add,rbG);
    Add+=sizeof(rbG);
    EEPROM.get(Add,rbB);
    Add+=sizeof(rbB);
    EEPROM.get(Add,rwR);
    Add+=sizeof(rwR);
    EEPROM.get(Add,rwG);
    Add+=sizeof(rwG);
    EEPROM.get(Add,rwB);
    Add+=sizeof(rwB);
}
// writeSettings stores certian variables in EEPROM
void writeSettings(){
    flag = 123;
    EEPROM.begin(50);
    int Add = 0;
    EEPROM.put(Add,flag);
    Add+=sizeof(flag);
    EEPROM.put(Add,rawOffset);
    Add+=sizeof(rawOffset);
    EEPROM.put(Add,offset);
    Add+=sizeof(offset);
    EEPROM.put(Add,scaleScale);
    Add+=sizeof(scaleScale);
    EEPROM.put(Add,rbR);
    Add+=sizeof(rbR);
    EEPROM.put(Add,rbG);
    Add+=sizeof(rbG);
    EEPROM.put(Add,rbB);
    Add+=sizeof(rbB);
    EEPROM.put(Add,rwR);
    Add+=sizeof(rwR);
    EEPROM.put(Add,rwG);
    Add+=sizeof(rwG);
    EEPROM.put(Add,rwB);
    Add+=sizeof(rwB);
    EEPROM.commit();
}
// Respond to client requests for data with all pertinent data in JSON format
void handleData() {
    
    // Used to store the actual response, which will be built following
    String webPage;
    // Create the root object
    DynamicJsonDocument doc(1024);

    doc["WEIGHT"] =    Weight; //Put Sensor value
    doc["TEMP"] =      rawTemp; //Reads Flash Button Status
    doc["HUMIDITY"]=   rawHumidity;
    doc["IP"]=IP;
    // Adjust and calculate the RGB values that will be reported
    
    if(colorPresent)  // Only check color every 4 seconds to conserve power
    {
        digitalWrite(2,HIGH);   // White LED on
        delay(5);              // Let it stabalize
        tcs.getRawData(&rawRed, &rawGreen, &rawBlue, &rawClear);
        digitalWrite(2,LOW);    // White LED off
        float dR = (float)(rwR-rbR);
        float dG = (float)(rwG-rbG);
        float dB = (float)(rwB-rbB);
        if(dR==0) dR = 1;
        if(dG==0) dG = 1;
        if(dB==0) dB = 1;
        // Scale 0-1
        float Red =     ((float)(rawRed-rbR))/dR;
        float Green =   ((float)(rawGreen-rbG))/dG;
        float Blue =    ((float)(rawBlue-rbB))/dB;
        // Clip 0-1 to ensure
        Red = constrain(Red,0,1);
        Green = constrain(Green,0,1);
        Blue = constrain(Blue,0,1);
        // Invert the color, apply an exponent and invert back to normal.  This is to adjust the nonlinear response of the color sensor.  1.8 was tuned by hand, you may choose a different value
        Red =   255*(1-pow(1-Red,1.8));
        Green = 255*(1-pow(1-Green,1.8));
        Blue =  255*(1-pow(1-Blue,1.8));

        doc["RED"]=        (int)Red;
        doc["GREEN"]=      (int)Green;
        doc["BLUE"]=       (int)Blue;
        doc["CLEAR"]=      rawClear;
    }
    // Copy doc into webPage and send
    serializeJson(doc,webPage);
    server.send(200, "text/html", webPage);  
}
void handleTare(){
    rawOffset = -rawWeight;
    server.send(200, "text/html", "Tare Successful");  
    writeSettings();
} 
void handleOffset(){
    if(server.arg("mass")=="")
    {
        offset = 0;
        server.send(200, "text/html", "No mass parameter found");
    }
    else
    {
        String mass = server.arg("mass");
        int valIn = mass.toInt();
        offset = valIn; 
        server.send(200, "text/html", "Offset Successful");  
    }
    writeSettings();    
}
void handleScale(){
    if(server.arg("mass")=="")
    {
        scaleScale = 1.0;
        server.send(200, "text/html", "No mass parameter found");
    }
    else
    {
        String mass = server.arg("mass");
        float valIn = mass.toFloat();
        float rawDenom = rawWeight+rawOffset;
        if(rawDenom==0)
        {
            server.send(200, "text/html", "Scale reading is zero, please place something on it and try again.");        
        } 
        else
        {
            scaleScale = valIn/(float)(rawWeight+rawOffset);
            server.send(200, "text/html", "Scale Successful");
        }  
    }
    writeSettings();
} 
void handleReset(){
    server.send(200, "text/html", "Reset all parameters");
    scaleScale = 1;
    rawOffset = 0;
    offset = 0;
    rbR = 0;
    rbG = 0;
    rbB = 0;
    rwR = 255;
    rwG = 255;
    rwB = 255;
    writeSettings();
}
void handleCalBlack(){
    server.send(200, "text/html", "Calibrated Black");
    
    rbR = rawRed;
    rbG = rawGreen;
    rbB = rawBlue;
}
void handleCalWhite(){
    server.send(200, "text/html", "Calibrated White");
    rwR = rawRed;
    rwG = rawGreen;
    rwB = rawBlue;
}
void handleRoot() {
    String s = MAIN_page;
    server.send(200, "text/html", s);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup(){
    readSettings();
    // If the EEPROM is fresh, the flag will not be "123", initialize values to defaults
    if(flag!=123)
    {
        rbR = 0;
        rbG = 0;
        rbB = 0;
        rwR = 255;
        rwG = 255;
        rwB = 255;
        scaleScale = 1;
        rawOffset = 0;
        offset = 0;
        writeSettings();
    }
    if(scaleScale == 0)
        scaleScale = 1;
    
    Serial.begin(9600);
    Serial.println("RFX Filament Monitor Loading...");
    Serial.print("rawOffset: ");
    Serial.println(rawOffset);
    
    pinMode(BUILTIN_LED, OUTPUT);
 
    //Initialize Display
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
        Serial.println(F("SSD1306 allocation failed"));
        displayPresent = false;
    }
    else
    {
        Serial.println("SSD1306 Found");
        displayPresent = true;
    }
    //Initialize Color Sensor
    if (tcs.begin()) {
        Serial.println("Color sensor Found");
        colorPresent = true;
    } 
    else 
    {
        Serial.println("No TCS34725 found ... check your connections");
        colorPresent = false;
    }
    // Setup and drive LOW the color sensor boards white LED
    pinMode(2, OUTPUT);
    digitalWrite(2, LOW);

    //Initialize Temp/Humidity
    dht.begin();

    //Initialize scale
    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

    //Initialize ESP8266WebServer
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (MDNS.begin(name)) {
        Serial.println("MDNS responder started");
    }
    server.on("/", handleRoot);
    server.on("/inline", []() {
        server.send(200, "text/plain", "this works as well");
    });
    server.on("/tare", handleTare);
    server.on("/scale", handleScale);     
    server.on("/offset", handleOffset);
    server.on("/reset", handleReset);
    
    server.on("/getData", handleData);
    server.on("/calWhite", handleCalWhite);
    server.on("/calBlack", handleCalBlack);
    server.onNotFound(handleNotFound);
    server.begin();
}
unsigned long prevTime=0;
unsigned long currentTime=0;
unsigned int interval = 1000;
void loop(){
    
    server.handleClient();
    MDNS.update();
    currentTime = millis();
    if(currentTime-prevTime > interval)
    {
        prevTime = currentTime;
        
        rawHumidity = dht.readHumidity();
        // Read temperature as Celsius (the default)
        rawTemp = dht.readTemperature();
        Serial.print("\tTemp(C): ");
        Serial.print((int)rawTemp);
        Serial.print(" Humidity(%): ");
        Serial.print((int)rawHumidity);
        // Handle scale
        if (scale.is_ready()) {
            //long reading = scale.read();
            rawWeight = scale.read();
            Weight = scaleScale * (float)(rawWeight + rawOffset) + offset;
            Serial.print("Weight: ");
            Serial.println(Weight);
        } else {
            Serial.println("\tHX711 not found.");
        }
        if(WiFi.status() == WL_CONNECTED) {
            
            Serial.print("Connected: ");
            // Format the IP into a readable string
            IPAddress I = WiFi.localIP();
            IP = String(I[0]) + String(".") +\
                    String(I[1]) + String(".") +\
                    String(I[2]) + String(".") +\
                    String(I[3])  ; 
            
            Serial.println(IP);
        }
        if(displayPresent)
            displayHomeScreen();
    }
}
void displayHomeScreen(){
    display.clearDisplay();

    // Show IPAddress center top of display
    display.setTextSize(1); // Draw 2X-scale text
    display.setTextColor(SSD1306_WHITE);

    // Determine the x value to center the IP address horizontally
    float x = ((float)SCREEN_WIDTH)/2.0;
    x -= ((float)(CHAR_WIDTH*sizeof(IP)))/2.0;
    display.setCursor((int)x, 0);
    display.println(IP);

    display.setTextSize(3);
    int clippedWeight = Weight;
    if(clippedWeight < 0)
        clippedWeight = 0;
    if(clippedWeight > 9999)
        clippedWeight = 9999;
 
    // Right justify the weight
    x=0;
    if(clippedWeight < 1000)
        x+=18;
    if(clippedWeight < 100)
        x+=18;
    if(clippedWeight < 10)
        x+=18;
    display.setCursor((int)x, 11);
    display.println(clippedWeight);
 
    // add weight units
    display.setTextSize(2);
    display.setCursor(72, 16);
    display.println("g");

    // draw screen seperator
    display.drawLine(85,12,85,32,SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(90, 16);

    // write humidity
    display.println(rawHumidity);
    display.setTextSize(1);
    display.setCursor(115, 24);
    display.println("%H");
    
    // commit changes
    display.display();      // Show initial text
}


