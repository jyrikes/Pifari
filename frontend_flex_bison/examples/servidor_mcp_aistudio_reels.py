#!/usr/bin/env python3
"""
Servidor MCP (2025-03-26) para Criacao de Videos de Reels (9:16)
Integrado ao Google AI Studio (Free Tier via GEMINI_API_KEY) + Prompts Veo 2 / FFmpeg.
Nunca registra, imprime ou expoe chaves de API em logs ou respostas.
"""
import json
import os
import sys
import urllib.error
import urllib.request
from pathlib import Path

PROTOCOL_VERSION = "2025-03-26"


def _load_dotenv_if_present() -> None:
    env_path = Path(__file__).resolve().parent.parent / ".env"
    if not env_path.is_file():
        return
    for raw in env_path.read_text(encoding="utf-8-sig", errors="ignore").splitlines():
        line = raw.strip().lstrip("\ufeff")
        if not line or line.startswith("#") or "=" not in line:
            continue
        k, v = line.split("=", 1)
        k = k.strip()
        v = v.strip().strip('"').strip("'")
        if k and v and k not in os.environ:
            os.environ[k] = v


_load_dotenv_if_present()


def _call_gemini_aistudio(prompt: str) -> str:
    api_key = os.environ.get("GEMINI_API_KEY", "").strip()
    model = os.environ.get("GEMINI_MODEL", "gemini-2.5-flash").strip() or "gemini-2.5-flash"
    if not api_key:
        return ""
    url = f"https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent?key={api_key}"
    payload = {
        "contents": [{"parts": [{"text": prompt}]}],
        "generationConfig": {
            "temperature": 0.4,
            "maxOutputTokens": 600,
            "thinkingConfig": {"thinkingBudget": 0},
        },
    }
    req = urllib.request.Request(
        url,
        data=json.dumps(payload).encode("utf-8"),
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=8.0) as resp:
            body = json.loads(resp.read().decode("utf-8"))
            cands = body.get("candidates", [])
            if cands:
                parts = cands[0].get("content", {}).get("parts", [])
                if parts and "text" in parts[0]:
                    clean = parts[0]["text"].strip().replace("\n", " ").replace("\r", " ")
                    clean = clean.replace('"', "'").replace("\\", "/")
                    return clean
    except Exception:
        return ""
    return ""


def tool_gerar_roteiro_reel(args: dict) -> str:
    tema = str(args.get("tema", "Produtividade e IA"))
    duracao = int(args.get("duracao_seg", 15))
    live_resp = _call_gemini_aistudio(
        f"Crie um roteiro curto de Reels vertical 9:16 de {duracao}s sobre '{tema}' "
        "em uma unica linha no formato HOOK(0-3s)|CORPO(3-12s)|CTA(12-15s)."
    )
    if live_resp:
        return f"[AI_STUDIO_LIVE] {live_resp}"
    return (
        f"[AI_STUDIO_READY] TEMA={tema} ({duracao}s, 9:16) | "
        "HOOK(0-3s): Pare de perder horas editando manualmente! | "
        "CORPO(3-12s): Com Pifari + Google AI Studio (Gemini e Veo), multiplos agentes criam roteiro, "
        "cenas verticais 1080x1920 e legendas em milissegundos. | "
        "CTA(12-15s): Salve este Reel e teste agora!"
    )


def tool_gerar_prompts_veo_vertical(args: dict) -> str:
    roteiro = str(args.get("roteiro", ""))
    return (
        "VEO2_SPEC[aspect=9:16,res=1080x1920,fps=24] | "
        "CENA_1(0-3s): Close-up dinamico neon cyberpunk developer screen, fast zoom-in, 9:16 vertical | "
        "CENA_2(3-12s): Holographic multi-agent workflow nodes connecting at high speed, cinematic lighting | "
        f"CENA_3(12-15s): Clean mobile Reel frame with bold call-to-action overlay | REF=({roteiro[:48]}...)"
    )


