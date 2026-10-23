#pragma once
#include "VerticalLayout.hpp"
#include "Common/FsUtils.hpp"
#include "Graphics/GameRenderContext.hpp"

namespace Vertical
{
    // The generator and runtime share this packaged layout, rather than
    // duplicating coordinates in PowerShell and renderer code.
    inline const JSON& HUDDefinition()
    {
        static const JSON definition = JSON::Load(FsUtils::GetResourcePath(U"mods/vertical/ui/ingame_layout.json"));
        if (!definition) throw Error{U"Missing vertical ingame layout"};
        return definition;
    }

    inline RectF HUDRect(StringView name)
    {
        const auto data = HUDDefinition()[U"hud"][name];
        const auto viewport = Viewport::Fit(GameRender::Width(), GameRender::Height());
        return {viewport.x(data[0].get<double>()), viewport.y(data[1].get<double>()),
            data[2].get<double>() * viewport.scale, data[3].get<double>() * viewport.scale};
    }
}
