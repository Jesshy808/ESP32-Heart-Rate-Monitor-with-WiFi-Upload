#include <Wire.h>
#include "MAX30105.h"
#include <GyverOLED.h>
#include <WiFi.h>

#include "heartRate.h"

MAX30105 particleSensor;
GyverOLED<SSH1106_128x64> oled;

//heart sensor
const byte RATE_SIZE = 4; //Increase this for more averaging
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute;
int beatAvg;

//data wifi stuff
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
const char* apiKey = "YOUR_THINGSPEAK_API_KEY";

// WiFi connection tracking
bool wifiConnected = false;
unsigned long lastWifiCheck = 0;
const unsigned long wifiCheckInterval = 10000; // Check every 10 seconds

//component stuff
int dataButton = 4;
int buzzer = 2;
int number = 0;
bool lastButtonState = LOW;

int redLed = 5;
int greenLed = 18;
int high = 95;

unsigned long lastAutoSendTime = 0;
const unsigned long sendCooldown = 15000;  // 15 seconds cooldown


void setup()
{
  Serial.begin(115200);
  Serial.println("Initializing...");

  //display
  oled.init();
  oled.clear();
  oled.autoPrintln(true);
  oled.setScale(1);

  //components
  pinMode(buzzer, OUTPUT);
  pinMode(dataButton, INPUT_PULLUP);
  pinMode(greenLed, OUTPUT);
  pinMode(redLed, OUTPUT);

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST))
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);

  // Start WiFi connection attempt
  Serial.println("Attempting to connect to WiFi...");
  WiFi.begin(ssid, password);
}

void loop()
{
  // Heartrate calculating stuff
  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true)
  {
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= RATE_SIZE;

      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  // Handle WiFi connection status (non-blocking)
  checkWiFiStatus();


  // Serial output
  Serial.print("IR=");
  Serial.print(irValue);
  Serial.print(", BPM=");
  Serial.print(beatsPerMinute);
  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);
  Serial.print(", WiFi=");
  Serial.println(wifiConnected ? "Connected" : "Disconnected");

  if (irValue < 50000)
    Serial.println(" No finger?");

  // WiFi data send
  bool buttonState = digitalRead(dataButton);
  if (buttonState == LOW && lastButtonState == HIGH) {
    if (wifiConnected) {
      sendNumber(beatAvg);
      Serial.print("Sent #");
      Serial.println(number);
      beatsPerMinute = 0.0;
      beatAvg = 0;
      } 
      else {
      Serial.println("Cannot send - WiFi not connected");
      beatsPerMinute = 0.0;
      beatAvg = 0;
    }
  }
  
  lastButtonState = buttonState;

  // Display - SHOWS WiFi STATUS
  oled.clear();
  oled.setCursor(0, 0);
  oled.print("BPM: ");
  oled.print(beatsPerMinute);
  oled.setCursor(0, 2);
  oled.print("Avg: ");
  oled.print(beatAvg);
  
  //different heart ranges and LED
  oled.setCursor(0, 4);
  if (beatAvg >= 40 && beatAvg <= high) {
    oled.print("Heart rate normal");
    digitalWrite(greenLed, HIGH);
    digitalWrite(redLed, LOW);
    noTone(buzzer);
  } // if high heartrate detected
  // Just time-based, no state flag needed
else if (beatAvg > high) {
  oled.setCursor(0, 4);
  oled.print("Heart rate high!");
  digitalWrite(greenLed, LOW);
  digitalWrite(redLed, HIGH);
  tone(buzzer, 440);
  
  // Simple: If 15 seconds passed since last send, send again
  if (millis() - lastAutoSendTime > sendCooldown) {
    if (wifiConnected) {
      sendNumber(beatAvg);
      lastAutoSendTime = millis();
      Serial.println("Auto-sent data!");
    }
  }
}
//if measurement still calibrating
  else {
    oled.print("Calibrating...");
    digitalWrite(greenLed, LOW);
    digitalWrite(redLed, LOW);
    noTone(buzzer);
  }

  // Show WiFi status on display
  oled.setCursor(0, 6);
  oled.print("WiFi: ");
  oled.print(wifiConnected ? "Online" : "Offline");
   
  oled.update();

  
}

// Non-blocking WiFi status check
void checkWiFiStatus() {
  unsigned long currentMillis = millis();
  
  // Only check WiFi status periodically to avoid slowing down the loop
  if (currentMillis - lastWifiCheck >= wifiCheckInterval) {
    lastWifiCheck = currentMillis;
    
    wl_status_t status = WiFi.status();
    
    if (status == WL_CONNECTED && !wifiConnected) {
      wifiConnected = true;
      Serial.println("WiFi connected!");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    } 
    else if (status != WL_CONNECTED && wifiConnected) {
      wifiConnected = false;
      Serial.println("WiFi disconnected!");
      
      // Try to reconnect
      WiFi.disconnect();
      WiFi.begin(ssid, password);
    }
    else if (status == WL_CONNECT_FAILED || status == WL_CONNECTION_LOST) {
      // If connection failed, try again
      Serial.println("WiFi connection failed, retrying...");
      WiFi.begin(ssid, password);
    }
  }
}

// Send bpm data function
void sendNumber(int value) {
  if(wifiConnected) {
    WiFiClient client;
    
    if(client.connect("api.thingspeak.com", 80)) {
      String url = "GET /update?api_key=";
      url += apiKey;
      url += "&field1=";
      url += String(value);
      url += " HTTP/1.1\r\nHost: api.thingspeak.com\r\n\r\n";
      
      client.print(url);
      delay(10);
      client.stop();
      
      Serial.print("Sent to ThingSpeak: ");
      Serial.println(value);
    } else {
      Serial.println("Failed to connect to ThingSpeak");
    }
  }
}