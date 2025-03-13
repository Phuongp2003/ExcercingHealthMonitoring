import pandas as pd
import matplotlib.pyplot as plt

# Read the CSV file
data = pd.read_csv('data/data_1741664379.csv')

# Convert the Timestamp column to datetime format with milliseconds
# data['Timestamp'] = pd.to_datetime(data['Timestamp'], unit='ms')

# Plot the data
plt.figure(figsize=(10, 6))
plt.plot(data['Timestamp'], data['IR Value'], label='IR Value')
plt.plot(data['Timestamp'], data['Red Value'], label='Red Value')
plt.xlabel('Timestamp')
plt.ylabel('Value')
plt.title('OxiSensor Data Plot')
plt.legend()
plt.grid(True)
plt.show()
