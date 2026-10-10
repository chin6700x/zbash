# ☁️ zbash Cloud Bridge (oxdb-production)

Zero-dependency Python 3 HTTP/RPC daemon bridging **zbash Web-OS** in your browser directly to your Google Cloud VM instance `oxdb-production`.

---

## 🎯 Target Cloud Instance Details
- **VM Name:** `oxdb-production`
- **Zone:** `us-central1-a`
- **Project ID:** `gen-lang-client-0327258259`
- **Project Number:** `660657792212`
- **Console SSH:** [Open GCP Web SSH Console](https://ssh.cloud.google.com/v2/ssh/projects/gen-lang-client-0327258259/zones/us-central1-a/instances/oxdb-production?authuser=0&hl=en_US&projectNumber=660657792212&useAdminProxy=true)

---

## 🚀 Quick Start on Google Cloud VM

### 1. Copy or paste `oxdb_bridge.py` onto `oxdb-production`
On your GCP VM terminal:
```bash
cat << 'EOF' > oxdb_bridge.py
# (Paste the contents of oxdb_bridge.py or download via curl)
EOF
```

### 2. Run the daemon
```bash
python3 oxdb_bridge.py --port 8765
```

Output:
```
=================================================================
⚡ ZBASH CLOUD BRIDGE DAEMON (oxdb-production)
📡 Listening on: http://0.0.0.0:8765
🔗 Healthcheck:  http://localhost:8765/health
⚙️  Target VM:    oxdb-production (us-central1-a)
🔒 Project:      gen-lang-client-0327258259 (660657792212)
=================================================================
```

### 3. Connect from Browser
- In **zbash Web-OS** (`http://localhost:8085` or your hosted domain):
  - Type `@cloud <command>` (e.g. `@cloud uname -a`, `@cloud docker ps`, `@cloud htop`)
  - Or switch to the **Google Cloud VM (oxdb)** tab in the title bar!
- Execution runs directly on your GCP VM and returns live stdout/stderr with real-time microsecond latency metrics.
