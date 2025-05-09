#pragma once
#include "Math.h"

namespace Color
{
    constexpr Vector4 Red{ 1.0f, 0.0f, 0.0f, 1.0f };
    constexpr Vector4 Green{ 0.0f, 1.0f, 0.0f, 1.0f };
    constexpr Vector4 Blue{ 0.0f, 0.0f, 1.0f, 1.0f };
    constexpr Vector4 White{ 1.0f, 1.0f, 1.0f, 1.0f };
    constexpr Vector4 Black{ 0.0f, 0.0f, 0.0f, 1.0f };
    constexpr Vector4 Yellow{ 1.0f, 1.0f, 0.0f, 1.0f };
    constexpr Vector4 Cyan{ 0.0f, 1.0f, 1.0f, 1.0f };
    constexpr Vector4 Magenta{ 1.0f, 0.0f, 1.0f, 1.0f };
    
    //255.0f가 max인 rgb값 전달
    static constexpr Vector4 ColorRGB(float r, float g, float b, float a)
    {
        float rf = Math::Clamp(r, 0.0f, 255.0f);
        float gf = Math::Clamp(g, 0.0f, 255.0f);
        float bf = Math::Clamp(b, 0.0f, 255.0f);
        float af = Math::Clamp(a, 0.0f, 255.0f);

        return Vector4(rf, gf, bf, af) * (1.0f / 255.0f);
    }
    //HexCode = 0xRRGGBB
    static constexpr Vector4 ColorHex(uint32_t HexCode)
    {
        uint8_t r = (HexCode >> 16) & 0xFF;
        uint8_t g = (HexCode >> 8) & 0xFF;
        uint8_t b = (HexCode) & 0xFF;

        return ColorRGB(static_cast<float>(r),
                           static_cast<float>(g),
                           static_cast<float>(b),
                           255.0f);
    }
}