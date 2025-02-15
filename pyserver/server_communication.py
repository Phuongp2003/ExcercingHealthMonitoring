import socket
import threading
import logging
from config import SERVER_PORT

# Configure logging
logging.basicConfig(level=logging.DEBUG)

def handle_client_connection(client_socket):
    try:
        while True:
            message = client_socket.recv(1024).decode()
            if message == "ping":
                client_socket.sendall("pong".encode())
                logging.info("Ping acknowledged")
            else:
                logging.info(f"Received: {message}")
    except Exception as e:
        logging.error(f"Client connection error: {e}")
    finally:
        client_socket.close()
        logging.info("Client disconnected")

def start_server(server_port=SERVER_PORT):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(('0.0.0.0', server_port))
    server_socket.listen(5)
    logging.info(f"Server listening on port {server_port}")

    while True:
        client_socket, addr = server_socket.accept()
        logging.info(f"Connected by {addr}")
        threading.Thread(target=handle_client_connection, args=(client_socket,)).start()
