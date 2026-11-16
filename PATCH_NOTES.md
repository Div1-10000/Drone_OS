# DroneOS multi-drone fixes

## Changes

- Replaced the shared browser fleet animation with independent per-drone mission state. A dispatched drone now follows its own finite route, reports progress, and stops at its destination. Other drones remain stationary unless selected or `All available drones` is selected.
- Task dispatch now calls the C++ API instead of only raising a browser event and starting the single-drone X1 mission.
- Added `POST /api/tasks/assign` to persist an assignment against the selected drone in SQLite and return its task ID.
- Added `POST /api/tasks/state` to persist `IN_PROGRESS` and `COMPLETED` transitions, including the associated mission status/progress.
- Startup now idempotently seeds five drone records, one default mission per drone, and one initial telemetry row per drone. Existing database contents are retained.
- Added `GET /api/database` fleet/task records and summary counts.
- Rebuilt the Database page with fleet registry and task-history tables, state labels, and database summary cards.
- Fixed the movement timer in `Drone` so it is per instance rather than a shared static timer.
- Updated README API and simulator notes.

## Verification performed

- Compiled `Database.cpp`, `Drone.cpp`, and `Kernel.cpp` with GCC C++17 warnings enabled.
- Compiled `ApiServer.cpp` with a temporary POSIX Winsock compatibility header for syntax checking; the actual project remains Windows/MSYS2 Winsock-based.
- Ran `node --check` on all modified JavaScript files.
- Ran SQLite integration checks for five seeded drones, assignment to a non-X1 drone, task-state transitions, mission-state updates, and valid JSON output.
- Ran HTTP integration checks against the API for database reads, task assignment, and task-state updates.

## Important simulator boundary

The fleet map's five independent movement paths are browser-simulated. Task assignment and lifecycle are persisted in the C++/SQLite backend. The original C++ flight-mission controls still operate the original DRONE-X1 core `Drone` instance; this project does not connect to or control physical aircraft.
