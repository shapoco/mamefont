#pragma once

#ifndef SPI_ALWAYS_INLINE
#define SPI_ALWAYS_INLINE inline __attribute__((always_inline))
#endif

#include <stdint.h>

#include <tiny85/gpio.hpp>

static constexpr uint8_t PORT_MOSI = 0;
static constexpr uint8_t PORT_SCK = 2;

class SPI {
 public:
  SPI_ALWAYS_INLINE void init() {
    gpio::setDirMulti((1 << PORT_MOSI) | (1 << PORT_SCK), true);
    gpio::writeMulti((1 << PORT_MOSI) | (1 << PORT_SCK), 0);
  }

  void writeBlocking(const uint8_t *data, uint8_t size) {
    for (int8_t i = size; i != 0; i--) {
      uint8_t byte = *(data++);
      for (int8_t j = 8; j != 0; j--) {
        gpio::write(PORT_MOSI, byte & 0x80);
        byte <<= 1;
        gpio::write(PORT_SCK, 1);
        gpio::write(PORT_SCK, 0);
      }
    }
  }

  SPI_ALWAYS_INLINE void writeBlocking(uint8_t data) { writeBlocking(&data, 1); }
};
