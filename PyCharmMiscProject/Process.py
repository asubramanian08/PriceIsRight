#!/usr/bin/env python3
"""
TPIR Showcase Showdown parser + validator

Input:
    tpir_episodes_combined.json

Outputs:
    tpir_structured_showdowns.json
    tpir_showdown_parse_errors.json
"""

import json
import re
from typing import List, Dict, Any, Tuple

INPUT_PATH = r"C:\Users\eggep\Downloads\tpir_episodes_combined.json"
OUTPUT_STRUCTURED = "tpir_structured_showdowns.json"
OUTPUT_ERRORS = "tpir_showdown_parse_errors.json"

# -----------------------------
# Low-level helpers
# -----------------------------

MONEY_RE = re.compile(r'^\$?\d{1,3}(?:,\d{3})*(?:\.\d+)?$')
SPIN_RE = re.compile(r'^\d{1,3}(?:\.\d+)?$')

KEYWORDS = {
    "Through", "to", "the", "Showcases", "Showcase", "Round", "Goes",
    "BONUS", "Bonus", "SPIN", "Spin", "bonus", "ROUND",
    "showcases", "showcase"
}

BAD_NAME_TOKENS = {
    "OVER", "STAYS", "THROUGH", "THROUG", "THROPHUGH", "THROPUGH",
    "THRU", "THO"
}


def tokenize(text: str) -> List[str]:
    """Normalize and split showdown text into tokens."""
    if not text:
        return []
    text = text.replace("\xa0", " ")
    text = text.replace("►", " ").replace(">", " ")
    text = text.replace("=", " ")
    text = re.sub(r"\s+", " ", text).strip()
    return text.split(" ") if text else []


def is_money_token(tok: str) -> bool:
    return bool(MONEY_RE.match(tok))


def parse_money(tok: str):
    tok = tok.replace("$", "").replace(",", "")
    try:
        return int(tok)
    except ValueError:
        try:
            return float(tok)
        except ValueError:
            return None


def parse_spin(tok: str):
    if not SPIN_RE.match(tok):
        return None
    try:
        return float(tok)
    except ValueError:
        return None


def is_spin_value(tok: str) -> bool:
    """Recognize wheel spin values (5–100 or exactly 1.00)."""
    v = parse_spin(tok)
    if v is None:
        return False
    if abs(v - 1.0) < 1e-6:
        return True
    return 5 <= v <= 100


def is_name_token(tok: str) -> bool:
    """Improved heuristic to reject fake names like OVER, STAYS, THROUG."""
    if not tok:
        return False
    if tok in KEYWORDS:
        return False
    if tok.upper() in BAD_NAME_TOKENS:
        return False
    if tok in {"And", "&", "AND"}:
        return False
    if tok.isupper() and len(tok) > 3:
        return False
    if not tok[0].isalpha() or not tok[0].isupper():
        return False
    return True


# -----------------------------
# Contestant segmentation
# -----------------------------

def segment_contestants(tokens: List[str]) -> List[Tuple[int, int]]:
    """
    Find ranges in tokens corresponding to contestants using [money][Name...] pattern.
    """
    starts = []
    i = 0
    while i < len(tokens):
        if is_money_token(tokens[i]):
            j = i + 1
            name_tokens = []
            while j < len(tokens) and is_name_token(tokens[j]):
                name_tokens.append(tokens[j])
                j += 1
            if name_tokens:
                starts.append(i)
                i = j
                continue
        i += 1

    segments = []
    for idx, start in enumerate(starts):
        end = starts[idx + 1] - 1 if idx + 1 < len(starts) else len(tokens) - 1
        segments.append((start, end))
    return segments


