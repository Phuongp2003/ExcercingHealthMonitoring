import csv
import time
import os
from flask import Flask, request, render_template, send_from_directory
from flask_socketio import SocketIO

app = Flask(__name__)
socketio = SocketIO(app)

BUFFER_SIZE = 1024
measurement_status = "Stopped"
clients = []
current_file = None
writer = None

def handle_client_connection(client_socket):
    global writer
    while True:
        data = client_socket.recv(BUFFER_SIZE).decode()
        if not data:
            break
        print("Received data:", data)
        if writer:
            for line in data.strip().split('\n'):
                writer.writerow(line.split(','))

@app.route('/')
def index():
    return render_template('index.html', ip=request.host, port=5000, status=measurement_status)

@app.route('/start')
def start_measurement():
    global measurement_status, current_file, writer
    current_file = open(f"data/data_{int(time.time())}.csv", mode='w', newline='')
    writer = csv.writer(current_file)
    writer.writerow(["Timestamp", "IR Value", "Red Value"])
    for client_socket in clients:
        client_socket.sendall(b"START\n")
    measurement_status = "Running"
    return "Measurement started"

@app.route('/stop')
def stop_measurement():
    global measurement_status, current_file, writer
    for client_socket in clients:
        client_socket.sendall(b"STOP\n")
    measurement_status = "Stopped"
    if current_file:
        current_file.close()
        current_file = None
        writer = None
    return "Measurement stopped"

@app.route('/files')
def list_files():
    files = os.listdir('data')
    return render_template('files.html', files=files)

@app.route('/files/<filename>')
def download_file(filename):
    return send_from_directory('data', filename)

@app.route('/status')
def get_status():
    return measurement_status

@socketio.on('connect')
def handle_connect():
    print('Client connected')
    clients.append(request.sid)

@socketio.on('disconnect')
def handle_disconnect():
    print('Client disconnected')
    clients.remove(request.sid)

@socketio.on('start_measurement')
def handle_start_measurement():
    start_measurement()

@socketio.on('stop_measurement')
def handle_stop_measurement():
    stop_measurement()

if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=5000)
