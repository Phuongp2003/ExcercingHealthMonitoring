import os
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from scipy import stats
import argparse
import glob
from datetime import datetime


def analyze_sampling_rate(file_path, window_size=100, plot=True):
    """
    Analyze the sampling rate of a sensor data file.

    Args:
        file_path: Path to the CSV file
        window_size: Size of the window for rolling statistics (number of samples)
        plot: Whether to generate plots

    Returns:
        Dictionary containing analysis results
    """
    # Read the CSV file
    print(f"Analyzing file: {os.path.basename(file_path)}")
    df = pd.read_csv(file_path)

    # Calculate time differences between consecutive samples
    df['delta_t'] = df['time_delta'].diff()

    # Skip the first row since diff() produces NaN for it
    df = df.iloc[1:].reset_index(drop=True)

    # Calculate frequency for each sample (1/delta_t)
    df['freq'] = 1 / df['delta_t']

    # Calculate overall statistics
    mean_freq = df['freq'].mean()
    median_freq = df['freq'].median()
    std_freq = df['freq'].std()
    min_freq = df['freq'].min()
    max_freq = df['freq'].max()

    # Calculate stability metrics
    cv = std_freq / mean_freq * 100  # Coefficient of Variation (%)

    # Calculate rolling statistics for detecting drift or instability
    df['rolling_freq'] = df['freq'].rolling(
        window=window_size, center=True).mean()
    df['rolling_std'] = df['freq'].rolling(
        window=window_size, center=True).std()

    # Find sections with highest variability
    df['rolling_cv'] = df['rolling_std'] / df['rolling_freq'] * 100
    max_var_section = df['rolling_cv'].nlargest(5)

    # Calculate expected samples vs actual samples
    total_time = df['time_delta'].max() - df['time_delta'].min()
    expected_samples = total_time * mean_freq
    actual_samples = len(df)
    sample_ratio = actual_samples / expected_samples

    # Detect outliers using IQR method
    Q1 = df['freq'].quantile(0.25)
    Q3 = df['freq'].quantile(0.75)
    IQR = Q3 - Q1
    outliers = df[(df['freq'] < (Q1 - 1.5 * IQR)) |
                  (df['freq'] > (Q3 + 1.5 * IQR))]

    # Create plots
    if plot:
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        file_name = os.path.basename(file_path).split('.')[0]
        output_dir = os.path.join(os.path.dirname(file_path), 'analysis')
        os.makedirs(output_dir, exist_ok=True)

        # Create a figure with multiple subplots
        fig, axs = plt.subplots(3, 1, figsize=(12, 16))

        # Plot 1: Sampling frequency over time
        axs[0].plot(df['time_delta'], df['freq'], 'b-', alpha=0.6)
        axs[0].plot(df['time_delta'], df['rolling_freq'], 'r-', linewidth=2)
        axs[0].axhline(y=mean_freq, color='g', linestyle='--',
                       label=f'Mean: {mean_freq:.2f} Hz')
        axs[0].axhline(y=40, color='m', linestyle=':', label='Target: 40 Hz')
        axs[0].set_title('Sampling Frequency Over Time')
        axs[0].set_xlabel('Time (s)')
        axs[0].set_ylabel('Frequency (Hz)')
        axs[0].legend()
        axs[0].grid(True)

        # Plot 2: Histogram of sampling frequencies
        axs[1].hist(df['freq'], bins=50, alpha=0.7)
        axs[1].axvline(x=mean_freq, color='r', linestyle='--',
                       label=f'Mean: {mean_freq:.2f} Hz (σ: {std_freq:.2f})')
        axs[1].axvline(x=40, color='g', linestyle=':', label='Target: 40 Hz')
        axs[1].set_title('Distribution of Sampling Frequencies')
        axs[1].set_xlabel('Frequency (Hz)')
        axs[1].set_ylabel('Count')
        axs[1].legend()
        axs[1].grid(True)

        # Plot 3: Sampling interval over time
        axs[2].plot(df['time_delta'], df['delta_t'] *
                    1000, 'b-', alpha=0.6)  # Convert to ms
        axs[2].axhline(y=1000/mean_freq, color='r', linestyle='--',
                       label=f'Mean: {1000/mean_freq:.2f} ms')
        axs[2].axhline(y=25, color='g', linestyle=':',
                       label='Target: 25 ms')
        axs[2].set_title('Sampling Interval Over Time')
        axs[2].set_xlabel('Time (s)')
        axs[2].set_ylabel('Interval (ms)')
        axs[2].legend()
        axs[2].grid(True)

        plt.tight_layout()
        plt.savefig(os.path.join(
            output_dir, f'{file_name}_analysis_{timestamp}.png'))
        plt.close()

        print(
            f"Plots saved to {os.path.join(output_dir, f'{file_name}_analysis_{timestamp}.png')}")

    # Create summary
    results = {
        'file': os.path.basename(file_path),
        'mean_frequency': mean_freq,
        'median_frequency': median_freq,
        'std_frequency': std_freq,
        'cv_percent': cv,
        'min_frequency': min_freq,
        'max_frequency': max_freq,
        'expected_samples': expected_samples,
        'actual_samples': actual_samples,
        'sample_ratio': sample_ratio,
        'outliers_count': len(outliers),
        'outliers_percent': len(outliers) / len(df) * 100,
        'highest_variability_sections': max_var_section.index.tolist(),
        'target_frequency': 40,
        'target_interval_ms': 25,
        'actual_interval_ms': 1000 / mean_freq
    }

    # Print summary
    print("\nSampling Rate Analysis Summary:")
    print(f"Mean Frequency: {mean_freq:.2f} Hz (Target: 40 Hz)")
    print(f"Mean Interval: {1000/mean_freq:.2f} ms (Target: 25 ms)")
    print(f"Stability (CV): {cv:.2f}% (lower is better)")
    print(f"Range: {min_freq:.2f} - {max_freq:.2f} Hz")
    print(f"Sample Capture Ratio: {sample_ratio:.2f}")
    print(f"Outliers: {len(outliers)} ({len(outliers)/len(df)*100:.2f}%)")

    if cv < 5:
        print("\nStability Assessment: EXCELLENT - Very stable sampling rate")
    elif cv < 10:
        print("\nStability Assessment: GOOD - Stable sampling rate with minor variations")
    elif cv < 15:
        print("\nStability Assessment: ACCEPTABLE - Sampling rate shows some instability")
    else:
        print("\nStability Assessment: POOR - Sampling rate is unstable")

    return results


