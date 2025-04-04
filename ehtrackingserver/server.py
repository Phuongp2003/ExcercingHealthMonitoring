import os
import socket
import threading
import time
import json
import logging
from datetime import datetime
from flask import Flask, render_template, jsonify, request

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler("server.log"),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger("OxiSensor")

# Configuration
TCP_HOST = '0.0.0.0'  # Listen on all available interfaces
TCP_PORT = 8889
HTTP_PORT = 8888

# Global variables
connected_devices = {}
is_collecting = False
last_data_info = None
tcp_server_socket = None
lock = threading.Lock()
device_status = None

# Flask app
app = Flask(__name__)

# Send TCP command to device


def send_tcp_command(command):
    global connected_devices, is_collecting, device_status

    if not connected_devices:
        logger.warning("No device connected to send command")
        return False

    # Get the first connected device (we'll support multiple devices later)
    device_addr = list(connected_devices.keys())[0]
    client_socket = connected_devices[device_addr]['socket']

    try:
        logger.info(f"Sending TCP command: {command} to {device_addr}")
        client_socket.send(f"{command}\n".encode('utf-8'))

        # Wait for response with timeout
        # Increased timeout for better reliability
        client_socket.settimeout(5.0)
        response = client_socket.recv(1024).decode('utf-8').strip()
        client_socket.settimeout(None)

        logger.info(f"Received response: {response}")

        # Handle STATUS command specifically
        if command == "STATUS":
            with lock:
                device_status = response
                logger.info(f"Updated device status to: {device_status}")
            return True

        # Handle START/STOP commands
        if command == "START":
            with lock:
                is_collecting = True
        elif command == "STOP":
            with lock:
                is_collecting = False

        return "OK" in response
    except Exception as e:
        logger.error(f"Error sending TCP command: {e}")
        # Check if the device is still connected, if not, mark it as disconnected
        if device_addr in connected_devices:
            try:
                # Try sending a small ping to check connection
                client_socket.send("\n".encode('utf-8'))
            except:
                logger.warning(
                    f"Device {device_addr} appears to be disconnected, removing")
                with lock:
                    if device_addr in connected_devices:
                        del connected_devices[device_addr]
        return False

# TCP server handler function


def handle_tcp_client(client_socket, addr):
    global connected_devices, device_status

    logger.info(f"New connection from {addr[0]}:{addr[1]}")

    # Add to connected devices
    with lock:
        connected_devices[addr] = {
            'socket': client_socket,
            'timestamp': datetime.now(),
            'last_data': None
        }

    try:
        # Set TCP keep-alive to detect dead connections
        client_socket.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
        if hasattr(socket, 'TCP_KEEPIDLE'):  # Linux only
            client_socket.setsockopt(
                socket.IPPROTO_TCP, socket.TCP_KEEPIDLE, 30)
        if hasattr(socket, 'TCP_KEEPINTVL'):
            client_socket.setsockopt(
                socket.IPPROTO_TCP, socket.TCP_KEEPINTVL, 15)
        if hasattr(socket, 'TCP_KEEPCNT'):
            client_socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPCNT, 5)

        while True:
            try:
                data = client_socket.recv(1024)
                if not data:
                    logger.info(
                        f"Client {addr} sent empty data, connection likely closed")
                    break

                message = data.decode('utf-8').strip()

                # Skip empty messages or messages with only newline characters
                if not message:
                    logger.debug(f"Ignoring empty message from {addr}")
                    continue

                logger.info(f"Received message from {addr}: {message}")

                if message == "HELLO":
                    # Send welcome message
                    client_socket.send("WELCOME\n".encode('utf-8'))
                    logger.info(f"Sent WELCOME to {addr}")
                elif message.startswith("OK:"):
                    # Acknowledgment from device, just log it
                    logger.info(f"Acknowledgment from {addr}: {message}")
                elif message.startswith("ERROR:") or "ERROR:" in message:
                    # Error message from device, just log it without responding
                    logger.info(f"Error from device {addr}: {message}")
                elif message.startswith("STATUS_INFO:"):
                    # Status info message from device after connection
                    with lock:
                        device_status = message[12:].strip()
                    logger.info(
                        f"Received status info from {addr}: {device_status}")
                    client_socket.send("OK: Status received\n".encode('utf-8'))
                else:
                    # Just echo back an OK for any other message
                    client_socket.send(
                        f"OK: {message} received\n".encode('utf-8'))
            except socket.timeout:
                logger.debug(f"Socket timeout from {addr}, continuing")
                continue
            except ConnectionResetError:
                logger.warning(f"Connection reset by {addr}")
                break
            except Exception as e:
                logger.error(f"Error receiving data from {addr}: {e}")
                break
    except Exception as e:
        logger.error(f"Error in TCP client handler: {e}")
    finally:
        # Clean up on disconnect
        logger.info(f"Client {addr} disconnected")
        with lock:
            if addr in connected_devices:
                del connected_devices[addr]
        try:
            client_socket.close()
        except:
            pass

