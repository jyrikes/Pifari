#!/usr/bin/env python3
"""
Controlled MCP Test Server for Pifari Agent Runtime & MCP Gateway.
Supports JSON-RPC 2.0 over:
  - stdio (default)
  - HTTP (--http <port>) with Mcp-Session-Id header and JSON/SSE response modes.
All operations are deterministic, local, and reversible.
"""

import json
import sys
import time
from http.server import BaseHTTPRequestHandler, HTTPServer

PROTOCOL_VERSION = "2025-03-26"

TOOLS = [
    {
        "name": "collect_dataset",
        "description": "Coleta serie numerica deterministica de telemetria cientifica",
        "annotations": {"readOnlyHint": True, "sideEffect": False},
        "inputSchema": {
            "type": "object",
            "properties": {"source": {"type": "string"}},
            "required": ["source"],
        },
    },
    {
        "name": "analyze_series",
        "description": "Analisa metricas estatisticas e anomalias de serie coletada",
        "annotations": {"readOnlyHint": True, "sideEffect": False},
        "inputSchema": {
            "type": "object",
            "properties": {"dataset": {"type": "string"}},
            "required": ["dataset"],
        },
    },
    {
        "name": "build_report",
        "description": "Sintetiza relatorio executivo consolidado a partir do contexto",
        "annotations": {"readOnlyHint": True, "sideEffect": False},
        "inputSchema": {
            "type": "object",
            "properties": {
                "collector_summary": {"type": "string"},
                "analysis_summary": {"type": "string"},
            },
        },
    },
    {
        "name": "write_audit_entry",
        "description": "Registra entrada mutavel de auditoria (nao idempotente)",
        "annotations": {"readOnlyHint": False, "sideEffect": True},
        "inputSchema": {
            "type": "object",
            "properties": {"entry": {"type": "string"}},
            "required": ["entry"],
        },
    },
    {
        "name": "slow_op",
        "description": "Operacao lenta controlada para testar timeout e cancelamento",
        "annotations": {"readOnlyHint": True, "sideEffect": False},
        "inputSchema": {
            "type": "object",
            "properties": {"delay_ms": {"type": "integer"}},
        },
    },
    {
        "name": "restricted_op",
        "description": "Operacao sensivel bloqueada por padrao pelo Policy Engine",
        "annotations": {"readOnlyHint": False, "sideEffect": True},
        "inputSchema": {"type": "object"},
    },
]

RESOURCES = [
    {
        "uri": "resource://pifari/runtime_spec",
        "name": "Pifari Runtime Spec",
        "mimeType": "application/json",
    },
    {
        "uri": "resource://pifari/dataset_catalog",
        "name": "Catalogo de Datasets de Teste",
        "mimeType": "text/plain",
    },
]


