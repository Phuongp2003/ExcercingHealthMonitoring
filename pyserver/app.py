import socket
from flask import Flask, render_template, send_from_directory, request
from flask_socketio import SocketIO
import logging
import time
from server import Server
from signal_processor import SignalProcessor
from config import SERVER_PORT
import csv
import os

# Initialize Flask app and SocketIO
app = Flask(__name__)
socketio = SocketIO(app)

# Configure logging
logging.basicConfig(level=logging.DEBUG)

# Global variable to store ESP connection info
esp_info = {}

@app.route('/')
def index():
    files = os.listdir('data')
    return render_template('index.html', files=files)

@app.route('/download/<filename>')
def download_file(filename):
    return send_from_directory('data', filename)

@app.route('/data', methods=['POST'])
def receive_data():
    data = request.json
    if not data:
        return "Invalid data", 400
    for entry in data:
        timestamp = time.strftime("%d/%b/%Y %H:%M:%S") + f".{int(time.time() * 1000) % 1000}"
        ir_value = entry[1]
        red_value = entry[2]
        if csv_writer:
            csv_writer.writerow([timestamp, ir_value, red_value])
    return "Data received", 200

@app.route('/data/create_csv', methods=['POST'])
def create_csv():
    global csv_file, csv_writer
    timestamp = time.strftime("%Y%m%d_%H%M%S")
    csv_file = open(f"data/sensor_data_{timestamp}.csv", mode='w', newline='')
    csv_writer = csv.writer(csv_file)
    csv_writer.writerow(["time", "irvalue", "redvalue"])
    return "CSV created", 200

# Global variable for CSV file write stream
csv_file = None
csv_writer = None

if __name__ == "__main__":
    # Initialize server
    processor = SignalProcessor()
    server = Server(processor, esp_info, csv_writer)

    socketio.run(app, host='0.0.0.0', port=5000, allow_unsafe_werkzeug=True)
    try:
        server.listen_for_data()
    except Exception as e:
        logging.error(f"Error: {e}. Reconnecting in 5 seconds...")
        time.sleep(5)
        server.listen_for_data()
    finally:
        if csv_file:
            csv_file.close()
