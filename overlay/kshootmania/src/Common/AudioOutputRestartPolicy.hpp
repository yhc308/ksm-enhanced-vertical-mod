#pragma once
#include "ksmaudio/ksmaudio.hpp"
#include <algorithm>
#include <cctype>

namespace AudioOutputRestart {
// Reported FiiO failure: BASS_Init(default/WASAPI) never returns after ASIO release.
// Avoid that same-process reopen; do not change working hot-switch paths.
inline bool Required(const ksmaudio::AudioOutputStatus& current, ksmaudio::AudioBackend requested) {
    if (current.backend != ksmaudio::AudioBackend::ASIO || requested != ksmaudio::AudioBackend::Default) return false;
    auto name = current.driver;
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return name.find("fiio") != std::string::npos;
}
}
