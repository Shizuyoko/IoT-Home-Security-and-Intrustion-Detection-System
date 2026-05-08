from flask import Flask, request, jsonify
from flask_cors import CORS
import sqlite3
import time

app = Flask(name)
CORS(app)

app = Flask(__name__)
DB_NAME = "events.db"


# ---------------------------
# Database helper functions
# ---------------------------
def get_db():
    conn = sqlite3.connect(DB_NAME)
    conn.row_factory = sqlite3.Row
    return conn


def create_table():
    conn = get_db()
    conn.execute("""
                 CREATE TABLE IF NOT EXISTS events (
                                                       id INTEGER PRIMARY KEY AUTOINCREMENT,
                                                       device_id TEXT,
                                                       event_type TEXT,
                                                       location TEXT,
                                                       timestamp TEXT
                 );
                 """)
    conn.commit()
    conn.close()


def insert_event(device, event_type, location, timestamp):
    conn = get_db()
    conn.execute("""
                 INSERT INTO events (device_id, event_type, location, timestamp)
                 VALUES (?, ?, ?, ?)
                 """, (device, event_type, location, timestamp))
    conn.commit()
    conn.close()


def delete_event(event_id):
    conn = get_db()
    conn.execute("DELETE FROM events WHERE id = ?", (event_id,))
    conn.commit()
    conn.close()


# ---------------------------
# Routes
# ---------------------------

# Health check
@app.route("/health", methods=["GET"])
def health():
    return jsonify({"status": "server running"})


# POST event
@app.route("/api/events/", methods=["POST"])
def add_event():
    try:
        data = request.get_json()

        device = data["device_id"]
        event_type = data["event_type"]
        location = data["location"]
        timestamp = data.get("timestamp", str(int(time.time())))

        insert_event(device, event_type, location, timestamp)

        return jsonify({"message": "event stored"})

    except Exception:
        return jsonify({"error": "invalid json"}), 400


# GET events
@app.route("/api/events/", methods=["GET"])
def get_events():
    conn = get_db()
    cursor = conn.cursor()

    device_id = request.args.get("device_id")

    if device_id:
        cursor.execute(
            "SELECT * FROM events WHERE device_id = ? ORDER BY timestamp DESC",
            (device_id,)
        )
    else:
        cursor.execute(
            "SELECT * FROM events ORDER BY timestamp DESC"
        )

    rows = cursor.fetchall()
    conn.close()

    events = [dict(row) for row in rows]
    return jsonify(events)


# DELETE event
@app.route("/api/events/<int:event_id>", methods=["DELETE"])
def remove_event(event_id):
    delete_event(event_id)
    return jsonify({"message": "event deleted"})


# ---------------------------
# Main
# ---------------------------
if __name__ == "__main__":
    create_table()
    print("Server running on port 8080")
    app.run(host="0.0.0.0", port=8080)