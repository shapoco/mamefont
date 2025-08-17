#pragma once

#include <stdint.h>

#ifdef MAMEFONT_DEBUG
#include <stdio.h>
#include <string.h>
#endif

#ifdef MAMEFONT_EXCEPTIONS
#include <stdexcept>
#endif

#ifdef MAMEFONT_USE_PROGMEM
#include <avr/pgmspace.h>
#endif

#define MAMEFONT_INLINE inline __attribute__((always_inline))
#define MAMEFONT_NOINLINE __attribute__((noinline))

namespace mamefont {

#ifdef MAMEFONT_USE_PROGMEM
static MAMEFONT_INLINE uint8_t readBlobU8(const uint8_t *ptr) {
  return pgm_read_byte(ptr);
}
#else
static MAMEFONT_INLINE uint8_t readBlobU8(const uint8_t *ptr) { return *ptr; }
#endif

static MAMEFONT_INLINE uint16_t readBlobU16(const uint8_t *ptr) {
  uint8_t lo = readBlobU8(ptr);
  uint8_t hi = readBlobU8(ptr + 1);
  return (static_cast<uint16_t>(hi) << 8) | lo;
}

static constexpr uint16_t DUMMY_ENTRY_POINT = 0xffff;
static constexpr uint8_t MAX_FRAGMENT_TABLE_SIZE = 64;

enum class Status : int8_t {
  SUCCESS = 0,

  // Generic Errors:
  CHAR_CODE_OUT_OF_RANGE = 1,
  GLYPH_NOT_DEFINED = 2,
  COORD_OUT_OF_RANGE = 3,

  // Critical Errors:
  NULL_POINTER = -1,
  UNKNOWN_OPCODE = -2,
  ABORTED_BY_ABO = -3,
  BUFFER_OVERRUN = -4,
};

static MAMEFONT_INLINE const char *statusToString(Status status) {
  switch (status) {
    case Status::SUCCESS: return "Success";
    case Status::CHAR_CODE_OUT_OF_RANGE: return "Character code out of range";
    case Status::GLYPH_NOT_DEFINED: return "Glyph not defined";
    case Status::COORD_OUT_OF_RANGE: return "Coordinate out of range";
    case Status::NULL_POINTER: return "Null pointer";
    case Status::UNKNOWN_OPCODE: return "Unknown opcode";
    case Status::ABORTED_BY_ABO: return "Aborted by ABO instruction";
    case Status::BUFFER_OVERRUN: return "Buffer Overrun";
    default: return "Unknown status";
  }
}

#ifdef MAMEFONT_EXCEPTIONS

class MameFontException : public std::runtime_error {
 public:
  const Status status;
  explicit MameFontException(Status status)
      : std::runtime_error(statusToString(status)), status(status) {}
};

#define MAMEFONT_THROW_OR_RETURN(status)   \
  do {                                     \
    if (static_cast<int8_t>(status) < 0) { \
      throw MameFontException(status);     \
    } else {                               \
      return (status);                     \
    }                                      \
  } while (0)
#else
#define MAMEFONT_THROW_OR_RETURN(status) return (status)
#endif

enum class PixelFormat : uint8_t {
  BW_1BIT = 0,    // todo rename to BPP1
  GRAY_2BIT = 1,  // todo rename to BPP2
  RESERVED2 = 2,
  RESERVED3 = 3,
};

static MAMEFONT_INLINE uint8_t getBitsPerPixel(PixelFormat bpp) {
  return (bpp == PixelFormat::BW_1BIT) ? 1 : 2;
}

static MAMEFONT_INLINE uint8_t getPixelsPerFrag(PixelFormat bpp) {
  return (bpp == PixelFormat::BW_1BIT) ? 8 : 4;
}

using frag_t = uint8_t;

#ifdef MAMEFONT_FRAG_INDEX_8BIT
using frag_index_t = int8_t;
#else
using frag_index_t = int16_t;
#endif

#ifdef MAMEFONT_PROGRAM_COUNTER_8BIT
using prog_cntr_t = uint8_t;
#else
using prog_cntr_t = uint16_t;
#endif

}  // namespace mamefont
