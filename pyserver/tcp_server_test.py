import socket

HOST = 'oxinheartbeat.phuongy.works'  # Domain name to connect to
PORT = 8882  # Port to connect to

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    print(f'Connected to {HOST}:{PORT}')
    s.sendall(b'Hello, server')
    data = s.recv(1024)
    print(f'Received: {data.decode()}')
