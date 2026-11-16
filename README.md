# DroneOS Complete

DroneOS is a C++17 drone operating-system simulator with a browser dashboard.

## What is included

- Existing main dashboard styling/layout is preserved.
- Mission Control includes an OpenStreetMap/Leaflet map and a five-drone browser simulation with separate routes, markers, battery estimates, GPS readouts, and selectable drone cards.
- Fleet task dispatch supports one selected drone or all five drones. Each selected drone follows its own finite route, reports progress, and stops at its destination instead of looping indefinitely.
- Task assignments are persisted per drone in SQLite. The task state moves through `ASSIGNED`, `IN_PROGRESS`, and `COMPLETED`; the related mission record is updated too.
- The Database page displays fleet registry and task-history tables plus record counts, populated from `GET /api/database`.
- Startup creates five drone records, one default mission per drone, and an initial telemetry row for each drone without duplicating existing records.
- Telemetry page and Mission Control show a continuously updating feed in the format `[TELEMETRY] ALT=120m | SPD=0m/s | BAT=100% | SIG=96.1%`.
- CPU, memory, storage, and power resource values update from the simulator loop and are refreshed in the browser frequently.
- Tasks, drone status, resources, database status, and settings pages consume the existing C++ API.
- Settings includes a selectable dashboard refresh interval and a backend connection check.

## Run on Windows

Requirements:
- CMake
- Ninja
- MSYS2 UCRT64 toolchain and SQLite3 development library at the paths used by `CMakeLists.txt`
- Python 3 for the static dashboard server

1. Open PowerShell in this folder.
2. Start the backend using:

   ```powershell
   .\run-droneos.ps1
   ```

   This configures/builds the C++ project and starts the simulator/API at `http://localhost:8080`. Keep the terminal open.

3. In a second PowerShell window, start the dashboard:

   ```powershell
   .\run-dashboard.ps1
   ```

4. Open `http://localhost:5500` in your browser.

The dashboard must be served over HTTP rather than opened as a `file://` URL. The C++ backend must be running for live values and mission controls to work.

## API endpoints

- `GET /api/status`
- `GET /api/drone`
- `GET /api/telemetry`
- `GET /api/resources`
- `GET /api/tasks`
- `GET /api/mission`
- `GET /api/database`
- `POST /api/mission/start`
- `POST /api/mission/pause`
- `POST /api/mission/resume`
- `POST /api/mission/reset`
- `POST /api/tasks/assign` with JSON `{ "drone": "DRONE-X1", "task": "Area survey" }`
- `POST /api/tasks/state` with JSON `{ "task_id": "1", "state": "IN_PROGRESS" }` or `COMPLETED`

## Measurement notes

This is a software simulator, not a physical drone controller. The five-drone fleet map simulates each drone's route and position in the browser. Task assignment and task lifecycle records are persisted by the C++ API in SQLite, while the original C++ flight-mission controls still operate the original DRONE-X1 core simulator. The other fleet positions shown in the registry begin with seeded per-drone telemetry; the map simulation is not a live hardware GPS feed. CPU, memory, storage, and power figures are simulator resource-manager readings, not calibrated host-computer measurements. OpenStreetMap tiles require an internet connection.
