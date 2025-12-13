#!/usr/bin/env python3
"""
TPIR Showcase Showdown simulation (C1 vs C2 vs C3)

What this script can do:
  1) Run a full Monte Carlo simulation of the Showcase Showdown with:
       - uniform wheel outcomes (5..100 step 5)
       - busting if total > 100
       - immediate spin-offs for ties among current leaders (C1/C2 tie resolved before C3 plays)
  2) Model C2 with a QRE/logit ("lambda") system in a middle region using payoff gaps Δ
  3) Optionally ESTIMATE those payoff gaps Δ via Monte Carlo, so you aren't hand-waving them
  4) Produce the sensitivity graphs you’ve been making:
       - for C1 first spin fixed at 60 or 65
       - compare C1 forced "stay" vs forced "spin again"
       - sweep over lambda_C2

Notes:
  - Wheel is assumed uniform.
  - C3 uses an equilibrium-like rule (spin if below best, stay if above best; if tie, spin if <=50 else stay).
  - C2 has:
      * tie-on-first-spin override: if tied with C1 at total T, spin if T<50 else stay
      * a special rule: if first spin == 70, stay with 90% probability
      * deterministic extremes: <=30 always spin, >=75 always stay
      * in the middle, QRE/logit around equilibrium using payoff gaps Δ and lambda_C2
"""

import random
import math
from dataclasses import dataclass
from typing import Dict, List, Optional, Tuple

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt


# ---------------------------
# Core wheel / tie mechanics
# ---------------------------

WHEEL_VALUES = list(range(5, 105, 5))  # 5,10,...,100


def spin_once() -> int:
    return random.choice(WHEEL_VALUES)


def take_second_spin_if_needed(first: int) -> int:
    """Return total after a second spin. Return 0 if bust."""
    second = spin_once()
    total = first + second
    return total if total <= 100 else 0


def resolve_spinoff(contenders: List[str]) -> str:
    """Repeated single-spin tie-break until unique winner."""
    while True:
        spins = {c: spin_once() for c in contenders}
        m = max(spins.values())
        winners = [c for c, v in spins.items() if v == m]
        if len(winners) == 1:
            return winners[0]


# ---------------------------
# Policies
# ---------------------------

def c3_policy(best_value: int, c3_first: int) -> str:
    """
    C3: equilibrium-like
      - if below best: spin
      - if above best: stay
      - if tie: spin if <=50 else stay
    """
    if c3_first < best_value:
        return "spin_again"
    if c3_first > best_value:
        return "stay"
    # tie
    return "spin_again" if c3_first <= 50 else "stay"


def c2_equilibrium_action(first_spin: int, c1_total: int) -> str:
    """
    Equilibrium-ish action for C2 (used as the "anchor" action in QRE).
    Includes tie-aware override because that’s analytically correct.
    """
    # tie on FIRST spin with C1 total: compare tie-off (50%) vs spin-to-beat ((100-T)/100)
    if first_spin == c1_total and c1_total != 0:
        return "spin_again" if c1_total < 50 else "stay"

    # baseline equilibrium-ish threshold (you can change these if you want)
    # Here: spin again if <=55 else stay
    if first_spin <= 55:
        return "spin_again"
    return "stay"


def qre_follow_prob(delta: float, lam: float) -> float:
    """Logit / quantal response probability of choosing the equilibrium action."""
    return 1.0 / (1.0 + math.exp(-lam * delta))


def c2_policy(
    first_spin: int,
    c1_total: int,
    lambda_c2: float,
    deltas_c2: Dict[int, float],
    force_70_stay_prob: float = 0.90,
    extreme_spin_leq: int = 30,
    extreme_stay_geq: int = 75,
) -> str:
    """
    C2 policy:
      - If first spin == 70: stay with 90% (or whatever you set)
      - tie-on-first-spin override: if tied with C1 at total T, spin if T<50 else stay
      - deterministic extremes: <=30 always spin, >=75 always stay
      - otherwise: QRE around equilibrium action using delta for that first spin
    """
    if first_spin == 70:
        return "stay" if random.random() < force_70_stay_prob else "spin_again"

    # tie override
    if first_spin == c1_total and c1_total != 0:
        return "spin_again" if c1_total < 50 else "stay"

    # deterministic extremes
    if first_spin <= extreme_spin_leq:
        return "spin_again"
    if first_spin >= extreme_stay_geq:
        return "stay"

    eq = c2_equilibrium_action(first_spin, c1_total)
    delta = deltas_c2.get(first_spin, 0.01)  # small default if missing
    p_follow = qre_follow_prob(delta, lambda_c2)

    if random.random() < p_follow:
        return eq
    return "stay" if eq == "spin_again" else "spin_again"


