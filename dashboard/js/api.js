/* DroneOS frontend API client. */
"use strict";

const DRONEOS_API_BASE = "http://localhost:8080/api";

async function droneOSRequest(endpoint, options = {}) {
    const response = await fetch(`${DRONEOS_API_BASE}${endpoint}`, {
        cache: "no-store",
        ...options,
        headers: {
            Accept: "application/json",
            ...(options.headers || {})
        }
    });

    const text = await response.text();
    let payload = null;
    if (text) {
        try { payload = JSON.parse(text); }
        catch { throw new Error(`Invalid JSON returned by backend (${endpoint})`); }
    }

    if (!response.ok) {
        throw new Error(payload?.error || `HTTP ${response.status} for ${endpoint}`);
    }
    return payload;
}

const DroneOSAPI = Object.freeze({
    getStatus: () => droneOSRequest("/status"),
    getDrone: () => droneOSRequest("/drone"),
    getTelemetry: () => droneOSRequest("/telemetry"),
    getResources: () => droneOSRequest("/resources"),
    getTasks: () => droneOSRequest("/tasks"),
    getMission: () => droneOSRequest("/mission"),
    getDatabase: () => droneOSRequest("/database"),
    assignTask: (drone, task) => droneOSRequest("/tasks/assign", {
        method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify({ drone, task })
    }),
    updateTaskState: (task_id, state) => droneOSRequest("/tasks/state", {
        method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify({ task_id: String(task_id), state })
    }),
    startMission: () => droneOSRequest("/mission/start", { method: "POST" }),
    pauseMission: () => droneOSRequest("/mission/pause", { method: "POST" }),
    resumeMission: () => droneOSRequest("/mission/resume", { method: "POST" }),
    resetMission: () => droneOSRequest("/mission/reset", { method: "POST" })
});

window.DroneOSAPI = DroneOSAPI;
