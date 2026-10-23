#pragma once
#include "VerticalModConfig.hpp"
#include "Common/FsUtils.hpp"
namespace Vertical
{
    inline FilePath UIPath(FilePathView legacyPath)
    {
        if (ModConfig::Load().enabled)
        {
            const auto path = FsUtils::GetResourcePath(U"mods/vertical/ui/" + FileSystem::FileName(legacyPath));
            if (FileSystem::Exists(path)) return path;
        }
        const auto enhanced = FsUtils::GetResourcePath(U"mods/enhanced/ui/" + FileSystem::FileName(legacyPath));
        if (FileSystem::Exists(enhanced)) return enhanced;
        return FsUtils::GetResourcePath(legacyPath);
    }
}
