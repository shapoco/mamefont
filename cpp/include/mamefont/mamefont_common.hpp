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

#ifdef MAMEFONT_DEBUG
extern bool verbose;
static inline void setVerbose(bool v) { verbose = v; }
#endif

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
  CHAR_CODE_OUT_OF_RANGE = 1,
  GLYPH_NOT_DEFINED = 2,
  NULL_POINTER = -1,
  UNKNOWN_OPCODE = -2,
  ABORTED_BY_ABO = -3,
};

static MAMEFONT_INLINE const char *statusToString(Status status) {
  switch (status) {
    case Status::SUCCESS:
      return "Success";
    case Status::CHAR_CODE_OUT_OF_RANGE:
      return "Character code out of range";
    case Status::GLYPH_NOT_DEFINED:
      return "Glyph not defined";
    case Status::NULL_POINTER:
      return "Null pointer";
    case Status::UNKNOWN_OPCODE:
      return "Unknown opcode";
    case Status::ABORTED_BY_ABO:
      return "Aborted by ABO instruction";
  }
  return "Unknown status";
}

#ifdef MAMEFONT_EXCEPTIONS

class MameFontException : public std::runtime_error {
 public:
  const Status status;
  explicit MameFontException(Status status)
      : std::runtime_error(statusToString(status)), status(status) {}
};

#define MAMEFONT_RETURN_ERROR(status) throw MameFontException(status)
#else
#define MAMEFONT_RETURN_ERROR(status) return (status)
#endif

enum class Operator : int8_t {
  NONE = 0,
  RPT,
  CPY,
  XOR,
  SFT,
  SFI,
  LUP,
  LUD,
  LDI,
  CPX,
  ABO,
  COUNT,
};

enum class PixelFormat : uint8_t {
  BW_1BIT = 0,
  GRAY_2BIT = 1,
  RESERVED2 = 2,
  RESERVED3 = 3,
};

static MAMEFONT_INLINE uint8_t getBitsPerPixel(PixelFormat bpp) {
  return (bpp == PixelFormat::BW_1BIT) ? 1 : 2;
}

static MAMEFONT_INLINE uint8_t getPixelsPerFrag(PixelFormat bpp) {
  return (bpp == PixelFormat::BW_1BIT) ? 8 : 4;
}

struct Instruction {
  uint8_t length = 0;
  uint8_t code[3] = {0x00, 0x00, 0x00};

  Instruction() = default;
  Instruction(uint8_t b1) : length(1), code{b1, 0x00, 0x00} {}
  Instruction(uint8_t b1, uint8_t b2) : length(2), code{b1, b2, 0x00} {}
  Instruction(uint8_t b1, uint8_t b2, uint8_t b3)
      : length(3), code{b1, b2, b3} {}
};

static MAMEFONT_INLINE constexpr uint8_t baseCodeOf(Operator op) {
  switch (op) {
    case Operator::RPT:
      return 0xE0;
    case Operator::CPY:
      return 0x40;
    case Operator::XOR:
      return 0xF0;
    case Operator::SFT:
      return 0x00;
    case Operator::SFI:
      return 0x68;
    case Operator::LUP:
      return 0x80;
    case Operator::LUD:
      return 0xC0;
    case Operator::LDI:
      return 0x60;
    case Operator::CPX:
      return 0x40;
    case Operator::ABO:
      return 0xFF;
    default:
#ifdef MAMEFONT_EXCEPTIONS
      throw std::invalid_argument("Unknown opcode");
#else
      return 0xFF;
#endif
  }
}

static MAMEFONT_INLINE constexpr uint8_t instSizeOf(Operator op) {
  switch (op) {
    case Operator::RPT:
    case Operator::CPY:
    case Operator::XOR:
    case Operator::SFT:
    case Operator::LUP:
    case Operator::LUD:
    case Operator::ABO:
      return 1;
    case Operator::SFI:
    case Operator::LDI:
      return 2;
    case Operator::CPX:
      return 3;
    default:
#ifdef MAMEFONT_EXCEPTIONS
      throw std::invalid_argument("Unknown opcode");
#else
      return 0xFF;
#endif
  }
}

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
      *ptr |= MASK;
    } else {
      *ptr &= ~MASK;
    }
  }
};

