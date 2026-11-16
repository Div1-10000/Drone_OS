PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS drones (
    drone_id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,
    model TEXT NOT NULL,
    status TEXT NOT NULL DEFAULT 'OFFLINE',
    battery REAL NOT NULL DEFAULT 100.0,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS missions (
    mission_id INTEGER PRIMARY KEY AUTOINCREMENT,
    drone_id INTEGER NOT NULL,
    mission_name TEXT NOT NULL,
    status TEXT NOT NULL DEFAULT 'PLANNED',
    progress REAL NOT NULL DEFAULT 0.0,
    started_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    completed_at DATETIME,

    FOREIGN KEY (drone_id)
        REFERENCES drones(drone_id)
        ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS tasks (
    task_id INTEGER PRIMARY KEY AUTOINCREMENT,
    mission_id INTEGER NOT NULL,
    task_name TEXT NOT NULL,
    priority TEXT NOT NULL,
    state TEXT NOT NULL DEFAULT 'READY',

    cpu_required REAL NOT NULL DEFAULT 0,
    memory_required REAL NOT NULL DEFAULT 0,
    power_required REAL NOT NULL DEFAULT 0,

    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    completed_at DATETIME,

    FOREIGN KEY (mission_id)
        REFERENCES missions(mission_id)
        ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS telemetry (
    telemetry_id INTEGER PRIMARY KEY AUTOINCREMENT,
    drone_id INTEGER NOT NULL,

    altitude REAL NOT NULL,
    speed REAL NOT NULL,
    battery REAL NOT NULL,
    signal REAL NOT NULL,
    temperature REAL NOT NULL,

    latitude REAL NOT NULL,
    longitude REAL NOT NULL,

    recorded_at DATETIME DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (drone_id)
        REFERENCES drones(drone_id)
        ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS events (
    event_id INTEGER PRIMARY KEY AUTOINCREMENT,
    drone_id INTEGER,

    event_type TEXT NOT NULL,
    message TEXT NOT NULL,
    severity TEXT NOT NULL DEFAULT 'INFO',

    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (drone_id)
        REFERENCES drones(drone_id)
        ON DELETE SET NULL
);

CREATE TABLE IF NOT EXISTS faults (
    fault_id INTEGER PRIMARY KEY AUTOINCREMENT,
    drone_id INTEGER NOT NULL,

    fault_type TEXT NOT NULL,
    severity TEXT NOT NULL,
    description TEXT NOT NULL,

    resolved INTEGER NOT NULL DEFAULT 0,

    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    resolved_at DATETIME,

    FOREIGN KEY (drone_id)
        REFERENCES drones(drone_id)
        ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_telemetry_drone
ON telemetry(drone_id);

CREATE INDEX IF NOT EXISTS idx_telemetry_time
ON telemetry(recorded_at);

CREATE INDEX IF NOT EXISTS idx_tasks_mission
ON tasks(mission_id);

CREATE INDEX IF NOT EXISTS idx_events_drone
ON events(drone_id);

CREATE INDEX IF NOT EXISTS idx_faults_drone
ON faults(drone_id);