def c1_policy_hybrid(
    first_spin: int,
    lambda_c1: float = 11.0,
    delta_c1: float = 0.05,
    extreme_spin_leq: int = 30,
    extreme_stay_geq: int = 75,
) -> str:
    """
    C1 hybrid policy for "normal play" (not forced mode).
      - deterministic extremes: <=30 spin, >=75 stay
      - middle: equilibrium anchor (spin if <=65 else stay) with soft noise
    """
    if first_spin <= extreme_spin_leq:
        return "spin_again"
    if first_spin >= extreme_stay_geq:
        return "stay"

    eq = "spin_again" if first_spin <= 65 else "stay"
    p_follow = qre_follow_prob(delta_c1, lambda_c1)

    if random.random() < p_follow:
        return eq
    return "stay" if eq == "spin_again" else "spin_again"


# ---------------------------
# One full showdown simulation
# ---------------------------

@dataclass
class SimOutcome:
    winner: Optional[str]              # "C1", "C2", "C3", or None (all bust)
    c1_total: int
    c2_total: int
    c3_total: int
    best_owner_before_c3: Optional[str]
    best_value_before_c3: int
    c1_forced: str                     # "stay", "spin_again", or "hybrid"
    c2_first: int
    c2_action: str
    c3_first: int
    c3_action: str
    tie12_resolved: bool
    tie23_resolved: bool


def simulate_showdown(
    *,
    c1_first_fixed: Optional[int] = None,
    c1_forced_action: str = "hybrid",  # "hybrid" | "stay" | "spin_again"
    lambda_c2: float = 15.0,
    deltas_c2: Optional[Dict[int, float]] = None,
    force_70_stay_prob: float = 0.90,
) -> SimOutcome:
    if deltas_c2 is None:
        deltas_c2 = {}

    # ----- C1 -----
    c1_first = c1_first_fixed if c1_first_fixed is not None else spin_once()

    if c1_forced_action == "stay":
        c1_total = c1_first
    elif c1_forced_action == "spin_again":
        c1_total = take_second_spin_if_needed(c1_first)
    elif c1_forced_action == "hybrid":
        a1 = c1_policy_hybrid(c1_first)
        c1_total = c1_first if a1 == "stay" else take_second_spin_if_needed(c1_first)
    else:
        raise ValueError("c1_forced_action must be 'hybrid', 'stay', or 'spin_again'")

    # ----- C2 -----
    c2_first = spin_once()
    c2_action = c2_policy(
        c2_first, c1_total,
        lambda_c2=lambda_c2,
        deltas_c2=deltas_c2,
        force_70_stay_prob=force_70_stay_prob,
    )
    c2_total = c2_first if c2_action == "stay" else take_second_spin_if_needed(c2_first)

    # Resolve C1 vs C2 immediately (spin-off if tie)
    tie12 = False
    if c1_total != 0 and c1_total == c2_total:
        tie12 = True
        best_owner = resolve_spinoff(["C1", "C2"])
        best_value = c1_total  # tied total
    else:
        if c1_total > c2_total:
            best_owner, best_value = "C1", c1_total
        elif c2_total > c1_total:
            best_owner, best_value = "C2", c2_total
        else:
            best_owner, best_value = None, 0  # both 0 (both bust) or both equal 0

    # ----- C3 -----
    c3_first = spin_once()
    c3_action = c3_policy(best_value, c3_first)
    c3_total = c3_first if c3_action == "stay" else take_second_spin_if_needed(c3_first)

    # Final resolution: compare C3 against best_owner/best_value
    tie23 = False
    if best_owner is None:
        winner = "C3" if c3_total != 0 else None
    else:
        if c3_total != 0 and c3_total == best_value:
            tie23 = True
            winner = resolve_spinoff([best_owner, "C3"])
        elif c3_total > best_value:
            winner = "C3"
        else:
            winner = best_owner

    return SimOutcome(
        winner=winner,
        c1_total=c1_total,
        c2_total=c2_total,
        c3_total=c3_total,
        best_owner_before_c3=best_owner,
        best_value_before_c3=best_value,
        c1_forced=c1_forced_action,
        c2_first=c2_first,
        c2_action=c2_action,
        c3_first=c3_first,
        c3_action=c3_action,
        tie12_resolved=tie12,
        tie23_resolved=tie23,
    )


# ---------------------------
# Payoff gap (Δ) estimation for C2
# ---------------------------

