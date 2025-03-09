import pandas as pd
import matplotlib.pyplot as plt

def plot_sensor_data(file_path):
    # Read the CSV file
    data = pd.read_csv(file_path)

    # Plot the data using the index
    plt.figure(figsize=(10, 5))
    plt.plot(data.index, data['irvalue'], label='IR Value')
    plt.plot(data.index, data['redvalue'], label='Red Value')
    plt.xlabel('Index')
    plt.ylabel('Sensor Values')
    plt.title('Sensor Data Plot')
    plt.legend()
    plt.xticks(rotation=45)
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    file_path = './data/sensor_data_20250310_025319.csv'
    plot_sensor_data(file_path)
