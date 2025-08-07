#pragma once

// https://www.waveshare.com/wiki/Pico-ResTouch-LCD-3.5

#ifndef ILI9488_ALWAYS_INLINE
#define ILI9488_ALWAYS_INLINE inline __attribute__((always_inline))
#endif

// #define ILI9488_RGB111
// #define ILI9488_RGB565

#include <stdint.h>

#include <avr/pgmspace.h>

#include "tiny85/gpio.hpp"
#include "tiny85/spi.hpp"

namespace ili9488 {

using coord_t = int16_t;
using offset_t = int16_t;

#ifdef ILI9488_RGB111
using color_t = uint8_t;
enum Color : color_t {
  BLACK = 0x00,
  BLUE = 0x01,
  GREEN = 0x02,
  CYAN = 0x03,
  RED = 0x04,
  MAGENTA = 0x05,
  YELLOW = 0x06,
  WHITE = 0x07,
};
#else
using color_t = uint16_t;
enum Color : color_t {
  BLACK = 0x0000,
  BLUE = 0x001F,
  GREEN = 0x07E0,
  CYAN = 0x07FF,
  RED = 0xF800,
  MAGENTA = 0xF81F,
  YELLOW = 0xFFE0,
  WHITE = 0xFFFF,
};
#endif

enum class Command : uint8_t {
  NOP = 0x00,
  SOFTWARE_RESET = 0x01,
  READ_DISP_ID = 0x04,
  READ_ERROR_DSI = 0x05,
  READ_DISP_STATUS = 0x09,
  READ_DISP_POWER_MODE = 0x0A,
  READ_DISP_MADCTRL = 0x0B,
  READ_DISP_PIXEL_FORMAT = 0x0C,
  READ_DISP_IMAGE_MODE = 0x0D,
  READ_DISP_SIGNAL_MODE = 0x0E,
  READ_DISP_SELF_DIAGNOSTIC = 0x0F,
  ENTER_SLEEP_MODE = 0x10,
  SLEEP_OUT = 0x11,
  PARTIAL_MODE_ON = 0x12,
  NORMAL_DISP_MODE_ON = 0x13,
  DISP_INVERSION_OFF = 0x20,
  DISP_INVERSION_ON = 0x21,
  PIXEL_OFF = 0x22,
  PIXEL_ON = 0x23,
  DISPLAY_OFF = 0x28,
  DISPLAY_ON = 0x29,
  COLUMN_ADDRESS_SET = 0x2A,
  PAGE_ADDRESS_SET = 0x2B,
  MEMORY_WRITE = 0x2C,
  MEMORY_READ = 0x2E,
  PARTIAL_AREA = 0x30,
  VERT_SCROLL_DEFINITION = 0x33,
  TEARING_EFFECT_LINE_OFF = 0x34,
  TEARING_EFFECT_LINE_ON = 0x35,
  MEMORY_ACCESS_CONTROL = 0x36,
  VERT_SCROLL_START_ADDRESS = 0x37,
  IDLE_MODE_OFF = 0x38,
  IDLE_MODE_ON = 0x39,
  INTERFACE_PIXEL_FORMAT = 0x3A,
  MEMORY_WRITE_CONTINUE = 0x3C,
  MEMORY_READ_CONTINUE = 0x3E,
  SET_TEAR_SCANLINE = 0x44,
  GET_SCANLINE = 0x45,
  WRITE_DISPLAY_BRIGHTNESS = 0x51,
  READ_DISPLAY_BRIGHTNESS = 0x52,
  WRITE_CTRL_DISPLAY = 0x53,
  READ_CTRL_DISPLAY = 0x54,
  WRITE_CONTENT_ADAPT_BRIGHTNESS = 0x55,
  READ_CONTENT_ADAPT_BRIGHTNESS = 0x56,
  WRITE_MIN_CAB_LEVEL = 0x5E,
  READ_MIN_CAB_LEVEL = 0x5F,
  READ_ABC_SELF_DIAG_RES = 0x68,
  READ_ID1 = 0xDA,
  READ_ID2 = 0xDB,
  READ_ID3 = 0xDC,
  INTERFACE_MODE_CONTROL = 0xB0,
  FRAME_RATE_CONTROL_NORMAL = 0xB1,
  FRAME_RATE_CONTROL_IDLE_8COLOR = 0xB2,
  FRAME_RATE_CONTROL_PARTIAL = 0xB3,
  DISPLAY_INVERSION_CONTROL = 0xB4,
  BLANKING_PORCH_CONTROL = 0xB5,
  DISPLAY_FUNCTION_CONTROL = 0xB6,
  ENTRY_MODE_SET = 0xB7,
  BACKLIGHT_CONTROL_1 = 0xB9,
  BACKLIGHT_CONTROL_2 = 0xBA,
  HS_LANES_CONTROL = 0xBE,
  POWER_CONTROL_1 = 0xC0,
  POWER_CONTROL_2 = 0xC1,
  POWER_CONTROL_NORMAL_3 = 0xC2,
  POWER_CONTROL_IDEL_4 = 0xC3,
  POWER_CONTROL_PARTIAL_5 = 0xC4,
  VCOM_CONTROL_1 = 0xC5,
  CABC_CONTROL_1 = 0xC6,
  CABC_CONTROL_2 = 0xC8,
  CABC_CONTROL_3 = 0xC9,
  CABC_CONTROL_4 = 0xCA,
  CABC_CONTROL_5 = 0xCB,
  CABC_CONTROL_6 = 0xCC,
  CABC_CONTROL_7 = 0xCD,
  CABC_CONTROL_8 = 0xCE,
  CABC_CONTROL_9 = 0xCF,
  NVMEM_WRITE = 0xD0,
  NVMEM_PROTECTION_KEY = 0xD1,
  NVMEM_STATUS_READ = 0xD2,
  READ_ID4 = 0xD3,
  ADJUST_CONTROL_1 = 0xD7,
  READ_ID_VERSION = 0xD8,
  POSITIVE_GAMMA_CORRECTION = 0xE0,
  NEGATIVE_GAMMA_CORRECTION = 0xE1,
  DIGITAL_GAMMA_CONTROL_1 = 0xE2,
  DIGITAL_GAMMA_CONTROL_2 = 0xE3,
  SET_IMAGE_FUNCTION = 0xE9,
  ADJUST_CONTROL_2 = 0xF2,
  ADJUST_CONTROL_3 = 0xF7,
  ADJUST_CONTROL_4 = 0xF8,
  ADJUST_CONTROL_5 = 0xF9,
  SPI_READ_COMMAND_SETTING = 0xFB,
  ADJUST_CONTROL_6 = 0xFC,
  ADJUST_CONTROL_7 = 0xFF,
};

static ILI9488_ALWAYS_INLINE void clipCoord(coord_t *x, coord_t *w,
                                            coord_t max) {
  if (*x < 0) {
    if (*x + *w <= 0) {
      *w = 0;
      return;
    }
    *w += *x;
    *x = 0;
  }
  if (*x + *w > max) {
    if (*x >= max) {
      *w = 0;
      return;
    }
    *w = max - *x;
  }
}

template <int8_t TParam_PORT_CS, int8_t TParam_PORT_DC, int8_t TParam_PORT_RST,
          uint16_t TParam_WIDTH = 480, uint16_t TParam_HEIGHT = 320>
class Display {
 public:
  static constexpr coord_t WIDTH = TParam_WIDTH;
  static constexpr coord_t HEIGHT = TParam_HEIGHT;
  static constexpr int8_t PORT_CS = TParam_PORT_CS;
  static constexpr int8_t PORT_DC = TParam_PORT_DC;
  static constexpr int8_t PORT_RST = TParam_PORT_RST;

