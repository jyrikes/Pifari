#!/usr/bin/env python3
"""
Servidor MCP de Referencia em Python (Protocolo 2025-03-26 sobre stdio)
Implementa exatamente as mesmas 4 ferramentas e 1 recurso de examples/servidor_mcp_pifari.pif
para comparacao direta (head-to-head) de desempenho, memoria e ergonomia.
"""

import json
import sys

PROTOCOL_VERSION = "2025-03-26"


def ping_echo(msg: str) -> str:
    return f"PONG:{msg}"


def soma_quadrados(limite: int) -> int:
    soma = 0
    for i in range(int(limite)):
        soma += i * i - i // 2
    return soma


def processar_vetor_f64(tamanho: int) -> float:
    n = int(tamanho)
    arr = [float(i) for i in range(n)]
    out = [0.0] * n
    for i in range(n):
        x = arr[i] * 1.75 + 2.5
        out[i] = x * x * 0.001 + x
    return out[-1]


def matmul_bench(dim: int) -> float:
    n = int(dim)
    A = [[(i + j + 1) * 0.5 for j in range(n)] for i in range(n)]
    B = [[(i * 2 + j + 1) * 0.25 for j in range(n)] for i in range(n)]
    C = [[0.0] * n for _ in range(n)]
    for i in range(n):
        row_a = A[i]
        row_c = C[i]
        for k in range(n):
            aik = row_a[k]
            row_b = B[k]
            for j in range(n):
                row_c[j] += aik * row_b[j]
    return C[n - 1][n - 1]


TOOLS = [
    {
        "name": "ping_echo",
        "description": "Ecoa mensagem para medir latencia pura JSON-RPC",
        "annotations": {"readOnlyHint": True, "sideEffect": False},
        "inputSchema": {"type": "object", "properties": {"msg": {"type": "string"}}},
    },
    {
        "name": "soma_quadrados",
        "description": "Calcula soma quadratica intensiva em CPU",
        "annotations": {"readOnlyHint": True, "sideEffect": False},
        "inputSchema": {"type": "object", "properties": {"limite": {"type": "string"}}},
    },
    {
        "name": "processar_vetor_f64",
        "description": "Executa cadeia vetorial F64",
        "annotations": {"readOnlyHint": True, "sideEffect": False},
        "inputSchema": {
            "type": "object",
            "properties": {"tamanho": {"type": "string"}},
        },
    },
    {
        "name": "matmul_bench",
        "description": "Executa multiplicacao de matrizes NxN",
        "annotations": {"readOnlyHint": True, "sideEffect": False},
        "inputSchema": {"type": "object", "properties": {"dim": {"type": "string"}}},
    },
]

RESOURCES = [
    {
        "uri": "resource://pifari/manifest",
        "name": "Manifesto do Servidor MCP Python",
        "mimeType": "application/json",
    }
]


def handle_msg(msg: dict):
    method = msg.get("method", "")
    req_id = msg.get("id", None)
    params = msg.get("params") or {}

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
                },
                "serverInfo": {"name": "python-reference-mcp", "version": "1.0.0"},
            },
        }

    if method == "ping":
        return {"jsonrpc": "2.0", "id": req_id, "result": {}}

    if method == "tools/list":
        return {"jsonrpc": "2.0", "id": req_id, "result": {"tools": TOOLS}}

    if method == "resources/list":
        return {"jsonrpc": "2.0", "id": req_id, "result": {"resources": RESOURCES}}

    if method == "resources/read":
        uri = params.get("uri", "")
        if uri == "resource://pifari/manifest":
            return {
                "jsonrpc": "2.0",
                "id": req_id,
                "result": {
                    "contents": [
                        {
                            "uri": uri,
                            "mimeType": "application/json",
                            "text": '{"server":"python-reference-mcp","language":"Python","protocol":"2025-03-26"}',
                        }
                    ]
                },
            }
        return {
            "jsonrpc": "2.0",
            "id": req_id,
            "error": {"code": -32602, "message": "Resource not found"},
        }

    if method == "tools/call":
        name = params.get("name", "")
        args = params.get("arguments") or {}
        if name == "ping_echo":
            res = ping_echo(str(args.get("msg", "")))
        elif name == "soma_quadrados":
            res = f"{soma_quadrados(int(args.get('limite', 0)))}"
        elif name == "processar_vetor_f64":
            res = f"{processar_vetor_f64(int(args.get('tamanho', 0))):.15g}"
        elif name == "matmul_bench":
            res = f"{matmul_bench(int(args.get('dim', 0))):.15g}"
        else:
            return {
                "jsonrpc": "2.0",
                "id": req_id,
                "error": {"code": -32601, "message": "Unknown tool"},
            }
        return {
            "jsonrpc": "2.0",
            "id": req_id,
            "result": {
                "content": [{"type": "text", "text": str(res)}],
                "isError": False,
            },
        }

    return {
        "jsonrpc": "2.0",
        "id": req_id,
        "error": {"code": -32601, "message": "Method not found"},
    }


def main():
    try:
        for raw in sys.stdin:
            line = raw.strip()
            if not line:
                continue
            msg = json.loads(line)
            resp = handle_msg(msg)
            if resp is not None:
                sys.stdout.write(json.dumps(resp, separators=(",", ":")) + "\n")
                sys.stdout.flush()
    except (BrokenPipeError, IOError):
        pass


if __name__ == "__main__":
    main()
