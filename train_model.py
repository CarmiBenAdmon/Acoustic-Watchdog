import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import classification_report, confusion_matrix
import joblib
import os

script_dir = os.path.dirname(os.path.abspath(__file__))

# 1. Load the three recorded classes
try:
    df_silence = pd.read_csv(os.path.join(script_dir, 'audio_samples_noese.csv'), header=None)
    df_clap = pd.read_csv(os.path.join(script_dir, 'audio_samples_kaf.csv'), header=None)
    df_shout = pd.read_csv(os.path.join(script_dir, 'audio_samples_shout.csv'), header=None)
    print(f"Loaded: Silence({len(df_silence)}), Claps({len(df_clap)}), Shouts({len(df_shout)})")
except FileNotFoundError as e:
    print(f"Error: {e}")
    exit()

# 2. Labels: 0 = silence/noise, 1 = clap, 2 = shout
df_silence['label'] = 0
df_clap['label'] = 1
df_shout['label'] = 2

# 3. Balance the classes to the size of the smallest one
min_samples = min(len(df_silence), len(df_clap), len(df_shout))
combined_df = pd.concat([
    df_silence.sample(min_samples),
    df_clap.sample(min_samples),
    df_shout.sample(min_samples)
], ignore_index=True)

X = combined_df.drop('label', axis=1)
y = combined_df['label']


# --- Optional normalization (max scaling) ---
# Divides each row by its max value, so the range is 0 to 1.
# Disabled: the model uses raw magnitudes, the same as the firmware.
#X = X.div(X.max(axis=1), axis=0).fillna(0)

# 4. Train/test split and training
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)
model = RandomForestClassifier(n_estimators=100, random_state=42)
model.fit(X_train, y_train)

print("\n--- Model Trained with Normalization ---")
print(confusion_matrix(y_test, model.predict(X_test)))

# 5. Save the model
joblib.dump(model, 'acoustic_watchdog_model.pkl')
