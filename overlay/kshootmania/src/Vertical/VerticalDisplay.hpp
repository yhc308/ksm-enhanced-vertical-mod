#pragma once
#include "VerticalModConfig.hpp"
namespace Vertical
{
    bool BeginDisplayChange(const ModConfig& next);
    bool IsDisplayChangePending();
    void UpdateDisplayConfirmation();
    void DrawDisplayConfirmation();
    int32 DisplayConfirmationSelection();
    void RestorePendingDisplayChange();
    void ApplyConfiguredDisplay();
    void SetDisplayConfigPersistence(bool enabled);
}
