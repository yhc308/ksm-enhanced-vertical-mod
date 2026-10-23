#pragma once
#include "Ini/ConfigIni.hpp"
#include "VerticalLayout.hpp"
#include "DepthImageSettings.hpp"

namespace Vertical
{
    inline bool& SettingsPersistence() {static bool enabled=true;return enabled;}
    inline void SaveOptionsConfiguration() {if(SettingsPersistence()) ConfigIni::Save();}
    // Only mod-owned keys may be written here. No timing, judgment, audio,
    // input, VSync or legacy screen configuration is modified.
    struct ModConfig
    {
        bool enabled = false;
        bool backgroundDepthEnabled = true;
        std::array<int,8> depthIntensities=kDepthImageDefaults;
        DisplayMode displayMode = DisplayMode::Windowed;
        String monitorId;
        Resolution windowSize{540, 960};
        LayoutSettings layout;

        static ModConfig Load()
        {
            ModConfig result;
            result.enabled = ConfigIni::GetBool(U"vertical_enabled", false);
            result.backgroundDepthEnabled=ConfigIni::GetBool(U"vertical_background_depth_enabled",true);
            for(size_t i=0;i<kDepthImageNames.size();++i)
                result.depthIntensities[i]=std::clamp(ConfigIni::GetInt(U"vertical_depth_strength_"+String{kDepthImageNames[i]},kDepthImageDefaults[i]),0,200);
            result.displayMode = static_cast<DisplayMode>(std::clamp(
                ConfigIni::GetInt(U"vertical_display_mode", 0), 0, 2));
            result.monitorId = ConfigIni::GetString(U"vertical_monitor", U"");
            const Resolution saved{ConfigIni::GetInt(U"vertical_width", 540),
                ConfigIni::GetInt(U"vertical_height", 960)};
            const auto resolutionText = String{ConfigIni::GetString(U"vertical_resolution", U"")}.split(U',');
            Resolution selected = saved;
            if (resolutionText.size() == 2)
                selected = {ParseOr<int32>(resolutionText[0], 540), ParseOr<int32>(resolutionText[1], 960)};
            for (const auto& preset : kPresets)
            {
                if (selected.width == preset.width && selected.height == preset.height)
                    result.windowSize = selected;
            }
            result.layout.laneWidth = ConfigIni::GetDouble(U"vertical_lane_width", kDefaultLaneWidth);
            result.layout.judgmentLineY = ConfigIni::GetDouble(U"vertical_judgment_y", kDefaultJudgmentLineY);
            result.layout.laneTopY = ConfigIni::GetDouble(U"vertical_lane_top", kDefaultLaneTopY);
            result.layout.sanitize();
            return result;
        }

        void Save() const
        {
            auto validatedLayout = layout;
            validatedLayout.sanitize();
            ConfigIni::SetBool(U"vertical_enabled", enabled);
            ConfigIni::SetBool(U"vertical_background_depth_enabled",backgroundDepthEnabled);
            for(size_t i=0;i<kDepthImageNames.size();++i)
                ConfigIni::SetInt(U"vertical_depth_strength_"+String{kDepthImageNames[i]},std::clamp(depthIntensities[i],0,200));
            ConfigIni::SetInt(U"vertical_display_mode", std::clamp(static_cast<int>(displayMode), 0, 2));
            ConfigIni::SetString(U"vertical_monitor", monitorId);
            Resolution validatedSize{540, 960};
            for (const auto& preset : kPresets)
            {
                if (windowSize.width == preset.width && windowSize.height == preset.height)
                    validatedSize = windowSize;
            }
            ConfigIni::SetInt(U"vertical_width", validatedSize.width);
            ConfigIni::SetInt(U"vertical_height", validatedSize.height);
            ConfigIni::SetString(U"vertical_resolution", U"{},{}"_fmt(validatedSize.width, validatedSize.height));
            ConfigIni::SetString(U"vertical_lane_width", U"{:.12f}"_fmt(validatedLayout.laneWidth));
            ConfigIni::SetString(U"vertical_judgment_y", U"{:.12f}"_fmt(validatedLayout.judgmentLineY));
            ConfigIni::SetString(U"vertical_lane_top", U"{:.12f}"_fmt(validatedLayout.laneTopY));
        }

        void ResetDepthImages() {depthIntensities=kDepthImageDefaults;}
        void ResetDepthImage(size_t index) {if(index<depthIntensities.size())depthIntensities[index]=kDepthImageDefaults[index];}
        void ResetLayout() { layout = LayoutSettings{}; }

        static bool MigrateLayout()
        {
            if (ConfigIni::GetInt(U"vertical_layout_version", 0) >= kLayoutVersion) return false;
            ConfigIni::SetString(U"vertical_lane_width", U"{:.12f}"_fmt(kDefaultLaneWidth));
            ConfigIni::SetString(U"vertical_judgment_y", U"{:.12f}"_fmt(kDefaultJudgmentLineY));
            ConfigIni::SetString(U"vertical_lane_top", U"{:.12f}"_fmt(kDefaultLaneTopY));
            ConfigIni::SetInt(U"vertical_layout_version", kLayoutVersion);
            return true;
        }
    };
    inline Optional<ModConfig>& DepthPreviewDraft() {static Optional<ModConfig> draft;return draft;}
    inline ModConfig EffectiveDepthConfig() {return DepthPreviewDraft() ? *DepthPreviewDraft() : ModConfig::Load();}
}
