#pragma once

#include <stdint.h>

#include "i2c.hpp"
#include "mamefont/mamefont.hpp"

#ifndef ALWAYS_INLINE
#define ALWAYS_INLINE __attribute__((always_inline))
#endif

#define SSD1306_X_CLIPPING_ENABLE (1)
#define SSD1306_Y_CLIPPING_ENABLE (0)

#define SSD1306_ENABLE_ROTATE (0)

namespace ssd1306 {

#if SSD1306_X_CLIPPING_ENABLE
using x_coord_t = int16_t;
#else
using x_coord_t = uint8_t;
#endif
#if SSD1306_Y_CLIPPING_ENABLE
using y_coord_t = int16_t;
#else
using y_coord_t = int8_t;
#endif

using i2c_buff_size_t = uint8_t;

enum Command : uint8_t {
  SET_MEM_MODE = 0x20,
  SET_COL_ADDR = 0x21,
  SET_PAGE_ADDR = 0x22,
  SET_HORIZ_SCROLL = 0x26,
  SET_SCROLL = 0x2E,
  SET_DISP_START_LINE = 0x40,
  SET_CONTRAST = 0x81,
  SET_CHARGE_PUMP = 0x8D,
  SET_SEG_REMAP = 0xA0,
  SET_ENTIRE_ON = 0xA4,
  SET_ALL_ON = 0xA5,
  SET_NORM_DISP = 0xA6,
  SET_INV_DISP = 0xA7,
  SET_MUX_RATIO = 0xA8,
  SET_DISP = 0xAE,
  SET_COM_OUT_DIR = 0xC0,
  SET_COM_OUT_DIR_FLIP = 0xC0,
  SET_DISP_OFFSET = 0xD3,
  SET_DISP_CLK_DIV = 0xD5,
  SET_PRECHARGE = 0xD9,
  SET_COM_PIN_CFG = 0xDA,
  SET_VCOM_DESEL = 0xDB,
};

template <uint8_t WIDTH = 128, uint8_t HEIGHT = 64,
          uint8_t DEV_ADDR = (0x3c << 1)>
class Lcd {
 public:
  static constexpr uint8_t NUM_ROWS = (HEIGHT + 7) / 8;
  I2C &i2c;

  static constexpr uint8_t cmds[] = {
      Command::SET_DISP,
      Command::SET_MUX_RATIO,
      HEIGHT - 1,
      Command::SET_DISP_OFFSET,
      0,
      Command::SET_DISP_START_LINE,
#if SSD1306_ENABLE_ROTATE
      Command::SET_SEG_REMAP | 0x01,
      Command::SET_COM_OUT_DIR | 0x08,
#else
      Command::SET_SEG_REMAP,
      Command::SET_COM_OUT_DIR,
#endif
      Command::SET_COM_PIN_CFG,
      (WIDTH == 128 && HEIGHT == 32)   ? 0x02
      : (WIDTH == 128 && HEIGHT == 64) ? 0x12
                                       : 0x02,
      Command::SET_CONTRAST,
      0xFF,
      Command::SET_ENTIRE_ON,
      Command::SET_NORM_DISP,
      Command::SET_DISP_CLK_DIV,
      0x80,
      Command::SET_CHARGE_PUMP,
      0x14,
  };

  Lcd(I2C &i2c) : i2c(i2c) {}

  void begin() {
    writeCommand(cmds, sizeof(cmds));

    // for (uint8_t page = 0; page < 8; page++) {
    //   for (uint8_t x = 0; x < W; x++) {
    //     buff[x] = x + page;
    //   }
    //   writeCommand(Command::SET_COL_ADDR, 0, W - 1);
    //   writeCommand(Command::SET_PAGE_ADDR, page, page);
    //   writeDataBlocking(buff, W);
    // }

    writeCommand(Command::SET_DISP | 1);
  }

