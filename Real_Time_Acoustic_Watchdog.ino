#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "arduinoFFT.h"
#include "model_data.h"
#include <WiFi.h>
#include "SinricPro.h"
#include "SinricProSwitch.h"

// --- Connection settings (Wi-Fi and SinricPro), kept out of version control ---
#include "secrets.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define SAMPLES 256
#define SAMPLING_FREQUENCY 16000
#define MIC_PIN 34

double vReal[SAMPLES];
double vImag[SAMPLES];
float features[SAMPLES / 2];
unsigned long sampling_period_us;

ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLES, SAMPLING_FREQUENCY);
Eloquent::ML::Port::ClapClassifier classifier;

void setup() {
  Serial.begin(115200);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) for(;;);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  
  // Initialize SinricPro
  SinricProSwitch &mySwitch = SinricPro[SWITCH_ID];
  SinricPro.begin(APP_KEY, APP_SECRET);

  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
  Serial.println("Watchdog Ready - Shout=ON, Clap=OFF");
}

void loop() {
  SinricPro.handle();

  double mean = 0;
  int maxVal = 0;
  int minVal = 4095;
  unsigned long microseconds;
  float confidence = 0;
  int result = 0;

  for (int i = 0; i < SAMPLES; i++) {
    microseconds = micros();
    int val = analogRead(MIC_PIN);
    vReal[i] = val;
    vImag[i] = 0;
    mean += val;
    if (val > maxVal) maxVal = val;
    if (val < minVal) minVal = val;
    while (micros() < (microseconds + sampling_period_us));
  }

  int peakToPeak = maxVal - minVal;

  mean /= SAMPLES;
  for (int i = 0; i < SAMPLES; i++) vReal[i] -= mean;
  FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(FFT_FORWARD);
  FFT.complexToMagnitude();

  for (int i = 0; i < (SAMPLES / 2); i++) {
    features[i] = (float)vReal[i];
  }

  result = classifier.predict(features, confidence);

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);

  if (result == 1) display.print("AI: Clap? ");
  else if (result == 2) display.print("AI: Shout? ");
  else display.print("AI: Silence ");
  display.print((int)(confidence * 100));
  display.print("%");

  // --- Decision: act only on a loud sound with a confident prediction ---
  if (peakToPeak > 100 && confidence > 0.85) {
    display.setTextSize(2);
    display.setCursor(0, 20);
    SinricProSwitch &mySwitch = SinricPro[SWITCH_ID];

    if (result == 2) { // shout
      display.print(">> LIGHT ON <<");
      mySwitch.sendPowerStateEvent(true); // ON
    } 
    else if (result == 1) { // clap
      display.print(">> LIGHT OFF <<");
      mySwitch.sendPowerStateEvent(false); // OFF
    }
    
    display.display();
    delay(1500); 
  }

  for (int i = 2; i < 62; i++) {
    int barHeight = map((int)vReal[i], 0, 2000, 0, 25);
    display.drawLine(i * 2, 63, i * 2, 63 - barHeight, SSD1306_WHITE);
  }
  display.display();
}