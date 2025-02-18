import socket
from flask import Flask, render_template
from flask_socketio import SocketIO
import logging
import matplotlib.pyplot as plt
import time
# from flask import request, jsonify  # Unused imports
# import threading  # Unused import
# from server_communication import start_server  # Unused import
from server import Server
from signal_processor import SignalProcessor
from config import SERVER_PORT

# Initialize Flask app and SocketIO
app = Flask(__name__)
socketio = SocketIO(app)

# Configure logging
logging.basicConfig(level=logging.DEBUG)

# Global variable to store ESP connection info
esp_info = {}

# def send_command_to_esp(command):
#     print(esp_info)
#     if 'ip' in esp_info and 'port' in esp_info:
#         try:
#             with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
#                 s.connect((esp_info['ip'], esp_info['port']))
#                 s.sendall(command.encode())
#                 s.close()
#         except ConnectionRefusedError:
#             logging.error("Connection refused by the ESP device")
#         except Exception as e:
#             logging.error(f"Error sending command to ESP: {e}")
#     else:
#         logging.error("ESP info not available")

# @app.route('/')
# def index():
#     return render_template('index.html')

if __name__ == "__main__":
    processor = SignalProcessor()
    server = Server(processor, esp_info)
    plt.ion()
    plt.show()
    try:
        server.listen_for_data()
    except Exception as e:
        logging.error(f"Error: {e}. Reconnecting in 5 seconds...")
        time.sleep(5)
        server.listen_for_data()
    socketio.run(app, host="0.0.0.0", port=5000)
