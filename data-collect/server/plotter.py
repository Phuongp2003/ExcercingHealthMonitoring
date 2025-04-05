import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os
import glob
import argparse
from scipy.signal import butter, filtfilt
import sys
from datetime import datetime


def find_latest_csv():
    """Find the most recent CSV file in the data/csv directory"""
    csv_dir = os.path.join('data', 'csv')
    if not os.path.exists(csv_dir):
        print(f"Error: Directory '{csv_dir}' not found")
        return None

    csv_files = glob.glob(os.path.join(csv_dir, '*.csv'))
    if not csv_files:
        print(f"Error: No CSV files found in '{csv_dir}'")
        return None

    latest_file = max(csv_files, key=os.path.getctime)
    return latest_file


def butter_bandpass(lowcut, highcut, fs, order=5):
    """Design a bandpass filter"""
    nyq = 0.5 * fs
    low = lowcut / nyq
    high = highcut / nyq
    b, a = butter(order, [low, high], btype='band')
    return b, a


def apply_filter(data, lowcut=0.5, highcut=5.0, fs=30.0, order=5):
    """Apply a bandpass filter to the data"""
    b, a = butter_bandpass(lowcut, highcut, fs, order=order)
    y = filtfilt(b, a, data)
    return y


def normalize_signal(signal):
    """Normalize signal to [0,1] range"""
    return (signal - np.min(signal)) / (np.max(signal) - np.min(signal))


def plot_raw_data(df, save_path=None):
    """Plot the raw IR and Red values over time"""
    plt.figure(figsize=(12, 6))
    plt.plot(df['time_delta'], df['ir'], 'r-', label='IR Signal', alpha=0.8)
    plt.plot(df['time_delta'], df['red'], 'b-', label='Red Signal', alpha=0.8)
    plt.xlabel('Time (seconds)')
    plt.ylabel('Sensor Values')
    plt.title('Raw Sensor Data')
    plt.legend()
    plt.grid(True)

    if save_path:
        plt.savefig(save_path)
        print(f"Plot saved to {save_path}")
    else:
        plt.show()


def plot_filtered_data(df, fs=None, save_path=None):
    """Plot filtered IR and Red signals to highlight pulse waveform"""
    # Calculate sampling frequency if not provided
    if fs is None:
        # Calculate average time delta between consecutive samples
        time_diffs = np.diff(df['time_delta'])
        avg_period = np.mean(time_diffs)
        fs = 1.0 / avg_period

    print(f"Sampling frequency: {fs:.2f} Hz")

    # Apply bandpass filter to isolate pulse frequencies (typically 0.5-5 Hz)
    ir_filtered = apply_filter(df['ir'], lowcut=0.5, highcut=5.0, fs=fs)
    red_filtered = apply_filter(df['red'], lowcut=0.5, highcut=5.0, fs=fs)

    # Normalize for better visualization
    ir_norm = normalize_signal(ir_filtered)
    red_norm = normalize_signal(red_filtered)

    plt.figure(figsize=(12, 8))

    # Plot Raw Data
    plt.subplot(2, 1, 1)
    plt.plot(df['time_delta'], normalize_signal(
        df['ir']), 'r-', label='IR', alpha=0.5)
    plt.plot(df['time_delta'], normalize_signal(
        df['red']), 'b-', label='Red', alpha=0.5)
    plt.xlabel('Time (seconds)')
    plt.ylabel('Normalized Signal')
    plt.title('Raw Sensor Data (Normalized)')
    plt.legend()
    plt.grid(True)

    # Plot Filtered Data
    plt.subplot(2, 1, 2)
    plt.plot(df['time_delta'], ir_norm, 'r-',
             label='IR (Filtered)', linewidth=1.5)
    plt.plot(df['time_delta'], red_norm, 'b-',
             label='Red (Filtered)', linewidth=1.5)
    plt.xlabel('Time (seconds)')
    plt.ylabel('Normalized Signal')
    plt.title('Bandpass Filtered Data (0.5-5 Hz)')
    plt.legend()
    plt.grid(True)

    plt.tight_layout()

    if save_path:
        plt.savefig(save_path)
        print(f"Plot saved to {save_path}")
    else:
        plt.show()


