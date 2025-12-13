#!/usr/bin/env python3
"""
Full TPIR Showcase Showdown Parser + Scenario Classification

Outputs:
    - structured_showdowns.json          (all parsed)
    - scenario_1_showdowns.json          (no busts, no spinoffs, no bonus)
    - scenario_2_showdowns.json          (>=1 bust, no spinoffs, no bonus)
    - scenario_3_showdowns.json          (>=1 spinoff, no bonus)   <-- NEW

Key parsing rule added:
    - A contestant only has a "second initial spin" if a '+' token appears.
      Otherwise, any additional numbers are treated as spin-off spins (unless BONUS).
"""

import json
import re

INPUT_PATH = "tpir_episodes_combined.json"

# -----------------------------
# Numeric parsing helpers
# -----------------------------

def num_from_token(tok: str):
    """Convert tokens like '35', '35+', '1.00', '3,413+' into floats."""
    clean = tok.strip().strip("=,>+")
    clean = clean.lstrip("$")
    if not clean:
        return None
    clean = clean.replace(",", "")
    try:
        val = float(clean)
    except:
        return None

    # Dataset quirk: "1.00" sometimes means 100
    if tok in ("1.00", "1.0"):
        return 100.0
    return val


def is_money_token(tokens, i):
    """Identify pre-wheel money based on formatting heuristics."""
    tok = tokens[i]
    if tok == "$":
        return False

    clean = tok.strip().lstrip("$").rstrip("+").rstrip(",")
    try:
        val = float(clean.replace(",", ""))
    except:
        return False

    # If previous is $, then this token is the numeric part of money
    if i > 0 and tokens[i - 1] == "$":
        return False

    if "," in clean or "." in clean:
        return True
    if val > 200:
        return True
    return False


def find_contestant_starts(tokens):
    """Locate all contestant starting indices (where money value appears)."""
    starts = []
    n = len(tokens)
    i = 0
    while i < n:
        tok = tokens[i]

        if tok == "$":
            if i + 1 < n and num_from_token(tokens[i + 1]) is not None:
                starts.append(i)
                i += 2
                continue

        if is_money_token(tokens, i):
            starts.append(i)

        i += 1
    return starts


# -----------------------------
# Parsing a single contestant
# -----------------------------

def parse_contestant(tokens, start_idx, end_idx):
    """
    Parse one contestant’s spins, bust, extras, totals, etc.

    IMPORTANT CHANGE:
      - second initial spin is only recognized if '+' is present as its own token
        somewhere in this contestant slice.
      - otherwise, treat ONLY the first number as the initial spin and everything
        else as spin-off (unless 'bonus' keyword exists).
    """
    # Money token logic
    if tokens[start_idx] == "$":
        money_idx = start_idx + 1
    else:
        money_idx = start_idx

    money_raw = tokens[money_idx]
    money_val = num_from_token(money_raw)
    if money_val is not None:
        try:
            money_val = int(round(money_val))
        except:
            money_val = None

    # Name + action tokens
    if money_idx + 1 >= end_idx:
        name = "UNKNOWN"
        action_tokens = []
    else:
        name = tokens[money_idx + 1].strip(",.")
        action_tokens = tokens[money_idx + 2:end_idx]

    # Flags
    text_lower = " ".join(action_tokens).lower()
    has_bonus_kw = "bonus" in text_lower

    # Detect explicit second-spin marker
    plus_present = any(t == "+" for t in action_tokens)

    # Extract numeric values (preserving order)
    numeric_vals = []
    for t in action_tokens:
        v = num_from_token(t)
        if v is not None:
            numeric_vals.append(v)

    first = numeric_vals[0] if len(numeric_vals) >= 1 else None
    second = None

    # Only assign a second initial spin if '+' appears
    if plus_present:
        # With "+", we interpret the second numeric as the second initial spin
        second = numeric_vals[1] if len(numeric_vals) >= 2 else None

    total = (first or 0) + (second or 0)

    # Extras are any remaining numerics after the initial spins
    if plus_present:
        extras = numeric_vals[2:]
    else:
        extras = numeric_vals[1:]

    # Remove “displayed total repeats” if they show up as extras
    # (covers cases where some rows include the total again)
    if first is not None and second is not None:
        sum_expected = first + second
        extras = [v for v in extras if abs(v - sum_expected) >= 1e-6]
    elif first is not None:
        extras = [v for v in extras if abs(v - first) >= 1e-6]

    # Bust check only applies when two initial spins exist
    bust = False
    if first is not None and second is not None and total > 100.0001:
        bust = True

    # Assign extras
    spin_off_spins = []
    bonus_spins = []
    if extras:
        if has_bonus_kw:
            bonus_spins = [{"value": float(v)} for v in extras]
        else:
            spin_off_spins = [{"value": float(v)} for v in extras]

    return {
        "name": name,
        "pre_wheel_winnings": money_val,
        "initial_spins": [
            {"spin_index": 1, "value": float(first) if first is not None else None},
            {"spin_index": 2, "value": float(second) if second is not None else None},
        ],
        "total": float(total),
        "bust": bust,
        "spin_off_spins": spin_off_spins,
        "bonus_spins": bonus_spins,
        "advanced_to_showcase": False,
    }


