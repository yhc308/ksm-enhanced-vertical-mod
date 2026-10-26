from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parent))
from PreviewTraceRules import verify_preview_loop_boundary, verify_preview_loop_period, verify_preview_edit_clock


def row(wall, chart):
    return {'wall': wall, 'chart': chart}


class PreviewTraceRulesTests(unittest.TestCase):
    def test_regular_wrap(self):
        verify_preview_loop_boundary(row(18, 17.99), row(18.04, .01))

    def test_independent_reference_renderer_pause(self):
        # Captured from unchanged executable SHA83055F2D..., not the candidate.
        verify_preview_loop_boundary(row(36.36333, 17.87756), row(37.02431, .48397))

    def test_old_reference_400ms_pause(self):
        verify_preview_loop_boundary(row(18.00909, 17.69548), row(18.47875, .10789))

    def test_early_reset_rejected(self):
        with self.assertRaises(AssertionError):
            verify_preview_loop_boundary(row(5, 5), row(5.02, .01))

    def test_late_or_invalid_reset_rejected(self):
        for a,b in [(row(19,19),row(19.02,0)), (row(18,17.99),row(18.02,2)), (row(18,17.99),row(17.99,0))]:
            with self.assertRaises(AssertionError):
                verify_preview_loop_boundary(a,b)

    def test_unobservable_multiple_loops_rejected(self):
        with self.assertRaises(AssertionError):
            verify_preview_loop_boundary(row(1,1), row(40,1))

    def test_regular_period(self):
        verify_preview_loop_period((row(18.0,17.99),row(18.02,0)), (row(36.0,17.99),row(36.02,0)))

    def test_delayed_reference_observation(self):
        verify_preview_loop_period((row(18.30359,17.91602),row(18.47562,.03159)),
                                   (row(36.36333,17.87756),row(37.02431,.48397)))

    def test_real_period_drift_rejected(self):
        for end in (35, 37, 40):
            with self.assertRaises(AssertionError):
                verify_preview_loop_period((row(18,17.99),row(18.02,0)), (row(end,17.99),row(end+.02,0)))

    def test_edit_does_not_restart_clock(self):
        verify_preview_edit_clock(row(9,8.5),row(9.02,8.52))

    def test_elapsed_time_during_stalled_edit(self):
        verify_preview_edit_clock(row(9.58385,9.15884),row(17.66159,17.23629))

    def test_edit_at_normal_loop_boundary(self):
        verify_preview_edit_clock(row(17.66159,17.23629),row(18.5953,.10842))

    def test_unexpected_edit_reset_or_jump_rejected(self):
        for a,b in [(row(5,5),row(5.02,0)), (row(5,5),row(5.02,5.5)), (row(5,5),row(6,5.1))]:
            with self.assertRaises(AssertionError):
                verify_preview_edit_clock(a,b)


if __name__ == '__main__':
    unittest.main()
