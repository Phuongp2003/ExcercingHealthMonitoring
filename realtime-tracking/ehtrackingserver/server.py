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
command_status = {"command": None, "timestamp": None,
                  "completed": False, "error": None}
# Add variables to track the last status update from device
last_device_state_update = 0
device_reported_state = "UNKNOWN"
device_is_collecting = False
command_in_progress = False

# Flask app
app = Flask(__name__)

# --- Device timeout monitor thread ---
def device_timeout_monitor(timeout_seconds=15):
    global connected_devices, is_collecting, device_status, last_device_state_update, device_reported_state, device_is_collecting, last_data_info, command_in_progress
    while True:
        time.sleep(2)
        with lock:
            if connected_devices and last_device_state_update > 0:
                elapsed = time.time() - last_device_state_update
                if elapsed > timeout_seconds:
                    logger.warning(f"Device timeout: no data/status received for {timeout_seconds}s, marking as disconnected")
                    # Đóng socket và reset trạng thái
                    for addr, device in list(connected_devices.items()):
                        try:
                            device['socket'].close()
                        except:
                            pass
                    connected_devices.clear()
                    is_collecting = False
                    device_status = None
                    device_reported_state = "DISCONNECTED"
                    device_is_collecting = False
                    last_data_info = None
                    command_in_progress = False
                    last_device_state_update = 0

# Send TCP command to device with better state tracking


def send_tcp_command(command):
    global connected_devices, is_collecting, device_status, command_status, command_in_progress, device_reported_state, device_is_collecting

    if not connected_devices:
        logger.warning("No device connected to send command")
        return False

    # Get the first connected device
    device_addr = list(connected_devices.keys())[0]
    client_socket = connected_devices[device_addr]['socket']

    try:
        logger.info(f"Sending TCP command: {command} to {device_addr}")

        # Track command status for verification
        with lock:
            command_status = {
                "command": command,
                "timestamp": time.time(),
                "completed": False,
                "error": None
            }
            command_in_progress = True

        client_socket.send(f"{command}\n".encode('utf-8'))

        # Wait for response with timeout
        client_socket.settimeout(5.0)
        try:
            response = client_socket.recv(1024).decode('utf-8').strip()
            client_socket.settimeout(None)
            logger.info(f"Received response: {response}")

            # Always update device status with latest response
            with lock:
                device_status = response
                logger.info(f"Updated device status to: {device_status}")

                # Handle START/STOP command success cases from direct response
                if command == "START" and "OK:" in response:
                    command_status["completed"] = True

                    # We'll still wait for a STATUS_INFO update to confirm the actual device state
                    # Don't update is_collecting yet until confirmed by device status
                elif command == "STOP" and "OK:" in response:
                    command_status["completed"] = True

                    # We'll still wait for a STATUS_INFO update to confirm the actual device state
                    # Don't update is_collecting yet until confirmed by device status
                elif "OK:" in response:
                    command_status["completed"] = True

                # Extract state information from response if available
                if "Current State:" in response:
                    try:
                        state_name = response.split("Current State:")[
                            1].split(",")[0].strip()
                        device_reported_state = state_name
                        device_is_collecting = (state_name == "COLLECTING")
                        last_device_state_update = time.time()
                    except Exception as e:
                        logger.error(f"Error parsing state from response: {e}")
        except socket.timeout:
            logger.warning(f"Timeout waiting for response to {command}")
            client_socket.settimeout(None)
            # We'll rely on subsequent STATUS_INFO updates to determine if command succeeded

        return command_status["completed"]
    except Exception as e:
        logger.error(f"Error sending TCP command: {e}")
        with lock:
            command_status["error"] = str(e)
            command_in_progress = False

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
    global connected_devices, device_status, device_reported_state, device_is_collecting, last_device_state_update

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

                        # Extract state and collection status directly from the device status update
                        if "Current State:" in device_status:
                            try:
                                state_name = device_status.split("Current State:")[
                                    1].split(",")[0].strip()
                                device_reported_state = state_name
                                # Consider both COLLECTING and PROCESSING states as "collecting data"
                                device_is_collecting = (
                                    state_name == "COLLECTING" or state_name == "PROCESSING")
                                last_device_state_update = time.time()

                                # Update the server's tracking to match the device's actual state
                                if is_collecting != device_is_collecting:
                                    logger.info(
                                        f"Syncing server state with device: {is_collecting} -> {device_is_collecting}")
                                    is_collecting = device_is_collecting

                                # If there's a command in progress, this status update might complete it
                                if command_in_progress and not command_status["completed"]:
                                    if (command_status["command"] == "START" and device_is_collecting) or \
                                       (command_status["command"] == "STOP" and not device_is_collecting):
                                        logger.info(
                                            f"Command {command_status['command']} completed through status update")
                                        command_status["completed"] = True
                                        command_in_progress = False
                            except Exception as e:
                                logger.error(
                                    f"Error parsing state from status update: {e}")

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
        # Nếu không còn thiết bị kết nối, trả về trạng thái disconnected và không trả về dữ liệu cũ
        if not connected_devices:
            return jsonify({
                'connected': False,
                'collecting': False,
                'last_data': None,
                'device_status': None,
                'current_state': "DISCONNECTED",
                'time_since_update': -1,
                'command_in_progress': False,
                'server_tracking_state': is_collecting
            })
        # ... existing code ...
        current_state = device_reported_state if device_reported_state != "UNKNOWN" else "Unknown"
        collecting_state = device_is_collecting
        time_since_update = time.time() - last_device_state_update if last_device_state_update > 0 else -1
        return jsonify({
            'connected': len(connected_devices) > 0,
            'collecting': collecting_state,
            'last_data': last_data_info,
            'device_status': device_status,
            'current_state': current_state,
            'time_since_update': time_since_update,
            'command_in_progress': command_in_progress,
            'server_tracking_state': is_collecting
        })


