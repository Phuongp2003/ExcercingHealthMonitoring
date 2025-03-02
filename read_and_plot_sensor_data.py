import pandas as pd
import matplotlib.pyplot as plt

def read_and_plot_sensor_data(file_path):
    # Read the CSV file
    data = pd.read_csv(file_path)
    # Plot the data
    data.plot()
    plt.xlabel('Index')
    plt.ylabel('Values')
    plt.title('Sensor Data')
    plt.show()

def show_by_file(name):
    file_path = f'./{name}.csv'
    read_and_plot_sensor_data(file_path)


if __name__ == "__main__":
    show_by_file("sensor_data_20250302_003414") # File binh thuong
    show_by_file("sensor_data_20250303_005834") # File di bo
