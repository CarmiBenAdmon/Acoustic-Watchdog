#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "arduinoFFT.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define SAMPLES 256              
#define SAMPLING_FREQUENCY 16000 // 16000 / 256 = 62.5 Hz per FFT bin
#define MIC_PIN 34               
#define BUTTON_PIN 13            

double vReal[SAMPLES];
double vImag[SAMPLES];
unsigned long sampling_period_us;

ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLES, SAMPLING_FREQUENCY);

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP); 
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) for(;;);
  display.clearDisplay();
  
  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
  Serial.println("System Ready. Press Button to save sample.");
}

void loop() {
  double mean = 0;
  int maxVal = 0;
  int minVal = 4095;
  unsigned long microseconds;

  // 1. Sampling, and tracking the peak-to-peak amplitude
  for (int i = 0; i < SAMPLES; i++) {
    microseconds = micros();
    int val = analogRead(MIC_PIN);
    vReal[i] = val;
    vImag[i] = 0;
    mean += val;

    // Track max and min to estimate loudness
    if (val > maxVal) maxVal = val;
    if (val < minVal) minVal = val;

    while (micros() < (microseconds + sampling_period_us));
  }

  int peakToPeak = maxVal - minVal; // Loudness of the captured sound

  // 2. Remove the DC offset
  mean /= SAMPLES;
  for (int i = 0; i < SAMPLES; i++) vReal[i] -= mean;

  // 3. FFT
  FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(FFT_FORWARD);
  FFT.complexToMagnitude();

  // 4. Auto-trigger: send a frame when the sound is loud enough
  // Adjust the threshold (200) to the needed sensitivity (check values in the Serial Plotter first)
  if (peakToPeak > 200) { 
    Serial.print("DATA_START,");
    for (int i = 0; i < (SAMPLES / 2); i++) {
      Serial.print(vReal[i], 2);
      if (i < (SAMPLES / 2) - 1) Serial.print(",");
    }
    Serial.println(",DATA_END");

    // Show on the display that a frame was captured
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("CAPTURED!");
    display.display();
    
    delay(200); // Short cooldown (200 ms) before the next capture
  }

  // 5. Live display while waiting for a trigger
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Waiting... P2P: ");
  display.print(peakToPeak);
  // Draw the spectrum
  for (int i = 2; i < (SAMPLES/2); i++) {
    int barHeight = map(vReal[i], 0, 1500, 0, 50);
    display.drawLine(i, 63, i, 63 - barHeight, SSD1306_WHITE);
  }
  display.display();
}