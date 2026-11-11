import joblib
from micromlgen import port

# 1. Load the trained model
model = joblib.load('acoustic_watchdog_model.pkl')

# 2. Convert the model to C++ code
# Name of the generated class
class_name = "ClapClassifier"
cpp_code = port(model, classname=class_name)

# 3. Save it as a header file for the Arduino sketch
with open("model_data.h", "w") as f:
    f.write(cpp_code)

print("C++ header file 'model_data.h' has been generated!")