  void writeCommand(const void *src, uint8_t size) {
    const uint8_t *rptr = (const uint8_t *)src;
    i2c.start(DEV_ADDR);
    for (uint8_t i = 0; i < size; i++) {
      i2c.write(0x80);
      i2c.write(rptr[i]);
    }
    i2c.end();
  }

  void writeCommand(uint8_t cmd) { writeCommand(&cmd, 1); }

  void writeCommand(uint8_t cmd, uint8_t param0) {
    const uint8_t buf[] = {cmd, param0};
    writeCommand(buf, sizeof(buf));
  }

  void writeCommand(uint8_t cmd, uint8_t param0, uint8_t param1) {
    const uint8_t buf[] = {cmd, param0, param1};
    writeCommand(buf, sizeof(buf));
  }

  void writeDataBlocking(const void *src, i2c_buff_size_t size, bool increment) {
#if 1
    const uint8_t *rptr = (const uint8_t *)src;
    i2c.start(DEV_ADDR);
    i2c.write(0x40);
    while (size-- > 0) {
      i2c.write(increment ? *(rptr++) : *rptr);
    }
    i2c.end();
#else
    constexpr uint8_t MAX_CHUNK_SIZE = 31;
    const uint8_t *rptr = (const uint8_t *)src;
    while (size > 0) {
      uint8_t chunkSize = size < MAX_CHUNK_SIZE ? size : MAX_CHUNK_SIZE;
      i2c.start(DEV_ADDR);
      i2c.write(0x40);
      for (uint8_t i = 0; i < chunkSize; i++) {
        i2c.write(increment ? *(rptr++) : *rptr);
      }
      i2c.end();
      size -= chunkSize;
    }
#endif
  }

  int16_t clipCoord(int16_t *x, uint8_t *w, uint8_t max) {
    if (*x < 0) {
      if (*x + *w <= 0) {
        *w = 0;
        return 0;
      }
      *w += *x;
      int16_t ret = -*x;
      *x = 0;
      return ret;
    }
    if (*x + *w > max) {
      if (*x >= max) {
        *w = 0;
        return 0;
      }
      *w = max - *x;
    }
    return 0;
  }

  int16_t clipRect(x_coord_t *x, y_coord_t *row, uint8_t *w, uint8_t *hRow,
                   uint8_t stride) {
    int16_t ret = 0;
#if SSD1306_X_CLIPPING_ENABLE
    ret += clipCoord(x, w, WIDTH);
#endif
#if SSD1306_Y_CLIPPING_ENABLE
    ret += clipCoord(row, hRow, NUM_ROWS) * stride;
#endif
    return ret;
  }

  void drawImage(const uint8_t *src, x_coord_t x, y_coord_t row, uint8_t stride,
                 uint8_t w, uint8_t hRow) {
#if SSD1306_X_CLIPPING_ENABLE || SSD1306_Y_CLIPPING_ENABLE
    src += clipRect(&x, &row, &w, &hRow, stride);
    if (w == 0 || hRow == 0) return;
#endif
    writeCommand(Command::SET_COL_ADDR, x, x + w - 1);
    while (hRow-- > 0) {
      writeCommand(Command::SET_PAGE_ADDR, row, row);
      writeDataBlocking(src, w, true);
      src += stride;
      row++;
    }
  }

  void fillRect(x_coord_t x, y_coord_t row, uint8_t w, uint8_t hRow,
                uint8_t color) {
#if SSD1306_X_CLIPPING_ENABLE || SSD1306_Y_CLIPPING_ENABLE
    clipRect(&x, &row, &w, &hRow, 0);
    if (w == 0 || hRow == 0) return;
#endif
    writeCommand(Command::SET_COL_ADDR, x, x + w - 1);
    while (hRow-- > 0) {
      writeCommand(Command::SET_PAGE_ADDR, row, row);
      writeDataBlocking(&color, w, false);
      row++;
    }
  }
};

}  // namespace ssd1306