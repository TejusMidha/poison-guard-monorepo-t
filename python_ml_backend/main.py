import sys
import asyncio
import json
import zmq
import zmq.asyncio
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse
from typing import List
from contextlib import asynccontextmanager

# Domain Classes
from ml_classes import PyTorchShadowModel, ARTDetector, RAGRegulatorySidecar, StatisticalFilter
from rl_warden import RLWarden

if sys.platform == 'win32':
    asyncio.set_event_loop_policy(asyncio.WindowsSelectorEventLoopPolicy())

# --- DASHBOARD HTML ---
html_content = """
<!DOCTYPE html>
<html>
<head>
    <title>PoisonGuard Level 3 Dashboard</title>
    <style>
        body { font-family: 'Courier New', monospace; background-color: #0d1117; color: #00ff00; padding: 20px; line-height: 1.2; }
        .header { border-bottom: 2px solid #00ff00; padding-bottom: 10px; margin-bottom: 20px; }
        .log-entry { margin-bottom: 10px; padding: 15px; border-left: 3px solid #30363d; background: #161b22; border-radius: 4px; }
        .alert { color: #ff3333; border-left: 5px solid #ff3333; background: #2d1111; animation: blink 1s infinite; }
        @keyframes blink { 0% { opacity: 1; } 50% { opacity: 0.8; } 100% { opacity: 1; } }
        .metric { color: cyan; font-weight: bold; }
        .rag-report { color: #8b949e; font-style: italic; margin-top: 5px; border-top: 1px solid #30363d; padding-top: 5px; }
    </style>
</head>
<body>
    <div class="header">
        <h1>🛡️ PoisonGuard: ML Core Telemetry</h1>
        <div id="status" style="color: cyan;">CONNECTING TO WEBSOCKET...</div>
    </div>
    <div id="messages"></div>

    <script>
        const messagesDiv = document.getElementById('messages');
        const statusDiv = document.getElementById('status');
        const ws = new WebSocket(`ws://${window.location.host}/ws/dashboard`);

        ws.onopen = () => { statusDiv.innerHTML = "STATUS: ONLINE - DSDP COMPLIANCE ENGINE ACTIVE"; };
        ws.onclose = () => { statusDiv.innerHTML = "STATUS: OFFLINE"; statusDiv.style.color = "red"; };

        ws.onmessage = (event) => {
            const payload = JSON.parse(event.data);
            const data = payload.data;
            const isPoison = data.active_threats > 0;
            
            const div = document.createElement('div');
            div.className = 'log-entry' + (isPoison ? ' alert' : '');
            
            const timestamp = new Date().toLocaleTimeString();
            div.innerHTML = `
                <strong>[${timestamp}] BATCH: ${data.batch_id}</strong><br>
                <span class="metric">RL REWARD: ${data.rl_reward}</span> | 
                <span class="metric">EPSILON: ${data.rl_new_eps}</span> |
                <span class="metric">INGESTION: ${data.ingestion_rate}</span><br>
                <div class="rag-report">${data.rag_explanation}</div>
                <pre style="color: #58a6ff; font-size: 11px;">Vector: ${JSON.stringify(data.cluster_delta)}</pre>
            `;
            
            if (isPoison) { div.innerHTML = "<h2>🚨 POISON CLUSTER ISOLATED 🚨</h2>" + div.innerHTML; }
            messagesDiv.insertBefore(div, messagesDiv.firstChild);
            if (messagesDiv.children.length > 20) messagesDiv.removeChild(messagesDiv.lastChild);
        };
    </script>
</body>
</html>
"""

# --- ML ENGINE INIT ---
shadow_model = PyTorchShadowModel()
art_detector = ARTDetector()
rl_warden = RLWarden()
rag_sidecar = RAGRegulatorySidecar()
stats_filter = StatisticalFilter()

@asynccontextmanager
async def lifespan(app: FastAPI):
    task = asyncio.create_task(zmq_ingestion_loop())
    yield
    task.cancel()

app = FastAPI(title="PoisonGuard", lifespan=lifespan)

class ConnectionManager:
    def __init__(self): self.active_connections: List[WebSocket] = []
    async def connect(self, ws: WebSocket): await ws.accept(); self.active_connections.append(ws)
    def disconnect(self, ws: WebSocket): self.active_connections.remove(ws)
    async def broadcast(self, msg: dict):
        for c in self.active_connections:
            try: await c.send_json(msg)
            except: pass

manager = ConnectionManager()

# --- THE ZMQ PIPELINE ---
async def zmq_ingestion_loop():
    ctx = zmq.asyncio.Context()
    sock = ctx.socket(zmq.PULL)
    sock.bind("tcp://127.0.0.1:5555")
    print("[ZMQ] Bridge Active. Awaiting Bare-Metal C++ Stream...")

    while True:
        try:
            msg = await sock.recv_string()
            payload = json.loads(msg)
            profile = payload.get("profile", "UPI")
            vector = payload["demo_vector"]

            # Layer 1: Stats
            is_stat_anomaly, stats_reason = stats_filter.check_heuristics(vector[0], vector[1])
            
            # Layer 2 & 3: Neural/Spectral
            ext = shadow_model.extract_activations(vector)
            is_poisoned, svd_flag = art_detector.detect_poison(ext)
            
            # RL Policy Tuning
            rl_feed = rl_warden.evaluate_action(is_poisoned or is_stat_anomaly, ext["true_label"])
            art_detector.eps_threshold = rl_feed["new_eps"]
            
            # RAG Auditor
            report = f"Integrity Verified. Metrics within {profile} norm."
            if is_poisoned or is_stat_anomaly or svd_flag:
                report = await asyncio.to_thread(
                    rag_sidecar.explain_threat,
                    payload["batch_id"], ext["mse_score"], vector, stats_reason, svd_flag, profile
                )

            # Broadcast
            await manager.broadcast({
                "type": "TELEMETRY_UPDATE",
                "data": {
                    "batch_id": payload["batch_id"],
                    "ingestion_rate": payload.get("ingestion_rate", "1.4 GB/s"),
                    "active_threats": 1 if (is_poisoned or is_stat_anomaly or svd_flag) else 0,
                    "rl_reward": rl_feed["reward"],
                    "rl_new_eps": rl_feed["new_eps"],
                    "rag_explanation": f"[{rl_feed['action_log']}] {report}",
                    "cluster_delta": [round(v, 4) for v in vector]
                }
            })
        except Exception as e:
            print(f"[ERROR] Pipeline Failure: {e}")

@app.get("/", response_class=HTMLResponse)
async def get_dashboard(): return html_content

@app.websocket("/ws/dashboard")
async def websocket_endpoint(ws: WebSocket):
    await manager.connect(ws)
    try:
        while True: await ws.receive_text()
    except WebSocketDisconnect:
        manager.disconnect(ws)