def plot_segments(df, window_size=10, save_path=None):
    """Plot data in smaller segments for detailed inspection"""
    total_time = df['time_delta'].max()
    num_segments = max(1, int(np.ceil(total_time / window_size)))

    for i in range(num_segments):
        start_time = i * window_size
        end_time = min((i + 1) * window_size, total_time)

        # Filter data for this time segment
        segment = df[(df['time_delta'] >= start_time)
                     & (df['time_delta'] < end_time)]

        if len(segment) == 0:
            continue

        plt.figure(figsize=(12, 6))
        plt.plot(segment['time_delta'], segment['ir'], 'r-', label='IR Signal')
        plt.plot(segment['time_delta'], segment['red'],
                 'b-', label='Red Signal')
        plt.xlabel('Time (seconds)')
        plt.ylabel('Sensor Values')
        plt.title(f'Segment {i+1}: {start_time:.1f}s - {end_time:.1f}s')
        plt.legend()
        plt.grid(True)

        if save_path:
            segment_path = save_path.replace('.png', f'_segment{i+1}.png')
            plt.savefig(segment_path)
            print(f"Segment {i+1} saved to {segment_path}")
        else:
            plt.show()


def plot_data_analysis(file_path):
    """Generate comprehensive analysis plots for the data"""
    # Create output directory
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_dir = os.path.join('data', 'analysis', 'plots')
    os.makedirs(output_dir, exist_ok=True)

    # Load data
    df = pd.read_csv(file_path)
    file_name = os.path.basename(file_path).split('.')[0]

    # Calculate sampling frequency
    time_diffs = np.diff(df['time_delta'])
    avg_period = np.mean(time_diffs)
    fs = 1.0 / avg_period

    # Generate plots
    print(f"Generating analysis plots for {file_name}")
    print(f"Found {len(df)} samples with average sampling rate of {fs:.2f} Hz")

    # Plot 1: Raw data
    raw_path = os.path.join(output_dir, f"{file_name}_raw_{timestamp}.png")
    plot_raw_data(df, save_path=raw_path)

    # Plot 2: Filtered data
    filtered_path = os.path.join(
        output_dir, f"{file_name}_filtered_{timestamp}.png")
    plot_filtered_data(df, fs=fs, save_path=filtered_path)

    print(f"Analysis complete. Plots saved to {output_dir}")
    return output_dir


def main():
    parser = argparse.ArgumentParser(
        description='Plot sensor data from CSV files')
    parser.add_argument(
        'file', nargs='?', help='CSV file to plot (uses most recent if not specified)')
    parser.add_argument('--raw', action='store_true', help='Plot raw data')
    parser.add_argument('--filtered', action='store_true',
                        help='Plot filtered data')
    parser.add_argument('--segments', action='store_true',
                        help='Plot data in segments')
    parser.add_argument('--segment-size', type=int,
                        default=10, help='Size of segments in seconds')
    parser.add_argument('--save', action='store_true',
                        help='Save plots instead of displaying')
    parser.add_argument('--analysis', action='store_true',
                        help='Run comprehensive analysis')

    args = parser.parse_args()

    # Determine which file to use
    if args.file:
        file_path = args.file
        if not os.path.exists(file_path):
            # Try with data/csv prefix
            alternate_path = os.path.join('data', 'csv', args.file)
            if os.path.exists(alternate_path):
                file_path = alternate_path
            else:
                print(f"Error: File not found: {args.file}")
                return
    else:
        file_path = find_latest_csv()
        if not file_path:
            return

    print(f"Using file: {file_path}")

    # Load the data
    try:
        df = pd.read_csv(file_path)
        print(f"Loaded {len(df)} data points")
    except Exception as e:
        print(f"Error loading CSV file: {str(e)}")
        return

    # Determine output path if saving
    output_path = None
    if args.save:
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        os.makedirs('plots', exist_ok=True)
        output_path = os.path.join(
            'plots', f"{os.path.basename(file_path).split('.')[0]}_{timestamp}.png")

    # Determine which plots to generate
    if args.analysis:
        plot_data_analysis(file_path)
    elif args.raw:
        plot_raw_data(df, save_path=output_path)
    elif args.filtered:
        plot_filtered_data(df, save_path=output_path)
    elif args.segments:
        plot_segments(df, window_size=args.segment_size, save_path=output_path)
    else:
        # Default: just plot raw data
        plot_raw_data(df, save_path=output_path)


if __name__ == "__main__":
    main()