struct FontFlags {
  using VerticalFragment = BitFlag<0, 7>;
  using FarPixelFirst = BitFlag<0, 6>;
  using PixelFormatField = BitField<uint8_t, uint8_t, 0, 4, 2>;
  using LargeFont = BitFlag<0, 2>;
  using Proportional = BitFlag<0, 1>;
  using HasExtendedHeader = BitFlag<0, 0>;

  uint8_t value;
  FontFlags() : value(0) {}
  FontFlags(uint8_t flags) : value(flags) {}

  MAMEFONT_INLINE bool verticalFragment() const {
#ifdef MAMEFONT_HORI_FRAG_ONLY
    return false;
#elif defined(MAMEFONT_VERT_FRAG_ONLY)
    return true;
#else
    return VerticalFragment::read(value);
#endif
  }

  MAMEFONT_INLINE bool farPixelFirst() const {
#ifdef MAMEFONT_NEAR1ST_ONLY
    return false;
#elif defined(MAMEFONT_FAR1ST_ONLY)
    return true;
#else
    return FarPixelFirst::read(value);
#endif
  }

  // todo: rename to pixelFormat()
  MAMEFONT_INLINE PixelFormat bitsPerPixel() const {
#if defined(MAMEFONT_1BPP_ONLY)
    return PixelFormat::BW_1BIT;
#elif defined(MAMEFONT_2BPP_ONLY)
    return PixelFormat::GRAY_2BIT;
#else
    return PixelFormatField::read(value) == 0 ? PixelFormat::BW_1BIT
                                              : PixelFormat::GRAY_2BIT;
#endif
  }

  MAMEFONT_INLINE bool largeFont() const {
#ifdef MAMEFONT_SMALL_ONLY
    return false;
#elif defined(MAMEFONT_LARGE_ONLY)
    return true;
#else
    return LargeFont::read(value);
#endif
  }

  MAMEFONT_INLINE bool proportional() const {
#ifdef MAMEFONT_MONOSPACED_ONLY
    return false;
#elif defined(MAMEFONT_PROPORTIONAL_ONLY)
    return true;
#else
    return Proportional::read(value);
#endif
  }

  MAMEFONT_INLINE bool hasExtendedHeader() const {
#ifdef MAMEFONT_NO_EXT_HEADER
    return false;
#else
    return HasExtendedHeader::read(value);
#endif
  }
};

struct FontHeader {
  static constexpr uint8_t SIZE = 8;
  using FormatVersion = BitField<uint8_t, uint8_t, 0, 0, 4>;
  using Flags = BitField<uint8_t, uint8_t, 1, 0, 8>;
  using FirstCode = BitField<uint8_t, uint8_t, 2, 0, 8>;
  using LastCode = BitField<uint8_t, uint8_t, 3, 0, 8>;
  using FragmentTableSize = BitField<uint8_t, uint8_t, 4, 0, 5, 2, 2>;
  using MaxGlyphWidth = BitField<uint8_t, uint8_t, 5, 0, 6, 1>;
  using GlyphHeight = BitField<uint8_t, uint8_t, 6, 0, 6, 1>;
  using XMonoSpacing = BitField<uint8_t, uint8_t, 7, 0, 4>;
  using YSpacing = BitField<uint8_t, uint8_t, 7, 4, 4>;

  uint8_t formatVersion;
  FontFlags flags;
  uint8_t firstCode;
  uint8_t lastCode;
  uint8_t fragmentTableSize;
  uint8_t maxGlyphWidth;
  uint8_t glyphHeight;
  uint8_t xMonoSpacing;
  uint8_t ySpacing;

  FontHeader() = default;
  FontHeader(const uint8_t *blob) { loadFromBlob(blob); }

