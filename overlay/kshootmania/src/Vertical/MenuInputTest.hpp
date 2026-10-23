#pragma once
#include "DiagnosticInputReplay.hpp"
#include "VerticalModConfig.hpp"
#include "Input/MenuInputProfiles.hpp"

namespace Vertical
{
    inline void RunMenuInputTest()
    {
        ConfigIni::Load();
        const auto original = ModConfig::Load();
        const int originalLaserType = ConfigIni::GetInt(ConfigIni::Key::kLaserInputType);
        ConfigIni::SetInt(ConfigIni::Key::kLaserInputType, ConfigIni::Value::LaserInputType::kKeyboard);
        DiagnosticInputReplay::Session session;
        TextWriter report{U"vertical-menu-input.csv"};
        report.writeln(U"enabled,profile,button,delta,expected,status");
        bool passed = true;
        for (const bool enabled : {false, true})
        {
            auto config = original;
            config.enabled = enabled;
            config.Save();
            for (const bool values : {false, true})
            {
                for (const Button button : std::array<Button, 10>{kButtonUp, kButtonDown, kButtonLeft, kButtonRight,
                    kButtonLeftLaserL, kButtonLeftLaserR, kButtonRightLaserL, kButtonRightLaserR,
                    kButtonBT_A, kButtonBT_D})
                {
                    DiagnosticInputReplay::Frame(0);
                    CursorInput input{values ? MenuInputProfiles::OptionValues() : MenuInputProfiles::OptionRows()};
                    DiagnosticInputReplay::Frame(DiagnosticInputReplay::Mask(button));
                    input.update();
                    const int expected = values
                        ? ((button == kButtonLeft || button == kButtonLeftLaserL) ? -1 :
                            (button == kButtonRight || button == kButtonLeftLaserR) ? 1 : 0)
                        : ((button == kButtonUp || button == kButtonRightLaserL) ? -1 :
                            (button == kButtonDown || button == kButtonRightLaserR) ? 1 : 0);
                    const bool ok = input.deltaCursor() == expected;
                    passed = passed && ok;
                    report.writeln(U"{},{},{},{},{},{}"_fmt(enabled, values ? U"option_values" : U"option_rows",
                        static_cast<int>(button), input.deltaCursor(), expected, ok ? U"PASS" : U"FAIL"));
                }
            }
        }
        ConfigIni::SetInt(ConfigIni::Key::kLaserInputType, originalLaserType);
        original.Save();
        report.writeln(U"overall,,,,,{}"_fmt(passed ? U"PASS" : U"FAIL"));
    }
}