def tool_gerar_pacote_render_ffmpeg(args: dict) -> str:
    roteiro = str(args.get("roteiro", ""))
    veo = str(args.get("prompts_veo", ""))
    metricas_fps = str(args.get("metricas_fps", "0"))
    return (
        "REEL_PACKAGE_OK | FORMAT=MP4_VERTICAL_1080x1920_24FPS | "
        f"PIFARI_PACING_SCORE={metricas_fps} | "
        "SRT_CAPTIONS=[1: 00:00:00,000 --> 00:00:03,000 Hook / 2: 00:00:03,000 --> 00:00:12,000 Body / "
        "3: 00:00:12,000 --> 00:00:15,000 CTA] | "
        "FFMPEG_CMD='ffmpeg -y -i veo_scene_9x16.mp4 -i tts_voice.wav -vf subtitles=reel.srt "
        "-c:v libx264 -pix_fmt yuv420p -c:a aac reel_final_1080x1920.mp4' | "
        f"SUMMARY=({roteiro[:40]} | {veo[:36]})"
    )


TOOLS = [
    {
        "name": "gerar_roteiro_reel",
        "description": "Gera roteiro estruturado de Reels (Hook 3s + Corpo + CTA) via Google AI Studio (Gemini)",
        "annotations": {"readOnlyHint": True, "sideEffect": False},
        "inputSchema": {
            "type": "object",
            "properties": {"tema": {"type": "string"}, "duracao_seg": {"type": "integer"}},
        },
    },
    {
        "name": "gerar_prompts_veo_vertical",
        "description": "Converte o roteiro em prompts visuais 9:16 (1080x1920) para o Google Veo 2 / Imagen 3",
        "annotations": {"readOnlyHint": True, "sideEffect": False},
        "inputSchema": {"type": "object", "properties": {"roteiro": {"type": "string"}}},
    },
    {
        "name": "gerar_pacote_render_ffmpeg",
        "description": "Monta pacote final de renderizacao 9:16 com legendas SRT e comando FFmpeg",
        "annotations": {"readOnlyHint": True, "sideEffect": False},
        "inputSchema": {
            "type": "object",
            "properties": {
                "roteiro": {"type": "string"},
                "prompts_veo": {"type": "string"},
                "metricas_fps": {"type": "string"},
            },
        },
    },
]


def handle_rpc(req: dict) -> dict | None:
    method = req.get("method", "")
    req_id = req.get("id")
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
                "serverInfo": {"name": "aistudio-reels-mcp", "version": "1.0.0"},
            },
        }
    if method == "tools/list":
        return {"jsonrpc": "2.0", "id": req_id, "result": {"tools": TOOLS}}
    if method == "resources/list":
        return {
            "jsonrpc": "2.0",
            "id": req_id,
            "result": {
                "resources": [
                    {
                        "uri": "resource://aistudio/reels_config",
                        "name": "Configuracao AI Studio & Veo Reels",
                        "mimeType": "application/json",
                    }
                ]
            },
        }
    if method == "resources/read":
        has_key = bool(os.environ.get("GEMINI_API_KEY", "").strip())
        cfg = {
            "provider": "Google AI Studio + Vertex Veo",
            "billing_required_for_gemini": False,
            "gemini_api_key_configured": has_key,
            "aspect_ratio": "9:16 (1080x1920)",
            "models": ["gemini-2.5-flash", "veo-2.0-generate-001", "imagen-3.0-generate-002"],
        }
        return {
            "jsonrpc": "2.0",
            "id": req_id,
            "result": {
                "contents": [
                    {
                        "uri": "resource://aistudio/reels_config",
                        "mimeType": "application/json",
                        "text": json.dumps(cfg),
                    }
                ]
            },
        }
    if method == "tools/call":
        params = req.get("params", {})
        tname = params.get("name", "")
        args = params.get("arguments", {}) or {}
        if tname == "gerar_roteiro_reel":
            out = tool_gerar_roteiro_reel(args)
        elif tname == "gerar_prompts_veo_vertical":
            out = tool_gerar_prompts_veo_vertical(args)
        elif tname == "gerar_pacote_render_ffmpeg":
            out = tool_gerar_pacote_render_ffmpeg(args)
        else:
            return {"jsonrpc": "2.0", "id": req_id, "error": {"code": -32601, "message": "Unknown tool"}}
        return {
            "jsonrpc": "2.0",
            "id": req_id,
            "result": {"content": [{"type": "text", "text": out}], "isError": False},
        }
    return {"jsonrpc": "2.0", "id": req_id, "error": {"code": -32601, "message": "Method not found"}}


def main() -> None:
    for raw in sys.stdin:
        line = raw.strip()
        if not line:
            continue
        resp = handle_rpc(json.loads(line))
        if resp is not None:
            sys.stdout.write(json.dumps(resp) + "\n")
            sys.stdout.flush()


if __name__ == "__main__":
    main()
