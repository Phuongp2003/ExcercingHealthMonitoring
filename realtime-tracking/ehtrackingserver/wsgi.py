import os
import threading
from server import app, start_tcp_server

# Only start TCP server when running via Gunicorn (not during reloads)
if os.environ.get("SERVER_RUNNING", "0") != "1":
    os.environ["SERVER_RUNNING"] = "1"

    # Start TCP server in a separate thread
    tcp_thread = threading.Thread(target=start_tcp_server)
    tcp_thread.daemon = True
    tcp_thread.start()

# Set Flask app to production mode
app.config['ENV'] = 'production'
app.config['DEBUG'] = False
app.config['TESTING'] = False

# This is the WSGI entry point for Gunicorn
application = app