  SPI &spi;

  Display(SPI &spi) : spi(spi) {}

  static constexpr uint8_t SPI_BUFF_SIZE = 16;
#ifdef ILI9488_RGB111
  static constexpr uint8_t NUM_PIXELS_IN_SPI_BUFF = SPI_BUFF_SIZE * 2;
#elif defined(ILI9488_RGB565)
  static constexpr uint8_t NUM_PIXELS_IN_SPI_BUFF = SPI_BUFF_SIZE / 2;
#else
  static constexpr uint8_t NUM_PIXELS_IN_SPI_BUFF = SPI_BUFF_SIZE / 3;
#endif
  uint8_t spiBuff[SPI_BUFF_SIZE];

  void init() {
    gpio::writeMulti((1 << PORT_CS) | (1 << PORT_DC), 1);
    if (PORT_RST >= 0) {
      gpio::setDirMulti((1 << PORT_CS) | (1 << PORT_DC) | (1 << PORT_RST),
                        true);
      gpio::write(PORT_RST, 0);
      delayMs(5);
      gpio::write(PORT_RST, 1);
      delayMs(5);
    } else {
      gpio::setDirMulti((1 << PORT_CS) | (1 << PORT_DC), true);
      delayMs(5);
    }

#if 1
    writeCommand(Command::SOFTWARE_RESET);
    delayMs(200);

    writeCommand(Command::SLEEP_OUT);
    delayMs(200);

#ifdef ILI9488_RGB111
    // RGB111 for SPI
    writeCommand(Command::IDLE_MODE_ON);
    writeCommand(Command::INTERFACE_PIXEL_FORMAT, 0x51);
#elif defined(ILI9488_RGB565)
    // RGB565 for parallel
    writeCommand(Command::INTERFACE_PIXEL_FORMAT, 0x55);
    // writeCommand(Command::PARTIAL_MODE_ON);
#else
    // RGB666 for SPI
    writeCommand(Command::INTERFACE_PIXEL_FORMAT, 0x56);
    // writeCommand(Command::PARTIAL_MODE_ON);
#endif

#if 0
    writeCommand(Command::MEMORY_ACCESS_CONTROL, 0x48);
#else
    // writeCommand(Command::MEMORY_ACCESS_CONTROL, 0xE8);
    writeCommand(Command::MEMORY_ACCESS_CONTROL, 0x28);
#endif

    writeCommand(Command::DISPLAY_ON);
    delayMs(25);
#endif
  }