  MAMEFONT_NOINLINE void loadFromBlob(const uint8_t *blob) {
    const uint8_t *ptr = blob;
    formatVersion = FontHeader::FormatVersion::read(readBlobU8(ptr++));
    flags = FontHeader::Flags::read(readBlobU8(ptr++));
    firstCode = FontHeader::FirstCode::read(readBlobU8(ptr++));
    lastCode = FontHeader::LastCode::read(readBlobU8(ptr++));
    fragmentTableSize = FontHeader::FragmentTableSize::read(readBlobU8(ptr++));
    maxGlyphWidth = FontHeader::MaxGlyphWidth::read(readBlobU8(ptr++));
    glyphHeight = FontHeader::GlyphHeight::read(readBlobU8(ptr++));
    uint8_t byte7 = readBlobU8(ptr++);
    xMonoSpacing = FontHeader::XMonoSpacing::read(byte7);
    ySpacing = FontHeader::YSpacing::read(byte7);
  }

#ifdef MAMEFONT_DEBUG
  void dumpHeader(const char *indent) const {
    printf("%sFormat Version  : %d\n", indent, formatVersion);
    printf("%sFlags           : 0x%02X\n", indent, flags.value);
    printf("%s  Vertical Frag.  : %s\n", indent,
           flags.verticalFragment() ? "Yes" : "No");
    printf("%s  Far Pixel First : %s\n", indent,
           flags.farPixelFirst() ? "Yes" : "No");
    printf("%s  Bits per Pixel  : %s\n", indent,
           flags.bitsPerPixel() == PixelFormat::BW_1BIT ? "1" : "2");
    printf("%s  Large Font      : %s\n", indent,
           flags.largeFont() ? "Yes" : "No");
    printf("%s  Proportional    : %s\n", indent,
           flags.proportional() ? "Yes" : "No");
    printf("%s  Has Ext. Header : %s\n", indent,
           flags.hasExtendedHeader() ? "Yes" : "No");
    printf("%sFirst Code      : 0x%02X\n", indent, firstCode);
    printf("%sLast Code       : 0x%02X\n", indent, lastCode);
    printf("%sFrag Table Size : %d\n", indent, fragmentTableSize);
    printf("%sMax Glyph Width : %d\n", indent, maxGlyphWidth);
    printf("%sGlyph Height    : %d\n", indent, glyphHeight);
    printf("%sX Spacing (Mono): %d\n", indent, xMonoSpacing);
    printf("%sY Spacing       : %d\n", indent, ySpacing);
  }
#endif
};

struct ExtendedHeader {
  using Size = BitField<uint16_t, uint8_t, 0, 0, 8, 2, 2>;
};

struct NormalGlyphEntry {
  static constexpr uint8_t SIZE = 2;
  using EntryPoint = BitField<uint16_t, uint16_t, 0, 0, 14>;
};

struct SmallGlyphEntry {
  static constexpr uint8_t SIZE = 1;
  using EntryPoint = BitField<uint16_t, uint8_t, 0, 0, 8, 0, 2>;
};

struct NormalGlyphDimension {
  static constexpr uint8_t SIZE = 2;
  using GlyphWidth = BitField<uint8_t, uint8_t, 0, 0, 6, 1>;
  using XSpacing = BitField<int8_t, int8_t, 1, 0, 5, -16>;
  using XStepBack = BitField<uint8_t, uint8_t, 1, 5, 3>;
};

struct SmallGlyphDimension {
  static constexpr uint8_t SIZE = 1;
  using GlyphWidth = BitField<uint8_t, uint8_t, 0, 0, 4, 1>;
  using XSpacing = BitField<int8_t, int8_t, 0, 4, 2>;
  using XStepBack = BitField<uint8_t, uint8_t, 0, 6, 2>;
};

struct Glyph {
 public:
  uint16_t entryPoint;
  uint8_t glyphWidth;  // todo: rename to width
  int8_t xSpacing;
  uint8_t xStepBack;
  MAMEFONT_INLINE bool isValid() const {
    return entryPoint != DUMMY_ENTRY_POINT;
  }
};

struct LUP {
  static constexpr uint8_t SIZE = 1;
  using Index = BitField<uint8_t, uint8_t, 0, 0, 6>;
};

struct LUD {
  static constexpr uint8_t SIZE = 1;
  using Index = BitField<uint8_t, uint8_t, 0, 0, 4>;
  using Step = BitFlag<0, 4>;
};

struct LDI {
  static constexpr uint8_t SIZE = 2;
  using Fragment = BitField<uint8_t, uint8_t, 1, 0, 8>;
};

