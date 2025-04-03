from sampling_analyzer import analyze_sampling_rate, analyze_stability_by_segments
from data_decoder import decode_sensor_data, save_decoded_data, decode_chunked_data
from flask import Flask, render_template, request, jsonify, send_from_directory, url_for
import matplotlib.pyplot as plt
import os
import socket
import threading
import time
import json
import datetime
import io
import base64
import matplotlib
import pandas as pd

matplotlib.use('Agg')  # Use non-interactive backend

# Configuration
TCP_IP = '0.0.0.0'  # Listen on all interfaces
# Get port values from environment variables if available
TCP_PORT = int(os.environ.get("TCP_PORT", 8889))
HTTP_PORT = int(os.environ.get("PORT", 8888))
DATA_DIR = 'data'
RAW_DIR = os.path.join(DATA_DIR, 'raw')
CSV_DIR = os.path.join(DATA_DIR, 'csv')
ANALYSIS_DIR = os.path.join(DATA_DIR, 'analysis')

# Ensure data directories exist
os.makedirs(RAW_DIR, exist_ok=True)
os.makedirs(CSV_DIR, exist_ok=True)
os.makedirs(ANALYSIS_DIR, exist_ok=True)

# Global state
connected_client = None
client_address = None
is_collecting = False
last_data = None
last_analysis = None

# Storage for chunked data sessions
chunks_store = {}

# Initialize Flask app
app = Flask(__name__)

# Lock for thread safety
lock = threading.Lock()


def timestamp_filename():
    """Generate a filename based on current timestamp"""
    return datetime.datetime.now().strftime("%Y%m%d_%H%M%S")


def handle_client_connection(client_socket, addr):
    """Handle TCP client connection"""
    global connected_client, client_address, is_collecting

    print(f"Connection established with {addr}")

    with lock:
        connected_client = client_socket
        client_address = addr

    try:
        while True:
            # Receive data from client
            data = client_socket.recv(1024)
            if not data:
                break

            message = data.decode('utf-8').strip()
            print(f"Received from {addr}: {message}")

            # Handle HELLO message
            if message == "HELLO":
                client_socket.send(b"WELCOME\n")
                print(f"Sent WELCOME to {addr}")
            # No longer handling Ping/Pong
    except Exception as e:
        print(f"Error handling client {addr}: {e}")
    finally:
        with lock:
            if connected_client == client_socket:
                connected_client = None
                client_address = None
                is_collecting = False
        client_socket.close()
        print(f"Connection closed with {addr}")


def tcp_server():
    """TCP server to handle sensor connections"""
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((TCP_IP, TCP_PORT))
    server.listen(5)
    print(f"TCP Server listening on {TCP_IP}:{TCP_PORT}")

    while True:
        client, addr = server.accept()
        client_thread = threading.Thread(
            target=handle_client_connection, args=(client, addr))
        client_thread.daemon = True
        client_thread.start()


def start_tcp_server():
    """Start TCP server in a separate thread"""
    server_thread = threading.Thread(target=tcp_server)
    server_thread.daemon = True
    server_thread.start()


