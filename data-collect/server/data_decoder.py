import pandas as pd
import numpy as np
import struct
from typing import List, Tuple, Union, BinaryIO


def decode_sensor_data(raw_data: Union[str, bytes]) -> pd.DataFrame:
    """
    Giải mã dữ liệu thô từ sensor thành DataFrame

    Parameters:
        raw_data (str or bytes): Dữ liệu thô nhận được từ sensor
                       Format: 
                       - Text: Dòng đầu: số lượng mẫu, Các dòng tiếp theo: timestamp,ir,red
                       - Binary: 4 bytes: số lượng mẫu, Tiếp theo: {timestamp(4), ir(4), red(4)} x n_samples

    Returns:
        pd.DataFrame: DataFrame với các cột:
            - timestamp: thời gian (ms)
            - ir: giá trị IR
            - red: giá trị RED
            - time_delta: thời gian tương đối so với mẫu đầu tiên (s)
    """

    # Check if input is binary data
    if isinstance(raw_data, bytes):
        return decode_binary_sensor_data(raw_data)
    else:
        return decode_text_sensor_data(raw_data)


def decode_text_sensor_data(raw_data: str) -> pd.DataFrame:
    """Decode sensor data in text format"""
    # Tách dữ liệu thành các dòng
    lines = raw_data.strip().split('\n')

    # Đọc số lượng mẫu từ dòng đầu
    n_samples = int(lines[0])

    # Tạo lists để lưu dữ liệu
    timestamps = []
    ir_values = []
    red_values = []

    # Đọc từng dòng dữ liệu
    for line in lines[1:n_samples+1]:
        timestamp, ir, red = map(int, line.split(','))
        timestamps.append(timestamp)
        ir_values.append(ir)
        red_values.append(red)

    # Tạo DataFrame
    df = pd.DataFrame({
        'timestamp': timestamps,
        'ir': ir_values,
        'red': red_values
    })

    # Tính thời gian tương đối (s)
    df['time_delta'] = (df['timestamp'] - df['timestamp'].iloc[0]) / 1000.0

    return df


def decode_binary_sensor_data(raw_data: bytes) -> pd.DataFrame:
    """Decode sensor data in binary format"""
    # Read number of samples (first 4 bytes)
    n_samples = struct.unpack('<I', raw_data[:4])[0]

    # Each sample is 12 bytes (4 for timestamp, 4 for IR, 4 for RED)
    timestamps = []
    ir_values = []
    red_values = []

    # Parse binary data for each sample
    offset = 4  # Start after the sample count
    for i in range(n_samples):
        if offset + 12 <= len(raw_data):
            timestamp = struct.unpack('<I', raw_data[offset:offset+4])[0]
            ir = struct.unpack('<I', raw_data[offset+4:offset+8])[0]
            red = struct.unpack('<I', raw_data[offset+8:offset+12])[0]

            timestamps.append(timestamp)
            ir_values.append(ir)
            red_values.append(red)

            offset += 12

    # Create DataFrame
    df = pd.DataFrame({
        'timestamp': timestamps,
        'ir': ir_values,
        'red': red_values
    })

    # Calculate relative time (s)
    if not df.empty:
        df['time_delta'] = (df['timestamp'] - df['timestamp'].iloc[0]) / 1000.0

    return df


def decode_chunked_data(chunks_data: List[dict]) -> pd.DataFrame:
    """
    Decode sensor data received in multiple chunks

    Parameters:
        chunks_data (List[dict]): List of dictionaries containing chunk information
                                Each dict should have:
                                - 'data': raw chunk data (str or bytes)
                                - 'chunk_index': index of this chunk
                                - 'start_index': starting sample index

    Returns:
        pd.DataFrame: DataFrame with combined data from all chunks
    """
    # Sort chunks by chunk_index to ensure correct order
    chunks_data.sort(key=lambda x: x['chunk_index'])

    # Initialize containers for combined data
    all_timestamps = []
    all_ir_values = []
    all_red_values = []

    # Process each chunk
    for chunk in chunks_data:
        # Decode this chunk
        if isinstance(chunk['data'], bytes):
            df_chunk = decode_binary_sensor_data(chunk['data'])
        else:
            df_chunk = decode_text_sensor_data(chunk['data'])

        # Add to combined datasets
        all_timestamps.extend(df_chunk['timestamp'].tolist())
        all_ir_values.extend(df_chunk['ir'].tolist())
        all_red_values.extend(df_chunk['red'].tolist())

    # Create combined DataFrame
    df = pd.DataFrame({
        'timestamp': all_timestamps,
        'ir': all_ir_values,
        'red': all_red_values
    })

    # Calculate relative time
    if not df.empty:
        df['time_delta'] = (df['timestamp'] - df['timestamp'].iloc[0]) / 1000.0

    return df


