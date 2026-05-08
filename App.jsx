import { useEffect, useState } from "react";

const EVENTS_URL = "https://true-lion-mansion.loca.lt/api/events/";

const STREAM_URL = "https://exceed-jacket-marathon-restrict.trycloudflare.com/stream";

const API_BASE = "https://true-lion-mansion.loca.lt";
const SNAPSHOTS_URL = `${API_BASE}/api/images`;

const ROLES = {
  Admin: {
    canSeeEvents: true,
    canSeeSnapshots: true,
    canControlSystem: true,
  },
  User: {
    canSeeEvents: true,
    canSeeSnapshots: true,
    canControlSystem: false,
  },
  Guest: {
    canSeeEvents: false,
    canSeeSnapshots: false,
    canControlSystem: false,
  },
};

function App() {
  const [loggedIn, setLoggedIn] = useState(false);
  const [armed, setArmed] = useState(false);
  const [role, setRole] = useState("");

  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [error, setError] = useState("");

  const [events, setEvents] = useState([]);
  const [loadingEvents, setLoadingEvents] = useState(false);
  const [backendError, setBackendError] = useState("");

  const [snapshots, setSnapshots] = useState([]);
  const [loadingSnapshots, setLoadingSnapshots] = useState(false);
  const [snapshotError, setSnapshotError] = useState("");

  const currentPermissions = ROLES[role] || ROLES.Guest;

  const handleLogin = () => {
    const cleanUsername = username.trim();
    const matchedRole = Object.keys(ROLES).find(
      (roleName) => roleName.toLowerCase() === cleanUsername.toLowerCase()
    );

    if (!matchedRole) {
      setError("Please enter a valid role: Admin, User, or Guest.");
      return;
    }

    if (password !== "IoT") {
      setError("Wrong password (Hint: IoT)");
      return;
    }

    setRole(matchedRole);
    setLoggedIn(true);
    setError("");
  };

  const logout = () => {
    setLoggedIn(false);
    setArmed(false);
    setRole("");
    setUsername("");
    setPassword("");
    setError("");
    setEvents([]);
    setSnapshots([]);
    setBackendError("");
    setSnapshotError("");
  };

  const formatEventType = (type) => {
    if (!type) return "Unknown Event";

    return type
      .replaceAll("_", " ")
      .replace(/\b\w/g, (char) => char.toUpperCase());
  };

  const formatTimestamp = (timestamp) => {
    if (!timestamp) return "No time";
    if (timestamp === "now") return "Now";

    if (!isNaN(timestamp)) {
      const num = Number(timestamp);

      if (num > 1000000000) {
        const date = new Date(num * 1000);
        if (!isNaN(date.getTime())) {
          return date.toLocaleString();
        }
      }

      return String(timestamp);
    }

    const date = new Date(timestamp);
    if (!isNaN(date.getTime())) {
      return date.toLocaleString();
    }

    return String(timestamp);
  };

  const fetchEvents = async (showLoading = false) => {
    try {
      if (showLoading) {
        setLoadingEvents(true);
      }

      setBackendError("");

      const response = await fetch(EVENTS_URL);

      if (!response.ok) {
        throw new Error(`Failed to fetch events: ${response.status}`);
      }

      const data = await response.json();
      setEvents(data);

      const latestStatusEvent = data.find(
        (event) =>
          event.event_type === "system_armed" ||
          event.event_type === "system_disarmed"
      );

      if (latestStatusEvent) {
        setArmed(latestStatusEvent.event_type === "system_armed");
      }
    } catch (err) {
      console.error("Fetch events error:", err);
      setBackendError("Could not connect to backend.");
    } finally {
      if (showLoading) {
        setLoadingEvents(false);
      }
    }
  };

  const fetchSnapshots = async (showLoading = false) => {
    try {
      if (showLoading) {
        setLoadingSnapshots(true);
      }

      setSnapshotError("");

      const response = await fetch(SNAPSHOTS_URL);

      if (!response.ok) {
        throw new Error(`Failed to fetch snapshots: ${response.status}`);
      }

      const data = await response.json();
      setSnapshots(data);
    } catch (err) {
      console.error("Fetch snapshots error:", err);
      setSnapshotError("Could not load snapshots.");
    } finally {
      if (showLoading) {
        setLoadingSnapshots(false);
      }
    }
  };

  useEffect(() => {
    if (!loggedIn) return;

    if (currentPermissions.canSeeEvents || currentPermissions.canControlSystem) {
      fetchEvents(true);
    }

    if (currentPermissions.canSeeSnapshots) {
      fetchSnapshots(true);
    }

    const interval = setInterval(() => {
      if (currentPermissions.canSeeEvents || currentPermissions.canControlSystem) {
        fetchEvents(false);
      }

      if (currentPermissions.canSeeSnapshots) {
        fetchSnapshots(false);
      }
    }, 5000);

    return () => clearInterval(interval);
  }, [loggedIn, role]);

  if (!loggedIn) {
    return (
      <div className="login-page">
        <div className="login-box">
          <h1>IoT Security System</h1>
          <p className="login-sub">Enter your credentials</p>

          <input
            type="text"
            placeholder="Username: Admin, User, or Guest"
            list="role-suggestions"
            value={username}
            onChange={(e) => setUsername(e.target.value)}
          />

          <datalist id="role-suggestions">
            <option value="Admin" />
            <option value="User" />
            <option value="Guest" />
          </datalist>

          <input
            type="password"
            placeholder="Password (IoT)"
            value={password}
            onChange={(e) => setPassword(e.target.value)}
          />

          {error && <p className="error">{error}</p>}

          <button className="login-btn" onClick={handleLogin}>
            Log In
          </button>
        </div>
      </div>
    );
  }

  return (
    <div className="dash">
      <header className="dash-header">
        <div>
          <h1 className="dash-title">IoT Security Dashboard</h1>
          <p className="dash-subtitle">
            Logged in as {role}
            {backendError && currentPermissions.canSeeEvents
              ? ` • ${backendError}`
              : ""}
          </p>
        </div>

        {currentPermissions.canControlSystem && (
          <div className={`status-pill ${armed ? "armed" : "disarmed"}`}>
            {armed ? "ARMED" : "DISARMED"}
          </div>
        )}
      </header>

      <main
        className="dash-grid"
        style={{
          gridTemplateColumns:
            currentPermissions.canSeeEvents && currentPermissions.canSeeSnapshots
              ? "320px 1fr 320px"
              : "1fr",
        }}
      >
        {currentPermissions.canSeeEvents && (
          <section className="card">
            <h2 className="card-title">Event Timeline</h2>

            {currentPermissions.canControlSystem && (
              <div className="btn-row">
                <button
                  className={`btn ${armed ? "primary" : "ghost"}`}
                  onClick={() => setArmed(true)}
                >
                  Arm
                </button>

                <button
                  className={`btn ${!armed ? "primary" : "ghost"}`}
                  onClick={() => setArmed(false)}
                >
                  Disarm
                </button>
              </div>
            )}

            <div className="timeline-list">
              {loadingEvents ? (
                <div className="event-item">
                  <div>
                    <b>Loading events...</b>
                  </div>
                  <span>Please wait</span>
                </div>
              ) : backendError ? (
                <div className="event-item">
                  <div>
                    <b>Backend error</b>
                  </div>
                  <span>Could not fetch data</span>
                </div>
              ) : events.length === 0 ? (
                <div className="event-item">
                  <div>
                    <b>No events found</b>
                  </div>
                  <span>Backend returned no data</span>
                </div>
              ) : (
                events.slice(0, 5).map((event) => (
                  <div className="event-item" key={event.id}>
                    <div>
                      <b>{formatEventType(event.event_type)}</b>
                      <div style={{ fontSize: "0.85rem", opacity: 0.8 }}>
                        {event.location} • {event.device_id}
                      </div>
                    </div>
                    <span>{formatTimestamp(event.timestamp)}</span>
                  </div>
                ))
              )}
            </div>
          </section>
        )}

        <section className="card">
          <h2 className="card-title">Live Feed</h2>

          <div className="live-feed-box">
            <img src={STREAM_URL} alt="Live Feed" className="live-feed-img" />
          </div>

          <p className="hint">Live feed from camera stream.</p>
        </section>

        {currentPermissions.canSeeSnapshots && (
          <section className="card">
            <h2 className="card-title">Event Snapshot Timeline</h2>

            <div className="snapshot-timeline-list">
              {loadingSnapshots ? (
                <div className="event-item">
                  <div>
                    <b>Loading snapshots...</b>
                  </div>
                  <span>Please wait</span>
                </div>
              ) : snapshotError ? (
                <div className="event-item">
                  <div>
                    <b>Snapshot error</b>
                  </div>
                  <span>{snapshotError}</span>
                </div>
              ) : snapshots.length === 0 ? (
                <div className="event-item">
                  <div>
                    <b>No snapshots found</b>
                  </div>
                  <span>Backend returned no images</span>
                </div>
              ) : (
                [...snapshots]
                  .sort((a, b) => b.localeCompare(a))
                  .slice(0, 3)
                  .map((filename) => (
                    <div className="snapshot-item" key={filename}>
                      <img
                        src={`${API_BASE}/images/${filename}`}
                        alt="Snapshot"
                        className="snapshot-img"
                      />
                      <div className="snapshot-meta">
                        <b>Snapshot</b>
                        <span>{filename}</span>
                      </div>
                    </div>
                  ))
              )}
            </div>
          </section>
        )}
      </main>

      <button className="logout-btn" onClick={logout}>
        Logout
      </button>
    </div>
  );
}

export default App;