# Start TCP server in a separate thread


def start_tcp_server():
    global tcp_server_socket

    tcp_server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    tcp_server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        tcp_server_socket.bind((TCP_HOST, TCP_PORT))
        tcp_server_socket.listen(5)
        logger.info(f"TCP Server listening on {TCP_HOST}:{TCP_PORT}")

        while True:
            client_sock, addr = tcp_server_socket.accept()
            client_thread = threading.Thread(
                target=handle_tcp_client, args=(client_sock, addr))
            client_thread.daemon = True
            client_thread.start()
    except Exception as e:
        logger.error(f"TCP server error: {e}")
    finally:
        if tcp_server_socket:
            tcp_server_socket.close()

# Flask routes


@app.route('/')
def index():
    return render_template('index.html', is_collecting=is_collecting)


@app.route('/status')
def status():
    with lock:
        return jsonify({
            'connected': len(connected_devices) > 0,
            'collecting': is_collecting,
            'last_data': last_data_info,
            'device_status': device_status
        })


@app.route('/start', methods=['POST'])
def start_collection():
    if len(connected_devices) == 0:
        return jsonify({'status': 'error', 'message': 'No device connected'})

    success = send_tcp_command("START")
    if success:
        return jsonify({'status': 'success'})
    else:
        return jsonify({'status': 'error', 'message': 'Failed to start collection'})


@app.route('/stop', methods=['POST'])
def stop_collection():
    if len(connected_devices) == 0:
        return jsonify({'status': 'error', 'message': 'No device connected'})

    success = send_tcp_command("STOP")
    if success:
        return jsonify({'status': 'success'})
    else:
        return jsonify({'status': 'error', 'message': 'Failed to stop collection'})


@app.route('/check-status', methods=['POST'])
def check_status():
    if len(connected_devices) == 0:
        return jsonify({'status': 'error', 'message': 'No device connected'})

    try:
        success = send_tcp_command("STATUS")
        if success:
            logger.info(f"Returning device status: {device_status}")
            return jsonify({
                'status': 'success',
                'device_status': device_status,
                'is_collecting': is_collecting,
                'is_processing': 'YES' in device_status and 'Processing: YES' in device_status,
                'buffer_ready': 'YES' in device_status and 'BufferReady: YES' in device_status
            })
        else:
            return jsonify({'status': 'error', 'message': 'Failed to check status - no response'})
    except Exception as e:
        logger.error(f"Exception in check_status: {e}")
        return jsonify({'status': 'error', 'message': f'Exception: {str(e)}'})


@app.route('/clear-connection', methods=['POST'])
def clear_connection():
    global connected_devices, is_collecting, device_status

    with lock:
        # Close all connected sockets
        for addr, device in connected_devices.items():
            try:
                device['socket'].close()
            except:
                pass

        connected_devices = {}
        is_collecting = False
        device_status = None

    logger.info("Cleared all connections")
    return jsonify({'status': 'success'})


@app.route('/data', methods=['POST'])
def receive_data():
    # Endpoint to receive data from ESP32
    global last_data_info

    try:
        data = request.json
        logger.debug(f"Received data: {data}")

        # Update the last data information
        with lock:
            timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

            # Create a data info object
            last_data_info = {
                'timestamp': timestamp,
                'heartRate': data.get('heartRate', 0),
                'oxygenLevel': data.get('oxygenLevel', 0),
                'actionClass': data.get('actionClass', -1),
                'confidence': data.get('confidence', 0)
            }

            # If we have a connected device, update its last data too
            if connected_devices:
                addr = list(connected_devices.keys())[0]
                connected_devices[addr]['last_data'] = last_data_info

        return jsonify({'status': 'success'})
    except Exception as e:
        logger.error(f"Error processing data: {e}")
        return jsonify({'status': 'error', 'message': str(e)})


if __name__ == '__main__':
    # Start TCP server in a thread
    tcp_thread = threading.Thread(target=start_tcp_server)
    tcp_thread.daemon = True
    tcp_thread.start()

    # Start Flask app
    logger.info(f"Starting web server on port {HTTP_PORT}")
    app.run(host='0.0.0.0', port=HTTP_PORT, debug=True, use_reloader=False)
