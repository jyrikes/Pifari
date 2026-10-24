#!/usr/bin/env python3
"""Worker de dispositivos do Pifari.

Roda no lado Windows (a iGPU não é alcançável de dentro do WSL) e responde ao
interpretador o que existe de hardware realmente utilizável.

Modos:
  --list   lista dispositivos e sai (usado por backend/devices.c)
  --stdio  servidor persistente para execução de kernels (E4)
"""

import sys


def detect_devices():
    """Devolve a lista de dispositivos OpenCL disponíveis."""
    devices = []
    try:
        import pyopencl as cl
    except Exception:
        return devices
    try:
        platforms = cl.get_platforms()
    except Exception:
        return devices

    index = 0
    for platform in platforms:
        try:
            raw = platform.get_devices()
        except Exception:
            continue
        for dev in raw:
            ext = dev.extensions or ""
            devices.append(
                {
                    "id": f"gpu:{index}",
                    "backend": "opencl",
                    "name": (dev.name or "GPU").strip(),
                    "mem_mb": int(dev.global_mem_size) // (1024 * 1024),
                    "workgroup": int(dev.max_work_group_size),
                    "units": int(dev.max_compute_units),
                    "fp64": "cl_khr_fp64" in ext,
                    "platform": (platform.name or "").strip(),
                    "version": (dev.version or "").strip(),
                }
            )
            index += 1
    return devices


def cmd_list():
    for dev in detect_devices():
        ops = ["map", "matmul", "relu"]
        fields = [
            "DEV",
            dev["id"],
            dev["backend"],
            dev["name"],
            str(dev["mem_mb"]),
            ",".join(ops),
            "fp64=%d" % (1 if dev["fp64"] else 0),
            "wg=%d" % dev["workgroup"],
            "cu=%d" % dev["units"],
        ]
        print("\t".join(fields), flush=True)
    print("END", flush=True)


def cmd_stdio():
    cmd_list()
    for line in sys.stdin:
        if line.strip() == "QUIT":
            break


def main(argv):
    if "--list" in argv:
        cmd_list()
        return 0
    if "--stdio" in argv:
        cmd_stdio()
        return 0
    print("uso: pifari_worker.py --list | --stdio", file=sys.stderr)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
