#!/usr/bin/env python3
"""
Compact analysis of the parsing/validation error file produced by Process.py.

- Reports frequency of each error type
- Shows 1–2 clean, human-readable examples per error type
"""

import json
from collections import Counter, defaultdict

# MODIFY THIS IF YOUR PATH DIFFERS
INPUT_PATH = r"C:\Users\eggep\PyCharmMiscProject\tpir_showdown_parse_errors.json"

# Max number of human-readable examples per error type
MAX_EXAMPLES_PER_ERROR_TYPE = 1


def main():
    with open(INPUT_PATH, "r", encoding="utf-8") as f:
        errors = json.load(f)

    print(f"\n=== ERROR DATASET SUMMARY ===")
    print(f"Total error showdowns: {len(errors)}\n")

    # Containers for counting and grouping
    type_counter = Counter()
    grouped_examples = defaultdict(list)

    for e in errors:
        parse_warnings = e.get("parse_warnings", []) or []
        validation_errors = e.get("validation_errors", []) or []
        combined_errors = parse_warnings + validation_errors

        if not combined_errors:
            combined_errors = ["unknown_error"]

        for err in combined_errors:
            type_counter[err] += 1
            if len(grouped_examples[err]) < MAX_EXAMPLES_PER_ERROR_TYPE:
                grouped_examples[err].append(e)

    # --------------------------------------------------
    # ERROR FREQUENCY REPORT
    # --------------------------------------------------
    print("=== ERROR TYPE FREQUENCY ===")
    for err, ct in type_counter.most_common():
        print(f"{err}: {ct}")

    # --------------------------------------------------
    # EXAMPLES
    # --------------------------------------------------
    print("\n=== HUMAN-READABLE ERROR EXAMPLES ===")
    for err, examples in grouped_examples.items():
        print(f"\n--- ERROR TYPE: {err} ---")
        for ex in examples:
            print(f"\nEpisode: {ex.get('episode_title')}   Date: {ex.get('iso_date')}")
            print(f"URL: {ex.get('url')}")
            print(f"Label: {ex.get('label')}")
            print(f"Raw text:\n  {ex.get('raw_text')}")

            print("\nParsed contestants:")
            for c in ex.get("parsed_contestants", []):
                spins = [s["value"] for s in c.get("initial_spins", [])]
                extras = [s["value"] for s in c.get("spin_off_spins", [])]
                print(f"  - {c.get('name')}: spins={spins}, extras={extras}, total={c.get('total')}")

            print(f"\nWinner: {ex.get('winner_name')}")
            print(f"Parse warnings: {ex.get('parse_warnings')}")
            print(f"Validation errors: {ex.get('validation_errors')}")
            print("-" * 60)

    print("\nDone.")


if __name__ == "__main__":
    main()
