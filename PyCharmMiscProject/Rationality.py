import numpy as np
import matplotlib.pyplot as plt

# λ values (rationality), log scale
lambdas = np.logspace(-1, 2, 200)  # 0.1 → 100

# Payoff gaps Δ for C2 decisions
# (equilibrium action differs by value)
deltas_C2 = {
    "50¢ (Spin is optimal)": 0.11,
    "55¢ (Spin is optimal)": 0.08,
    "60¢ (Stay is optimal)": 0.04,
    "65¢ (Stay is optimal)": 0.015,
}

def qre_probability(delta, lam):
    """Probability of choosing the equilibrium action"""
    return 1 / (1 + np.exp(-lam * delta))

# Plot
plt.figure(figsize=(10, 6))

for label, delta in deltas_C2.items():
    probs = qre_probability(delta, lambdas)
    plt.plot(lambdas, probs * 100, label=label)

plt.xscale("log")
plt.xlabel("C2 Rationality (λ)")
plt.ylabel("Probability of Choosing Equilibrium Action (%)")
plt.title("C2 Decision Probability vs Rationality (QRE Model)")
plt.grid(True, linestyle=":")
plt.legend()
plt.tight_layout()
plt.show()
