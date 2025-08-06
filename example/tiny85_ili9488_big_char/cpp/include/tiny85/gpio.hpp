#pragma once

#ifndef GPIO_ALWAYS_INLINE
#define GPIO_ALWAYS_INLINE inline __attribute__((always_inline))
#endif

#include <stdint.h>

#include <avr/io.h>
#include <util/delay.h>

static GPIO_ALWAYS_INLINE void delayMs(uint16_t ms) { _delay_ms(ms); }

namespace gpio {

static GPIO_ALWAYS_INLINE void setDirMulti(uint8_t portMask, bool output) {
  if (output) {
    DDRB |= portMask;
  } else {
    DDRB &= ~portMask;
  }
}

static GPIO_ALWAYS_INLINE void writeMulti(uint8_t portMask, uint8_t value) {
  if (value) {
    PORTB |= portMask;
  } else {
    PORTB &= ~portMask;
  }
}

static GPIO_ALWAYS_INLINE void write(uint8_t port, uint8_t value) {
  writeMulti(1 << port, value);
}

}  // namespace gpio
