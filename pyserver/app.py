import socket
import numpy as np
from scipy.signal import find_peaks
from flask import Flask, jsonify, render_template
import threading
from flask_socketio import SocketIO, emit
import time

# Cấu hình server
SERVER_IP = "0.0.0.0"  # Lắng nghe mọi IP
SERVER_PORT = 8882
UDP_PORT = 8888  # Cổng UDP cho Broadcast

# Dữ liệu tín hiệu từ ESP8266
signal_data = []

# Hàm tính nhịp tim và nồng độ oxy
import numpy as np
from scipy import signal

def process_signal(ir_signal, red_signal, green_signal):
    ir_signal = np.array(ir_signal)
    red_signal = np.array(red_signal)
    green_signal = np.array(green_signal)
    
    try:
        # Tính BPM (Heart Rate)
        peaks, _ = find_peaks(ir_signal, distance=50)
        
        if len(peaks) < 2:
            return None, None
        
        peak_times = peaks / 100  # Tính thời gian giữa các đỉnh (giả sử lấy mẫu 100 Hz)
        avg_period = np.mean(np.diff(peak_times))
        bpm = 60 / avg_period
        
        # Tính SpO2 (Oxygen Saturation)
        ir_avg = np.mean(ir_signal)
        red_avg = np.mean(red_signal)
        
        R = red_avg / ir_avg
        spo2 = 110 - 25 * R
        
        return bpm, spo2
    except Exception as e:
        print(f"Error processing signal: {e}")
        return None, None

# Initialize Flask app and SocketIO
app = Flask(__name__)
socketio = SocketIO(app)

# Lắng nghe dữ liệu TCP từ ESP8266
def listen_for_data():
    global signal_data
    while True:
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.bind((SERVER_IP, SERVER_PORT))
                s.listen(1)
                print(f"Listening on {SERVER_IP}:{SERVER_PORT}...")
                while True:
                    conn, addr = s.accept()
                    print(f"Connected by {addr}")
                    with conn:
                        while True:
                            data = conn.recv(4096)
                            if not data:
                                break
                            print(f"Received: {data.decode()}")
                            for part in data.decode().split('\r\n'):
                                if part == "ping":
                                    conn.sendall("Ping received".encode())
                                    print("Ping acknowledged")
                                else:
                                    cleaned_data = part.strip()
                                    try:
                                        values = [int(x, 16) for x in cleaned_data.split(',')]
                                        ir_signal = values[0::3]
                                        red_signal = values[1::3]
                                        green_signal = values[2::3]
                                        heart_rate, oxygen_saturation = process_signal(ir_signal, red_signal, green_signal)
                                        if heart_rate is not None and oxygen_saturation is not None:
                                            print(f"Heart rate: {heart_rate} bpm, Oxygen saturation: {oxygen_saturation:.2f}%")
                                            socketio.emit('update', {'heart_rate': heart_rate, 'oxygen_saturation': oxygen_saturation})
                                    except ValueError as e:
                                        print(f"Error parsing data: {e}")
                    print(f"Disconnected from {addr}")
        except Exception as e:
            print(f"Error: {e}. Reconnecting in 5 seconds...")
            time.sleep(5)

# Lắng nghe UDP Broadcast từ ESP8266
def listen_for_discovery():
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp_socket:
        udp_socket.bind((SERVER_IP, UDP_PORT))
        print(f"Listening for UDP discovery on port {UDP_PORT}...")
        while True:
            data, addr = udp_socket.recvfrom(1024)
            if data.decode() == "DISCOVER_SERVER":
                server_ip = socket.gethostbyname(socket.gethostname())
                udp_socket.sendto(f"SERVER_IP:{server_ip}".encode(), addr)
                print(f"Sent server IP {server_ip} to {addr}")

# TODO: cần tạo thêm socket để cập nhật realtime cho biểu đồ nếu cần, hoặc áp dụng mấy thư viện gì đó cho hiện realtime (TKinter gì đó)

@app.route('/')
def index():
    return render_template('index.html')

# Chạy các luồng song song
threading.Thread(target=listen_for_data, daemon=True).start()
threading.Thread(target=listen_for_discovery, daemon=True).start()

# Run the Flask app with SocketIO
if __name__ == "__main__":
    socketio.run(app, host="0.0.0.0", port=5000)
