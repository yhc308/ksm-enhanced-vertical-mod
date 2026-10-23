#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace Vertical
{
    // Measured portrait guide. These describe visible geometry, not chart values.
    inline constexpr double kDefaultLaneWidth = 557.417294 / 720.0;
    inline constexpr double kDefaultJudgmentLineY = 906.0 / 1280.0;
    inline constexpr double kDefaultLaneTopY = 448.0 / 1280.0;
    inline constexpr double kVanishingPointGap = 48.0 / 1280.0;
    inline constexpr double kButtonWidthRatio = 0.75;
    inline constexpr double kLaserRailWidthRatio = 0.125;
    inline constexpr double kHighwayUVScaleX = .9925;
    inline constexpr double kJudgeTextureT = 1.0 - 14.0 / 1024;
    inline constexpr double kTailLogicalPixels = 28.0;
    inline constexpr double kOrangeCoreWidthRatio = 668.25415456 / 720.0;
    inline constexpr int kLayoutVersion = 3;
    enum class DisplayMode { Windowed, Fullscreen, BorderlessFullscreen };
    struct Resolution { int width; int height; };
    inline constexpr std::array<Resolution, 6> kPresets{{
        {360, 640}, {540, 960}, {720, 1280},
        {900, 1600}, {1080, 1920}, {1440, 2560}
    }};

    struct LayoutSettings
    {
        double laneWidth = kDefaultLaneWidth;
        double judgmentLineY = kDefaultJudgmentLineY;
        double laneTopY = kDefaultLaneTopY;

        void sanitize()
        {
            laneWidth = std::isfinite(laneWidth) ? std::clamp(laneWidth, 0.70, 0.96) : kDefaultLaneWidth;
            judgmentLineY = std::isfinite(judgmentLineY) ? std::clamp(judgmentLineY, 0.65, 0.90) : kDefaultJudgmentLineY;
            laneTopY = std::isfinite(laneTopY) ? std::clamp(laneTopY, 0.20, 0.40) : kDefaultLaneTopY;
        }
    };

    // Separate from legacy ScreenUtils::Scaled. This is a viewport mapping,
    // not a camera transform: camera integration must project every lane anchor.
    struct Viewport
    {
        double scale;
        double left;
        double top;

        static Viewport Fit(int width, int height)
        {
            const double s = std::min(std::max(width, 1) / 720.0, std::max(height, 1) / 1280.0);
            return {s, (std::max(width, 1) - 720.0 * s) * 0.5,
                (std::max(height, 1) - 1280.0 * s) * 0.5};
        }
        double x(double logical) const { return left + logical * scale; }
        double y(double logical) const { return top + logical * scale; }
    };
}