struct SFT {
  static constexpr uint8_t SIZE = 1;
  using RepeatCount = BitField<uint8_t, uint8_t, 0, 0, 2, 1>;
  using Size = BitField<uint8_t, uint8_t, 0, 2, 2, 1>;
  using PostSet = BitFlag<0, 4>;
  using Right = BitFlag<0, 5>;
};

struct SFI {
  static constexpr uint8_t SIZE = 2;
  using RepeatCount = BitField<uint8_t, uint8_t, 1, 0, 3, 1>;
  using PreShift = BitFlag<1, 3>;
  using PostSet = BitFlag<1, SFT::PostSet::POS>;
  using Right = BitFlag<1, SFT::Right::POS>;
  using Period = BitField<uint8_t, uint8_t, 1, 6, 2, 2>;
};

struct RPT {
  static constexpr uint8_t SIZE = 1;
  using RepeatCount = BitField<uint8_t, uint8_t, 0, 0, 4, 1>;
};

struct XOR {
  static constexpr uint8_t SIZE = 1;
  using Pos = BitField<uint8_t, uint8_t, 0, 0, 3>;
  using Width2Bit = BitFlag<0, 3>;
};

struct CPY {
  static constexpr uint8_t SIZE = 1;
  using Length = BitField<uint8_t, uint8_t, 0, 0, 3, 1>;
  using Offset = BitField<uint8_t, uint8_t, 0, 3, 2>;
  using ByteReverse = BitFlag<0, 5>;
};

struct CPX {
  static constexpr uint8_t SIZE = 3;
  using Offset = BitField<uint16_t, uint16_t, 0, 0, 9>;
  using Inverse = BitFlag<1, 1>;
  using Length = BitField<uint8_t, uint8_t, 1, 2, 4, 4, 4>;
  using ByteReverse = BitFlag<1, 6>;
  using PixelReverse = BitFlag<1, 7>;
};

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

#ifdef MAMEFONT_DEBUG
const char *mnemonicOf(Operator op);
#endif

static MAMEFONT_INLINE uint8_t getRightMaskU8(uint8_t width) {
  // clang-format off
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
  // clang-format on
}

static MAMEFONT_INLINE uint16_t getRightMaskU16(uint8_t width) {
  // clang-format off
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
  // clang-format on
}

static MAMEFONT_INLINE uint8_t reversePixels(uint8_t b, PixelFormat bpp) {
  if (bpp == PixelFormat::BW_1BIT) {
    b = ((b & 0x55) << 1) | ((b & 0xaa) >> 1);
  }
  b = ((b & 0x33) << 2) | ((b & 0xcc) >> 2);
  b = (b << 4) | (b >> 4);
  return b;
}

static MAMEFONT_INLINE uint16_t encodeShiftState2bpp(frag_t frag, bool right,
                                                     bool postSet) {
  uint16_t state = 0;
  uint8_t filler = postSet ? 3 : 0;
  uint8_t l, c, r;
  c = (frag >> 6) & 0x3;
  l = right ? filler : c;
  uint8_t tmp = frag;
  frag <<= 2;
  frag |= right ? (tmp & 0x3) : filler;
  for (uint8_t i = 0; i < 4; i++) {
    r = (frag >> 6) & 0x3;
    frag <<= 2;
    uint8_t next3b;
    if (c == 0) {
      next3b = 0b000;
    } else if (c == 3) {
      next3b = 0b111;
    } else if (l < r) {
      next3b = (c == 1) ? 0b001 : 0b011;
    } else if (l == r) {
      next3b = (c == 1) ? 0b010 : 0b101;
    } else {
      next3b = (c == 1) ? 0b100 : 0b110;
    }
    state <<= 3;
    state |= next3b;
    l = c;
    c = r;
  }
  return state;
}

static MAMEFONT_INLINE frag_t decodeShiftState2bpp(uint16_t state) {
  frag_t frag = 0;
  for (uint8_t i = 0; i < 4; i++) {
    uint8_t pix = 0;
    if (state & 0x800) pix++;
    state <<= 1;
    if (state & 0x800) pix++;
    state <<= 1;
    if (state & 0x800) pix++;
    state <<= 1;
    frag <<= 2;
    frag |= pix;
  }
  return frag;
}

}  // namespace mamefont
