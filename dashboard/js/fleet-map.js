/* Independent, finite multi-drone mission simulation for the fleet map. */
"use strict";
(() => {
  const names = ["DRONE-X1", "DRONE-X2", "DRONE-X3", "DRONE-X4", "DRONE-X5"];
  const colors = ["#06b6d4", "#8b5cf6", "#22c55e", "#f59e0b", "#ef4444"];
  const home = [28.6139, 77.2090];
  const routeOffsets = [
    [[0,0],[.0007,.0007],[.0014,.0015],[.0021,.0024]],
    [[-.0005,-.0004],[-.0001,.0004],[.0006,.0010],[.0012,.0017]],
    [[.0004,-.0010],[.0008,-.0002],[.0015,.0005],[.0020,.0012]],
    [[-.0010,.0006],[-.0004,.0010],[.0003,.0018],[.0010,.0024]],
    [[.0002,.0003],[.0007,-.0003],[.0013,-.0008],[.0019,-.0012]]
  ];
  let map = null, markers = [], routeLines = [], timer = null;
  const drones = names.map((name, i) => ({
    name, color: colors[i], route: routeOffsets[i].map(([a,b]) => [home[0]+a, home[1]+b]),
    battery: 100 - i * 3, status: "READY", task: "No task assigned", progress: 0,
    lat: home[0] + routeOffsets[i][0][0], lng: home[1] + routeOffsets[i][0][1], speed: 0,
    segment: 0, segmentProgress: 0, active: false
  }));
  const el = id => document.getElementById(id);
  const escapeHtml = value => String(value ?? "").replace(/[&<>"']/g, c => ({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;","'":"&#39;"}[c]));
  function icon(d) {
    return L.divIcon({ className: "fleet-drone-icon", html: `<div style="--drone-color:${d.color}"><span>✥</span><small>${d.name}</small></div>`, iconSize: [88,42], iconAnchor: [18,18] });
  }
  function init() {
    const node = el("fleetLeafletMap");
    if (!node || !window.L || map) return;
    map = L.map(node, { zoomControl: true, preferCanvas: true }).setView(home, 16);
    L.tileLayer("https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png", { maxZoom: 20, attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a>' }).addTo(map);
    drones.forEach((d, i) => {
      routeLines[i] = L.polyline(d.route, { color:d.color, weight:3, opacity:.72, dashArray:"7 7" }).addTo(map);
      markers[i] = L.marker([d.lat,d.lng], { icon:icon(d), title:d.name }).addTo(map);
      updatePopup(d, i);
    });
    L.circleMarker(home, { radius:6, color:"#fff", weight:2, fillColor:"#111827", fillOpacity:1 }).addTo(map).bindTooltip("Fleet home / launch point");
    renderFleet();
    if (timer) clearInterval(timer);
    timer = setInterval(step, 350);
  }
  function updatePopup(d, i) {
    if (!markers[i]) return;
    markers[i].setPopupContent(`<b>${d.name}</b><br>Status: ${d.status}<br>Task: ${escapeHtml(d.task)}<br>Progress: ${Math.round(d.progress)}%<br>Battery: ${Math.round(d.battery)}%<br>GPS: ${d.lat.toFixed(6)}, ${d.lng.toFixed(6)}`);
  }
  function step() {
    if (!map) return;
    drones.forEach((d, i) => {
      if (d.active) {
        const a = d.route[d.segment], b = d.route[d.segment + 1];
        d.segmentProgress += 0.018 + i * 0.001;
        if (d.segmentProgress >= 1) {
          d.segmentProgress = 0;
          d.segment++;
          if (d.segment >= d.route.length - 1) {
            d.segment = d.route.length - 1;
            d.active = false; d.status = "COMPLETED"; d.speed = 0; d.progress = 100;
            if (d.taskId && window.DroneOSAPI) window.DroneOSAPI.updateTaskState(d.taskId, "COMPLETED").catch(() => {});
            d.lat = d.route[d.segment][0]; d.lng = d.route[d.segment][1];
          }
        }
        if (d.active) {
          const from = d.route[d.segment], to = d.route[d.segment + 1];
          d.lat = from[0] + (to[0] - from[0]) * d.segmentProgress;
          d.lng = from[1] + (to[1] - from[1]) * d.segmentProgress;
          d.progress = ((d.segment + d.segmentProgress) / (d.route.length - 1)) * 100;
          d.speed = 3.5 + i * .4;
          d.battery = Math.max(5, d.battery - .004);
        }
      }
      markers[i].setLatLng([d.lat,d.lng]);
      updatePopup(d, i);
    });
    renderFleet();
  }
  function renderFleet() {
    const grid = el("fleetDroneGrid"); if (!grid) return;
    grid.innerHTML = drones.map(d => `<button class="fleet-drone-card" data-drone="${d.name}" style="--drone-color:${d.color}"><span class="fleet-drone-card-top"><i class="fa-solid fa-drone"></i><b>${d.name}</b><em>${d.status}</em></span><span class="fleet-task-name">${escapeHtml(d.task)}</span><span class="fleet-card-metrics"><span>BAT <b>${Math.round(d.battery)}%</b></span><span>SPD <b>${d.speed.toFixed(1)} m/s</b></span></span><span class="fleet-card-gps">${d.lat.toFixed(5)}, ${d.lng.toFixed(5)} · ${Math.round(d.progress)}%</span></button>`).join("");
    grid.querySelectorAll("[data-drone]").forEach(button => button.addEventListener("click", () => { const d=drones.find(x=>x.name===button.dataset.drone); const i=names.indexOf(d?.name); if (d && map) { map.flyTo([d.lat,d.lng],18,{duration:.5}); markers[i].openPopup(); } }));
    const count=el("fleetCount"); if(count) count.textContent=`${drones.length} DRONES`;
  }
  window.addEventListener("droneos:fleet-assign", event => {
    const { taskName, assigned, taskIds = {} } = event.detail || {};
    const targets = assigned === "all" ? drones : drones.filter(d => d.name === assigned);
    targets.forEach(d => {
      d.task = taskName; d.status = "IN MISSION"; d.progress = 0; d.taskId = taskIds[d.name] || null;
      if (d.taskId && window.DroneOSAPI) window.DroneOSAPI.updateTaskState(d.taskId, "IN_PROGRESS").catch(() => {});
      d.segment = 0; d.segmentProgress = 0; d.active = true; d.speed = 3.5;
      d.lat = d.route[0][0]; d.lng = d.route[0][1];
    });
    const feedback=el("missionFeedback"); if(feedback) feedback.textContent=`${taskName} dispatched to ${targets.map(d=>d.name).join(", ")}.`;
    renderFleet();
  });
  const observer = new MutationObserver(() => { if (el("fleetLeafletMap")) init(); });
  observer.observe(document.body,{childList:true,subtree:true});
  document.addEventListener("visibilitychange",()=>{if(!document.hidden&&map)setTimeout(()=>map.invalidateSize(),100);});
  setInterval(()=>{if(el("fleetLeafletMap")&&!map)init();},700);
})();
