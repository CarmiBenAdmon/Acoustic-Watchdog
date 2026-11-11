import serial
import csv
import time
import os

# Folder where this script is located
script_dir = os.path.dirname(os.path.abspath(__file__))
# Build the output file path inside the script folder
FILENAME = os.path.join(script_dir, "audio_samples_shout.csv")

# --- Settings (set the COM port shown in the Arduino IDE) ---
SERIAL_PORT = 'COM6'  
BAUD_RATE = 115200
# Output file name: change it for each sound class you record
FILENAME = "audio_samples_kaf.csv" 

def collect_data():
    try:
        # Open the serial port
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        time.sleep(2)  # Wait for the board to reset
        print(f"Connected to {SERIAL_PORT}. Waiting for button press on ESP32...")

        while True:
            if ser.in_waiting > 0:
                # Read one line from the serial port
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                
                # Keep only complete FFT frames
                if line.startswith("DATA_START") and line.endswith("DATA_END"):
                    # Remove the start/end tags and split the values
                    raw_data = line.replace("DATA_START,", "").replace(",DATA_END", "")
                    data_points = raw_data.split(",")
                    
                    # Append the frame to the CSV file
                    with open(FILENAME, mode='a', newline='') as file:
                        writer = csv.writer(file)
                        writer.writerow(data_points)
                    
                    print(f"Sample saved! (Total points: {len(data_points)})")

    except KeyboardInterrupt:
        print("\nStopping data collection...")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        ser.close()

if __name__ == "__main__":
    collect_data()