def save_decoded_data(df: pd.DataFrame, output_file: str):
    """
    Lưu dữ liệu đã giải mã vào file CSV

    Parameters:
        df (pd.DataFrame): DataFrame chứa dữ liệu đã giải mã
        output_file (str): Đường dẫn file output
    """
    df.to_csv(output_file, index=False)


def load_decoded_data(input_file: str) -> pd.DataFrame:
    """
    Đọc dữ liệu đã giải mã từ file CSV

    Parameters:
        input_file (str): Đường dẫn file input

    Returns:
        pd.DataFrame: DataFrame chứa dữ liệu đã giải mã
    """
    return pd.read_csv(input_file)


def handle_chunked_http_request(headers, body, chunks_store=None):
    """
    Handle HTTP request with chunked data

    Parameters:
        headers (dict): HTTP headers
        body (bytes or str): HTTP request body
        chunks_store (dict, optional): Storage for chunks across requests

    Returns:
        dict: Updated chunks_store
    """
    # Initialize chunks storage if not provided
    if chunks_store is None:
        chunks_store = {}

    # Extract chunk information from headers
    chunk_index = int(headers.get('X-Chunk-Index', 0))
    total_chunks = int(headers.get('X-Total-Chunks', 1))
    total_samples = int(headers.get('X-Total-Samples', 0))
    start_index = int(headers.get('X-Chunk-Start-Index', 0))

    # Generate a unique ID based on total samples and timestamp if not present
    session_id = headers.get('X-Session-ID',
                             f"session_{total_samples}_{int(time.time())}")

    # Initialize session entry if not exists
    if session_id not in chunks_store:
        chunks_store[session_id] = {
            'chunks': [],
            'total_chunks': total_chunks,
            'total_samples': total_samples,
            'received_chunks': 0,
            'timestamp': time.time()
        }

    # Add this chunk to the session
    chunks_store[session_id]['chunks'].append({
        'data': body,
        'chunk_index': chunk_index,
        'start_index': start_index
    })
    chunks_store[session_id]['received_chunks'] += 1

    # Check if all chunks are received
    if chunks_store[session_id]['received_chunks'] >= total_chunks:
        # All chunks received, process the complete dataset
        df = decode_chunked_data(chunks_store[session_id]['chunks'])

        # Save the completed dataset
        timestamp = int(time.time())
        filename = f"sensor_data_{timestamp}.csv"
        save_decoded_data(df, filename)

        print(f"Complete dataset received and saved as {filename}")

        # Clean up this session
        del chunks_store[session_id]
    else:
        print(
            f"Received chunk {chunk_index+1}/{total_chunks} for session {session_id}")

    return chunks_store


# Example usage:
if __name__ == "__main__":
    # Example raw data in text format
    example_data_text = """3
1680512345000,1000,2000
1680512345040,1100,2100
1680512345080,1200,2200
"""

    # Decode text data
    df_text = decode_sensor_data(example_data_text)
    print("Decoded text data:")
    print(df_text)

    # Example raw data in binary format
    example_data_binary = struct.pack('<I', 3) + \
        struct.pack('<III', 1680512345000, 1000, 2000) + \
        struct.pack('<III', 1680512345040, 1100, 2100) + \
        struct.pack('<III', 1680512345080, 1200, 2200)

    # Decode binary data
    df_binary = decode_sensor_data(example_data_binary)
    print("\nDecoded binary data:")
    print(df_binary)

    # Save to file
    save_decoded_data(df_text, "example_decoded_text.csv")
    save_decoded_data(df_binary, "example_decoded_binary.csv")

    # Load from file
    loaded_df_text = load_decoded_data("example_decoded_text.csv")
    loaded_df_binary = load_decoded_data("example_decoded_binary.csv")
    print("\nLoaded text data:")
    print(loaded_df_text)
    print("\nLoaded binary data:")
    print(loaded_df_binary)

    # Example for handling chunked data
    import time

    # Simulate chunks of data
    chunk1 = {
        'data': "2\n1000,100,200\n1025,110,210\n",
        'chunk_index': 0,
        'start_index': 0
    }

    chunk2 = {
        'data': "2\n1050,120,220\n1075,130,230\n",
        'chunk_index': 1,
        'start_index': 2
    }

    # Combine chunks
    df_combined = decode_chunked_data([chunk1, chunk2])
    print("Combined data from chunks:")
    print(df_combined)

    # Save combined data
    save_decoded_data(df_combined, "combined_chunks_example.csv")
