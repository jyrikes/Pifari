"""MLP 2-4-1 XOR from scratch: Python standard library only, no NumPy/ML frameworks."""
from time import perf_counter
from math import exp

X1 = [0.0, 0.0, 1.0, 1.0]
X2 = [0.0, 1.0, 0.0, 1.0]
Y = [0.0, 1.0, 1.0, 0.0]
TX1 = [0.1, 0.1, 0.9, 0.9]
TX2 = [0.1, 0.9, 0.1, 0.9]
TY = [0.0, 1.0, 1.0, 0.0]
INITIAL = [0.12, -0.18, 0.04, -0.22, 0.16, -0.03, 0.21, -0.17, 0.08]
EPOCHS = 3000
LR = 0.7

def sigmoid(x):
    return 1.0 / (1.0 + exp(-x))

def predict(w, x1, x2):
    h1 = sigmoid(w[0]*x1 + w[1]*x2 + w[2])
    h2 = sigmoid(w[3]*x1 + w[4]*x2 + w[5])
    return sigmoid(w[6]*h1 + w[7]*h2 + w[8])

def train():
    w = INITIAL.copy()
    for _ in range(EPOCHS):
        for i in range(4):
            x1, x2, target = X1[i], X2[i], Y[i]
            z1 = w[0]*x1 + w[1]*x2 + w[2]
            z2 = w[3]*x1 + w[4]*x2 + w[5]
            h1, h2 = sigmoid(z1), sigmoid(z2)
            out = sigmoid(w[6]*h1 + w[7]*h2 + w[8])
            do = (target - out) * out * (1.0 - out)
            d1 = h1 * (1.0 - h1) * w[6] * do
            d2 = h2 * (1.0 - h2) * w[7] * do
            w[6] += LR * do * h1
            w[7] += LR * do * h2
            w[8] += LR * do
            w[0] += LR * d1 * x1
            w[1] += LR * d1 * x2
            w[2] += LR * d1
            w[3] += LR * d2 * x1
            w[4] += LR * d2 * x2
            w[5] += LR * d2
    return w

def accuracy(w, xs1, xs2, ys, emit=True, label="test"):
    correct = 0
    for i, (x1, x2, target) in enumerate(zip(xs1, xs2, ys)):
        p = predict(w, x1, x2)
        correct += int((p >= 0.5) == (target == 1.0))
        if emit:
            print(f"{label} {i} target {target:g} prob {p:.6f}")
    return 100.0 * correct / len(ys)

def main():
    # Median of five training runs; use a fresh copy of identical initial weights each time.
    samples = []
    w = None
    for _ in range(5):
        t0 = perf_counter()
        w = train()
        samples.append((perf_counter() - t0) * 1000.0)
    samples.sort()
    print(f"train_ms_median {samples[len(samples)//2]:.3f}")
    print(f"epochs {EPOCHS}")
    print(f"train_accuracy_pct {accuracy(w, X1, X2, Y, True, 'train'):.0f}")
    print(f"test_accuracy_pct {accuracy(w, TX1, TX2, TY, True, 'test'):.0f}")
    print("weights", " ".join(f"{v:.6f}" for v in w))

if __name__ == "__main__":
    main()
