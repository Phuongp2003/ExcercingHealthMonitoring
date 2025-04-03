"""
WSGI configuration file for OxiSensor application.
This file serves as the entry point for WSGI-compatible web servers
like Gunicorn or uWSGI.
"""

from server import app as application
import os
import sys

# Add the current directory to the Python path to ensure
# we can import from server.py
current_dir = os.path.dirname(os.path.abspath(__file__))
if current_dir not in sys.path:
    sys.path.insert(0, current_dir)

# Import the Flask application from server.py

# This allows the application to be run with a WSGI server
# Example usage with Gunicorn:
# gunicorn -w 4 -b 0.0.0.0:8888 wsgi:application

if __name__ == "__main__":
    # When run directly, this will start the Flask development server
    application.run(host='0.0.0.0', port=8888, debug=True)
