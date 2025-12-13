#!/usr/bin/env python3
"""
Extended statistical analysis for TPIR structured showdown data.

Works with the *flattened* showdown JSON format:
    combined_showdowns_scenarios_1_2_3.json

Keeps:
    - Mid-value spin decision breakdown (40,45,50,55,60,65)
    - Per-player spin-again breakdown (positions 1,2,3)
    - Extraction of contestants with 6+ extra spins (human readable)
    - Graphs of spin / total distributions
    - Statistical tests to validate spin randomness and internal consistency
"""

import json
from collections import Counter
import numpy as np

INPUT_PATH = r"C:\Users\eggep\Downloads\combined_showdowns_scenarios_1_2_3.json"

MID_VALUES = [40, 45, 50, 55, 60, 65]

# -----------------------------
# Optional plotting
# -----------------------------
try:
    import matplotlib.pyplot as plt
    HAS_MPL = True
except ImportError:
    HAS_MPL = False
    plt = None

# -----------------------------
# Optional scipy for p-values
# -----------------------------
try:
    from scipy import stats
    SCIPY_AVAILABLE = True
except ImportError:
    SCIPY_AVAILABLE = False


# -----------------------------
# Plotting helpers
# -----------------------------
def plot_distributions(spin_values, first_spin_values, second_spin_values, totals):
    if not HAS_MPL:
        print("\n[Plots] matplotlib not available; skipping graph generation.")
        return

    if len(spin_values) == 0:
        print("\n[Plots] No spin data to plot.")
        return

    unique_vals = np.sort(np.unique(spin_values))

    # 1. All spins distribution
    plt.figure(figsize=(8, 4))
    plt.hist(spin_values, bins=len(unique_vals), rwidth=0.9)
    plt.xlabel("Spin value")
    plt.ylabel("Count")
    plt.title("Distribution of all spins (first + second)")
    plt.tight_layout()
    plt.savefig("spin_distribution_all.png")
    plt.close()

    # 2. First vs second spins
    if len(first_spin_values) > 0 and len(second_spin_values) > 0:
        plt.figure(figsize=(8, 4))
        plt.hist(first_spin_values, bins=len(unique_vals), alpha=0.5, rwidth=0.9, label="First spins")
        plt.hist(second_spin_values, bins=len(unique_vals), alpha=0.5, rwidth=0.9, label="Second spins")
        plt.xlabel("Spin value")
        plt.ylabel("Count")
        plt.title("First vs second spin distributions")
        plt.legend()
        plt.tight_layout()
        plt.savefig("spin_distribution_first_vs_second.png")
        plt.close()

    # 3. Totals distribution
    if len(totals) > 0:
        plt.figure(figsize=(8, 4))
        plt.hist(totals, bins=20, rwidth=0.9)
        plt.xlabel("Total score (sum of spins)")
        plt.ylabel("Count")
        plt.title("Distribution of contestant totals")
        plt.tight_layout()
        plt.savefig("total_distribution.png")
        plt.close()

    print("\n[Plots] Saved:")
    print("  spin_distribution_all.png")
    print("  spin_distribution_first_vs_second.png (if second spins exist)")
    print("  total_distribution.png (if totals exist)")


# -----------------------------
# Statistical tests
# -----------------------------
def chi_square_gof(observed_counts, label):
    """Chi-square goodness-of-fit vs uniform over the observed categories."""
    observed_counts = np.array(observed_counts, dtype=float)
    total = observed_counts.sum()
    k = len(observed_counts)
    if k <= 1 or total == 0:
        print(f"{label}: not enough data for chi-square GOF.")
        return

    expected = np.full(k, total / k)
    chi2 = ((observed_counts - expected) ** 2 / expected).sum()
    df = k - 1
    if SCIPY_AVAILABLE:
        p = stats.chi2.sf(chi2, df)
        print(f"{label}: chi2 = {chi2:.2f}, df = {df}, p = {p:.4g}")
    else:
        print(f"{label}: chi2 = {chi2:.2f}, df = {df} (SciPy not installed; p-value not computed)")