@app.route('/start', methods=['POST'])
def start_collection():
    global command_in_progress, device_is_collecting, is_collecting

    if len(connected_devices) == 0:
        return jsonify({'status': 'error', 'message': 'No device connected'})

    # First check current status to avoid unnecessary commands
    try:
        # Don't send command if already in the correct state according to device
        if device_is_collecting:
            logger.info("Device already collecting, no need to send START")
            return jsonify({'status': 'success', 'message': 'Already collecting'})

        # Another command is already in progress
        if command_in_progress:
            return jsonify({'status': 'error', 'message': 'Another command is already in progress'})
    except Exception as e:
        logger.warning(f"Error in pre-check before START: {e}")

    # Send START command
    success = send_tcp_command("START")

    # Wait for status update from device to confirm state change
    # We'll wait for up to 5 seconds for a status update to confirm
    start_time = time.time()
    timeout = 5.0
    verification_attempts = 0

    logger.info("Waiting for device status update to confirm START command...")

    while time.time() - start_time < timeout:
        time.sleep(0.5)  # Brief pause between checks
        verification_attempts += 1

        # The device should send a STATUS_INFO update soon
        with lock:
            # If we've received a status update since sending the command
            if last_device_state_update > command_status["timestamp"]:
                if device_is_collecting:
                    logger.info(
                        f"Device confirmed state change to COLLECTING after {verification_attempts} attempts")
                    is_collecting = True  # Update server state to match device
                    command_in_progress = False
                    return jsonify({'status': 'success'})
                else:
                    # State update received but device still not collecting
                    if verification_attempts >= 5:  # Give it a few attempts
                        logger.warning(
                            "Device reported status after START but still not collecting")
                        command_in_progress = False
                        return jsonify({'status': 'error', 'message': 'Command sent but device did not start collecting'})

    # If we get here, we timed out waiting for device confirmation
    with lock:
        # Check one more time in case an update came in during jsonify
        if device_is_collecting:
            logger.info("Device state eventually changed to COLLECTING")
            is_collecting = True
            command_in_progress = False
            return jsonify({'status': 'success'})

        command_in_progress = False
        error_msg = "Timed out waiting for device to confirm state change"
        logger.warning(error_msg)

        # Force a status check to see what happened
        send_tcp_command("STATUS")

        return jsonify({
            'status': 'pending',
            'message': 'Command sent but waiting for device confirmation'
        })


@app.route('/stop', methods=['POST'])
def stop_collection():
    global command_in_progress, device_is_collecting, is_collecting

    if len(connected_devices) == 0:
        return jsonify({'status': 'error', 'message': 'No device connected'})

    # First check current status to avoid unnecessary commands
    try:
        # Don't send command if already in the correct state according to device
        if not device_is_collecting:
            logger.info("Device already idle, no need to send STOP")
            return jsonify({'status': 'success', 'message': 'Already stopped'})

        # Another command is already in progress
        if command_in_progress:
            return jsonify({'status': 'error', 'message': 'Another command is already in progress'})
    except Exception as e:
        logger.warning(f"Error in pre-check before STOP: {e}")

    # Send STOP command
    success = send_tcp_command("STOP")

    # Wait for status update from device to confirm state change
    # We'll wait for up to 5 seconds for a status update to confirm
    start_time = time.time()
    timeout = 5.0
    verification_attempts = 0

    logger.info("Waiting for device status update to confirm STOP command...")

    while time.time() - start_time < timeout:
        time.sleep(0.5)  # Brief pause between checks
        verification_attempts += 1

        # The device should send a STATUS_INFO update soon
        with lock:
            # If we've received a status update since sending the command
            if last_device_state_update > command_status["timestamp"]:
                if not device_is_collecting:
                    logger.info(
                        f"Device confirmed state change to IDLE after {verification_attempts} attempts")
                    is_collecting = False  # Update server state to match device
                    command_in_progress = False
                    return jsonify({'status': 'success'})
                else:
                    # State update received but device still collecting
                    if verification_attempts >= 5:  # Give it a few attempts
                        logger.warning(
                            "Device reported status after STOP but still collecting")
                        command_in_progress = False
                        return jsonify({'status': 'error', 'message': 'Command sent but device did not stop collecting'})

    # If we get here, we timed out waiting for device confirmation
    with lock:
        # Check one more time in case an update came in during jsonify
        if not device_is_collecting:
            logger.info("Device state eventually changed to IDLE")
            is_collecting = False
            command_in_progress = False
            return jsonify({'status': 'success'})

        command_in_progress = False
        error_msg = "Timed out waiting for device to confirm state change"
        logger.warning(error_msg)

        # Force a status check to see what happened
        send_tcp_command("STATUS")

        return jsonify({
            'status': 'pending',
            'message': 'Command sent but waiting for device confirmation'
        })


