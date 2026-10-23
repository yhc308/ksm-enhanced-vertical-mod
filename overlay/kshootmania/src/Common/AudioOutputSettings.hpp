#pragma once
#include <Siv3D.hpp>
#include "Common/FsUtils.hpp"
#include <filesystem>
#include <fstream>

namespace AudioOutputSettings {
inline bool NeedsRestart(StringView selected, StringView active, StringView genericASIO) {
    // Generic ASIO selects the first installed driver, which may differ from the
    // named ASIO driver currently playing. Compare the actual selections.
    return (selected == U"asio" && !genericASIO.isEmpty() ? genericASIO : selected) != active;
}
inline void Log(const String& message) {
    const auto utf8 = FsUtils::GetResourcePath(U"log/audio-output-switch.log").toUTF8();
    std::ofstream stream{std::filesystem::path{
        std::u8string_view{reinterpret_cast<const char8_t*>(utf8.data()), utf8.size()}}, std::ios::app};
    stream << Time::GetMillisec() << " " << message.toUTF8() << std::endl;
}
}
