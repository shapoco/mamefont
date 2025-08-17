#pragma once

#include "mamefont/mamefont_common.hpp"

namespace mamefont {

template <uint8_t Param_BYTE_OFFSET, uint8_t Param_POS>
struct BitFlag {
  static constexpr uint8_t BYTE_OFFSET = Param_BYTE_OFFSET;
  static constexpr uint8_t POS = Param_POS;
  static constexpr uint8_t MASK = (1 << POS);

  static MAMEFONT_INLINE bool read(uint8_t value) {
    return 0 != (value & MASK);
  }

  static MAMEFONT_INLINE uint8_t place(bool value) { return value ? MASK : 0; }

  static MAMEFONT_INLINE void write(uint8_t *ptr, bool value) {
    if (value) {
      *(ptr + BYTE_OFFSET) |= MASK;
    } else {
      *(ptr + BYTE_OFFSET) &= ~MASK;
    }
  }
};

template <typename TRaw, typename TPacked, uint8_t Param_BYTE_OFFSET,
          uint8_t Param_POS, uint8_t Param_WIDTH, TRaw Param_MIN = 0,
          TRaw Param_STEP = 1>
struct BitField {
  static constexpr uint8_t BYTE_OFFSET = Param_BYTE_OFFSET;
  static constexpr uint8_t POS = Param_POS;
  static constexpr uint8_t WIDTH = Param_WIDTH;
  static constexpr TRaw STEP = Param_STEP;
  static constexpr TRaw MIN = Param_MIN;
  static constexpr TRaw MAX = MIN + ((1 << WIDTH) - 1) * STEP;
  static constexpr TPacked MASK = ((1 << WIDTH) - 1) << POS;

  static MAMEFONT_INLINE bool inRange(TRaw value, bool *rangeError = nullptr,
                                      bool *stepError = nullptr) {
    bool rangeOk = MIN <= value && value <= MAX;
    bool stepOk = (value - MIN) % STEP == 0;
    if (rangeError) *rangeError = !rangeOk;
    if (stepError) *stepError = !stepOk;
    return rangeOk && stepOk;
  }

  static MAMEFONT_INLINE TRaw read(TPacked value) {
    if ((1 << POS) == STEP) {
      if (WIDTH != sizeof(TPacked) * 8) {
        value &= MASK;
      }
      return MIN + value;
    } else {
      if (POS != 0) {
        value >>= POS;
      }
      value &= ((1 << WIDTH) - 1);
      return MIN + STEP * value;
    }
  }

  static MAMEFONT_INLINE TPacked place(int value, const char *name = "Value") {
#ifdef MAMEFONT_EXCEPTIONS
    bool rangeError, stepError;
    inRange(value, &rangeError, &stepError);
    if (rangeError) {
      throw std::out_of_range(std::string(name) + " " + std::to_string(value) +
                              " out of range " + std::to_string(MIN) + ".." +
                              std::to_string(MAX));
    }
    if (stepError) {
      throw std::invalid_argument(
          std::string(name) + " " + std::to_string(value) +
          " is not a multiple of step " + std::to_string(STEP));
    }
#endif
    return ((value - MIN) / STEP) << POS;
  }

  static MAMEFONT_INLINE void write(uint8_t *ptr, int value,
                                    const char *name = "Value") {
    TPacked rawValue = place(value);
    ptr += BYTE_OFFSET;
    if (sizeof(TPacked) == 1) {
      *ptr = (*ptr & ~MASK) | rawValue;
    } else {
      uint8_t Lo = *ptr;
      uint8_t Hi = *(ptr + 1);
      Lo &= (~MASK) & 0xFF;
      Hi &= (~MASK) >> 8;
      Lo |= rawValue & 0xFF;
      Hi |= rawValue >> 8;
      *ptr = Lo;
      *(ptr + 1) = Hi;
    }
  }
};

}  // namespace mamefont