def run_statistical_tests(spin_values, first_spin_values, second_spin_values, totals, spin_pairs):
    print("\n=== RANDOMNESS / INTERNAL CONSISTENCY TESTS ===")

    # Basic sanity check on allowed values
    all_unique = np.sort(np.unique(spin_values))
    print("\nDistinct spin values seen:", all_unique)

    if len(all_unique) > 0:
        diffs = np.diff(all_unique)
        if len(diffs) > 0:
            min_step = np.min(diffs)
            max_step = np.max(diffs)
            print(f"Approximate step size between outcomes: min={min_step}, max={max_step}")

    # --------- 1. First-spin distribution vs uniform ---------
    if len(first_spin_values) > 0:
        vals_first, counts_first = np.unique(first_spin_values, return_counts=True)
        print("\nFirst-spin counts by value:")
        for v, c in zip(vals_first, counts_first):
            print(f"  {v}: {c}")
        chi_square_gof(counts_first, "First-spin uniformity (GOF vs equal probabilities)")
    else:
        print("\nNo first spins found; cannot test first-spin uniformity.")

    # --------- 2. All spins vs uniform ---------
    if len(spin_values) > 0:
        vals_all, counts_all = np.unique(spin_values, return_counts=True)
        print("\nAll-spin counts by value (first + second):")
        for v, c in zip(vals_all, counts_all):
            print(f"  {v}: {c}")
        chi_square_gof(counts_all, "All-spin uniformity (GOF vs equal probabilities)")
    else:
        print("\nNo spins found; cannot test overall uniformity.")

    # --------- 3. First vs second spin distribution (contingency chi-square) ---------
    if len(first_spin_values) > 0 and len(second_spin_values) > 0:
        vals_first, counts_first = np.unique(first_spin_values, return_counts=True)
        vals_second, counts_second = np.unique(second_spin_values, return_counts=True)
        all_vals = np.union1d(vals_first, vals_second)

        first_row = np.zeros(len(all_vals), dtype=int)
        second_row = np.zeros(len(all_vals), dtype=int)

        first_map = dict(zip(vals_first, counts_first))
        second_map = dict(zip(vals_second, counts_second))

        for i, v in enumerate(all_vals):
            first_row[i] = first_map.get(v, 0)
            second_row[i] = second_map.get(v, 0)

        contingency = np.vstack([first_row, second_row])

        print("\nFirst vs second spin contingency table (rows: first/second; cols: values):")
        print("Values:", all_vals)
        print(contingency)

        if SCIPY_AVAILABLE:
            chi2, p, df, expected = stats.chi2_contingency(contingency)
            print(f"\nFirst vs second spin distribution chi-square:")
            print(f"  chi2 = {chi2:.2f}, df = {df}, p = {p:.4g}")
        else:
            print("\nFirst vs second spin chi-square: SciPy not installed; only raw contingency table shown.")
    else:
        print("\nNot enough data for first/second spin comparison.")

    # --------- 4. First vs second spin mean comparison ---------
    if len(first_spin_values) > 0 and len(second_spin_values) > 0:
        mean1 = np.mean(first_spin_values)
        mean2 = np.mean(second_spin_values)
        print(f"\nFirst vs second spin means:")
        print(f"  First mean  = {mean1:.3f}")
        print(f"  Second mean = {mean2:.3f}")

        if SCIPY_AVAILABLE:
            t_stat, p_val = stats.ttest_ind(first_spin_values, second_spin_values, equal_var=False)
            print(f"  t-test (Welch): t = {t_stat:.3f}, p = {p_val:.4g}")
        else:
            print("  t-test: SciPy not installed; only means reported.")
    else:
        print("\nNot enough data for first/second spin mean comparison.")

    # --------- 5. Correlation of first and second spins within contestants ---------
    if len(spin_pairs) > 0:
        pairs = np.array(spin_pairs)
        s1 = pairs[:, 0]
        s2 = pairs[:, 1]
        if len(s1) > 1:
            corr_matrix = np.corrcoef(s1, s2)
            corr = corr_matrix[0, 1]
            print(f"\nCorrelation between first and second spin values (for contestants who spun twice): {corr:.3f}")
        else:
            print("\nNot enough paired spins to compute correlation.")
    else:
        print("\nNo contestants with both first and second spins; cannot compute spin correlation.")

    print("\nRandomness / consistency tests complete.")


