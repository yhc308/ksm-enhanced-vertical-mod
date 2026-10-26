# Windows frame-pacing regression fix

This is a separate performance correction exposed during the style refactor, not a source-format-only change. Earlier capped/native performance failures are retained in REFACTOR_VALIDATION.md.

## Problem and experiment

At a 300fps limit the first normal refactored executable improved average frame intervals but exceeded the accepted p99 threshold in one native-play orientation. Raising the cap removed the difference. Test-only instrumentation showed variable frame-work tails and substantial sleep/wakeup tails on both original and refactored binaries. The original limiter used std::this_thread::sleep_until.

An isolated high-resolution timer prototype passed three native DXGI captures per binary/orientation: landscape p99 6.6035 -> 5.8183 ms; portrait 6.5779 -> 6.0882 ms. These prototype results motivated the correction; the final implementation subsequently passed the full functional, exact-appearance, normal-runtime and final native/long-diagnostic performance checks. See REFACTOR_VALIDATION.md for the raw evidence and retained short-capture failures.

## Preserved behavior and ownership

FrameRateLimit retains its target-FPS option, accumulated absolute steady-clock deadline, 10ms drift clamp, vsync bypass and configuration format. Windows uses one lazily created thread-local WindowsWaitableTimer. It waits on a high-resolution native timer without spinning, rounds relative intervals upward to 100ns and rechecks the steady-clock deadline after a wake so it cannot return early. The RAII owner is noncopyable/nonmovable and closes its native handle.

If timer creation, arming or waiting fails, FrameRateLimit uses the existing std::this_thread::sleep_until path. Other platforms keep that same path. No global timer-resolution setting, driver, service, priority, audio clock, judge offset or input/gauge calculation is changed. There is no per-frame allocation, logging or file access in this helper.

## Tests

Six actual Win32 Catch2 cases (16 assertions) cover missing handles, expired/future deadlines, no early return, ownership/close, an invalid native timer-operation fallback, and native high-resolution operation. They run headlessly without Siv3D and inside the Windows game unit suite. A Windows GitHub Actions job runs the same tests independently of the SDK. The original rolling-window and game tests remain.

Final integrated validation passed: 27 functional stages, 43 cases / 3,982 assertions in each Windows configuration, 81 exact APS images, 110 judge-setting images with the exact FPS-label exclusion, both input replays, fresh ordinary Release/Debug builds and normal smokes, final-payload preservation, native-play captures and the predeclared long diagnostic performance protocol. The source-only destination build also passed. Prototype/instrumented timings and the retained short-capture FAIL are not relabeled as passing final-binary measurements.
