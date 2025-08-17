#pragma once

#include "mamefont/bit_field.hpp"
#include "mamefont/mamefont_common.hpp"

namespace mamefont {

static MAMEFONT_INLINE uint8_t getRightMaskU8(uint8_t width) {
  switch (width) {
    case 0: return 0x00;
    case 1: return 0x01;
    case 2: return 0x03;
    case 3: return 0x07;
    case 4: return 0x0F;
    case 5: return 0x1F;
    case 6: return 0x3F;
    case 7: return 0x7F;
    default: return 0xFF;
  }
}

static MAMEFONT_INLINE uint16_t getRightMaskU16(uint8_t width) {
  switch (width) {
    case 0: return 0x0000;
    case 1: return 0x0001;
    case 2: return 0x0003;
    case 3: return 0x0007;
    case 4: return 0x000F;
    case 5: return 0x001F;
    case 6: return 0x003F;
    case 7: return 0x007F;
    case 8: return 0x00FF;
    case 9: return 0x01FF;
    case 10: return 0x03FF;
    case 11: return 0x07FF;
    case 12: return 0x0FFF;
    case 13: return 0x1FFF;
    case 14: return 0x3FFF;
    case 15: return 0x7FFF;
    default: return 0xFFFF;
  }
}

static MAMEFONT_INLINE uint8_t reversePixels(uint8_t b, PixelFormat bpp) {
  if (bpp == PixelFormat::BW_1BIT) {
    b = ((b & 0x55) << 1) | ((b & 0xaa) >> 1);
  }
  b = ((b & 0x33) << 2) | ((b & 0xcc) >> 2);
  b = (b << 4) | (b >> 4);
  return b;
}

}  // namespace mamefont