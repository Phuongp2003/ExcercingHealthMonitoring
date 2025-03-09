import pandas as pd
import matplotlib.pyplot as plt

# Load the filtered data
data = pd.read_csv('./filtered_sensor_data_20250304_133748.csv')

# Plotting function
def plot_filtered_data(data, original_column, filtered_column, title):
    plt.figure(figsize=(12, 6))
    plt.plot(data.index, data[original_column], label='Original')
    plt.plot(data.index, data[filtered_column], label='Filtered')
    plt.xlabel('Index')
    plt.ylabel('Value')
    plt.title(title)
    plt.legend()
    plt.show()

# Plot original and filtered data for redValue
plot_filtered_data(data, 'redvalue', 'redValue_MAF', 'Red Value - Moving Average Filter') # Bad
plot_filtered_data(data, 'redvalue', 'redValue_Butterworth', 'Red Value - Butterworth Filter') # Good

# Plot original and filtered data for irValue
plot_filtered_data(data, 'irvalue', 'irValue_MAF', 'IR Value - Moving Average Filter') # Bad
plot_filtered_data(data, 'irvalue', 'irValue_Butterworth', 'IR Value - Butterworth Filter') # Good

# 2 Filter - Bad
plot_filtered_data(data, 'redvalue', 'redValue_MAF_Butterworth', 'Red Value - Moving Average Filter -> Butterworth Filter')
plot_filtered_data(data, 'redvalue', 'redValue_Butterworth_MAF', 'Red Value - Butterworth Filter -> Moving Average Filter')

# 2 Filter - Bad
plot_filtered_data(data, 'irvalue', 'irValue_MAF_Butterworth', 'IR Value - Moving Average Filter -> Butterworth Filter')
plot_filtered_data(data, 'irvalue', 'irValue_Butterworth_MAF', 'IR Value - Butterworth Filter -> Moving Average Filter')
