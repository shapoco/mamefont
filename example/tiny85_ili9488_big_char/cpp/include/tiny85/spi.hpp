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

  void sendDummyClocks(uint8_t count) {
    USIDR = 0;
    while (count--) {
      USICR = (1 << USIWM0) | (1 << USITC);
    }
  }

  SPI_ALWAYS_INLINE void writeSingleByte(uint8_t data) {
#if 1
    constexpr uint8_t clkH = (1 << USIWM0) | (1 << USITC);
    constexpr uint8_t clkL = (1 << USIWM0) | (1 << USITC) | (1 << USICLK);
    USIDR = data;
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
#else
    uint8_t byte = data;
    for (int8_t j = 8; j != 0; j--) {
      gpio::write(PORT_MOSI, byte & 0x80);
      byte <<= 1;
      gpio::write(PORT_SCK, 1);
      gpio::write(PORT_SCK, 0);
    }
#endif
  }

  void writeByte(uint8_t data, uint16_t repeat = 1) {
    while (repeat--) {
      writeSingleByte(data);
    }
  }

  void writeArray(const uint8_t *data, uint8_t size) {
    for (uint8_t i = size; i != 0; i--) {
      writeSingleByte(*(data++));
    }
  }
};
