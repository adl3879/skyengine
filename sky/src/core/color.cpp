#include "color.h"

LinearColor LinearColor::fromRGB(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a)
{
    return sky::rgbToLinear(r, g, b, a);
}