def analyze_stability_by_segments(file_path, segment_size=500):
    """
    Analyze sampling rate stability by dividing data into segments.

    Args:
        file_path: Path to the CSV file
        segment_size: Number of samples per segment

    Returns:
        DataFrame with segment statistics
    """
    print(f"\nSegment Analysis for: {os.path.basename(file_path)}")
    df = pd.read_csv(file_path)

    # Calculate time differences
    df['delta_t'] = df['time_delta'].diff()
    df = df.iloc[1:].reset_index(drop=True)
    df['freq'] = 1 / df['delta_t']

    # Divide into segments
    segment_count = len(df) // segment_size
    segments = []

    for i in range(segment_count):
        start_idx = i * segment_size
        end_idx = start_idx + segment_size
        segment = df.iloc[start_idx:end_idx]

        if len(segment) > 10:  # Ensure segment has enough data
            mean_freq = segment['freq'].mean()
            std_freq = segment['freq'].std()
            cv = std_freq / mean_freq * 100

            segments.append({
                'segment': i+1,
                'start_time': segment['time_delta'].min(),
                'end_time': segment['time_delta'].max(),
                'mean_freq': mean_freq,
                'std_freq': std_freq,
                'cv_percent': cv,
                'min_freq': segment['freq'].min(),
                'max_freq': segment['freq'].max()
            })

    # Create segment summary dataframe
    segment_df = pd.DataFrame(segments)

    # Print segment summary
    pd.set_option('display.max_rows', None)
    pd.set_option('display.width', 200)

    print("\nSegment Analysis Summary:")
    print(segment_df[['segment', 'start_time',
          'end_time', 'mean_freq', 'cv_percent']])

    # Check if segments are consistent
    segment_cv = segment_df['mean_freq'].std(
    ) / segment_df['mean_freq'].mean() * 100
    print(f"\nSegment Consistency (CV between segments): {segment_cv:.2f}%")

    if segment_cv < 3:
        print("Segment Consistency Assessment: EXCELLENT - Very consistent across all segments")
    elif segment_cv < 5:
        print("Segment Consistency Assessment: GOOD - Consistent with minor variations between segments")
    elif segment_cv < 10:
        print(
            "Segment Consistency Assessment: ACCEPTABLE - Some variations between segments")
    else:
        print(
            "Segment Consistency Assessment: POOR - Significant variations between segments")

    return segment_df


def main():
    parser = argparse.ArgumentParser(
        description='Analyze sensor sampling rate stability')
    parser.add_argument('file', nargs='?', help='CSV file path to analyze')
    parser.add_argument('--all', action='store_true',
                        help='Analyze all CSV files in the directory')
    parser.add_argument('--window', type=int, default=100,
                        help='Window size for rolling statistics')
    parser.add_argument('--noplot', action='store_true',
                        help='Disable plot generation')
    parser.add_argument('--segment', type=int, default=500,
                        help='Segment size for stability analysis')

    args = parser.parse_args()

    # If no file specified and not --all, use the most recent file
    if not args.file and not args.all:
        csv_dir = os.path.join('data', 'csv')
        csv_files = glob.glob(os.path.join(csv_dir, '*.csv'))
        if csv_files:
            latest_file = max(csv_files, key=os.path.getctime)
            print(
                f"No file specified. Using most recent file: {os.path.basename(latest_file)}")
            files_to_analyze = [latest_file]
        else:
            print("No CSV files found in data/csv directory.")
            return
    # If --all flag is set, analyze all files
    elif args.all:
        csv_dir = os.path.join('data', 'csv')
        files_to_analyze = glob.glob(os.path.join(csv_dir, '*.csv'))
        if not files_to_analyze:
            print("No CSV files found in data/csv directory.")
            return
    # Otherwise use the specified file
    else:
        files_to_analyze = [args.file]

    # Analyze each file
    for file_path in files_to_analyze:
        analyze_sampling_rate(
            file_path, window_size=args.window, plot=not args.noplot)
        analyze_stability_by_segments(file_path, segment_size=args.segment)
        print("\n" + "="*80 + "\n")


if __name__ == "__main__":
    main()
