#pragma once
#include <Siv3D.hpp>
namespace Vertical {
inline constexpr std::array<StringView,8> kDepthImageNames{U"arrow",U"techno",U"wave",U"smoke",U"snow",U"sakura",U"hidden",U"custom"};
inline constexpr std::array<int,8> kDepthImageDefaults{100,100,100,100,35,35,0,25};
inline Optional<int> ParseDepthPercent(StringView text) {
    if(text.empty() || text.size()>3)return none;
    int value=0;for(char32 c:text){if(c<U'0'||c>U'9')return none;value=value*10+static_cast<int>(c-U'0');}
    return value<=200 ? Optional<int>{value} : none;
}
}
