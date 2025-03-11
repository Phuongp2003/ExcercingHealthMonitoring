import socket
import csv
import time
import os
from flask import Flask, request, render_template, send_from_directory
import threading

app = Flask(__name__)

TCP_PORT = 8888
BUFFER_SIZE = 1024
measurement_status = "Stopped"
clients = []
current_file = None
writer = None

def get_ip_address():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # doesn't even have to be reachable
        s.connect(('10.254.254.254', 1))
        ip = s.getsockname()[0]
    except Exception:
        ip = '0.0.0.0'
    finally:
        s.close()
    return ip

TCP_IP = get_ip_address()

def start_tcp_server():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((TCP_IP, TCP_PORT))
    server_socket.listen(5)
    print("TCP server listening on", TCP_IP, ":", TCP_PORT)
    while True:
        client_socket, addr = server_socket.accept()
        print("Connection from:", addr)
        clients.append(client_socket)
        threading.Thread(target=handle_client_connection, args=(client_socket,)).start()

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
    return render_template('index.html', ip=TCP_IP, port=TCP_PORT, status=measurement_status)

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