def handle_rpc_Object(msg: dict):
    method = msg.get("method", "")
    req_id = msg.get("id", None)
    params = msg.get("params") or {}

    # Notifications do not return a JSON-RPC response
    if req_id is None or method.startswith("notifications/"):
        return None

    if method == "initialize":
        return {
            "jsonrpc": "2.0",
            "id": req_id,
            "result": {
                "protocolVersion": PROTOCOL_VERSION,
                "capabilities": {
                    "tools": {"listChanged": False},
                    "resources": {"subscribe": False, "listChanged": False},
                    "prompts": {"listChanged": False},
                },
                "serverInfo": {
                    "name": "pifari-mock-mcp-server",
                    "version": "1.0.0",
                },
            },
        }

    if method == "tools/list":
        return {
            "jsonrpc": "2.0",
            "id": req_id,
            "result": {"tools": TOOLS},
        }

    if method == "resources/list":
        return {
            "jsonrpc": "2.0",
            "id": req_id,
            "result": {"resources": RESOURCES},
        }

    if method == "resources/read":
        uri = params.get("uri", "")
        if uri == "resource://pifari/runtime_spec":
            text = (
                '{"engine":"Pifari-Agent-Runtime","version":"3.1","mcp":"2025-03-26"}'
            )
        elif uri == "resource://pifari/dataset_catalog":
            text = "datasets: [sensor_alpha, quantum_flux, mlp_weights]"
        else:
            return {
                "jsonrpc": "2.0",
                "id": req_id,
                "error": {"code": -32602, "message": f"Resource not found: {uri}"},
            }
        return {
            "jsonrpc": "2.0",
            "id": req_id,
            "result": {
                "contents": [{"uri": uri, "mimeType": "text/plain", "text": text}]
            },
        }

    if method == "tools/call":
        name = params.get("name", "")
        args = params.get("arguments") or {}

        if name == "collect_dataset":
            src = args.get("source", "default")
            text = f"dataset={src};samples=4;values=[12.5,18.0,24.5,31.0];checksum=OK"
            return {
                "jsonrpc": "2.0",
                "id": req_id,
                "result": {
                    "content": [{"type": "text", "text": text}],
                    "isError": False,
                },
            }

        if name == "analyze_series":
            ds = args.get("dataset", "unknown")
            text = f"analysis_for=({ds});mean=21.50;max=31.00;anomaly=false;confidence=0.99"
            return {
                "jsonrpc": "2.0",
                "id": req_id,
                "result": {
                    "content": [{"type": "text", "text": text}],
                    "isError": False,
                },
            }

        if name == "build_report":
            c_sum = args.get("collector_summary", "")
            a_sum = args.get("analysis_summary", "")
            text = f"RELATORIO_CONSOLIDADO | Coleta=[{c_sum}] | Analise=[{a_sum}] | Status=VERIFICADO"
            return {
                "jsonrpc": "2.0",
                "id": req_id,
                "result": {
                    "content": [{"type": "text", "text": text}],
                    "isError": False,
                },
            }

        if name == "write_audit_entry":
            entry = args.get("entry", "noop")
            text = f"AUDIT_COMMITTED:{entry}"
            return {
                "jsonrpc": "2.0",
                "id": req_id,
                "result": {
                    "content": [{"type": "text", "text": text}],
                    "isError": False,
                },
            }

        if name == "slow_op":
            delay_ms = int(args.get("delay_ms", 400))
            time.sleep(delay_ms / 1000.0)
            return {
                "jsonrpc": "2.0",
                "id": req_id,
                "result": {
                    "content": [
                        {"type": "text", "text": f"completed_after_{delay_ms}ms"}
                    ],
                    "isError": False,
                },
            }

        if name == "restricted_op":
            return {
                "jsonrpc": "2.0",
                "id": req_id,
                "result": {
                    "content": [{"type": "text", "text": "RESTRICTED_EXECUTED"}],
                    "isError": False,
                },
            }

        return {
            "jsonrpc": "2.0",
            "id": req_id,
            "error": {"code": -32601, "message": f"Unknown tool: {name}"},
        }

    return {
        "jsonrpc": "2.0",
        "id": req_id,
        "error": {"code": -32601, "message": f"Method not found: {method}"},
    }


def run_stdio():
    try:
        for raw_line in sys.stdin:
            line = raw_line.strip()
            if not line:
                continue
            try:
                msg = json.loads(line)
            except Exception as exc:
                err = {
                    "jsonrpc": "2.0",
                    "id": None,
                    "error": {"code": -32700, "message": f"Parse error: {exc}"},
                }
                sys.stdout.write(json.dumps(err, separators=(",", ":")) + "\n")
                sys.stdout.flush()
                continue

            resp = handle_rpc_Object(msg)
            if resp is not None:
                sys.stdout.write(json.dumps(resp, separators=(",", ":")) + "\n")
                sys.stdout.flush()
    except (BrokenPipeError, IOError):
        try:
            sys.stdout.close()
        except Exception:
            pass


class McpHttpHandler(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        pass

    def do_POST(self):
        length = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(length).decode("utf-8", errors="replace")
        try:
            msg = json.loads(body)
        except Exception:
            self.send_response(400)
            self.end_headers()
            return

        resp = handle_rpc_Object(msg)
        if resp is None:
            self.send_response(202)
            self.send_header("Mcp-Session-Id", "pifari-http-session-01")
            self.send_header("Content-Length", "2")
            self.end_headers()
            self.wfile.write(b"{}")
            return

        payload = json.dumps(resp, separators=(",", ":")).encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Mcp-Session-Id", "pifari-http-session-01")
        self.send_header("Content-Length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)


def run_http(port: int):
    server = HTTPServer(("127.0.0.1", port), McpHttpHandler)
    server.serve_forever()


if __name__ == "__main__":
    if len(sys.argv) >= 3 and sys.argv[1] == "--http":
        run_http(int(sys.argv[2]))
    else:
        run_stdio()