def estimate_delta_c2_for_first_spin(
    first_spin: int,
    *,
    trials: int,
    lambda_c2_for_eval: float,
    deltas_c2_for_eval: Dict[int, float],
    force_70_stay_prob: float = 0.90,
) -> Tuple[float, float, float]:
    """
    Estimate Δ for C2 at a particular first_spin:
      Δ = P(C2 wins | C2 takes equilibrium action) - P(C2 wins | C2 takes deviation action)

    Returns: (delta, p_eq, p_dev)

    Important:
      - This compares C2's win rate under two forced actions (eq vs dev)
      - C1 is simulated under its HYBRID policy (not forced), so C2 sees realistic C1 totals.
    """
    eq_wins = 0
    dev_wins = 0

    for _ in range(trials):
        # simulate C1 under hybrid to generate a realistic c1_total for the context
        c1_out = simulate_showdown(
            c1_first_fixed=None,
            c1_forced_action="hybrid",
            lambda_c2=lambda_c2_for_eval,
            deltas_c2=deltas_c2_for_eval,
            force_70_stay_prob=force_70_stay_prob,
        )
        c1_total = c1_out.c1_total

        # determine what "equilibrium" action would be in THIS context
        eq_action = c2_equilibrium_action(first_spin, c1_total)
        dev_action = "stay" if eq_action == "spin_again" else "spin_again"

        # ---- EQ branch ----
        # Re-play a showdown, but force:
        #   - C1 to use the SAME fixed total context? We can’t directly “pin” totals without contortions,
        #     so instead we re-simulate the whole game with a fixed C1 first spin and hybrid play.
        # For payoff-gap stability, we focus on C2’s relative outcome under forcing eq vs dev,
        # keeping everything else identically distributed. This is standard Monte Carlo.
        #
        # We’ll emulate forcing C2 by running a custom mini-sim below rather than calling simulate_showdown twice.

        # Simulate C1 (one more time) to pair with forced C2 action:
        c1_first = spin_once()
        a1 = c1_policy_hybrid(c1_first)
        c1_total2 = c1_first if a1 == "stay" else take_second_spin_if_needed(c1_first)

        # Force C2 eq
        c2_total_eq = first_spin if eq_action == "stay" else take_second_spin_if_needed(first_spin)

        # resolve C1 vs C2
        if c1_total2 != 0 and c1_total2 == c2_total_eq:
            best_owner_eq = resolve_spinoff(["C1", "C2"])
            best_value_eq = c1_total2
        else:
            if c1_total2 > c2_total_eq:
                best_owner_eq, best_value_eq = "C1", c1_total2
            elif c2_total_eq > c1_total2:
                best_owner_eq, best_value_eq = "C2", c2_total_eq
            else:
                best_owner_eq, best_value_eq = None, 0

        # C3
        c3_first = spin_once()
        a3 = c3_policy(best_value_eq, c3_first)
        c3_total = c3_first if a3 == "stay" else take_second_spin_if_needed(c3_first)

        if best_owner_eq is None:
            winner_eq = "C3" if c3_total != 0 else None
        else:
            if c3_total != 0 and c3_total == best_value_eq:
                winner_eq = resolve_spinoff([best_owner_eq, "C3"])
            elif c3_total > best_value_eq:
                winner_eq = "C3"
            else:
                winner_eq = best_owner_eq

        if winner_eq == "C2":
            eq_wins += 1

        # ---- DEV branch ----
        # Re-simulate fresh (same distribution) with forced deviation action.
        c1_first = spin_once()
        a1 = c1_policy_hybrid(c1_first)
        c1_total3 = c1_first if a1 == "stay" else take_second_spin_if_needed(c1_first)

        c2_total_dev = first_spin if dev_action == "stay" else take_second_spin_if_needed(first_spin)

        if c1_total3 != 0 and c1_total3 == c2_total_dev:
            best_owner_dev = resolve_spinoff(["C1", "C2"])
            best_value_dev = c1_total3
        else:
            if c1_total3 > c2_total_dev:
                best_owner_dev, best_value_dev = "C1", c1_total3
            elif c2_total_dev > c1_total3:
                best_owner_dev, best_value_dev = "C2", c2_total_dev
            else:
                best_owner_dev, best_value_dev = None, 0

        c3_first = spin_once()
        a3 = c3_policy(best_value_dev, c3_first)
        c3_total = c3_first if a3 == "stay" else take_second_spin_if_needed(c3_first)

        if best_owner_dev is None:
            winner_dev = "C3" if c3_total != 0 else None
        else:
            if c3_total != 0 and c3_total == best_value_dev:
                winner_dev = resolve_spinoff([best_owner_dev, "C3"])
            elif c3_total > best_value_dev:
                winner_dev = "C3"
            else:
                winner_dev = best_owner_dev

        if winner_dev == "C2":
            dev_wins += 1

    p_eq = eq_wins / trials
    p_dev = dev_wins / trials
    delta = p_eq - p_dev
    return delta, p_eq, p_dev


