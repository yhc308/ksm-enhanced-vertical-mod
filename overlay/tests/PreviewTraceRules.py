"""Validate the unchanged 18-second preview clock with sampled frame times.

An unchanged reference executable reproduces false fixed-first-frame failures
when its owned renderer is paused for 600ms. Sampling uncertainty is bounded by
the actual adjacent frame interval; it is not an increase to the loop length.
"""
LOOP_SECONDS = 18.0
CLOCK_TOLERANCE_SECONDS = 0.12


def verify_preview_loop_boundary(previous, current):
    gap = current['wall'] - previous['wall']
    assert 0 <= gap < LOOP_SECONDS, 'unobservable or reversed preview frame interval'
    assert 0 <= previous['chart'] < 18.1 and 0 <= current['chart'] < LOOP_SECONDS, 'invalid chart range at wrap'
    progress = LOOP_SECONDS - previous['chart'] + current['chart']
    assert abs(progress - gap) <= CLOCK_TOLERANCE_SECONDS, (
        f'invalid 18 second preview wrap: {previous["chart"]} -> {current["chart"]}, '
        f'wall gap={gap}, chart progress including wrap={progress}')


def verify_preview_loop_period(first, second):
    # The true event is inside [last old frame, first new frame]. Compare the
    # event intervals, not two arbitrarily late first-new-frame timestamps.
    first_previous, first_current = first
    second_previous, second_current = second
    lower = second_previous['wall'] - first_current['wall']
    upper = second_current['wall'] - first_previous['wall']
    assert lower <= LOOP_SECONDS + CLOCK_TOLERANCE_SECONDS and upper >= LOOP_SECONDS - CLOCK_TOLERANCE_SECONDS, (
        f'loop duration drift: event interval [{lower}, {upper}] excludes 18 seconds')


def verify_preview_edit_clock(previous, current):
    if current['chart'] < previous['chart'] - 10:
        verify_preview_loop_boundary(previous, current)
    else:
        wall = current['wall'] - previous['wall']
        chart = current['chart'] - previous['chart']
        assert wall >= 0 and chart >= 0, 'edit restarted or reversed playback'
        assert abs(chart - wall) <= CLOCK_TOLERANCE_SECONDS, 'edit changed playback clock rate'
