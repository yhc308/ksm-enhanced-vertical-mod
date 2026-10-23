#pragma once
#include <cstdint>
#include <array>
#include <optional>

namespace Vertical::DiagnosticInputReplay
{
    struct State
    {
        uint32_t held = 0;
        uint32_t previous = 0;
        bool laserDigital = true;
        std::array<double, 2> laserDelta{};
    };
    inline std::optional<State> state;
    inline uint32_t Mask(int button) { return button >= 0 && button < 32 ? uint32_t{1} << button : 0; }
    inline bool Active() { return state.has_value(); }
    inline bool Held(int button) { return (state->held & Mask(button)) != 0; }
    inline bool Down(int button) { return (state->held & ~state->previous & Mask(button)) != 0; }
    inline bool Up(int button) { return (~state->held & state->previous & Mask(button)) != 0; }
    inline void Frame(uint32_t held, std::array<double, 2> delta = {})
    {
        state->previous = state->held;
        state->held = held;
        state->laserDelta = delta;
    }
    class Session
    {
    public:
        Session() { if (state) throw Error{U"Replay already active"}; state.emplace(); }
        ~Session() { state.reset(); }
        Session(const Session&) = delete;
        Session& operator=(const Session&) = delete;
    };
}
