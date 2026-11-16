/* DroneOS navigation and page views. */
"use strict";

(() => {
    const $ = (selector, root = document) => root.querySelector(selector);
    const $$ = (selector, root = document) => [...root.querySelectorAll(selector)];

    const dashboard = $(".dashboard");
    const footer = $(".footer");
    const main = $(".main");
    if (!dashboard || !main) return;

    const page = document.createElement("section");
    page.id = "pageView";
    page.className = "page-view";
    page.hidden = true;
    dashboard.insertAdjacentElement("afterend", page);

    const events = [];
    window.DroneOSEvents = events;

    function esc(value) {
        return String(value ?? "").replace(/[&<>"']/g, c => ({
            "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;"
        })[c]);
    }

    function addEvent(message, type = "info") {
        events.unshift({ time: new Date().toLocaleTimeString("en-IN", { hour12: false }), message, type });
        events.splice(20);
        const list = $(".events");
        if (list) {
            list.innerHTML = events.slice(0, 5).map(item => `
                <div class="event"><i class="fa-solid ${item.type === "success" ? "fa-circle-check success" : item.type === "warning" ? "fa-triangle-exclamation warning" : "fa-circle-info info"}"></i>
                <span>${esc(item.time)}</span><strong>${esc(item.message)}</strong></div>`).join("");
        }
    }
    window.DroneOSAddEvent = addEvent;

    function missionMarkup() {
        return `
        <div class="page-heading"><div><span class="panel-label">FLIGHT OPERATIONS</span><h1>Mission Control</h1><p>Live mission map, waypoints, route tracking and flight controls.</p></div><span class="page-chip" id="missionPageStatus">CONNECTING</span></div>
        <div class="mission-map-layout">
          <article class="panel page-panel mission-map-panel">
            <div class="panel-heading"><div><span class="panel-label">LIVE NAVIGATION</span><h2>Mission Map</h2></div><span class="gps-status"><i class="fa-solid fa-satellite-dish"></i> GPS TRACKING</span></div>
            <div class="fleet-map-toolbar"><span><i class="fa-solid fa-satellite-dish"></i> LIVE SIMULATED GPS</span><span id="fleetMapConnection">WAITING FOR BACKEND</span></div>
            <div id="fleetLeafletMap" class="fleet-leaflet-map" aria-label="Live map with multiple simulated drones"></div>
            <div class="mission-map-legend"><span><i class="legend-drone"></i> Active drone</span><span><i class="legend-route"></i> Planned route</span><span><i class="legend-waypoint"></i> Waypoint</span><span class="map-note">OpenStreetMap tiles · simulated flight positions</span></div>
            <article class="panel fleet-list-panel"><div class="panel-heading"><div><span class="panel-label">FLEET OVERVIEW</span><h2>All drones</h2></div><span class="page-chip" id="fleetCount">5 SIMULATED</span></div><div class="fleet-drone-grid" id="fleetDroneGrid"><div class="fleet-loading">Connecting to DroneOS…</div></div></article>
          </article>
          <aside class="mission-side-column">
            <article class="panel page-panel">
              <div class="panel-heading"><div><span class="panel-label">MISSION STATUS</span><h2 id="missionPageName">AUTONOMOUS SURVEY</h2></div><span class="active-badge" id="missionPageBadge">IDLE</span></div>
              <div class="mission-page-stats"><div><span>MISSION ID</span><strong id="missionPageId">—</strong></div><div><span>WAYPOINT</span><strong id="missionWaypoint">0 / 4</strong></div><div><span>PROGRESS</span><strong id="missionPageProgress">0%</strong></div></div>
              <div class="page-progress"><span id="missionPageProgressBar"></span></div>
              <div class="mission-controls"><button class="control-button primary" id="missionStart"><i class="fa-solid fa-play"></i> Start</button><button class="control-button" id="missionPause"><i class="fa-solid fa-pause"></i> Pause</button><button class="control-button" id="missionResume"><i class="fa-solid fa-forward"></i> Resume</button><button class="control-button danger" id="missionReset"><i class="fa-solid fa-rotate-left"></i> Reset</button></div>
              <p class="control-feedback" id="missionFeedback" role="status">Waiting for backend…</p>
              <div class="data-list mission-quick-stats"><div><span>Altitude</span><strong id="pageAltitude">— m</strong></div><div><span>Speed</span><strong id="pageSpeed">— m/s</strong></div><div><span>Battery</span><strong id="pageBattery">—%</strong></div><div><span>Signal</span><strong id="pageSignal">—%</strong></div></div>
            </article>
            <article class="panel page-panel"><div class="panel-heading"><div><span class="panel-label">TASK DISPATCH</span><h2>Assign a mission</h2></div></div><label class="fleet-form-label" for="fleetTaskName">Task name</label><input class="fleet-input" id="fleetTaskName" value="Area survey" maxlength="80"><label class="fleet-form-label" for="fleetDroneSelect">Drone assignment</label><select class="fleet-input" id="fleetDroneSelect"><option value="all">All available drones</option><option value="DRONE-X1">DRONE-X1</option><option value="DRONE-X2">DRONE-X2</option><option value="DRONE-X3">DRONE-X3</option><option value="DRONE-X4">DRONE-X4</option><option value="DRONE-X5">DRONE-X5</option></select><button class="control-button primary fleet-assign-button" id="fleetAssignTask"><i class="fa-solid fa-paper-plane"></i> Assign & start</button><p class="page-note" id="fleetTaskFeedback">This dispatch runs in the software simulator; it does not control physical drones.</p></article>
            <article class="panel page-panel"><div class="panel-heading"><div class="panel-title">FLIGHT PLAN</div><span class="panel-label">4 WAYPOINTS</span></div><div class="waypoint-list" id="missionWaypoints">
              <div class="waypoint-item"><span class="waypoint-number">1</span><div><strong>Survey Point Alpha</strong><small>28.6145, 77.2098 · 140 m</small></div><span class="waypoint-state">PENDING</span></div>
              <div class="waypoint-item"><span class="waypoint-number">2</span><div><strong>Survey Point Bravo</strong><small>28.6152, 77.2105 · 150 m</small></div><span class="waypoint-state">PENDING</span></div>
              <div class="waypoint-item"><span class="waypoint-number">3</span><div><strong>Survey Point Charlie</strong><small>28.6143, 77.2110 · 135 m</small></div><span class="waypoint-state">PENDING</span></div>
              <div class="waypoint-item"><span class="waypoint-number">4</span><div><strong>Return to Home</strong><small>28.6139, 77.2090 · 120 m</small></div><span class="waypoint-state">PENDING</span></div>
            </div></article>
          </aside>
        </div>
        <article class="panel page-panel telemetry-console-panel"><div class="panel-heading"><div><span class="panel-label">LIVE BACKEND STREAM</span><h2>Telemetry Console</h2></div><span class="gps-status"><i class="fa-solid fa-circle"></i> STREAMING</span></div><div id="missionTelemetryLog" class="telemetry-log" aria-live="polite"><div>[TELEMETRY] Waiting for first backend sample…</div></div></article>`;
    }

    function genericPageMarkup(key) {
        const titles = {
            DRONE: ["Drone Status", "Live status and position reported by the simulator."],
            TASKS: ["Operating System Tasks", "Tasks and scheduler state from the C++ backend."],
            TELEMETRY: ["Telemetry Monitor", "Live sensor values from the simulated drone."],
            RESOURCES: ["Resource Manager", "CPU, memory, power, and storage usage."],
            DATABASE: ["Database Status", "SQLite persistence used by DroneOS."],
            SETTINGS: ["System Settings", "Connection information for this local simulator."]
        };
        const [title, subtitle] = titles[key] || [key, "DroneOS system view."];
        if (key === "TASKS") return `<div class="page-heading"><div><span class="panel-label">SCHEDULER</span><h1>${title}</h1><p>${subtitle}</p></div><span class="page-chip">LIVE</span></div><article class="panel page-panel"><div class="table-scroll"><table class="data-table"><thead><tr><th>Task</th><th>Priority</th><th>State</th><th>CPU</th><th>Memory</th><th>Power</th></tr></thead><tbody id="tasksPageBody"><tr><td colspan="6">Waiting for backend…</td></tr></tbody></table></div></article>`;
        if (key === "TELEMETRY") return `<div class="page-heading"><div><span class="panel-label">SENSOR STREAM</span><h1>${title}</h1><p>${subtitle}</p></div><span class="page-chip">LIVE</span></div><div class="data-card-grid"><article class="panel page-panel"><div class="data-list"><div><span>Altitude</span><strong id="telemetryPageAltitude">—</strong></div><div><span>Speed</span><strong id="telemetryPageSpeed">—</strong></div><div><span>Battery</span><strong id="telemetryPageBattery">—</strong></div><div><span>Signal</span><strong id="telemetryPageSignal">—</strong></div><div><span>Temperature</span><strong id="telemetryPageTemperature">—</strong></div><div><span>Latitude</span><strong id="telemetryPageLatitude">—</strong></div><div><span>Longitude</span><strong id="telemetryPageLongitude">—</strong></div></div></article></div><article class="panel page-panel telemetry-console-panel"><div class="panel-heading"><div class="panel-title">LIVE TELEMETRY STREAM</div><span class="page-chip">AUTO-REFRESH</span></div><div id="missionTelemetryLog" class="telemetry-log" aria-live="polite"><div>[TELEMETRY] Waiting for first backend sample…</div></div></article>`;
        if (key === "RESOURCES") return `<div class="page-heading"><div><span class="panel-label">KERNEL RESOURCES</span><h1>${title}</h1><p>${subtitle}</p></div><span class="page-chip">LIVE</span></div><div class="data-card-grid"><article class="panel page-panel"><div class="data-list"><div><span>CPU Usage</span><strong id="resourcesPageCpu">—%</strong></div><div><span>Memory Usage</span><strong id="resourcesPageMemory">—%</strong></div><div><span>Power Consumption</span><strong id="resourcesPagePower">— W</strong></div><div><span>Storage Usage</span><strong id="resourcesPageStorage">—%</strong></div></div></article></div>`;
        if (key === "DATABASE") return `<div class="page-heading"><div><span class="panel-label">PERSISTENCE LAYER</span><h1>${title}</h1><p>Live records for every drone, mission, task and telemetry stream.</p></div><span class="page-chip" id="databasePageStatus">CHECKING</span></div>
          <div class="database-summary-grid"><article class="panel database-stat"><span>REGISTERED DRONES</span><strong id="dbDroneCount">—</strong></article><article class="panel database-stat"><span>MISSIONS</span><strong id="dbMissionCount">—</strong></article><article class="panel database-stat"><span>TASK RECORDS</span><strong id="dbTaskCount">—</strong></article><article class="panel database-stat"><span>TELEMETRY SAMPLES</span><strong id="dbTelemetryCount">—</strong></article></div>
          <article class="panel page-panel database-panel"><div class="panel-heading"><div><span class="panel-label">FLEET REGISTRY</span><h2>Drone records</h2></div><span class="page-chip">ALL DRONES</span></div><div class="table-scroll"><table class="data-table database-table"><thead><tr><th>Drone</th><th>Status</th><th>Battery</th><th>Altitude</th><th>Speed</th><th>Mission</th><th>Telemetry</th></tr></thead><tbody id="databaseDronesBody"><tr><td colspan="7">Waiting for database records…</td></tr></tbody></table></div></article>
          <article class="panel page-panel database-panel"><div class="panel-heading"><div><span class="panel-label">PERSISTED WORK</span><h2>Task history</h2></div><span class="page-chip">LATEST FIRST</span></div><div class="table-scroll"><table class="data-table database-table"><thead><tr><th>Task ID</th><th>Task</th><th>Drone</th><th>Mission</th><th>Priority</th><th>State</th><th>Created</th></tr></thead><tbody id="databaseTasksBody"><tr><td colspan="7">No task records yet.</td></tr></tbody></table></div></article>
          <article class="panel page-panel"><div class="data-list"><div><span>Engine</span><strong id="databaseEngine">SQLite</strong></div><div><span>Database file</span><strong id="databasePath">database/droneos.db</strong></div><div><span>Connection</span><strong id="databaseConnection">Checking…</strong></div></div></article>`;
        if (key === "SETTINGS") return `<div class="page-heading"><div><span class="panel-label">CONFIGURATION</span><h1>${title}</h1><p>${subtitle}</p></div><span class="page-chip">LOCAL</span></div><article class="panel page-panel"><div class="data-list"><div><span>API base URL</span><strong>http://localhost:8080/api</strong></div><div><span>Refresh interval</span><strong id="settingsRefreshValue">300 ms</strong></div><div><span>Backend</span><strong id="settingsBackendStatus">Checking…</strong></div></div><div class="settings-controls"><label for="settingRefreshRate">Live data refresh rate</label><select id="settingRefreshRate"><option value="250">250 ms — Fast</option><option value="300" selected>300 ms — Recommended</option><option value="500">500 ms — Balanced</option><option value="1000">1 second</option><option value="2000">2 seconds</option></select><button class="control-button primary" id="settingsCheckConnection"><i class="fa-solid fa-plug"></i> Check Connection</button><p class="page-note" id="settingsFeedback">Refresh rate affects how often the dashboard requests new backend data. Actual sensor sampling is controlled by the simulator.</p></div><p class="page-note">Start the DroneOS C++ executable before opening the dashboard. Keep this dashboard served from the project’s dashboard folder.</p></article>`;
        return `<div class="page-heading"><div><span class="panel-label">FLIGHT SYSTEM</span><h1>${title}</h1><p>${subtitle}</p></div><span class="page-chip">LIVE</span></div><article class="panel page-panel"><div class="data-list"><div><span>Drone ID</span><strong id="dronePageId">—</strong></div><div><span>Status</span><strong id="dronePageStatus">—</strong></div><div><span>Flight mode</span><strong id="dronePageMode">—</strong></div><div><span>Altitude</span><strong id="dronePageAltitude">— m</strong></div><div><span>Battery</span><strong id="dronePageBattery">—%</strong></div><div><span>Coordinates</span><strong id="dronePageCoordinates">—</strong></div></div></article>`;
    }

    async function runMissionCommand(command) {
        const feedback = $("#missionFeedback");
        const buttons = $$("#pageView .control-button");
        buttons.forEach(button => button.disabled = true);
        if (feedback) feedback.textContent = "Sending command to DroneOS backend…";
        try {
            const result = await window.DroneOSAPI[command]();
            if (!result?.success) throw new Error(result?.message || "Backend rejected the command.");
            if (feedback) feedback.textContent = result.message;
            addEvent(result.message, "success");
            window.dispatchEvent(new CustomEvent("droneos:fleet-mission-state", { detail: { active: command === "startMission" || command === "resumeMission" } }));
            window.dispatchEvent(new CustomEvent("droneos:refresh"));
        } catch (error) {
            if (feedback) feedback.textContent = `Command failed: ${error.message}`;
            addEvent(`Mission command failed: ${error.message}`, "warning");
        } finally {
            buttons.forEach(button => button.disabled = false);
        }
    }

    function showPage(key) {
        const isDashboard = key === "DASHBOARD";
        dashboard.hidden = !isDashboard;
        page.hidden = isDashboard;
        if (footer) footer.hidden = false;
        $$(".nav-link").forEach(button => button.classList.toggle("active", button.dataset.page === key));
        if (isDashboard) return;
        page.innerHTML = key === "MISSIONS" ? missionMarkup() : genericPageMarkup(key);
        if (key === "MISSIONS") {
            $("#missionStart").addEventListener("click", () => runMissionCommand("startMission"));
            $("#missionPause").addEventListener("click", () => runMissionCommand("pauseMission"));
            $("#missionResume").addEventListener("click", () => runMissionCommand("resumeMission"));
            $("#missionReset").addEventListener("click", () => runMissionCommand("resetMission"));
            const assignButton = $("#fleetAssignTask");
            if (assignButton) assignButton.addEventListener("click", async () => {
                const taskName = $("#fleetTaskName").value.trim() || "Area survey";
                const assigned = $("#fleetDroneSelect").value;
                const feedback = $("#fleetTaskFeedback");
                assignButton.disabled = true;
                if (feedback) feedback.textContent = "Saving task assignment…";
                try {
                    const targets = assigned === "all"
                        ? ["DRONE-X1", "DRONE-X2", "DRONE-X3", "DRONE-X4", "DRONE-X5"]
                        : [assigned];
                    const taskIds = {};
                    for (const droneName of targets) {
                        const result = await window.DroneOSAPI.assignTask(droneName, taskName);
                        if (!result?.success) throw new Error(result?.message || `Could not assign ${droneName}`);
                        taskIds[droneName] = result.task_id;
                    }
                    window.dispatchEvent(new CustomEvent("droneos:fleet-assign", { detail: { taskName, assigned, taskIds } }));
                    if (feedback) feedback.textContent = `“${taskName}” saved and dispatched to ${targets.join(", ")}. Each drone follows its own route and stops at its destination.`;
                    window.dispatchEvent(new CustomEvent("droneos:refresh"));
                } catch (error) {
                    if (feedback) feedback.textContent = `Task assignment failed: ${error.message}`;
                } finally { assignButton.disabled = false; }
            });
        }
        if (key === "SETTINGS") {
            $("#settingRefreshRate").addEventListener("change", event => {
                const interval = Number(event.target.value);
                window.dispatchEvent(new CustomEvent("droneos:refresh-interval", { detail: interval }));
                $("#settingsFeedback").textContent = `Dashboard refresh changed to ${interval} ms.`;
            });
            $("#settingsCheckConnection").addEventListener("click", async () => {
                const feedback = $("#settingsFeedback");
                feedback.textContent = "Checking backend connection…";
                try {
                    const status = await window.DroneOSAPI.getStatus();
                    feedback.textContent = `Connected successfully. Backend status: ${status.status || "ONLINE"}.`;
                    addEvent("Backend connection checked successfully", "success");
                } catch (error) {
                    feedback.textContent = `Connection failed: ${error.message}`;
                    addEvent(`Backend check failed: ${error.message}`, "warning");
                }
            });
        }
        window.dispatchEvent(new CustomEvent("droneos:refresh"));
    }

    $$(".nav-link").forEach(button => {
        const key = ($("span", button)?.textContent || "DASHBOARD").trim().toUpperCase();
        button.dataset.page = key;
        button.addEventListener("click", () => showPage(key));
    });

    $$(".tasks-panel .view-button").forEach(button => button.addEventListener("click", () => showPage("TASKS")));
    $$(".events-card .view-button").forEach(button => button.addEventListener("click", () => { addEvent("Events panel opened"); }));
    $$(".header-actions .notification-button").forEach(button => button.addEventListener("click", () => {
        const latest = events[0];
        addEvent(latest ? `Latest event: ${latest.message}` : "No new browser events", "info");
    }));
    $$(".header-actions .header-button:not(.notification-button)").forEach(button => button.addEventListener("click", () => showPage("SETTINGS")));

    window.DroneOSApp = { showPage, addEvent };
})();
