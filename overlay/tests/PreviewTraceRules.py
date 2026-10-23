"""Sampling rules for preview traces; the expected music loop remains 18 seconds."""


def verify_preview_loop_boundary(previous, current):
    frame_gap = current["wall"] - previous["wall"]
    assert frame_gap >= 0, "preview wall clock moved backwards"
    # A hitch can skip the final 100 ms sample without changing the loop itself.
    earliest_previous = 18.0 - max(0.1, frame_gap)
    assert earliest_previous < previous["chart"] < 18.1 and 0 <= current["chart"] < 0.2, (
        "invalid 18 second preview wrap: "
        f"{previous['chart']} -> {current['chart']}, observed frame gap={frame_gap}"
    )
