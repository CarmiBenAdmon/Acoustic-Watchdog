# Real-Time Acoustic Watchdog

Sound classification on an ESP32 that controls a smart light. A shout turns the light on, a clap turns it off, and everything else is ignored. The classification runs on the microcontroller itself, with no PC or cloud inference.

## How it works

1. **Sampling** – An analog microphone is sampled by the ESP32 ADC at 16 kHz. Each frame is 256 samples (16 ms).  
2. **Preprocessing** – The mean is subtracted to remove the DC offset, and a Hamming window is applied.  
3. **FFT** – A 256-point FFT gives the magnitude of each frequency bin (62.5 Hz per bin). The first 128 bins (0–8 kHz) are used as features.  
4. **Classification** – A Random Forest model (100 trees) runs on the ESP32 and classifies the frame as silence/noise, clap or shout.  
5. **Decision** – An action is taken only if the sound is loud enough (peak-to-peak \> 100\) and at least 85 of the 100 trees agree. A 1.5 s cooldown prevents one sound from triggering twice.  
6. **Output** – The ESP32 sends an on/off event to SinricPro over Wi-Fi, and shows the result and a live spectrum on an OLED display.

## Hardware

| Component | Connection |
| :---- | :---- |
| ESP32 dev board | – |
| Analog microphone module (electret \+ amplifier) | Analog out → GPIO34 (ADC1) |
| SSD1306 OLED, 128x64 | I2C, address 0x3C (SDA → GPIO21, SCL → GPIO22) |
| Smart light | Controlled through SinricPro |

GPIO34 was chosen because it belongs to ADC1. On the ESP32, ADC2 cannot be used while Wi-Fi is active.

## Repository structure

| File | Purpose |
| :---- | :---- |
| `Real_Time_Acoustic_Watchdog.ino` | Main firmware: sampling, FFT, classification, Wi-Fi and display |
| `model_data.h` | The trained model, converted to C++ (with an added vote-based confidence score) |
| `sketch_jan7b.ino` | Data collection firmware: sends FFT frames to the PC over UART |
| `serial_reading.py` | Reads the frames from the serial port and saves them to CSV |
| `train_model.py` | Trains the Random Forest model |
| `convert_to_cpp.py` | Converts the trained model to a C++ header |
| `audio_samples_*.csv` | Recorded dataset (silence/noise, clap, shout) |

## Setup

### Firmware

1. Install the Arduino libraries: `Adafruit GFX`, `Adafruit SSD1306`, `arduinoFFT`, `SinricPro`.  
2. Copy `secrets.example.h` to `secrets.h` and fill in your Wi-Fi and SinricPro details. `secrets.h` is listed in `.gitignore` and is never committed.  
3. Build and flash `Real_Time_Acoustic_Watchdog.ino` to the ESP32.

### Training your own model

1. Flash `sketch_jan7b.ino`. It sends a frame each time the sound is loud enough.  
2. Set the output file name in `serial_reading.py`, run it, and record each class separately.  
3. Install the Python packages: `pyserial`, `pandas`, `scikit-learn`, `joblib`, `micromlgen`.  
4. Run `train_model.py`, then `convert_to_cpp.py` to generate a new `model_data.h`.

## Results

The classes were balanced to 95 frames each, with an 80/20 train/test split.

- The most informative bins are around 440–625 Hz (voice range) and around 5.5 kHz (clap energy)

The dataset is small and was recorded in one room, so performance in other rooms and at other distances still needs testing.

## Limitations and next steps

- **Sampling** is timed in software with a busy-wait loop. A hardware timer interrupt, or I2S with DMA, would give exact timing.  
- **The main loop is blocking**: no samples are taken during the FFT, the display update or the cooldown. A double buffer, with sampling and processing in separate FreeRTOS tasks on the two cores, would make sampling continuous.  
  