def generate_analysis(csv_filename):
    """Generate analysis for the given CSV file and return plots as base64"""
    file_path = os.path.join(CSV_DIR, csv_filename)

    try:
        # Generate plots but don't save them to disk
        analysis_results = analyze_sampling_rate(file_path, plot=False)

        # Get the dataframe for custom plotting
        df = analyze_sampling_rate.get_dataframe(file_path)

        # Ensure we have valid mean frequency values (not NaN or undefined)
        mean_freq = analysis_results.get('mean_frequency', 0)
        std_freq = analysis_results.get('std_frequency', 0)

        # Check for NaN or None values and replace with defaults
        if mean_freq is None or pd.isna(mean_freq):
            mean_freq = 0
        if std_freq is None or pd.isna(std_freq):
            std_freq = 0

        # Create plots in memory
        plots = {}

        # Plot 1: Sampling frequency over time
        fig, ax = plt.subplots(figsize=(10, 6))
        window_size = 100  # Rolling window size

        ax.plot(df['time_delta'], df['freq'], 'b-', alpha=0.6)
        ax.plot(df['time_delta'], df['freq'].rolling(
            window=window_size, center=True).mean(), 'r-', linewidth=2)
        ax.axhline(y=mean_freq, color='g', linestyle='--',
                   label=f"Mean: {mean_freq:.2f} Hz")
        ax.axhline(y=40, color='m', linestyle=':',
                   label='Target: 40 Hz')  # Changed from 30 to 40
        ax.set_title('Sampling Frequency Over Time')
        ax.set_xlabel('Time (s)')
        ax.set_ylabel('Frequency (Hz)')
        ax.legend()
        ax.grid(True)

        # Convert to base64
        buffer = io.BytesIO()
        plt.tight_layout()
        plt.savefig(buffer, format='png')
        buffer.seek(0)
        plots['frequency_time'] = base64.b64encode(
            buffer.read()).decode('utf-8')
        plt.close()

        # Plot 2: Histogram of sampling frequencies
        fig, ax = plt.subplots(figsize=(10, 6))
        ax.hist(df['freq'], bins=50, alpha=0.7)
        ax.axvline(x=mean_freq, color='r', linestyle='--',
                   label=f"Mean: {mean_freq:.2f} Hz (σ: {std_freq:.2f})")
        ax.axvline(x=40, color='g', linestyle=':',
                   label='Target: 40 Hz')  # Changed from 30 to 40
        ax.set_title('Distribution of Sampling Frequencies')
        ax.set_xlabel('Frequency (Hz)')
        ax.set_ylabel('Count')
        ax.legend()
        ax.grid(True)

        # Convert to base64
        buffer = io.BytesIO()
        plt.tight_layout()
        plt.savefig(buffer, format='png')
        buffer.seek(0)
        plots['frequency_hist'] = base64.b64encode(
            buffer.read()).decode('utf-8')
        plt.close()

        # Plot 3: Sampling interval over time
        fig, ax = plt.subplots(figsize=(10, 6))
        ax.plot(df['time_delta'], df['delta_t'] *
                1000, 'b-', alpha=0.6)  # Convert to ms

        # Safely calculate the mean interval
        mean_interval = 0
        if mean_freq > 0:
            mean_interval = 1000/mean_freq

        ax.axhline(y=mean_interval, color='r', linestyle='--',
                   label=f"Mean: {mean_interval:.2f} ms")
        # Changed from 33.33 to 25
        ax.axhline(y=25, color='g', linestyle=':', label='Target: 25 ms')
        ax.set_title('Sampling Interval Over Time')
        ax.set_xlabel('Time (s)')
        ax.set_ylabel('Interval (ms)')
        ax.legend()
        ax.grid(True)

        # Convert to base64
        buffer = io.BytesIO()
        plt.tight_layout()
        plt.savefig(buffer, format='png')
        buffer.seek(0)
        plots['interval_time'] = base64.b64encode(
            buffer.read()).decode('utf-8')
        plt.close()

        # Get segment analysis
        try:
            segment_df = analyze_stability_by_segments(
                file_path, segment_size=500)
            # Create a JSON-serializable version of the segment data
            segments = segment_df.to_dict(orient='records')
        except Exception as e:
            print(f"Error in segment analysis: {e}")
            segments = []

        # Store analysis with plots and results
        analysis_data = {
            'filename': csv_filename,
            'plots': plots,
            'results': analysis_results,
            'segments': segments,
            'timestamp': timestamp_filename()
        }

        return analysis_data

    except Exception as e:
        import traceback
        print(f"Error in generate_analysis: {e}")
        traceback.print_exc()

        # Return basic analysis data with error information
        return {
            'filename': csv_filename,
            'error': str(e),
            'plots': {},
            'results': {'error': str(e)},
            'segments': [],
            'timestamp': timestamp_filename()
        }


