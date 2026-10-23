#!/usr/bin/env python3
"""
Benchmark Comparativo Head-to-Head:
Servidor MCP em Pifari (examples/servidor_mcp_pifari.pif)
  vs.
Servidor MCP em Python (examples/servidor_mcp_python_ref.py)

Mede sobre pipes stdio reais (JSON-RPC 2.0 / MCP 2025-03-26):
1. Cold-Start + Handshake (`initialize`) em ms
2. Throughput (req/s) e latencia p50/p99 de protocolo (`ping_echo`, 400 chamadas)
3. Ferramenta MCP CPU escalar (`soma_quadrados`, N=200.000)
4. Ferramenta MCP Vetorial F64 (`processar_vetor_f64`, N=100.000)
5. Ferramenta MCP Algebra Linear (`matmul_bench`, 35x35)
6. Consumo real de memoria residente (VmRSS em KB/MB via /proc/<pid>/status)
7. Ergonomia e tamanho de codigo (Linhas de Codigo / LOC)
"""

import json
import os
import statistics
import subprocess
import time


def read_vm_rss_kb(pid: int) -> int:
    try:
        with open(f"/proc/{pid}/status", "r", encoding="utf-8") as f:
            for line in f:
                if line.startswith("VmRSS:"):
                    parts = line.split()
                    return int(parts[1])
    except Exception:
        pass
    return 0


def rpc_call(proc: subprocess.Popen, req_id: int, method: str, params: dict) -> dict:
    payload = json.dumps(
        {"jsonrpc": "2.0", "id": req_id, "method": method, "params": params},
        separators=(",", ":"),
    )
    proc.stdin.write(payload + "\n")
    proc.stdin.flush()
    line = proc.stdout.readline()
    if not line:
        raise RuntimeError(f"EOF inesperado do servidor MCP em {method}")
    return json.loads(line)


def measure_cold_start_ms(cmd: list[str], runs: int = 7) -> float:
    samples = []
    for _ in range(runs):
        t0 = time.perf_counter()
        p = subprocess.Popen(
            cmd,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
        )
        resp = rpc_call(
            p,
            1,
            "initialize",
            {
                "protocolVersion": "2025-03-26",
                "capabilities": {},
                "clientInfo": {"name": "bench-client", "version": "1.0"},
            },
        )
        t1 = time.perf_counter()
        assert resp["result"]["protocolVersion"] == "2025-03-26"
        p.stdin.close()
        p.wait(timeout=3)
        samples.append((t1 - t0) * 1000.0)
    return statistics.median(samples)