# -----------------------------
# Winner resolution (handles spin-offs)
# -----------------------------

def resolve_winner(contestants):
    """
    Winner = highest non-bust initial total.
    If tie for best initial total, break tie using spin_off_spins in order.
    """
    # initial total (ignore spin-off/bonus)
    init_totals = []
    for c in contestants:
        a = c["initial_spins"][0]["value"]
        b = c["initial_spins"][1]["value"]
        init_total = (a or 0) + (b or 0)
        init_totals.append(init_total)

    # best among non-bust
    best = -1
    best_idxs = []
    for i, c in enumerate(contestants):
        if c["bust"]:
            continue
        t = init_totals[i]
        if t > best:
            best = t
            best_idxs = [i]
        elif t == best:
            best_idxs.append(i)

    if not best_idxs:
        return None

    if len(best_idxs) == 1:
        return best_idxs[0]

    # tie-break via spin-off spins
    k = 0
    while True:
        spins = []
        for i in best_idxs:
            so = contestants[i]["spin_off_spins"]
            spins.append(so[k]["value"] if k < len(so) else None)

        # if no one has a spin at this depth, unresolved
        if all(v is None for v in spins):
            return None

        # compare available spins
        maxv = max(v for v in spins if v is not None)
        new_best = [best_idxs[j] for j, v in enumerate(spins) if v == maxv]

        if len(new_best) == 1:
            return new_best[0]

        best_idxs = new_best
        k += 1


# -----------------------------
# Parsing a full showdown
# -----------------------------

def parse_showdown(text: str):
    tokens = text.split()
    starts = find_contestant_starts(tokens)

    contestants = []
    has_bonus = False
    has_spinoff = False

    for idx, start in enumerate(starts):
        end = starts[idx + 1] if idx + 1 < len(starts) else len(tokens)
        c = parse_contestant(tokens, start, end)
        contestants.append(c)
        if c["bonus_spins"]:
            has_bonus = True
        if c["spin_off_spins"]:
            has_spinoff = True

    winner_index = resolve_winner(contestants)
    if winner_index is not None:
        contestants[winner_index]["advanced_to_showcase"] = True

    return {
        "raw_text": text,
        "contestants": contestants,
        "has_bonus": has_bonus,
        "has_spinoff": has_spinoff,
        "winner_index": winner_index,
    }


# -----------------------------
# Scenario classification
# -----------------------------

def classify_scenario(showdown):
    contestants = showdown["contestants"]

    # Must be exactly 3 contestants
    if len(contestants) != 3:
        return None

    # No bonus spins allowed in any scenario you’re tracking here
    if showdown["has_bonus"]:
        return None

    # All must have first spin
    for c in contestants:
        if c["initial_spins"][0]["value"] is None:
            return None

    busts = sum(c["bust"] for c in contestants)

    # Scenario 3: has a spin-off, no bonus
    if showdown["has_spinoff"]:
        return 3

    # Scenario 1/2: no spinoff, no bonus
    if busts == 0:
        return 1
    if busts >= 1:
        return 2
    return None


# -----------------------------
# Main pipeline
# -----------------------------

def main():
    with open(INPUT_PATH, "r", encoding="utf-8") as f:
        episodes = json.load(f)

    structured_showdowns = []
    scenario_1_list = []
    scenario_2_list = []
    scenario_3_list = []

    for ep in episodes:
        for sd in ep.get("showcase_showdowns", []):
            parsed = parse_showdown(sd.get("text", ""))

            sc = classify_scenario(parsed)
            parsed["scenario"] = sc

            structured_showdowns.append(parsed)

            if sc == 1:
                scenario_1_list.append(parsed)
            elif sc == 2:
                scenario_2_list.append(parsed)
            elif sc == 3:
                scenario_3_list.append(parsed)

    with open("structured_showdowns.json", "w", encoding="utf-8") as f:
        json.dump(structured_showdowns, f, indent=2)

    with open("scenario_1_showdowns.json", "w", encoding="utf-8") as f:
        json.dump(scenario_1_list, f, indent=2)

    with open("scenario_2_showdowns.json", "w", encoding="utf-8") as f:
        json.dump(scenario_2_list, f, indent=2)

    with open("scenario_3_showdowns.json", "w", encoding="utf-8") as f:
        json.dump(scenario_3_list, f, indent=2)

    print("Total parsed:", len(structured_showdowns))
    print("Scenario 1:", len(scenario_1_list))
    print("Scenario 2:", len(scenario_2_list))
    print("Scenario 3:", len(scenario_3_list))


if __name__ == "__main__":
    main()
