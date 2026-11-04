"""Ensure USB quality checks detect added clicks, missing blocks and ADSR faults.

Run using the MCP audio-test Python environment (requirements-audio-test.txt).
"""
from pathlib import Path
import sys
import unittest

import numpy as np

sys.path.insert(0, str((Path(__file__).resolve().parents[2] / 'platformio') / '../_debug/analysis'))
from audio_quality import RATE, default_reference, envelope_reference, match_reference, match_release


class QualityAnalysis(unittest.TestCase):
    def setUp(self):
        rng = np.random.default_rng(127)
        self.source = rng.uniform(-.4, .4, RATE//5)
        self.offset = 1234

    def capture(self, source):
        mono = np.pad(source, (self.offset, 500))
        return np.column_stack([mono, mono])

    def test_clean_source_alignment_and_fixed_gain(self):
        result = match_reference(self.capture(self.source*.75), self.source)
        self.assertEqual(result['offset_samples'], self.offset)
        self.assertGreater(min(result['snr_db']), 100)
        np.testing.assert_allclose(result['gain'], [.75, .75])
        self.assertEqual(result['missing_blocks'], 0)

    def test_added_click_and_single_channel_fault(self):
        recording = self.capture(self.source)
        recording[self.offset+500, 1] += .8
        result = match_reference(recording, self.source)
        self.assertLess(result['snr_db'][1], 35)
        self.assertGreater(result['added_jump_dbfs'], -5)
        self.assertGreater(result['snr_db'][0], 100)

    def test_missing_audio_block(self):
        damaged = self.source.copy()
        damaged[1024:1152] = 0
        result = match_reference(self.capture(damaged), self.source)
        self.assertGreaterEqual(result['missing_blocks'], 1)
        self.assertLess(min(result['snr_db']), 35)

    def test_stale_forced_release_is_not_hidden_by_gain_fit(self):
        damaged = self.source.copy()
        count = int(.005*RATE)
        damaged[:count] *= np.linspace(1, 0, count)
        result = match_reference(self.capture(damaged), self.source)
        self.assertLess(min(result['snr_db']), 35)

    def test_native_zero_attack_model_does_not_hide_five_ms_fault(self):
        expected = default_reference(self.source)
        self.assertGreater(min(match_reference(self.capture(expected), expected)['snr_db']), 100)
        damaged = self.source.copy()
        count = int(.005*RATE)
        damaged[:count] *= np.linspace(1, 0, count)
        result = match_reference(self.capture(damaged), expected)
        self.assertLess(min(result['snr_db']), 35)

    def test_source_decay_is_not_mistaken_for_envelope(self):
        expected = envelope_reference(self.source, attack=.0625, decay=.0625, sustain=.5)
        good = match_reference(self.capture(expected), expected)
        bad = match_reference(self.capture(self.source), expected)
        self.assertGreater(min(good['snr_db']), 100)
        self.assertLess(min(bad['snr_db']), 22)

    def test_release_slope_fixed_despite_panel_timing_uncertainty(self):
        t = np.arange(len(self.source))/RATE
        released = self.source*np.clip(1-(t-.06)/.03125, 0, 1)
        recording = self.capture(released)
        correct = match_release(recording, self.source, .03125)
        wrong = match_release(recording, self.source, .09375)
        self.assertGreater(min(correct['snr_db']), 100)
        self.assertAlmostEqual(correct['gate_ms'], 60)
        self.assertLess(min(wrong['snr_db']), 22)


if __name__ == '__main__':
    unittest.main()