# -----------------------------
# Main analysis
# -----------------------------
def analyze(showdowns):
    """
    showdowns: list of showdown dicts (already flattened).
    Each showdown has:
        - contestants: [...]
        - winner_index, has_bonus, has_spinoff, scenario, raw_text, etc.
    """
    print(f"\n=== DATASET SUMMARY ===")
    print(f"Total structured showdowns: {len(showdowns)}")

    spin_values = []
    first_spin_values = []
    second_spin_values = []
    totals = []
    spin_pairs = []  # (s1, s2) for contestants who actually spun twice

    bust_count = 0
    hits_100 = 0
    bonus_count = 0
    winner_by_position = Counter()
    spin_off_counts = Counter()
    extreme_spin_off_examples = []

    # Global mid-value tracking
    spin_decisions = {v: {"count": 0, "spin_again": 0} for v in MID_VALUES}

    # Per-player mid-value tracking
    spin_decisions_by_pos = {
        v: {
            1: {"count": 0, "spin_again": 0},
            2: {"count": 0, "spin_again": 0},
            3: {"count": 0, "spin_again": 0},
        }
        for v in MID_VALUES
    }

    for sd in showdowns:
        contestants = sd.get("contestants", [])

        winner_index = sd.get("winner_index")
        if isinstance(winner_index, int):
            winner_by_position[winner_index + 1] += 1

        for idx, c in enumerate(contestants):
            pos = c.get("position", idx + 1)

            # Busts
            if c.get("bust", False):
                bust_count += 1

            # Bonus spins
            if len(c.get("bonus_spins", []) or []) > 0:
                bonus_count += 1

            # Spin-off stats
            extra_spins_list = c.get("spin_off_spins", []) or []
            extra = len(extra_spins_list)
            spin_off_counts[extra] += 1  # keep original behavior (including 0s)

            if extra >= 6:
                extreme_spin_off_examples.append({
                    "episode_title": sd.get("episode_title"),
                    "iso_date": sd.get("iso_date"),
                    "url": sd.get("url"),
                    "label": sd.get("label") or sd.get("scenario"),
                    "contestant_name": c.get("name"),
                    "initial_spins": c.get("initial_spins"),
                    "extra_spins": extra_spins_list,
                    "notes": c.get("notes"),
                    "raw_text": sd.get("raw_text")
                })

            # Initial spins (safe extraction)
            spins = c.get("initial_spins", []) or []
            s1 = spins[0].get("value") if len(spins) > 0 and isinstance(spins[0], dict) else None
            s2 = spins[1].get("value") if len(spins) > 1 and isinstance(spins[1], dict) else None

            if s1 is not None:
                first_spin_values.append(s1)
            if s2 is not None:
                second_spin_values.append(s2)
            if s1 is not None and s2 is not None:
                spin_pairs.append((s1, s2))

            # GLOBAL mid-value spin decision collection
            if s1 in MID_VALUES:
                spin_decisions[s1]["count"] += 1
                if s2 is not None:
                    spin_decisions[s1]["spin_again"] += 1

                # PER-PLAYER decision collection
                if pos in (1, 2, 3):
                    spin_decisions_by_pos[s1][pos]["count"] += 1
                    if s2 is not None:
                        spin_decisions_by_pos[s1][pos]["spin_again"] += 1

            # All spins (initial spins only, same as your original)
            for sp in spins:
                if isinstance(sp, dict) and sp.get("value") is not None:
                    spin_values.append(sp["value"])

            # 1.00 hits (your original code was counting 1.0)
            # Keep same behavior: any value == 1.0 counts as a "1.00 hit"
            if any(
                (isinstance(sp, dict) and sp.get("value") is not None and abs(sp["value"] - 1.0) < 1e-6)
                for sp in spins
            ):
                hits_100 += 1

            # Totals
            if c.get("total") is not None:
                totals.append(c["total"])

    # Convert to numpy
    spin_values = np.array(spin_values, dtype=float) if len(spin_values) else np.array([])
    first_spin_values = np.array(first_spin_values, dtype=float) if len(first_spin_values) else np.array([])
    second_spin_values = np.array(second_spin_values, dtype=float) if len(second_spin_values) else np.array([])
    totals = np.array(totals, dtype=float) if len(totals) else np.array([])

    # -------------------------
    # PRINT OUTPUT
    # -------------------------
    print("\n=== MID-VALUE SPIN DECISION ANALYSIS ===")
    for v in MID_VALUES:
        ct = spin_decisions[v]["count"]
        again = spin_decisions[v]["spin_again"]
        if ct > 0:
            pct = again / ct * 100
            print(f"First spin = {v}: spun again {pct:.1f}% of the time ({again}/{ct})")
        else:
            print(f"First spin = {v}: no data")

    print("\n=== MID-VALUE SPIN DECISIONS BY PLAYER POSITION ===")
    for v in MID_VALUES:
        print(f"\nValue {v}:")
        for pos in (1, 2, 3):
            ct = spin_decisions_by_pos[v][pos]["count"]
            again = spin_decisions_by_pos[v][pos]["spin_again"]
            if ct > 0:
                pct = again / ct * 100
                print(f"  Player {pos}: spun again {pct:.1f}% ({again}/{ct})")
            else:
                print(f"  Player {pos}: no data")

    print("\n=== EXTREME EXTRA SPIN CASES (≥ 6 EXTRA SPINS) ===")
    if not extreme_spin_off_examples:
        print("No contestants found with 6 or more extra spins.")
    else:
        for idx, ex in enumerate(extreme_spin_off_examples, start=1):
            print(f"\n--- Example {idx} ---")
            print(f"Episode: {ex.get('episode_title')}   Date: {ex.get('iso_date')}")
            print(f"URL: {ex.get('url')}")
            print(f"Showdown: {ex.get('label')}")
            print(f"Contestant: {ex.get('contestant_name')}")
            init_vals = []
            if isinstance(ex.get("initial_spins"), list):
                for s in ex["initial_spins"]:
                    if isinstance(s, dict):
                        init_vals.append(s.get("value"))
            extra_vals = []
            if isinstance(ex.get("extra_spins"), list):
                for s in ex["extra_spins"]:
                    if isinstance(s, dict):
                        extra_vals.append(s.get("value"))
                    else:
                        extra_vals.append(s)
            print(f"Initial spins: {init_vals}")
            print(f"Extra spins ({len(extra_vals)}): {extra_vals}")
            print(f"Notes: {ex.get('notes')}")
            print(f"Raw text: {ex.get('raw_text')}")

    print("\n=== SPIN DISTRIBUTIONS (BASIC STATS) ===")
    if len(spin_values) > 0:
        print(f"Total spins: {len(spin_values)}")
        print(f"Mean: {spin_values.mean():.2f}  Median: {np.median(spin_values):.2f}  Std: {spin_values.std():.2f}")
        print(f"Min: {spin_values.min()}  Max: {spin_values.max()}")
    else:
        print("No spin values found.")

    print("\n=== FIRST SPIN VS SECOND SPIN (BASIC STATS) ===")
    if len(first_spin_values) > 0:
        print(f"First spin mean: {first_spin_values.mean():.2f}")
    else:
        print("No first spins found.")

    if len(second_spin_values) > 0:
        print(f"Second spin mean: {second_spin_values.mean():.2f}")
    else:
        print("No second spins found.")

    if len(first_spin_values) > 0:
        print(f"Spin-again rate overall: {len(second_spin_values)/len(first_spin_values):.3f}")

    print("\n=== OTHER EVENTS ===")
    print(f"1.00 hits: {hits_100}")
    print(f"Bonus spins detected: {bonus_count}")
    print(f"Busts: {bust_count}")

    print("\n=== SPIN-OFF FREQUENCY ===")
    for k, v in sorted(spin_off_counts.items()):
        print(f"{k} extra spins: {v} contestants")

    print("\n=== WINNER BY POSITION ===")
    total_wins = sum(winner_by_position.values())
    for pos, ct in sorted(winner_by_position.items()):
        pct = ct / total_wins * 100 if total_wins > 0 else 0.0
        print(f"Position {pos}: {ct} wins ({pct:.2f}%)")

    print("\n=== TOTAL SCORE STATISTICS ===")
    if len(totals) > 0:
        print(f"Mean total: {totals.mean():.2f}")
        print(f"Median total: {np.median(totals):.2f}")
        print(f"StdDev total: {totals.std():.2f}")
    else:
        print("No totals available.")

    # Plots + deeper statistical tests
    plot_distributions(spin_values, first_spin_values, second_spin_values, totals)
    run_statistical_tests(spin_values, first_spin_values, second_spin_values, totals, spin_pairs)

    print("\nDone.")


def main():
    with open(INPUT_PATH, "r", encoding="utf-8") as f:
        showdowns = json.load(f)

    if not isinstance(showdowns, list):
        raise ValueError("Expected top-level JSON to be a list of showdown records.")

    analyze(showdowns)


if __name__ == "__main__":
    main()
