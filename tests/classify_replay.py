"""Classify replay_reference.mjs JSON output or a --report file.

Usage: python3 tests/classify_replay.py <log-or-report.json> [shown differences]
Use replay --report (or --verbose) for per-slot counts and the median decision reached.
"""

import argparse
import collections
import json
import pathlib


def read_replay(path):
    text = pathlib.Path(path).read_text()
    try:
        document = json.loads(text)
    except json.JSONDecodeError:
        documents = [json.loads(line) for line in text.splitlines() if line.strip()]
    else:
        if "summary" in document:
            return document["summary"], document["results"]
        documents = [document]
    summaries = [value for value in documents if "games" in value]
    if len(summaries) != 1:
        raise ValueError("expected exactly one replay JSON summary; use replay --report for details")
    return summaries[0], [value for value in documents if "status" in value]


def classify(summary, results, shown):
    print({key: summary[key] for key in ("games", "identical", "untranslated", "different", "decisions")})
    complete = len(results) == summary["games"]
    if complete:
        kinds = collections.Counter(result["status"] for result in results)
        for status in ("identical", "untranslated", "different"):
            if kinds[status] != summary[status]:
                raise ValueError(f"summary/results count mismatch for {status}")
        decisions = sorted(result["decisions"] for result in results)
        print("median decision reached:", decisions[len(decisions) // 2] if decisions else "-")
        slots = collections.Counter(result["slot"] for result in results if result["status"] == "untranslated")
        print("slots stopping most games:", slots.most_common(20))
    else:
        print("slot counts and median unavailable in summary-only log; use replay --report or --verbose")
    differences = [result for result in results if result["status"] == "different"]
    for result in differences[:shown]:
        print("-----\n" + json.dumps(result, indent=2))
    if len(differences) < summary["different"]:
        print("difference details unavailable; use replay --report or --verbose")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input")
    parser.add_argument("shown", nargs="?", type=int, default=3)
    options = parser.parse_args()
    if options.shown < 0:
        parser.error("shown differences must be nonnegative")
    summary, results = read_replay(options.input)
    classify(summary, results, options.shown)