# Add method to get dataframe to sampling_analyzer


def get_dataframe(file_path):
    """Extract dataframe with calculated sampling metrics from CSV"""
    df = pd.read_csv(file_path)
    df['delta_t'] = df['time_delta'].diff()
    df = df.iloc[1:].reset_index(drop=True)
    df['freq'] = 1 / df['delta_t']
    return df


# Add this method to the analyze_sampling_rate function
analyze_sampling_rate.get_dataframe = get_dataframe


def run_analysis(csv_filename):
    """Generate analysis in a background thread"""
    try:
        global last_analysis
        analysis_data = generate_analysis(csv_filename)
        with lock:
            last_analysis = analysis_data
        print(f"Analysis completed for {csv_filename}")
    except Exception as e:
        print(f"Error generating analysis: {e}")


@app.route('/data', methods=['POST'])
def receive_data():
    """Receive sensor data, supporting both regular and chunked transfers"""
    global last_data, last_analysis, chunks_store, is_collecting

    try:
        # Check if this is a chunked request
        is_chunked = 'X-Chunk-Index' in request.headers

        is_collecting = False
        if is_chunked:
            # Handle chunked data
            return handle_chunked_data()
        else:
            # Handle regular (non-chunked) data
            raw_data = request.data.decode('utf-8')
            return process_complete_data(raw_data)
    except Exception as e:
        print(f"Error processing data: {e}")
        return str(e), 500


def handle_chunked_data():
    """Handle a chunk of data from a multi-part transfer"""
    global chunks_store, last_data

    try:
        # Extract chunk information from headers
        chunk_index = int(request.headers.get('X-Chunk-Index', 0))
        total_chunks = int(request.headers.get('X-Total-Chunks', 1))
        total_samples = int(request.headers.get('X-Total-Samples', 0))
        start_index = int(request.headers.get('X-Chunk-Start-Index', 0))

        # Log received headers for debugging
        print(
            f"Received chunk with headers: index={chunk_index}, total={total_chunks}, samples={total_samples}")

        # Generate a unique session ID that's consistent across chunks from the same collection
        session_key = f"{total_samples}"
        if chunk_index == 0:
            session_id = f"session_{session_key}_{int(time.time())}"
            # Store the session ID in a file for other chunks to reference
            with open(os.path.join(RAW_DIR, f"session_{session_key}.id"), 'w') as f:
                f.write(session_id)
        else:
            # Read the session ID from file if it exists, otherwise generate temporary one
            try:
                with open(os.path.join(RAW_DIR, f"session_{session_key}.id"), 'r') as f:
                    session_id = f.read().strip()
            except FileNotFoundError:
                # Fallback if the session file is missing
                session_id = f"session_{session_key}_{int(time.time())}"

        # Get raw data from request
        raw_data = request.data.decode('utf-8')

        # Save raw chunk data for debugging
        timestamp = timestamp_filename()
        chunk_filename = f"{timestamp}_chunk{chunk_index}.raw"
        with open(os.path.join(RAW_DIR, chunk_filename), 'w') as f:
            f.write(raw_data)

        # Initialize or update session
        with lock:
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
                'data': raw_data,
                'chunk_index': chunk_index,
                'start_index': start_index
            })
            chunks_store[session_id]['received_chunks'] += 1
            received_count = chunks_store[session_id]['received_chunks']

        # Log status
        print(
            f"Received chunk {chunk_index+1}/{total_chunks} for session {session_id} ({received_count}/{total_chunks})")

        # Check if all chunks received in a separate thread to avoid blocking the response
        if received_count >= total_chunks:
            # Process in background to avoid blocking the response
            threading.Thread(
                target=process_complete_chunks,
                args=(session_id, session_key),
                daemon=True
            ).start()

        # Always return success immediately to avoid client hanging
        return f"Chunk {chunk_index+1}/{total_chunks} received", 200

    except Exception as e:
        # Log the full exception and stack trace
        import traceback
        print(f"Error in handle_chunked_data: {e}")
        traceback.print_exc()
        return f"Error: {str(e)}", 500


