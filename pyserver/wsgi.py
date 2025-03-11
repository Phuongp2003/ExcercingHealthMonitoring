from app import app, socketio
import threading

# Start the TCP server in a separate thread
tcp_thread = threading.Thread(target=start_tcp_server)
tcp_thread.start()

if __name__ == "__main__":
    socketio.run(app)
