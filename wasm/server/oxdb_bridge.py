#!/usr/bin/env python3
"""
oxdb_bridge.py - Lightweight Zero-Dependency Cloud Bridge for zbash Web-OS
Author: Wirot Chookeaw (chin6700x)
Copyright: (c) 2026 Wirot Chookeaw.
License: Apache License 2.0

Target: Google Cloud VM 'oxdb-production' (us-central1-a)
Project ID: gen-lang-client-0327258259
Project Number: 660657792212

Usage:
    python3 oxdb_bridge.py [--port 8765] [--host 0.0.0.0]
"""

import sys
import os
import json
import subprocess
import argparse
from http.server import HTTPServer, BaseHTTPRequestHandler

DEFAULT_PORT = 8765
DEFAULT_HOST = "0.0.0.0"

class BridgeHandler(BaseHTTPRequestHandler):
    def _set_headers(self, status=200, content_type="application/json"):
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type, Authorization")
        self.end_headers()

    def do_OPTIONS(self):
        self._set_headers(200)

    def do_GET(self):
        if self.path == "/health" or self.path == "/":
            payload = {
                "status": "online",
                "instance": "oxdb-production",
                "zone": "us-central1-a",
                "project_id": "gen-lang-client-0327258259",
                "project_number": "660657792212",
                "os": os.uname().sysname if hasattr(os, "uname") else "Linux",
                "release": os.uname().release if hasattr(os, "uname") else "",
                "arch": os.uname().machine if hasattr(os, "uname") else "x86_64"
            }
            self._set_headers(200)
            self.wfile.write(json.dumps(payload, indent=2).encode("utf-8"))
        else:
            self._set_headers(404)
            self.wfile.write(b'{"error": "Not Found"}')

    def do_POST(self):
        if self.path == "/exec":
            content_length = int(self.headers.get("Content-Length", 0))
            body = self.rfile.read(content_length).decode("utf-8")
            
            try:
                data = json.loads(body)
                command = data.get("command", "")
            except Exception:
                command = body.strip()

            if not command:
                self._set_headers(400)
                self.wfile.write(b'{"error": "Empty command"}')
                return

            print(f"[oxdb-production] Executing: {command}")

            try:
                # Execute in bash/sh with timeout protection
                proc = subprocess.run(
                    command,
                    shell=True,
                    executable="/bin/bash" if os.path.exists("/bin/bash") else "/bin/sh",
                    capture_output=True,
                    text=True,
                    timeout=30
                )
                res = {
                    "returncode": proc.returncode,
                    "stdout": proc.stdout,
                    "stderr": proc.stderr,
                    "host": "oxdb-production"
                }
                self._set_headers(200)
                self.wfile.write(json.dumps(res).encode("utf-8"))
            except subprocess.TimeoutExpired:
                self._set_headers(504)
                self.wfile.write(json.dumps({"error": "Command execution timed out (30s limit)"}).encode("utf-8"))
            except Exception as e:
                self._set_headers(500)
                self.wfile.write(json.dumps({"error": str(e)}).encode("utf-8"))
        else:
            self._set_headers(404)
            self.wfile.write(b'{"error": "Endpoint not found"}')

def run_server(host=DEFAULT_HOST, port=DEFAULT_PORT):
    server = HTTPServer((host, port), BridgeHandler)
    print("=" * 65)
    print("⚡ ZBASH CLOUD BRIDGE DAEMON (oxdb-production)")
    print(f"📡 Listening on: http://{host}:{port}")
    print("🔗 Healthcheck:  http://localhost:8765/health")
    print("⚙️  Target VM:    oxdb-production (us-central1-a)")
    print("🔒 Project:      gen-lang-client-0327258259 (660657792212)")
    print("=" * 65)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[oxdb-production] Daemon stopped gracefully.")
        server.server_close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="zbash Cloud Bridge for oxdb-production")
    parser.add_argument("--host", default=DEFAULT_HOST, help="Host to bind (default: 0.0.0.0)")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help="Port to listen (default: 8765)")
    args = parser.parse_args()
    run_server(args.host, args.port)
