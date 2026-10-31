"""Regression checks for JSON summary logs, verbose logs and detailed replay reports."""

import contextlib
import io
import json
import pathlib
import unittest
from unittest.mock import patch

from classify_replay import classify, read_replay


class ClassifyReplayTest(unittest.TestCase):
    summary = {"games": 3, "identical": 1, "untranslated": 1, "different": 1, "decisions": 48}
    results = [
        {"index": 0, "status": "identical", "decisions": 30},
        {"index": 1, "status": "untranslated", "slot": 2496, "decisions": 15},
        {"index": 2, "status": "different", "decisions": 3, "path": ".state.dice.0"},
    ]

    def parse(self, text):
        with patch.object(pathlib.Path, "read_text", return_value=text):
            return read_replay("unused")

    def render(self, parsed, shown=3):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            classify(*parsed, shown)
        return output.getvalue()

    def test_summary_counts_do_not_depend_on_printed_game_blocks(self):
        parsed = self.parse(json.dumps(self.summary))
        self.assertEqual(parsed, (self.summary, []))
        text = self.render(parsed)
        self.assertIn("'untranslated': 1, 'different': 1", text)
        self.assertIn("median unavailable", text)

    def test_default_log_keeps_real_difference_details(self):
        parsed = self.parse(json.dumps(self.results[2]) + "\n" + json.dumps(self.summary))
        self.assertIn('"path": ".state.dice.0"', self.render(parsed))
        self.assertNotIn('"path"', self.render(parsed, shown=0))

    def test_verbose_and_report_formats_agree(self):
        verbose = self.parse("\n".join(map(json.dumps, [*self.results, self.summary])))
        report = self.parse(json.dumps({"summary": self.summary, "results": self.results}, indent=2))
        self.assertEqual(verbose, report)
        text = self.render(report)
        self.assertIn("median decision reached: 15", text)
        self.assertIn("[(2496, 1)]", text)

    def test_inconsistent_report_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "count mismatch"):
            self.render(({**self.summary, "identical": 2}, self.results))

    def test_missing_summary_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "exactly one"):
            self.parse(json.dumps(self.results[0]))


if __name__ == "__main__":
    unittest.main()