def parse_contestant_segment(tokens: List[str], start: int, end: int) -> Dict[str, Any]:
    """Parse one contestant's section of the showdown."""
    prize_tok = tokens[start]
    pre_wheel = parse_money(prize_tok)

    # Name
    i = start + 1
    name_tokens = []
    while i <= end and is_name_token(tokens[i]):
        name_tokens.append(tokens[i])
        i += 1
    name = " ".join(name_tokens) if name_tokens else None

    rest_tokens = tokens[i:end + 1]

    # Spins
    spin_vals, spin_raw = [], []
    for t in rest_tokens:
        if is_spin_value(t):
            v = parse_spin(t)
            if v is not None:
                spin_vals.append(v)
                spin_raw.append(t)

    # Initial spins
    initial_spins = []
    for idx in range(2):
        if idx < len(spin_vals):
            initial_spins.append({"spin_index": idx + 1, "value": spin_vals[idx], "raw": spin_raw[idx]})
        else:
            initial_spins.append({"spin_index": idx + 1, "value": None, "raw": None})

    # Extra spins
    extra_spins = []
    if len(spin_vals) > 2:
        for extra_idx in range(2, len(spin_vals)):
            extra_spins.append({
                "round": extra_idx - 1,
                "value": spin_vals[extra_idx],
                "raw": spin_raw[extra_idx]
            })

    # Total
    vals = [s["value"] for s in initial_spins if s["value"] is not None]
    total = sum(vals) if vals else None
    bust = total is not None and total > 100

    segment_text = " ".join(rest_tokens).strip()

    # Improved advancement detection
    advanced = False
    if re.search(r"through\s+to\s+the\s+show", segment_text, re.IGNORECASE):
        advanced = True
    if re.search(r"goes\s+to\s+(the\s+)?showcase", segment_text, re.IGNORECASE):
        advanced = True

    # Bonus
    bonus_spins = []
    if re.search(r"Bonus", segment_text, re.IGNORECASE):
        m_val = re.search(r"Bonus(?:\s+Spin)?\s+(\d{1,3}(?:\.\d+)?)", segment_text, re.IGNORECASE)
        wheel_val = float(m_val.group(1)) if m_val else None

        m_cash = re.search(r"\$ ?(10,?000|25,?000|5,?000)", segment_text)
        prize = int(m_cash.group(1).replace(",", "")) if m_cash else None

        if wheel_val is not None or prize is not None:
            bonus_spins.append({
                "spin_index": 1,
                "wheel_value": wheel_val,
                "cash_prize": prize,
                "raw": segment_text,
                "interpreted_from": "explicit" if (m_val or m_cash) else "unknown"
            })

    return {
        "name": name,
        "pre_wheel_winnings": pre_wheel,
        "pre_wheel_winnings_raw": prize_tok,
        "initial_spins": initial_spins,
        "total": total,
        "bust": bust,
        "spin_off_spins": extra_spins,
        "bonus_spins": bonus_spins,
        "advanced_to_showcase": advanced,
        "notes": segment_text
    }


# -----------------------------
# Showdown parsing
# -----------------------------

def parse_showdown(text: str) -> Dict[str, Any]:
    tokens = tokenize(text)
    segments = segment_contestants(tokens)

    warnings = []
    if len(segments) < 2:
        warnings.append("too_few_contestants")

    contestants = []
    for idx, (start, end) in enumerate(segments):
        c = parse_contestant_segment(tokens, start, end)
        c["position"] = idx + 1
        contestants.append(c)
        if c["name"] is None or c["initial_spins"][0]["value"] is None:
            warnings.append(f"contestant_{idx+1}_missing_core_fields")

    # Winner detection
    advanced_indices = [i for i, c in enumerate(contestants) if c["advanced_to_showcase"]]
    winner_index = None

    if len(advanced_indices) == 1:
        winner_index = advanced_indices[0]
    elif len(advanced_indices) == 0 and contestants:
        best_total = -1
        best_idx = None
        for i, c in enumerate(contestants):
            t = c["total"]
            if t is None or t > 100:
                continue
            if t > best_total:
                best_total = t
                best_idx = i
        if best_idx is not None:
            winner_index = best_idx
            warnings.append("winner_inferred_by_total")
        else:
            warnings.append("no_non_bust_winner")
    else:
        warnings.append("multiple_advanced_flags")

    winner_name = contestants[winner_index]["name"] if winner_index is not None else None

    # Parse status
    if winner_name is None or any("missing_core_fields" in w for w in warnings):
        parse_status = "error"
    else:
        parse_status = "ok" if not warnings else "partial"

    return {
        "raw_text": text,
        "contestants": contestants,
        "winner_name": winner_name,
        "winner_index": winner_index,
        "parse_status": parse_status,
        "parse_warnings": warnings
    }


# -----------------------------
# Validation
# -----------------------------

