from app import app, start_tcp_server
import threading

if __name__ == '__main__':
    tcp_thread = threading.Thread(target=start_tcp_server)
    tcp_thread.start()
    app.run(host='0.0.0.0', port=5000)