  void clear(color_t color) { fillRect(0, 0, WIDTH, HEIGHT, color); }

  void setPixel(coord_t x, coord_t y, color_t color) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
    setAddress(x, y, 1, 1);
    commandStart(Command::MEMORY_WRITE);
#ifdef ILI9488_RGB111
    spi.writeByte(color);
#elif defined(ILI9488_RGB565)
    spiBuff[0] = color >> 8;
    spiBuff[1] = color & 0xFF;
    spi.writeArray(spiBuff, 2);
#else
    spiBuff[0] = (color & 0xF100) >> 8;
    spiBuff[1] = (color & 0x07E0) >> 3;
    spiBuff[2] = (color & 0x001F) << 3;
    spi.writeArray(spiBuff, 3);
#endif
    commandEnd();
  }

  void fillRect(coord_t x, coord_t y, coord_t w, coord_t h, color_t color) {
    clipRect(&x, &y, &w, &h);

    if (w <= 0 || h <= 0) return;

#ifdef ILI9488_RGB111
    color &= 0x7;
    color |= color << 3;
#elif defined(ILI9488_RGB565)
    for (int8_t i = 0; i < SPI_BUFF_SIZE; i += 2) {
      spiBuff[i] = color >> 8;
      spiBuff[i + 1] = color & 0xFF;
    }
#else
    spiBuff[0] = (color & 0xF100) >> 8;
    spiBuff[1] = (color & 0x07E0) >> 3;
    spiBuff[2] = (color & 0x001F) << 3;
    for (int8_t i = 3; i < NUM_PIXELS_IN_SPI_BUFF * 3; i++) {
      spiBuff[i] = spiBuff[i - 3];
    }
#endif

    setAddress(x, y, w, h);
    commandStart(Command::MEMORY_WRITE);
    coord_t pixelsToSend = 0;
    for (coord_t y = h; y != 0; y--) {
      pixelsToSend += w;
      while (pixelsToSend >= NUM_PIXELS_IN_SPI_BUFF) {
#ifdef ILI9488_RGB111
        spi.writeByte(color, NUM_PIXELS_IN_SPI_BUFF / 2);
#elif defined(ILI9488_RGB565)
        spi.writeArray(spiBuff, NUM_PIXELS_IN_SPI_BUFF * 2);
#else
        spi.writeArray(spiBuff, NUM_PIXELS_IN_SPI_BUFF * 3);
#endif
        pixelsToSend -= NUM_PIXELS_IN_SPI_BUFF;
      }
    }
    if (pixelsToSend > 0) {
#ifdef ILI9488_RGB111
      spi.writeByte(color, (pixelsToSend + 1) / 2);
#elif defined(ILI9488_RGB565)
      spi.writeArray(spiBuff, pixelsToSend * 2);
#else
      spi.writeArray(spiBuff, pixelsToSend * 3);
#endif
    }
    commandEnd();
  }