def validate_showdown_struct(sd: Dict[str, Any]) -> List[str]:
    errors = []
    contestants = sd.get("contestants", [])
    winner_index = sd.get("winner_index")
    winner_name = sd.get("winner_name")

    # Too few contestants
    if len(contestants) < 2:
        errors.append("val_too_few_contestants")

    # Spin validation
    for ci, c in enumerate(contestants):
        spins = c.get("initial_spins", [])
        total = c.get("total")
        vals = []

        for s in spins:
            v = s["value"]
            if v is None:
                continue
            vals.append(v)
            if abs(v - 1.0) < 1e-6:
                continue
            if not (5 <= v <= 100 and abs(v % 5) < 1e-6):
                errors.append(f"val_spin_out_of_range_c{ci+1}_v{v}")

        recomputed = sum(vals) if vals else None
        if total is not None and recomputed is not None:
            if abs(total - recomputed) > 1e-6:
                errors.append(f"val_total_mismatch_c{ci+1}_total{total}_recomputed{recomputed}")

    # Winner validation
    if winner_index is None or winner_name is None:
        errors.append("val_no_winner")
    else:
        if not (0 <= winner_index < len(contestants)):
            errors.append("val_winner_index_out_of_range")
        else:
            Tw = contestants[winner_index]["total"]
            if Tw is None:
                errors.append("val_winner_missing_total")
            else:
                if Tw > 100:
                    errors.append("val_winner_bust_total")
                for j, c in enumerate(contestants):
                    if j == winner_index:
                        continue
                    T = c["total"]
                    if T is None or T > 100:
                        continue
                    if Tw < T:
                        errors.append("val_winner_not_highest_non_bust")
                        break

    # Advanced flag logic
    adv = [i for i, c in enumerate(contestants) if c["advanced_to_showcase"]]
    if len(adv) > 1:
        errors.append("val_multiple_advanced_flags")
    elif len(adv) == 1 and winner_index is not None:
        if adv[0] != winner_index:
            errors.append("val_advanced_not_winner")

    return errors


# -----------------------------
# Keep/Reject rules
# -----------------------------

STRUCTURAL_WARNINGS = {
    "too_few_contestants",
    "no_non_bust_winner",
    "multiple_advanced_flags"
}


def is_structural_warning(w: str) -> bool:
    return (
        w in STRUCTURAL_WARNINGS or
        "missing_core_fields" in w
    )


# -----------------------------
# Top-level driver
# -----------------------------

def main():
    with open(INPUT_PATH, "r", encoding="utf-8") as f:
        episodes = json.load(f)

    structured_episodes = []
    error_showdowns = []

    total_showdowns = 0
    kept_showdowns = 0
    parse_error_count = 0
    validation_error_count = 0

    for ep in episodes:
        new_ep = dict(ep)
        parsed_showdowns = []

        for sd in ep.get("showcase_showdowns", []):
            total_showdowns += 1
            text = sd.get("text", "")
            label = sd.get("label")

            parsed = parse_showdown(text)
            warnings = parsed["parse_warnings"]
            val_errors = validate_showdown_struct(parsed)
            parsed["validation_errors"] = val_errors
            parsed["label"] = label

            # Decide if showdown is usable
            structural_problem = any(is_structural_warning(w) for w in warnings)

            if (
                parsed["parse_status"] in ("ok", "partial")
                and not val_errors
                and not structural_problem
                and parsed["winner_name"] is not None
            ):
                parsed["status"] = "kept"
                parsed_showdowns.append(parsed)
                kept_showdowns += 1
            else:
                parsed["status"] = "error"
                if parsed["parse_status"] == "error" or structural_problem:
                    parse_error_count += 1
                if val_errors:
                    validation_error_count += 1

                error_showdowns.append({
                    "episode_title": ep.get("episode_title"),
                    "iso_date": ep.get("iso_date"),
                    "url": ep.get("url"),
                    "label": label,
                    "raw_text": text,
                    "parse_status": parsed["parse_status"],
                    "parse_warnings": warnings,
                    "validation_errors": val_errors,
                    "parsed_contestants": parsed.get("contestants", []),
                    "winner_name": parsed.get("winner_name"),
                    "winner_index": parsed.get("winner_index"),
                })

        if parsed_showdowns:
            new_ep["parsed_showdowns"] = parsed_showdowns
            structured_episodes.append(new_ep)

    # Output
    with open(OUTPUT_STRUCTURED, "w", encoding="utf-8") as f:
        json.dump(structured_episodes, f, ensure_ascii=False, indent=2)

    with open(OUTPUT_ERRORS, "w", encoding="utf-8") as f:
        json.dump(error_showdowns, f, ensure_ascii=False, indent=2)

    print(f"Structured episodes written to: {OUTPUT_STRUCTURED}")
    print(f"Problematic/odd showdowns written to: {OUTPUT_ERRORS}")
    print(f"Total showdowns seen: {total_showdowns}")
    print(f"Showdowns kept (good): {kept_showdowns}")
    print(f"Showdowns with structural issues: {parse_error_count}")
    print(f"Showdowns with validation issues: {validation_error_count}")
    print(f"Episodes with at least one valid showdown: {len(structured_episodes)}")


if __name__ == "__main__":
    main()