def process_complete_chunks(session_id, session_key):
    """Process complete chunks in a background thread"""
    global chunks_store, last_data

    try:
        with lock:
            # Get session data under lock
            if session_id not in chunks_store:
                print(f"Error: Session {session_id} no longer exists")
                return

            session_data = chunks_store[session_id]
            chunks_list = session_data['chunks']
            total_chunks = session_data['total_chunks']

        print(
            f"Processing {len(chunks_list)}/{total_chunks} chunks for session {session_id}")

        # Decode all chunks into a single DataFrame
        df = decode_chunked_data(chunks_list)

        # Generate complete filenames with consistent naming based on session
        session_timestamp = datetime.datetime.fromtimestamp(
            session_data['timestamp']
        ).strftime("%Y%m%d_%H%M%S")

        complete_raw = f"{session_timestamp}_complete.raw"
        complete_csv = f"{session_timestamp}.csv"

        # Save combined raw data for reference
        combined_raw = ""
        for chunk in sorted(chunks_list, key=lambda x: x['chunk_index']):
            combined_raw += chunk['data'] + "\n"

        raw_path = os.path.join(RAW_DIR, complete_raw)
        with open(raw_path, 'w') as f:
            f.write(combined_raw)
        print(f"Saved combined raw data to {raw_path}")

        # Save combined CSV data
        csv_path = os.path.join(CSV_DIR, complete_csv)
        save_decoded_data(df, csv_path)
        print(f"Saved combined CSV data to {csv_path}")

        # Verify the file was actually saved
        if not os.path.exists(csv_path):
            raise IOError(f"Failed to save CSV file to {csv_path}")

        # Update last data info
        with lock:
            last_data = {
                "filename": complete_csv,
                "samples": len(df),
                "timestamp": session_timestamp,
                "has_analysis": True,
                "chunked": True,
                "chunks": total_chunks
            }

            # Clean up this session
            if session_id in chunks_store:
                del chunks_store[session_id]

        # Clean up session tracking file
        try:
            os.remove(os.path.join(RAW_DIR, f"session_{session_key}.id"))
        except:
            pass  # Ignore errors cleaning up the session file

        # Run analysis in background
        analysis_thread = threading.Thread(
            target=run_analysis, args=(complete_csv,))
        analysis_thread.daemon = True
        analysis_thread.start()

        print(f"Successfully processed all chunks into {len(df)} samples")

    except Exception as e:
        import traceback
        print(f"Error processing complete chunks: {e}")
        traceback.print_exc()

        # Clean up session on error
        with lock:
            if session_id in chunks_store:
                del chunks_store[session_id]


def process_complete_data(raw_data):
    """Process complete (non-chunked) data"""
    global last_data

    # Generate filenames
    timestamp = timestamp_filename()
    raw_filename = f"{timestamp}.raw"
    csv_filename = f"{timestamp}.csv"

    print(f"Received regular data: {len(raw_data)} bytes")
    print(f"Saving as {raw_filename} and {csv_filename}")

    # Save raw data
    with open(os.path.join(RAW_DIR, raw_filename), 'w') as f:
        f.write(raw_data)

    # Process and save as CSV
    df = decode_sensor_data(raw_data)
    save_decoded_data(df, os.path.join(CSV_DIR, csv_filename))

    # Generate analysis in a separate thread
    analysis_thread = threading.Thread(
        target=run_analysis, args=(csv_filename,))
    analysis_thread.daemon = True
    analysis_thread.start()

    # Update last data info
    with lock:
        last_data = {
            "filename": csv_filename,
            "samples": len(df),
            "timestamp": timestamp,
            "has_analysis": True,
            "chunked": False
        }

    print(f"Successfully processed {len(df)} samples")
    return "OK", 200

# Routes