def benchmark_server(label: str, cmd: list[str]) -> dict:
    cold_ms = measure_cold_start_ms(cmd, runs=7)

    proc = subprocess.Popen(
        cmd,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1,
    )

    req_id = 1
    init_resp = rpc_call(
        proc,
        req_id,
        "initialize",
        {
            "protocolVersion": "2025-03-26",
            "capabilities": {},
            "clientInfo": {"name": "bench-client", "version": "1.0"},
        },
    )
    assert init_resp["result"]["protocolVersion"] == "2025-03-26"

    req_id += 1
    tools_resp = rpc_call(proc, req_id, "tools/list", {})
    assert len(tools_resp["result"]["tools"]) == 4

    # 1. Protocolo puro: 400 chamadas ping_echo
    ping_latencies = []
    t_ping_start = time.perf_counter()
    for i in range(400):
        req_id += 1
        t0 = time.perf_counter()
        r = rpc_call(
            proc,
            req_id,
            "tools/call",
            {"name": "ping_echo", "arguments": {"msg": f"pkt_{i}"}},
        )
        t1 = time.perf_counter()
        assert r["result"]["content"][0]["text"] == f"PONG:pkt_{i}"
        ping_latencies.append((t1 - t0) * 1000.0)
    t_ping_total = time.perf_counter() - t_ping_start
    ping_rps = 400.0 / t_ping_total
    ping_latencies.sort()
    ping_p50 = ping_latencies[len(ping_latencies) // 2]
    ping_p99 = ping_latencies[int(len(ping_latencies) * 0.99)]

    # 2. Soma quadrados (N=200.000) — warmup + 7 execucoes
    soma_samples = []
    soma_val = None
    for i in range(8):
        req_id += 1
        t0 = time.perf_counter()
        r = rpc_call(
            proc,
            req_id,
            "tools/call",
            {"name": "soma_quadrados", "arguments": {"limite": 200000}},
        )
        t1 = time.perf_counter()
        soma_val = r["result"]["content"][0]["text"]
        if i > 0:
            soma_samples.append((t1 - t0) * 1000.0)

    # 3. Pipeline Vetorial F64 (N=100.000) — warmup + 7 execucoes
    vec_samples = []
    vec_val = None
    for i in range(8):
        req_id += 1
        t0 = time.perf_counter()
        r = rpc_call(
            proc,
            req_id,
            "tools/call",
            {"name": "processar_vetor_f64", "arguments": {"tamanho": 100000}},
        )
        t1 = time.perf_counter()
        vec_val = float(r["result"]["content"][0]["text"])
        if i > 0:
            vec_samples.append((t1 - t0) * 1000.0)

    # 4. Multiplicacao de Matrizes (35x35) — warmup + 5 execucoes
    mat_samples = []
    mat_val = None
    for i in range(6):
        req_id += 1
        t0 = time.perf_counter()
        r = rpc_call(
            proc,
            req_id,
            "tools/call",
            {"name": "matmul_bench", "arguments": {"dim": 35}},
        )
        t1 = time.perf_counter()
        mat_val = float(r["result"]["content"][0]["text"])
        if i > 0:
            mat_samples.append((t1 - t0) * 1000.0)

    rss_kb = read_vm_rss_kb(proc.pid)

    proc.stdin.close()
    proc.wait(timeout=3)

    return {
        "label": label,
        "cold_start_ms": cold_ms,
        "ping_rps": ping_rps,
        "ping_p50_ms": ping_p50,
        "ping_p99_ms": ping_p99,
        "soma_ms": statistics.median(soma_samples),
        "soma_val": soma_val,
        "vec_ms": statistics.median(vec_samples),
        "vec_val": vec_val,
        "matmul_ms": statistics.median(mat_samples),
        "matmul_val": mat_val,
        "rss_kb": rss_kb,
    }


def count_non_empty_lines(path: str) -> int:
    with open(path, "r", encoding="utf-8") as f:
        return sum(
            1
            for line in f
            if line.strip()
            and not line.strip().startswith("@")
            and not line.strip().startswith("#")
            and not line.strip().startswith('"""')
        )


def main():
    print("=== BENCHMARK HEAD-TO-HEAD: SERVIDOR MCP PIFARI vs PYTHON ===")
    pif = benchmark_server("Pifari MCP Server", ["./pifari", "examples/servidor_mcp_pifari.pif"])
    py = benchmark_server("Python MCP Server", ["python3", "examples/servidor_mcp_python_ref.py"])

    # Validacao de equivalencia matematica exata das ferramentas
    assert pif["soma_val"] == py["soma_val"], f"Diferenca em soma_quadrados: {pif['soma_val']} vs {py['soma_val']}"
    assert abs(pif["vec_val"] - py["vec_val"]) < 1e-6, f"Diferenca em vetor F64: {pif['vec_val']} vs {py['vec_val']}"
    assert abs(pif["matmul_val"] - py["matmul_val"]) < 1e-6, f"Diferenca em matmul: {pif['matmul_val']} vs {py['matmul_val']}"

    pif_loc = count_non_empty_lines("examples/servidor_mcp_pifari.pif")
    py_loc = count_non_empty_lines("examples/servidor_mcp_python_ref.py")

    print(f"\n[Validacao Matematica] OK (soma={pif['soma_val']}, vec={pif['vec_val']:.4f}, matmul={pif['matmul_val']:.4f})")
    print("-" * 82)
    print(f"{'Metrica':<38} | {'Pifari MCP':>14} | {'Python MCP':>14} | {'Vantagem Pifari':>14}")
    print("-" * 82)
    print(
        f"{'1. Cold-Start + Handshake (ms)':<38} | {pif['cold_start_ms']:>11.2f} ms | {py['cold_start_ms']:>11.2f} ms | {py['cold_start_ms']/pif['cold_start_ms']:>12.2f}x"
    )
    print(
        f"{'2. Throughput JSON-RPC (req/s)':<38} | {pif['ping_rps']:>10.0f} r/s | {py['ping_rps']:>10.0f} r/s | {pif['ping_rps']/py['ping_rps']:>12.2f}x"
    )
    print(
        f"{'3. Latencia Protocolo p50 (ms)':<38} | {pif['ping_p50_ms']:>11.3f} ms | {py['ping_p50_ms']:>11.3f} ms | {py['ping_p50_ms']/pif['ping_p50_ms']:>12.2f}x"
    )
    print(
        f"{'4. Tool CPU Escalar (200k iter) (ms)':<38} | {pif['soma_ms']:>11.3f} ms | {py['soma_ms']:>11.3f} ms | {py['soma_ms']/pif['soma_ms']:>12.2f}x"
    )
    print(
        f"{'5. Tool Vetorial F64 (100k elem) (ms)':<38} | {pif['vec_ms']:>11.3f} ms | {py['vec_ms']:>11.3f} ms | {py['vec_ms']/pif['vec_ms']:>12.2f}x"
    )
    print(
        f"{'6. Tool Matmul 35x35 (ms)':<38} | {pif['matmul_ms']:>11.3f} ms | {py['matmul_ms']:>11.3f} ms | {py['matmul_ms']/pif['matmul_ms']:>12.2f}x"
    )
    if pif["rss_kb"] > 0 and py["rss_kb"] > 0:
        print(
            f"{'7. Memoria RAM Residente (VmRSS)':<38} | {pif['rss_kb']/1024.0:>11.2f} MB | {py['rss_kb']/1024.0:>11.2f} MB | {py['rss_kb']/pif['rss_kb']:>11.2f}x menor"
        )
    print(
        f"{'8. Linhas de Codigo (LOC efetivas)':<38} | {pif_loc:>10d} LOC | {py_loc:>10d} LOC | {py_loc/pif_loc:>11.2f}x menor"
    )
    print("-" * 82)


if __name__ == "__main__":
    main()