@app.route('/check-status', methods=['POST'])
def check_status():
    if len(connected_devices) == 0:
        return jsonify({'status': 'error', 'message': 'No device connected'})

    try:
        success = send_tcp_command("STATUS")
        if success:
            # Parse current state from status response
            current_state = "Unknown"
            is_actually_collecting = False

            if device_status and "Current State:" in device_status:
                state_part = device_status.split("Current State:")[
                    1].split(",")[0].strip()
                current_state = state_part
                # Consider both COLLECTING and PROCESSING states as active measurement
                is_actually_collecting = (
                    state_part == "COLLECTING" or state_part == "PROCESSING")

                # Update our local tracking to match device
                with lock:
                    if is_collecting != is_actually_collecting:
                        logger.warning(
                            f"State mismatch detected: server thinks {is_collecting}, device reports {is_actually_collecting}")
                        is_collecting = is_actually_collecting

            logger.info(
                f"Returning device status: {device_status}, current state: {current_state}")
            return jsonify({
                'status': 'success',
                'device_status': device_status,
                'is_collecting': is_actually_collecting,
                'is_processing': 'Processing: YES' in device_status if device_status else False,
                'current_state': current_state,
                'server_thinks_collecting': is_collecting
            })
        else:
            return jsonify({'status': 'error', 'message': 'Failed to check status - no response'})
    except Exception as e:
        logger.error(f"Exception in check_status: {e}")
        return jsonify({'status': 'error', 'message': f'Exception: {str(e)}'})


@app.route('/list-states', methods=['POST'])
def list_states():
    if len(connected_devices) == 0:
        return jsonify({'status': 'error', 'message': 'No device connected'})

    try:
        success = send_tcp_command("STATES")
        if success:
            return jsonify({
                'status': 'success',
                'states_info': device_status
            })
        else:
            return jsonify({'status': 'error', 'message': 'Failed to get states list - no response'})
    except Exception as e:
        logger.error(f"Exception in list_states: {e}")
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

            # Map activity class to descriptive name
            activity_name = "Unknown"
            activity_class = data.get('actionClass', -1)
            if activity_class == 0:
                activity_name = "Resting after exercise"
            elif activity_class == 1:
                activity_name = "Sitting"
            elif activity_class == 2:
                activity_name = "Walking"

            # Create a data info object with activity name
            last_data_info = {
                'timestamp': timestamp,
                'heartRate': data.get('heartRate', 0),
                'oxygenLevel': data.get('oxygenLevel', 0),
                'actionClass': activity_class,
                'activityName': activity_name,
                'confidence': data.get('confidence', 0),
                'deviceState': data.get('deviceState', 'Unknown')
            }

            # Update the collecting flag based on the device state in the data
            if 'isCollecting' in data:
                is_collecting = data.get('isCollecting', False)
                logger.info(
                    f"Updated is_collecting to {is_collecting} based on data payload")

            # If we have a connected device, update its last data too
            if connected_devices:
                addr = list(connected_devices.keys())[0]
                connected_devices[addr]['last_data'] = last_data_info

        return jsonify({'status': 'success'})
    except Exception as e:
        logger.error(f"Error processing data: {e}")
        return jsonify({'status': 'error', 'message': str(e)})


@app.route('/static/<path:path>')
def serve_static(path):
    return app.send_static_file(os.path.join('static', path))


# Create a directory for static files if it doesn't exist
if not os.path.exists('static'):
    os.makedirs('static')
if not os.path.exists('static/css'):
    os.makedirs('static/css')
if not os.path.exists('static/js'):
    os.makedirs('static/js')


if __name__ == '__main__':
    # Start device timeout monitor
    timeout_thread = threading.Thread(target=device_timeout_monitor, args=(15,))
    timeout_thread.daemon = True
    timeout_thread.start()

    # Start TCP server in a thread
    tcp_thread = threading.Thread(target=start_tcp_server)
    tcp_thread.daemon = True
    tcp_thread.start()

    # Start Flask app
    logger.info(f"Starting web server on port {HTTP_PORT}")
    app.run(host='0.0.0.0', port=HTTP_PORT, debug=True, use_reloader=False)
