#pragma once
#include <NocoUI.hpp>

namespace MusicGame::Graphics {
inline void ApplyTimingTextStyle(noco::Canvas& canvas, StringView prefix, Color color,
    double opacity, bool glow) {
    const double strength = glow ? 0.36 : 1.0;
    const auto alpha = static_cast<uint8>(Clamp(opacity * 255 * strength, 0.0, 255.0));
    const Color fill = glow ? color : Color{ColorF{color}.lerp(Palette::White, 0.08)};
    const Color top{fill.r, fill.g, fill.b, alpha};
    const Color bottom{static_cast<uint8>(fill.r * 0.96), static_cast<uint8>(fill.g * 0.96),
        static_cast<uint8>(fill.b * 0.96), alpha};
    canvas.setParamValues({{String{prefix} + U"Top", top}, {String{prefix} + U"Bottom", bottom},
        {String{prefix} + U"Outline", Color{fill.r, fill.g, fill.b, static_cast<uint8>(alpha * 0.95)}},
        {String{prefix} + U"Shadow", Color{0, 0, 0, static_cast<uint8>(glow ? 0 : alpha * 0.18)}}});
}
}