  void drawImage1bpp(const uint8_t *src, uint16_t stride, coord_t x, coord_t y,
                     coord_t w, coord_t h, color_t fgColor = 0xFFFF,
                     color_t bgColor = 0x0000) {
    coord_t dx = x;
    coord_t dy = y;
    clipRect(&dx, &dy, &w, &h);
    if (w <= 0 || h <= 0) return;
    src += (dx - x) / 8;
    src += (dy - y) * stride;

    const uint8_t *linePtr = src;

    setAddress(x, y, w, h);
    commandStart(Command::MEMORY_WRITE);
    uint8_t buffSize = 0;
    for (coord_t iy = h; iy != 0; iy--) {
      const uint8_t *pixelPtr = linePtr;
      uint8_t byte;
      for (coord_t ix = 0; ix < w; ix++) {
        if ((ix & 7) == 0) {
          byte = *pixelPtr++;
        }
        color_t color = (byte & 1) ? fgColor : bgColor;
        byte >>= 1;
#ifdef ILI9488_RGB111
        if ((buffSize & 1) == 0) {
          spiBuff[buffSize / 2] = color << 3;
        } else {
          spiBuff[buffSize / 2] |= color;
        }
        buffSize++;
        buffSize %= SPI_BUFF_SIZE * 2;
        if (buffSize == 0) {
          spi.writeArray(spiBuff, SPI_BUFF_SIZE);
        }
#elif defined(ILI9488_RGB565)
        spiBuff[buffSize++] = color >> 8;
        spiBuff[buffSize++] = color & 0xFF;
        buffSize %= SPI_BUFF_SIZE;
        if (buffSize == 0) {
          spi.writeArray(spiBuff, SPI_BUFF_SIZE);
        }
#else
        spiBuff[buffSize++] = (color & 0xF100) >> 8;
        spiBuff[buffSize++] = (color & 0x07E0) >> 3;
        spiBuff[buffSize++] = (color & 0x001F) << 3;
        if (buffSize >= NUM_PIXELS_IN_SPI_BUFF * 3) {
          spi.writeArray(spiBuff, NUM_PIXELS_IN_SPI_BUFF * 3);
          buffSize = 0;
        }
#endif
      }
      linePtr += stride;
    }
    if (buffSize > 0) {
#ifdef ILI9488_RGB111
      spi.writeArray(spiBuff, (buffSize + 1) / 2);
#else
      spi.writeArray(spiBuff, buffSize);
#endif
    }
    commandEnd();
  }

  void setAddress(coord_t x, coord_t y, coord_t w, coord_t h) {
    coord_t x_end = x + w - 1;
    coord_t y_end = y + h - 1;
    uint8_t tmp[4];
    tmp[0] = x >> 8;
    tmp[1] = x & 0xFF;
    tmp[2] = x_end >> 8;
    tmp[3] = x_end & 0xFF;
    writeCommand(Command::COLUMN_ADDRESS_SET, tmp, 4);
    tmp[0] = y >> 8;
    tmp[1] = y & 0xFF;
    tmp[2] = y_end >> 8;
    tmp[3] = y_end & 0xFF;
    writeCommand(Command::PAGE_ADDRESS_SET, tmp, 4);
  }

  void clipRect(coord_t *x, coord_t *y, coord_t *w, coord_t *h) {
    clipCoord(x, w, WIDTH);
    clipCoord(y, h, HEIGHT);
  }

  void writeCommand(Command cmd, const uint8_t *data, uint8_t size) {
    commandStart(cmd);
#ifdef ILI9488_RGB565
    uint8_t tmp[2];
    tmp[0] = 0x00;
    for (int8_t i = size; i != 0; i--) {
      tmp[1] = *(data++);
      spi.writeArray(tmp, 2);
    }
#else
    spi.writeArray(data, size);
#endif
    commandEnd();
  }
  ILI9488_ALWAYS_INLINE void writeCommand(Command cmd) {
    writeCommand(cmd, nullptr, 0);
  }
  ILI9488_ALWAYS_INLINE void writeCommand(Command cmd, uint8_t data) {
    writeCommand(cmd, &data, 1);
  }

  void commandStart(Command cmd) {
    gpio::writeMulti((1 << PORT_CS) | (1 << PORT_DC), 0);
#ifdef ILI9488_RGB565
    uint8_t tmp[] = {0x00, static_cast<uint8_t>(cmd)};
    spi.writeArray(tmp, 2);
#else
    spi.writeByte(static_cast<uint8_t>(cmd));
#endif
    gpio::write(PORT_DC, 1);
  }

  ILI9488_ALWAYS_INLINE void commandEnd() {
    gpio::writeMulti((1 << PORT_CS) | (1 << PORT_DC), 1);
  }
};

}  // namespace ili9488