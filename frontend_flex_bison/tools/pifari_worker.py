#!/usr/bin/env python3
"""Worker de dispositivos do Pifari.

Roda no lado Windows (a iGPU não é alcançável de dentro do WSL) e responde ao
interpretador o que existe de hardware realmente utilizável.

Modos:
  --list   lista dispositivos e sai (usado por backend/devices.c)
  --stdio  servidor persistente para execução de kernels (usado por backend/gpu_client.c)

Protocolo --stdio: uma requisição por linha, resposta por linha.
  AFFINE <n> <escala> <deslocamento> <base64 de n doubles>
  -> OK <base64 de n doubles>   ou   ERR <mensagem>
  QUIT
"""

import base64
import sys

import numpy as np

_KERNEL = """
__kernel void affine_relu(__global const float *x, __global float *y,
                          const float scale, const float bias, const int n) {
    int i = get_global_id(0);
    if (i < n) {
        float v = scale * x[i] + bias;
        y[i] = v > 0.0f ? v : 0.0f;
    }
}
"""


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
        fields = [
            "DEV",
            dev["id"],
            dev["backend"],
            dev["name"],
            str(dev["mem_mb"]),
            "map,matmul,relu",
            "fp64=%d" % (1 if dev["fp64"] else 0),
            "wg=%d" % dev["workgroup"],
            "cu=%d" % dev["units"],
        ]
        print("\t".join(fields), flush=True)
    print("END", flush=True)


class Gpu:
    """Contexto OpenCL reaproveitado entre requisições."""

    def __init__(self):
        import pyopencl as cl

        self.cl = cl
        self.platform = cl.get_platforms()[0]
        self.device = self.platform.get_devices()[0]
        self.ctx = cl.Context([self.device])
        self.queue = cl.CommandQueue(self.ctx)
        self.prog = cl.Program(self.ctx, _KERNEL).build()
        # Kernel reutilizado entre chamadas: recuperar de novo custa caro.
        self._affine = cl.Kernel(self.prog, "affine_relu")
        self._affine.set_scalar_arg_dtypes([None, None, np.float32, np.float32, np.int32])

    def affine_relu(self, x, scale, bias):
        cl = self.cl
        n = len(x)
        mf = cl.mem_flags
        xb = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=x)
        yb = cl.Buffer(self.ctx, mf.WRITE_ONLY, xb.size)
        kernel = self._affine
        gws = (int((n + 255) // 256) * 256,)
        kernel(
            self.queue,
            gws,
            (256,),
            xb,
            yb,
            np.float32(scale),
            np.float32(bias),
            np.int32(n),
        )
        out = np.empty_like(x)
        cl.enqueue_copy(self.queue, out, yb)
        self.queue.finish()
        return out


def cmd_stdio():
    gpu = None
    for raw in sys.stdin:
        line = raw.strip()
        if not line:
            continue
        if line == "QUIT":
            break
        if line.startswith("AFFINE "):
            try:
                _, n_s, scale_s, bias_s, payload = line.split(" ", 4)
                n = int(n_s)
                data = base64.b64decode(payload)
                x = np.frombuffer(data, dtype=np.float64)[:n].astype(np.float32)
                if gpu is None:
                    gpu = Gpu()
                y = gpu.affine_relu(x, float(scale_s), float(bias_s))
                out = y.astype(np.float64).tobytes()
                print("OK " + base64.b64encode(out).decode("ascii"), flush=True)
            except Exception as exc:  # noqa: BLE001 - erro volta como texto para o runtime
                print("ERR " + str(exc).replace("\n", " ")[:300], flush=True)
        else:
            print("ERR comando desconhecido: " + line[:60], flush=True)


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
