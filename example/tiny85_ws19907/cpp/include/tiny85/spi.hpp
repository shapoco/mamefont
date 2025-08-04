#pragma once

#ifndef SPI_ALWAYS_INLINE
#define SPI_ALWAYS_INLINE inline __attribute__((always_inline))
#endif

#include <stdint.h>

#include <tiny85/gpio.hpp>

static constexpr uint8_t PORT_MOSI = 1;
static constexpr uint8_t PORT_SCK = 2;

class SPI {
 public:
  SPI_ALWAYS_INLINE void init() {
    gpio::setDirMulti((1 << PORT_MOSI) | (1 << PORT_SCK), true);
    gpio::writeMulti((1 << PORT_MOSI) | (1 << PORT_SCK), 0);
  }

  void writeBlocking(const uint8_t *data, uint8_t size) {
#if 1
    constexpr uint8_t clkH = (1 << USIWM0) | (1 << USITC);
    constexpr uint8_t clkL = (1 << USIWM0) | (1 << USITC) | (1 << USICLK);
    for (uint8_t i = size; i != 0; i--) {
      USIDR = *(data++);
      USICR = clkH;
      USICR = clkL;
      USICR = clkH;
      USICR = clkL;
      USICR = clkH;
      USICR = clkL;
      USICR = clkH;
      USICR = clkL;
      USICR = clkH;
      USICR = clkL;
      USICR = clkH;
      USICR = clkL;
      USICR = clkH;
      USICR = clkL;
      USICR = clkH;
      USICR = clkL;
    }
#else
    for (uint8_t i = size; i != 0; i--) {
      uint8_t byte = *(data++);
      for (int8_t j = 8; j != 0; j--) {
        gpio::write(PORT_MOSI, byte & 0x80);
        byte <<= 1;
        gpio::write(PORT_SCK, 1);
        gpio::write(PORT_SCK, 0);
      }
    }
#endif
  }

  SPI_ALWAYS_INLINE void writeBlocking(uint8_t data) {
    writeBlocking(&data, 1);
  }
};
