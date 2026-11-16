/* DroneOS live dashboard: polls the real C++ API and updates all views. */
"use strict";

(() => {
    const $ = (id) => document.getElementById(id);
    const text = (id, value) => { const el = $(id); if (el) el.textContent = value; };
    const clamp = (value, min = 0, max = 100) => Math.max(min, Math.min(max, Number(value) || 0));
    const rounded = (value, digits = 0) => Number(Number(value || 0).toFixed(digits));
    const percent = value => `${Math.round(clamp(value))}%`;
    const safe = value => String(value ?? "").replace(/[&<>"']/g, c => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" })[c]);

    const state = {
        connected: false,
        status: {}, drone: {}, telemetry: {}, resources: {}, tasks: [], mission: {}, database: {},
        pollInProgress: false,
        startedAt: Date.now(),
        telemetryLines: [],
        trailPoints: []
    };

    function updateClock() {
        const now = new Date();
        text("systemTime", now.toLocaleTimeString("en-IN", { hour12: false }));
        const date = document.querySelector(".clock span");
        if (date) date.textContent = now.toLocaleDateString("en-IN", { day: "2-digit", month: "short", year: "numeric" }).toUpperCase();
    }

    function setStatus(connected, message = "") {
        const wasConnected = state.connected;
        state.connected = connected;
        if (connected && !wasConnected && window.DroneOSApp) {
            window.DroneOSApp.addEvent("Connected to live C++ backend", "success");
        }
        const label = document.querySelector(".system-online strong");
        const dot = document.querySelector(".system-online .pulse-dot");
        if (label) label.textContent = connected ? `SYSTEM ${state.status.status || "ONLINE"}` : "BACKEND DISCONNECTED";
        if (dot) {
            dot.style.background = connected ? "#4ade80" : "#fb7185";
            dot.style.boxShadow = connected ? "0 0 12px #4ade80" : "0 0 12px #fb7185";
        }
        const chip = $("missionPageStatus");
        if (chip) chip.textContent = connected ? "BACKEND CONNECTED" : "BACKEND OFFLINE";
        text("settingsBackendStatus", connected ? "Connected" : "Disconnected");
        text("databasePageStatus", connected ? "CONNECTED" : "OFFLINE");
        text("databaseConnection", connected ? "Connected" : "Disconnected");
        if (!connected && message && window.DroneOSApp) {
            // Keep the UI calm: don't add an event on every failed polling cycle.
            if (!state.lastConnectionError || state.lastConnectionError !== message) {
                window.DroneOSApp.addEvent(`Backend connection error: ${message}`, "warning");
                state.lastConnectionError = message;
            }
        } else if (connected) state.lastConnectionError = "";
    }

    function updateTelemetry(data) {
        state.telemetry = data || {};
        const t = state.telemetry;
        const altitude = rounded(t.altitude, 1), speed = rounded(t.speed, 1);
        const battery = rounded(t.battery), signal = rounded(t.signal), temperature = rounded(t.temperature, 1);
        text("altitudeValue", rounded(t.altitude));
        text("speedValue", rounded(t.speed));
        text("batteryValue", battery);
        text("signalValue", signal);
        text("telemetryAltitude", rounded(t.altitude));
        text("telemetrySpeed", rounded(t.speed));
        text("telemetryBattery", battery);
        text("telemetrySignal", signal);
        text("mapAltitude", `${rounded(t.altitude)} m`);

        const coordinates = document.querySelectorAll(".map-info strong");
        if (coordinates.length >= 2) {
            coordinates[0].textContent = `${Number(t.latitude || 0).toFixed(4)}° ${Number(t.latitude || 0) >= 0 ? "N" : "S"}`;
            coordinates[1].textContent = `${Number(t.longitude || 0).toFixed(4)}° ${Number(t.longitude || 0) >= 0 ? "E" : "W"}`;
        }
        text("pageAltitude", `${altitude} m`);
        text("pageSpeed", `${speed} m/s`);
        text("pageBattery", `${battery}%`);
        text("pageSignal", `${signal}%`);
        text("pageLatitude", Number(t.latitude || 0).toFixed(6));
        text("pageLongitude", Number(t.longitude || 0).toFixed(6));
        text("telemetryPageAltitude", `${altitude} m`);
        text("telemetryPageSpeed", `${speed} m/s`);
        text("telemetryPageBattery", `${battery}%`);
        text("telemetryPageSignal", `${signal}%`);
        text("telemetryPageTemperature", `${temperature} °C`);
        text("telemetryPageLatitude", Number(t.latitude || 0).toFixed(6));
        text("telemetryPageLongitude", Number(t.longitude || 0).toFixed(6));
        const line = `[TELEMETRY] ALT=${altitude}m | SPD=${speed}m/s | BAT=${battery}% | SIG=${Number(signal).toFixed(1)}%`;
        if (state.telemetryLines[state.telemetryLines.length - 1] !== line) {
            state.telemetryLines.push(`${new Date().toLocaleTimeString("en-IN", { hour12: false })}  ${line}`);
            if (state.telemetryLines.length > 80) state.telemetryLines.shift();
        }
        const log = $("missionTelemetryLog");
        if (log) {
            log.innerHTML = state.telemetryLines.slice(-32).map(entry => `<div>${safe(entry)}</div>`).join("");
            log.scrollTop = log.scrollHeight;
        }
        const mapCoords = $("missionMapCoords");
        if (mapCoords) mapCoords.textContent = `${Number(t.latitude || 0).toFixed(6)}, ${Number(t.longitude || 0).toFixed(6)}`;
        updateMissionMap(t);
    }

    function updateMissionMap(t) {
        const drone = $("missionMapDrone");
        const trail = $("missionTrailLine");
        if (!drone || !t) return;
        const latitude = Number(t.latitude || 28.6139);
        const longitude = Number(t.longitude || 77.2090);
        const x = Math.max(90, Math.min(920, 180 + ((longitude - 77.2090) / 0.002) * 590));
        const y = Math.max(60, Math.min(480, 420 - ((latitude - 28.6139) / 0.0013) * 290));
        drone.setAttribute("transform", `translate(${x.toFixed(1)} ${y.toFixed(1)})`);
        const point = `${x.toFixed(1)},${y.toFixed(1)}`;
        const last = state.trailPoints[state.trailPoints.length - 1];
        if (last !== point) {
            state.trailPoints.push(point);
            if (state.trailPoints.length > 180) state.trailPoints.shift();
        }
        if (trail) trail.setAttribute("points", state.trailPoints.join(" "));
    }

    function updateDrone(data) {
        state.drone = data || {};
        text("dronePageId", data.drone_id ?? "—");
        text("dronePageStatus", data.status || "—");
        text("dronePageMode", data.flight_mode || "—");
        text("dronePageAltitude", `${rounded(data.altitude)} m`);
        text("dronePageBattery", `${rounded(data.battery)}%`);
        text("dronePageCoordinates", `${Number(data.latitude || 0).toFixed(4)}, ${Number(data.longitude || 0).toFixed(4)}`);
        const badge = document.querySelector(".drone-status .active-badge, .drone-status .panel-heading .active-badge");
        if (badge) badge.textContent = data.status || "OFFLINE";
    }

    function updateResources(data) {
        state.resources = data || {};
        const cpu = clamp(data.cpu), memoryMb = Number(data.memory || 0), storage = clamp(data.storage), power = Number(data.power || 0);
        const memoryPercent = clamp(memoryMb / 8192 * 100);
        text("cpuValue", percent(cpu));
        text("memoryValue", percent(memoryPercent));
        text("storageValue", percent(storage));
        text("powerValue", `${Math.round(power)} W`);
        text("resourcesPageCpu", percent(cpu));
        text("resourcesPageMemory", `${percent(memoryPercent)} (${Math.round(memoryMb)} MB)`);
        text("resourcesPagePower", `${Math.round(power)} W`);
        text("resourcesPageStorage", percent(storage));
        const memorySmall = document.querySelector(".memory-gauge + .resource-info small");
        if (memorySmall) memorySmall.textContent = `${(memoryMb / 1024).toFixed(1)} / 8 GB`;
        const storageSmall = document.querySelector(".storage-gauge + .resource-info small");
        if (storageSmall) storageSmall.textContent = `${storage.toFixed(1)} / 100 units`;
        const cpuGauge = document.querySelector(".cpu-gauge");
        const memGauge = document.querySelector(".memory-gauge");
        const storageGauge = document.querySelector(".storage-gauge");
        if (cpuGauge) cpuGauge.style.background = `conic-gradient(var(--cyan) 0 ${cpu}%, rgba(255,255,255,.06) ${cpu}% 100%)`;
        if (memGauge) memGauge.style.background = `conic-gradient(#9d55ff 0 ${memoryPercent}%, rgba(255,255,255,.06) ${memoryPercent}% 100%)`;
        if (storageGauge) storageGauge.style.background = `conic-gradient(var(--orange) 0 ${storage}%, rgba(255,255,255,.06) ${storage}% 100%)`;
        const powerGauge = document.querySelector(".power-gauge");
        if (powerGauge) {
            const powerPercent = clamp(power / 600 * 100);
            powerGauge.style.background = `conic-gradient(var(--green) 0 ${powerPercent}%, rgba(255,255,255,.06) ${powerPercent}% 100%)`;
        }
    }

    function renderTasks() {
        const rows = state.tasks.map(task => {
            const priority = String(task.priority || "LOW").toLowerCase();
            const status = String(task.state || "READY").toLowerCase();
            const memory = Number(task.memory || 0);
            return `<div class="task-row"><span class="task-name"><i class="fa-solid fa-microchip"></i>${safe(task.name)}</span><span><b class="priority ${safe(priority)}">${safe(task.priority)}</b></span><span><b class="status ${safe(status)}">${safe(task.state)}</b></span><span>${rounded(task.cpu)}%</span><span>${rounded(memory)} MB</span><span>${status === "running" ? "RUNNING" : "—"}</span></div>`;
        }).join("");
        const dashboardTable = document.querySelector(".tasks-panel .task-table");
        if (dashboardTable) dashboardTable.innerHTML = `<div class="task-header"><span>TASK NAME</span><span>PRIORITY</span><span>STATUS</span><span>CPU</span><span>MEMORY</span><span>TIME</span></div>${rows || "<div class='task-row'><span>No tasks returned by backend.</span></div>"}`;
        const body = $("tasksPageBody");
        if (body) body.innerHTML = state.tasks.map(task => `<tr><td>${safe(task.name)}</td><td><span class="priority ${safe(String(task.priority || "").toLowerCase())}">${safe(task.priority)}</span></td><td><span class="status ${safe(String(task.state || "").toLowerCase())}">${safe(task.state)}</span></td><td>${rounded(task.cpu)}%</td><td>${rounded(task.memory)} MB</td><td>${rounded(task.power)} W</td></tr>`).join("") || `<tr><td colspan="6">No tasks returned by backend.</td></tr>`;
    }

    function updateMission(data) {
        state.mission = data || {};
        const progress = clamp(data.progress);
        text("missionProgress", `${Math.round(progress)}%`);
        const bar = $("missionProgressBar");
        if (bar) bar.style.width = `${progress}%`;
        const status = String(data.status || "IDLE").toUpperCase();
        const badge = document.querySelector(".mission-card .mission-badge");
        if (badge) badge.textContent = status;
        const missionTitle = document.querySelector(".mission-card h2");
        if (missionTitle) missionTitle.textContent = data.name || "AUTONOMOUS SURVEY";
        const missionId = document.querySelector(".mission-details > div:first-child strong");
        if (missionId) missionId.textContent = `MSN-${data.mission_id ?? "—"}`;
        const startTime = document.querySelector(".mission-details > div:nth-child(2) strong");
        if (startTime && status === "IDLE") startTime.textContent = "NOT STARTED";
        else if (startTime && startTime.textContent === "NOT STARTED") startTime.textContent = new Date().toLocaleTimeString("en-IN", { hour12: false });
        const current = Number(data.current_waypoint || 0), total = Number(data.total_waypoints || 0);
        text("missionPageName", data.name || "AUTONOMOUS SURVEY");
        text("missionPageId", data.mission_id ?? "—");
        text("missionWaypoint", `${current} / ${total}`);
        text("missionPageProgress", `${Math.round(progress)}%`);
        text("missionPageStatus", state.connected ? "BACKEND CONNECTED" : "BACKEND OFFLINE");
        text("missionPageBadge", status);
        const pageBar = $("missionPageProgressBar");
        if (pageBar) pageBar.style.width = `${progress}%`;
        const waypointItems = document.querySelectorAll("#missionWaypoints .waypoint-item");
        waypointItems.forEach((item, index) => {
            const mapWaypoint = document.querySelector(`.map-waypoint[data-waypoint="${index + 1}"]`);
            const stateLabel = item.querySelector(".waypoint-state");
            const reached = status === "COMPLETED" || index < current - 1;
            const active = (status === "RUNNING" || status === "RETURNING") && index === current - 1;
            if (stateLabel) stateLabel.textContent = reached ? "REACHED" : active ? "IN PROGRESS" : status === "IDLE" ? "READY" : "PENDING";
            item.classList.toggle("reached", reached);
            item.classList.toggle("current", active);
            if (mapWaypoint) {
                mapWaypoint.classList.toggle("reached", reached);
                mapWaypoint.classList.toggle("current", active);
            }
        });
        const start = $("missionStart"), pause = $("missionPause"), resume = $("missionResume");
        if (start) start.disabled = status === "RUNNING" || status === "RETURNING" || status === "PAUSED";
        if (pause) pause.disabled = status !== "RUNNING" && status !== "RETURNING";
        if (resume) resume.disabled = status !== "PAUSED";
        const duration = Math.max(0, Math.floor((Date.now() - state.startedAt) / 1000));
        text("missionDuration", status === "IDLE" ? "00:00:00" : `${String(Math.floor(duration / 3600)).padStart(2, "0")}:${String(Math.floor(duration / 60) % 60).padStart(2, "0")}:${String(duration % 60).padStart(2, "0")}`);
    }

    function renderDatabase(data) {
        const records = data?.records || {};
        const drones = Array.isArray(records.drones) ? records.drones : [];
        const tasks = Array.isArray(records.tasks) ? records.tasks : [];
        const summary = records.summary || {};
        text("dbDroneCount", summary.drone_count ?? drones.length);
        text("dbMissionCount", summary.mission_count ?? "—");
        text("dbTaskCount", summary.task_count ?? tasks.length);
        text("dbTelemetryCount", summary.telemetry_count ?? "—");
        const droneBody = $("databaseDronesBody");
        if (droneBody) droneBody.innerHTML = drones.length ? drones.map(d => `
            <tr><td><strong>${safe(d.name)}</strong><small class="db-subcell">${safe(d.model)}</small></td>
            <td><span class="db-state ${String(d.status).toLowerCase()}">${safe(d.status)}</span></td>
            <td>${rounded(d.battery)}%</td><td>${rounded(d.altitude,1)} m</td><td>${rounded(d.speed,1)} m/s</td>
            <td>${safe(d.mission)}<small class="db-subcell">${safe(d.mission_status)}</small></td>
            <td>${Number(d.telemetry_samples || 0).toLocaleString("en-IN")}</td></tr>`).join("") : `<tr><td colspan="7">No drone records found.</td></tr>`;
        const taskBody = $("databaseTasksBody");
        if (taskBody) taskBody.innerHTML = tasks.length ? tasks.map(t => `
            <tr><td>#${Number(t.task_id)}</td><td><strong>${safe(t.task_name)}</strong></td><td>${safe(t.drone)}</td>
            <td>${safe(t.mission)}</td><td>${safe(t.priority)}</td><td><span class="db-state ${String(t.state).toLowerCase()}">${safe(t.state)}</span></td>
            <td>${safe(t.created_at || "—")}</td></tr>`).join("") : `<tr><td colspan="7">No tasks have been assigned yet.</td></tr>`;
    }

    async function sync() {
        if (state.pollInProgress || !window.DroneOSAPI) return;
        state.pollInProgress = true;
        try {
            const results = await Promise.allSettled([
                DroneOSAPI.getStatus(), DroneOSAPI.getDrone(), DroneOSAPI.getTelemetry(),
                DroneOSAPI.getResources(), DroneOSAPI.getTasks(), DroneOSAPI.getMission(), DroneOSAPI.getDatabase()
            ]);
            const endpoints = ["status", "drone", "telemetry", "resources", "tasks", "mission", "database"];
            const errors = [];
            results.forEach((result, index) => {
                if (result.status === "fulfilled") state[endpoints[index]] = result.value;
                else errors.push(`${endpoints[index]}: ${result.reason?.message || "request failed"}`);
            });
            const essentialOK = results.slice(0, 6).every(result => result.status === "fulfilled");
            setStatus(essentialOK, errors.join("; "));
            if (results[0].status === "fulfilled") {
                const online = state.status.status === "ONLINE";
                const label = document.querySelector(".system-online strong");
                if (label) label.textContent = online ? "SYSTEM ONLINE" : "SYSTEM OFFLINE";
            }
            if (results[1].status === "fulfilled") updateDrone(state.drone);
            if (results[2].status === "fulfilled") updateTelemetry(state.telemetry);
            if (results[3].status === "fulfilled") updateResources(state.resources);
            if (results[4].status === "fulfilled") renderTasks();
            if (results[5].status === "fulfilled") updateMission(state.mission);
            if (results[6].status === "fulfilled") {
                text("databaseEngine", state.database.engine || "SQLite");
                text("databasePath", state.database.database || "database/droneos.db");
                renderDatabase(state.database);
            }
        } catch (error) {
            setStatus(false, error.message);
        } finally {
            state.pollInProgress = false;
        }
    }

    let refreshTimer = null;
    let refreshInterval = 300;
    function setRefreshInterval(milliseconds) {
        refreshInterval = Math.max(250, Math.min(5000, Number(milliseconds) || 300));
        if (refreshTimer) clearInterval(refreshTimer);
        refreshTimer = setInterval(() => { void sync(); }, refreshInterval);
        text("settingsRefreshValue", `${refreshInterval} ms`);
        void sync();
    }
    window.DroneOSSetRefreshInterval = setRefreshInterval;
    window.addEventListener("droneos:refresh", () => { void sync(); });
    window.addEventListener("droneos:refresh-interval", event => setRefreshInterval(event.detail));
    updateClock();
    setInterval(updateClock, 1000);
    void sync();
    setRefreshInterval(refreshInterval);
})();