def build_deltas_c2(
    spins: List[int],
    *,
    trials_per_spin: int = 120_000,
    lambda_c2_for_eval: float = 15.0,
    force_70_stay_prob: float = 0.90,
) -> Dict[int, float]:
    """
    Estimate a delta table for a list of first-spin values.
    """
    deltas: Dict[int, float] = {}
    # start with a tiny baseline so c2_policy can run during eval without missing keys
    baseline: Dict[int, float] = {s: 0.01 for s in spins}
    for s in spins:
        d, p_eq, p_dev = estimate_delta_c2_for_first_spin(
            s,
            trials=trials_per_spin,
            lambda_c2_for_eval=lambda_c2_for_eval,
            deltas_c2_for_eval=baseline,
            force_70_stay_prob=force_70_stay_prob,
        )
        deltas[s] = max(0.0, d)  # clamp negative deltas to 0 if you want purely “follow eq” incentives
        print(f"[Δ-est] spin={s:>3}  Δ={deltas[s]:.4f}  p_eq={p_eq:.4f}  p_dev={p_dev:.4f}")
    return deltas


# ---------------------------
# Sensitivity sweep + plotting
# ---------------------------

def sensitivity_sweep(
    *,
    c1_first_spins: List[int],
    lambdas_c2: List[float],
    trials: int,
    deltas_c2: Dict[int, float],
    force_70_stay_prob: float = 0.90,
) -> pd.DataFrame:
    rows = []
    for fs in c1_first_spins:
        for forced in ["stay", "spin_again"]:
            for lam in lambdas_c2:
                wins = 0
                for _ in range(trials):
                    out = simulate_showdown(
                        c1_first_fixed=fs,
                        c1_forced_action=forced,
                        lambda_c2=lam,
                        deltas_c2=deltas_c2,
                        force_70_stay_prob=force_70_stay_prob,
                    )
                    wins += 1 if out.winner == "C1" else 0

                p = wins / trials
                se = math.sqrt(p * (1 - p) / trials)
                rows.append({
                    "C1 first spin": fs,
                    "Forced strategy": forced,
                    "lambda_C2": lam,
                    "Win rate": p,
                    "SE": se,
                    "CI95_low": max(0.0, p - 1.96 * se),
                    "CI95_high": min(1.0, p + 1.96 * se),
                })
    return pd.DataFrame(rows)


def plot_sweep(df: pd.DataFrame, c1_first_spins: List[int]) -> None:
    fig, axes = plt.subplots(1, len(c1_first_spins), figsize=(6 * len(c1_first_spins), 5), sharey=True)
    if len(c1_first_spins) == 1:
        axes = [axes]

    for ax, fs in zip(axes, c1_first_spins):
        for strategy, style in [("stay", "--"), ("spin_again", "-")]:
            sub = df[(df["C1 first spin"] == fs) & (df["Forced strategy"] == strategy)].sort_values("lambda_C2")
            ax.errorbar(
                sub["lambda_C2"],
                sub["Win rate"] * 100,
                yerr=(1.96 * sub["SE"] * 100),
                marker="o",
                linestyle=style,
                capsize=3,
                label=strategy.title(),
            )
        ax.set_xscale("log")
        ax.set_title(f"C1 first spin {fs}¢ vs λ_C2")
        ax.set_xlabel("λ_C2 (log scale)")
        ax.set_ylabel("C1 win rate (%)")
        ax.grid(True, linestyle=":")
        ax.legend()

    plt.tight_layout()
    plt.show()


# ---------------------------
# Main (example usage)
# ---------------------------

def main():
    random.seed(42)
    np.random.seed(42)

    # Option A: Use your own delta table (fast)
    deltas_c2 = {
        35: 0.18,
        40: 0.16,
        45: 0.14,
        50: 0.11,
        55: 0.08,
        60: 0.04,
        65: 0.015,
    }

    # Option B (recommended if you want “more accurate” deltas): estimate them.
    # This can take time depending on trials_per_spin.
    deltas_c2 = build_deltas_c2(
        spins=[35, 40, 45, 50, 55, 60, 65],
        trials_per_spin=120_000,
        lambda_c2_for_eval=15.0,
        force_70_stay_prob=0.90,
    )

    # Sweep lambdas for C2
    lambdas = [0.5, 1, 2, 5, 11, 15, 30, 60, 100]

    df = sensitivity_sweep(
        c1_first_spins=[55, 60, 65, 70],
        lambdas_c2=lambdas,
        trials=200_000,               # bump to 200k+ if you want tighter error bars
        deltas_c2=deltas_c2,
        force_70_stay_prob=0.90,
    )

    print(df.head(10))
    plot_sweep(df, [55, 60, 65, 70])

    # Optional: show a table slice for 65
    tbl = df[df["C1 first spin"] == 65].pivot(index="lambda_C2", columns="Forced strategy", values="Win rate")
    print("\nC1=65¢ win rates:\n", (tbl * 100).round(3))


if __name__ == "__main__":
    main()
