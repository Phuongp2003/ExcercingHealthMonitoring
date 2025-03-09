import pandas as pd
import numpy as np
from scipy.signal import butter, filtfilt

# Load the data
data = pd.read_csv('./sensor_data_20250304_133748.csv')

# Apply Moving Average Filter (MAF)
def moving_average_filter(data, window_size):
    return data.rolling(window=window_size).mean()

# Apply Butterworth Filter
def butterworth_filter(data, cutoff, fs, order=5):
    nyquist = 0.5 * fs
    normal_cutoff = cutoff / nyquist
    b, a = butter(order, normal_cutoff, btype='low', analog=False)
    y = filtfilt(b, a, data)
    return y

# Parameters
window_size = 5  # for MAF
cutoff = 2.0  # for Butterworth filter
fs = 12  # sampling frequency

# Apply filters for redValue
data['redValue_MAF'] = moving_average_filter(data['redvalue'], window_size)
data['redValue_Butterworth'] = butterworth_filter(data['redvalue'], cutoff, fs)

# Apply filters for irValue
data['irValue_MAF'] = moving_average_filter(data['irvalue'], window_size)
data['irValue_Butterworth'] = butterworth_filter(data['irvalue'], cutoff, fs)

# Apply two filters sequentially for redValue
redValue_MAF_Butterworth = butterworth_filter(data['redValue_MAF'].dropna(), cutoff, fs)
data['redValue_MAF_Butterworth'] = pd.Series(redValue_MAF_Butterworth).reset_index(drop=True)
data['redValue_Butterworth_MAF'] = moving_average_filter(pd.Series(data['redValue_Butterworth']), window_size).reset_index(drop=True)

# Apply two filters sequentially for irValue
irValue_MAF_Butterworth = butterworth_filter(data['irValue_MAF'].dropna(), cutoff, fs)
data['irValue_MAF_Butterworth'] = pd.Series(irValue_MAF_Butterworth).reset_index(drop=True)
data['irValue_Butterworth_MAF'] = moving_average_filter(pd.Series(data['irValue_Butterworth']), window_size).reset_index(drop=True)

# Save the filtered data
data.to_csv('./filtered_sensor_data_20250304_133748.csv', index=False)
