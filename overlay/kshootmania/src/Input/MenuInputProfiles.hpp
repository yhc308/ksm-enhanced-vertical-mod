#pragma once
#include "Cursor/CursorInput.hpp"

namespace MenuInputProfiles
{
    // Original OPTION: right laser moves rows; left laser changes values.
    inline CursorInput::CreateInfo OptionRows()
    {
        return {.type = CursorInput::Type::Vertical,
            .buttonFlags = CursorButtonFlags::kArrowOrLaser | CursorButtonFlags::kMouseWheel,
            .buttonIntervalSec = 0.1, .buttonIntervalSecFirst = 0.5};
    }
    inline CursorInput::CreateInfo OptionValues()
    {
        return {.type = CursorInput::Type::Horizontal,
            .buttonFlags = CursorButtonFlags::kArrowOrLaser,
            .buttonIntervalSec = 0.1, .buttonIntervalSecFirst = 0.5};
    }
}
