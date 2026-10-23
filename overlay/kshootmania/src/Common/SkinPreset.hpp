#pragma once
#include <Siv3D.hpp>
#include "MusicGame/Judgment/JudgmentDefines.hpp"

namespace SkinPreset
{
    enum class MissingMode { Default, Error };
    enum class IndicatorPortrait { Off, Bottom, Both };
    enum class KeyBeamMode { Off, Normal, AllCrit, AllPerfect, OnlyCrit, OnlyPerfect };
    // Presentation policy only: never changes the recorded judgment or score.
    inline Optional<MusicGame::Judgment::KeyBeamType> DisplayBeam(MusicGame::Judgment::KeyBeamType source, KeyBeamMode mode)
    {
        using B=MusicGame::Judgment::KeyBeamType;
        const bool successful=source==B::kNear || source==B::kNearFast || source==B::kNearSlow || source==B::kCritical || source==B::kPerfectCritical;
        switch(mode) {
        case KeyBeamMode::Off:return none;
        case KeyBeamMode::AllCrit:return successful?B::kCritical:source;
        case KeyBeamMode::AllPerfect:return successful?B::kPerfectCritical:source;
        case KeyBeamMode::OnlyCrit:return source==B::kCritical || source==B::kPerfectCritical ? Optional<B>{B::kCritical} : none;
        case KeyBeamMode::OnlyPerfect:return source==B::kPerfectCritical ? Optional<B>{B::kPerfectCritical} : none;
        default:return source;
        }
    }
    struct Settings
    {
        String folder = U"default";
        MissingMode missing = MissingMode::Default;
        int32 background1Brightness = 100;
        int32 background2Opacity = 100;
        double laserWidthPercent = 100.0;
        int32 noteHeightPercent = 100;
        IndicatorPortrait indicatorPortrait = IndicatorPortrait::Bottom;
        bool indicatorLandscape = true;
        KeyBeamMode keyBeamMode = KeyBeamMode::Normal;
        void sanitize();
        bool operator==(const Settings&) const = default;
    };
    struct Entry { String folder, author, description; };
    void Initialize();
    Array<Entry> Discover();
    const Settings& Current();
    void Activate(const Settings& settings);
    bool Save(const Settings& settings);
    FilePath Resolve(FilePathView logicalPath);
    Texture LoadTexture(FilePathView logicalPath, TextureDesc desc = TextureDesc::Unmipped);
    void RegisterTextures();
    inline constexpr double kVerticalLaserPercent = 100.0 * 8.0 / 11.0;
    double StepLaserPercent(double value, int32 direction);
}
