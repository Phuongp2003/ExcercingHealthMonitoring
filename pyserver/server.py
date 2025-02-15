import socket
import time

class Server:
    def __init__(self, processor, socketio, esp_info):
        self.processor = processor
        self.socketio = socketio
        self.esp_info = esp_info
        self.partial_data = ""

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
                                print(f"Data received: {data.decode()}")
                                self.handle_received_data(data.decode(), conn)
                        print(f"Disconnected from {addr}")
            except Exception as e:
                print(f"Error: {e}. Reconnecting in 5 seconds...")
                time.sleep(5)

    def handle_received_data(self, data, conn):
        self.partial_data += data
        if "\n" in self.partial_data:
            line, self.partial_data = self.partial_data.split("\n", 1)
        else:
            line = self.partial_data
            self.partial_data = ""
        try:
            if line.strip() == "ping":
                conn.sendall("pong".encode())
                print("Ping acknowledged")
            else:
                print(f"Handling data: {line.strip()}")
                self.handle_data(line.strip())
        except Exception as e:
            print(f"Error handling received data: {e}")

    def handle_data(self, data):
        try:
            values = [int(x, 16) for x in data.split(',')]
            ir_signal = values[0::3]
            red_signal = values[1::3]
            green_signal = values[2::3]
            print(f"IR Signal: {ir_signal}")
            print(f"Red Signal: {red_signal}")
            print(f"Green Signal: {green_signal}")

            heart_rate, oxygen_saturation = self.processor.process_signal(ir_signal, red_signal, green_signal)
            if heart_rate is not None and oxygen_saturation is not None:
                print(f"Heart rate: {heart_rate} bpm, Oxygen saturation: {oxygen_saturation:.2f}%")
                self.socketio.emit('update', {'heart_rate': heart_rate, 'oxygen_saturation': oxygen_saturation})
            else:
                print("No valid heart rate or oxygen saturation calculated.")
        except ValueError as e:
            print(f"Error parsing data: {e}")
        except Exception as e:
            print(f"Unexpected error: {e}")

    def handle_frequency_change(self, command):
        frequency = command.split(':')[1]
        print(f"Changing frequency to {frequency}")
        # Add any additional handling if needed

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