@app.route('/')
def index():
    """Render the main dashboard"""
    return render_template('index.html', is_collecting=is_collecting)


@app.route('/files')
def files():
    """List all available CSV files"""
    csv_files = [f for f in os.listdir(CSV_DIR) if f.endswith('.csv')]
    csv_files.sort(reverse=True)  # Most recent first
    return render_template('files.html', files=csv_files)


@app.route('/files/<filename>')
def download_file(filename):
    """Download a specific CSV file"""
    return send_from_directory(CSV_DIR, filename, as_attachment=True)


@app.route('/analysis/<filename>')
def view_analysis(filename):
    """View analysis for a specific CSV file"""
    try:
        analysis_data = generate_analysis(filename)
        return render_template('analysis.html', analysis=analysis_data)
    except Exception as e:
        return f"Error generating analysis: {str(e)}", 500


@app.route('/latest-analysis')
def latest_analysis():
    """View analysis for the most recent CSV file"""
    global last_data

    # First priority: Use the last_data information if available
    with lock:
        if last_data and "filename" in last_data:
            filename = last_data["filename"]
            if os.path.exists(os.path.join(CSV_DIR, filename)):
                return view_analysis(filename)

    # Second priority: Find the most recent file by modification time
    csv_files = [f for f in os.listdir(CSV_DIR) if f.endswith('.csv')]
    if not csv_files:
        return "No data files available for analysis", 404

    # Get file info with both creation and modification times
    file_info = []
    for f in csv_files:
        full_path = os.path.join(CSV_DIR, f)
        file_info.append({
            'filename': f,
            'ctime': os.path.getctime(full_path),
            'mtime': os.path.getmtime(full_path)
        })

    # Use modification time (mtime) to find the most recent file
    latest_file = max(file_info, key=lambda x: x['mtime'])['filename']
    print(f"Loading latest analysis for file: {latest_file}")

    return view_analysis(latest_file)


@app.route('/start', methods=['POST'])
def start_collection():
    """Start data collection on the sensor"""
    global is_collecting

    with lock:
        if not connected_client:
            return jsonify({"status": "error", "message": "No sensor connected"})
        if is_collecting:
            return jsonify({"status": "error", "message": "Already collecting data"})

        try:
            connected_client.send(b"START\n")
            is_collecting = True
            return jsonify({"status": "success"})
        except Exception as e:
            return jsonify({"status": "error", "message": str(e)})


@app.route('/stop', methods=['POST'])
def stop_collection():
    """Stop data collection on the sensor"""
    global is_collecting

    with lock:
        if not connected_client:
            return jsonify({"status": "error", "message": "No sensor connected"})
        if not is_collecting:
            return jsonify({"status": "error", "message": "Not collecting data"})

        try:
            connected_client.send(b"STOP\n")
            is_collecting = False
            return jsonify({"status": "success"})
        except Exception as e:
            return jsonify({"status": "error", "message": str(e)})


@app.route('/clear-connection', methods=['POST'])
def clear_connection():
    """Clear any existing connection"""
    global connected_client, client_address, is_collecting

    with lock:
        if connected_client:
            try:
                connected_client.close()
            except:
                pass
            connected_client = None
            client_address = None
            is_collecting = False

    return jsonify({"status": "success"})


@app.route('/status')
def get_status():
    """Get current system status"""
    global connected_client, client_address, is_collecting, last_data

    with lock:
        return jsonify({
            "connected": connected_client is not None,
            "client_address": client_address,
            "collecting": is_collecting,
            "last_data": last_data
        })


if __name__ == "__main__":
    # Start TCP server
    start_tcp_server()

    print(f"Starting Oxi Sensor Server:")
    print(f"- TCP Server on port {TCP_PORT}")
    print(f"- HTTP Server on port {HTTP_PORT}")
    print(f"- Data directory: {DATA_DIR}")

    # Start Flask server
    app.run(host='0.0.0.0', port=HTTP_PORT, debug=True, use_reloader=False)
