import socket
import time
import matplotlib.pyplot as plt
import numpy as np

class Server:
    def __init__(self, processor, esp_info):
        self.processor = processor
        self.esp_info = esp_info
        self.partial_data = ""
        self.ir_data = []
        self.red_data = []
        self.green_data = []

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
                                self.handle_received_data(data)
                        print(f"Disconnected from {addr}")
            except Exception as e:
                print(f"Error: {e}. Reconnecting in 5 seconds...")
                time.sleep(5)

    # Xử lý dữ liệu nhận
    def handle_received_data(self, data):
        self.partial_data += data.decode('latin1')
        while len(self.partial_data) >= 13:  # 1 byte marker + 3 * 4 bytes values
            if self.partial_data[0] == '\xAA':  # Check for the marker
                packet = self.partial_data[:13]
                self.partial_data = self.partial_data[13:]
                ir_value = int.from_bytes(packet[1:5].encode('latin1'), 'little')
                red_value = int.from_bytes(packet[5:9].encode('latin1'), 'little')
                green_value = int.from_bytes(packet[9:13].encode('latin1'), 'little')
                self.ir_data.append(ir_value)
                self.red_data.append(red_value)
                self.green_data.append(green_value)
                self.update_plot()
            else:
                self.partial_data = self.partial_data[1:]

    # Cập nhật đồ thị đã khởi tạo. NOTE: cần khởi tạo 1 plt trước
    def update_plot(self):
        plt.clf()
        plt.plot(self.ir_data[-100:], label='IR')
        plt.plot(self.red_data[-100:], label='Red')
        plt.plot(self.green_data[-100:], label='Green')
        plt.legend()
        plt.pause(0.01)

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
