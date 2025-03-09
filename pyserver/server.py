import socket
import time
import numpy as np
import zlib
import base64
import json

class Server:
    def __init__(self, processor, esp_info, csv_writer):
        self.processor = processor
        self.esp_info = esp_info
        self.csv_writer = csv_writer
        self.partial_data = ""
        self.ir_data = []
        self.red_data = []
        self.green_data = []
        self.csv_created = False  # Flag to check if CSV file is created

    # Xác nhận kết nối từ ESP, xử lý luồng nhận
    def listen_for_data(self):
        while True:
            try:
                with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                    s.bind(("0.0.0.0", 8882))
                    s.listen(1)
                    print("Listening on 0.0.0.0:8882...")
                    while True:
                        conn, addr = s.accept()
                        print(f"Connected by {addr}")
                        self.esp_info['ip'] = addr[0]
                        self.esp_info['port'] = addr[1]
                        with conn:
                            while True:
                                data = conn.recv(4096)
                                if not data:
                                    break
                                print(f"Data received: {data}")
                                if not self.csv_created:
                                    self.handle_csv_creation_request(data)
                                else:
                                    decompressed_data = zlib.decompress(base64.b64decode(data))
                                    self.handle_received_data(decompressed_data)
                        print(f"Disconnected from {addr}")
            except Exception as e:
                print(f"Error: {e}. Reconnecting in 5 seconds...")
                time.sleep(5)

    # Handle CSV creation request
    def handle_csv_creation_request(self, data):
        request = data.decode('latin1')
        if request == "CREATE_CSV":
            timestamp = time.strftime("%Y%m%d_%H%M%S")
            csv_filename = f"./sensor_data_{timestamp}.csv"
            with open(csv_filename, 'w', newline='') as csvfile:
                self.csv_writer = csv.writer(csvfile)
                self.csv_writer.writerow(["Timestamp", "IR Value", "Red Value"])
            self.csv_created = True
            print("CSV file created")
            # Send response to ESP
            response = "CSV created"
            conn.sendall(response.encode('latin1'))

    # Xử lý dữ liệu nhận
    def handle_received_data(self, data):
        data = json.loads(data.decode('latin1'))
        for packet in data:
            ir_value = packet[0]
            red_value = packet[1]
            self.ir_data.append(ir_value)
            self.red_data.append(red_value)
            # Write to CSV file
            if self.csv_writer:
                self.csv_writer.writerow([time.strftime("%Y-%m-%d %H:%M:%S"), ir_value, red_value])

    # Không sử dụng: mở server cho việc truy tìm của ESP
    def listen_for_discovery(self):
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp_socket:
            udp_socket.bind(("0.0.0.0", 8888))
            print("Listening for UDP discovery on port 8888...")
            while True:
                data, addr = udp_socket.recvfrom(1024)
                if data.decode() == "DISCOVER_SERVER":
                    server_ip = socket.gethostbyname(socket.gethostname())
                    udp_socket.sendto(f"SERVER_IP:{server_ip}".encode(), addr)
                    print(f"Sent server IP {server_ip} to {addr}")
