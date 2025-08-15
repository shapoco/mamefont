#pragma once

#ifdef MAMEFONT_EXCEPTIONS
#include <stdexcept>
#endif

#include "mamefont/cursor.hpp"
#include "mamefont/font.hpp"
#include "mamefont/glyph_buffer.hpp"
#include "mamefont/mamefont_common.hpp"

namespace mamefont {

class StateMachine {
 private:
  FontFlags flags;
  const frag_t *fragTable;
  const uint8_t *bytecode;
  uint8_t glyphHeight;

  uint8_t *buffData;

  int8_t numLanesToGlyphEnd = 0;

  prog_cntr_t programCounter = 0;
  frag_t lastFragment = 0;

  AddrRule rule;
  Cursor writeCursor;

 public:
#ifdef MAMEFONT_DEBUG
  Operator dbgLastOp = Operator::NONE;
  Cursor dbgDumpCursor;
  Operator *dbgOpLog = nullptr;
  uint8_t dbgLastInstByte1 = 0;
  prog_cntr_t dbgStartPc = 0;
  prog_cntr_t dbgLastPc = 0;

  int dbgNumInstsPerOpr[static_cast<int>(Operator::COUNT)] = {0};
  int dbgGenFragsPerOpr[static_cast<int>(Operator::COUNT)] = {0};
  void logInstructionPerformance(Operator op, int size) {
    dbgNumInstsPerOpr[static_cast<int>(op)]++;
    dbgGenFragsPerOpr[static_cast<int>(op)] += size;
  }
#endif

  StateMachine(const Font &font) {
    this->flags = font.header.flags;
    this->fragTable = font.blob + font.fragmentTableOffset();
    this->bytecode = font.blob + font.byteCodeOffset();
    this->glyphHeight = font.header.glyphHeight;

#ifdef MAMEFONT_HORI_FRAG_ONLY
    bool verticalFrag = false;
#elif defined(MAMEFONT_VERT_FRAG_ONLY)
    bool verticalFrag = true;
#else
    bool verticalFrag = flags.verticalFragment();
#endif

    if (verticalFrag) {
      if (flags.bitsPerPixel() == PixelFormat::BW_1BIT) {
        rule.lanesPerGlyph = (glyphHeight + 7) / 8;
      } else {
        rule.lanesPerGlyph = (glyphHeight + 3) / 4;
      }
#ifndef MAMEFONT_VERT_FRAG_ONLY
      rule.fragStride = 1;
#endif
    } else {
      rule.fragsPerLane = glyphHeight;
#ifndef MAMEFONT_HORI_FRAG_ONLY
      rule.laneStride = 1;
#endif
    }
  }

  Status run(Glyph &glyph, const GlyphBuffer &buff) {
    buffData = buff.data;

#ifdef MAMEFONT_HORI_FRAG_ONLY
    bool verticalFrag = false;
#elif defined(MAMEFONT_VERT_FRAG_ONLY)
    bool verticalFrag = true;
#else
    bool verticalFrag = flags.verticalFragment();
#endif

    int8_t glyphWidth = glyph.glyphWidth;
    if (verticalFrag) {
      rule.fragsPerLane = glyphWidth;
#ifndef MAMEFONT_HORI_FRAG_ONLY
      rule.laneStride = buff.stride;
#endif
    } else {
#ifndef MAMEFONT_VERT_FRAG_ONLY
      rule.fragStride = buff.stride;
#endif

      if (flags.bitsPerPixel() == PixelFormat::BW_1BIT) {
        rule.lanesPerGlyph = (glyphWidth + 7) / 8;
      } else {
        rule.lanesPerGlyph = (glyphWidth + 3) / 4;
      }
    }
    numLanesToGlyphEnd = rule.lanesPerGlyph;

    writeCursor.reset();

    lastFragment = 0x00;
    programCounter = glyph.entryPoint;
#ifdef MAMEFONT_DEBUG
    dbgStartPc = programCounter;
    dbgLastPc = programCounter;
#endif

    if (programCounter == DUMMY_ENTRY_POINT) {
#if MAMEFONT_EXCEPTIONS
      throw std::runtime_error("Glyph is empty.");
#else
      numLanesToGlyphEnd = 0;
#endif
    }

#ifdef MAMEFONT_DEBUG
    dbgDumpCursor = writeCursor;
    if (dbgOpLog) delete[] dbgOpLog;
    dbgOpLog = new Operator[buff.stride * glyphHeight];
    memset(dbgOpLog, 0, buff.stride * glyphHeight * sizeof(Operator));
    memset(dbgNumInstsPerOpr, 0, sizeof(dbgNumInstsPerOpr));
    memset(dbgGenFragsPerOpr, 0, sizeof(dbgGenFragsPerOpr));

    if (verbose) {
      printf("  entryPoint    : %d\n", programCounter);
      printf("  glyphWidth    : %d\n", glyphWidth);
      printf("  fragsPerLane  : %d\n", rule.fragsPerLane);
      printf("  lanesPerGlyph : %d\n", rule.lanesPerGlyph);
      printf("  fragStride    : %d\n", rule.fragStride);
      printf("  laneStride    : %d\n", rule.laneStride);
    }
#endif

    while (numLanesToGlyphEnd > 0) {
      uint8_t inst = readBlobU8(bytecode + (programCounter++));
#ifdef MAMEFONT_DEBUG
      dbgLastInstByte1 = inst;
#endif

      if ((inst & 0x80) == 0) {
        if ((inst & 0x40) == 0) {
          // 0x00-3F
          SFT(inst);
        } else if (((inst & 0x20) == 0) || (((inst & 0x07) != 0))) {
          // 0x40-7F except 0x60, 0x68, 0x70, 0x78
          if (inst == 0x40) {
#ifdef MAMEFONT_NO_CPX
            MAMEFONT_RETURN_ERROR(Status::UNKNOWN_OPCODE);
#else
            CPX(inst);
#endif
          } else {
            CPY(inst);
          }
        } else if ((inst & 0x10) == 0) {
          // 0x60, 0x68
          if ((inst & 0x08) == 0) {
            LDI(inst);
          } else {
#ifdef MAMEFONT_NO_SFI
            MAMEFONT_RETURN_ERROR(Status::UNKNOWN_OPCODE);
#else
            SFI(inst);
#endif
          }
        } else {
          // 0x70, 0x78
          MAMEFONT_RETURN_ERROR(Status::UNKNOWN_OPCODE);
        }
      } else {
        if ((inst & 0x40) == 0) {
          // 0x80-BF
          LUP(inst);
        } else if ((inst & 0x20) == 0) {
          // 0xC0-DF
          LUD(inst);
        } else if ((inst & 0x10) == 0) {
          // 0xE0-EF
          RPT(inst);
        } else {
          // 0xF0-FE
          if (inst == 0xFF) {
            MAMEFONT_RETURN_ERROR(Status::ABORTED_BY_ABO);
          } else {
            XOR(inst);
          }
        }
      }
    }
    return Status::SUCCESS;
  }

 private:
#ifdef MAMEFONT_DEBUG

#define MAMEFONT_BEFORE_OP(op, inst_size, fmt, ...)                          \
  do {                                                                       \
    dbgLastOp = op;                                                          \
    dbgDumpCursor = writeCursor;                                             \
    dbgOpLog[dbgDumpCursor.offset] = op;                                     \
    if (!verbose) break;                                                     \
    char logBuff[128];                                                       \
    char *logPtr = logBuff;                                                  \
    char *logEnd = logBuff + sizeof(logBuff);                                \
    const int pc = programCounter - 1;                                       \
    logPtr += snprintf(logPtr, logEnd - logPtr, "    pc=%5d, ofst=%4d,", pc, \
                       (int)dbgDumpCursor.offset);                           \
    for (int i = 0; i < 4; i++) {                                            \
      if (i < (inst_size)) {                                                 \
        logPtr += snprintf(logPtr, logEnd - logPtr, " %02X",                 \
                           readBlobU8(bytecode + (pc + i)));                 \
      } else {                                                               \
        logPtr += snprintf(logPtr, logEnd - logPtr, "   ");                  \
      }                                                                      \
    }                                                                        \
    logPtr += snprintf(logPtr, logEnd - logPtr, "%-4s", mnemonicOf(op));     \
    logPtr += snprintf(logPtr, logEnd - logPtr, (fmt), ##__VA_ARGS__);       \
    printf("%-64s", logBuff);                                                \
    if (logPtr - logBuff > 64) {                                             \
      printf("\n");                                                          \
      for (int i = 0; i < 64; i++) printf(" ");                              \
    }                                                                        \
    printf("--> ");                                                          \
  } while (false)

#define MAMEFONT_AFTER_OP(len)                      \
  do {                                              \
    dbgLastPc = programCounter;                     \
    logInstructionPerformance(dbgLastOp, (len));    \
    if (!verbose) break;                            \
    for (int i = 0; i < (len); i++) {               \
      if (i % 16 == 0 && i > 0) {                   \
        for (int j = 0; j < 64 + 4; j++) {          \
          printf(" ");                              \
        }                                           \
      }                                             \
      printf("%02X ", readPostIncr(dbgDumpCursor)); \
      if ((i + 1) % 16 == 0 || (i + 1) == (len)) {  \
        printf("\n");                               \
      }                                             \
    }                                               \
  } while (false)

#else

#define MAMEFONT_BEFORE_OP(op, fmt, ...) \
  do {                                   \
  } while (false)

#define MAMEFONT_AFTER_OP(len) \
  do {                         \
  } while (false)

#endif

  MAMEFONT_INLINE
  void LUP(uint8_t inst) {
    uint8_t index = LUP::Index::read(inst);

    MAMEFONT_BEFORE_OP(Operator::LUP, 1, "(idx=%d)", (int)index);

    write(readBlobU8(fragTable + index));

    MAMEFONT_AFTER_OP(1);
  }

  MAMEFONT_INLINE
  void LUD(uint8_t inst) {
    uint8_t index = LUD::Index::read(inst);
    bool step = LUD::Step::read(inst);

    MAMEFONT_BEFORE_OP(Operator::LUD, 1, "(idx=%d, step=%d)", (int)index,
                       (step ? 1 : 0));

    const frag_t *ptr = fragTable + index;
    write(readBlobU8(ptr));
    if (step) ptr++;
    write(readBlobU8(ptr));

    MAMEFONT_AFTER_OP(2);
  }

  MAMEFONT_INLINE
  void SFT(uint8_t inst) {
    uint8_t size = SFT::Size::read(inst);
    uint8_t rpt = SFT::RepeatCount::read(inst);
    uint8_t sfiFlags = inst & (SFI::Right::MASK | SFI::PostSet::MASK);
    static_assert(SFT::Right::MASK == SFI::Right::MASK &&
                      SFT::PostSet::MASK == SFI::PostSet::MASK,
                  "SFT and SFI flags must match");
    bool right = SFI::Right::read(sfiFlags);
    bool postSet = SFI::PostSet::read(sfiFlags);

    MAMEFONT_BEFORE_OP(Operator::SFT, 1, "(dir=%c, postOp=%c, size=%d, rpt=%d)",
                       (right ? 'R' : 'L'), (postSet ? 'S' : 'C'), (int)size,
                       (int)rpt);

    shiftCore(sfiFlags, size, rpt, 1);

    MAMEFONT_AFTER_OP(rpt);
  }

#ifndef MAMEFONT_NO_SFI
  MAMEFONT_INLINE
  void SFI(uint8_t inst) {
    uint8_t byte2 = readBlobU8(bytecode + (programCounter + 0));
    uint8_t rpt = SFI::RepeatCount::read(byte2);
    uint8_t period = SFI::Period::read(byte2);
    uint8_t sfiFlags =
        byte2 & (SFI::PreShift::MASK | SFI::Right::MASK | SFI::PostSet::MASK);
    bool right = SFI::Right::read(sfiFlags);
    bool postSet = SFI::PostSet::read(sfiFlags);
    bool preShift = SFI::PreShift::read(sfiFlags);

    MAMEFONT_BEFORE_OP(Operator::SFI, 2,
                       "(dir=%c, period=%d, shift1st=%d, postOp=%c, rpt=%d)",
                       (right ? 'R' : 'L'), (int)period, (preShift ? 1 : 0),
                       (postSet ? 'S' : 'C'), (int)rpt);

    shiftCore(sfiFlags, 1, rpt, period);

    programCounter += 1;

    MAMEFONT_AFTER_OP(rpt * period + (preShift ? 1 : 0));
  }
#endif

#if defined(MAMEFONT_1BPP_ONLY)
  using shift_state_t = frag_t;
#else
  using shift_state_t = uint16_t;

#endif

#ifndef MAMEFONT_NO_SFI
  MAMEFONT_NOINLINE
#endif
  void shiftCore(uint8_t sfiFlags, uint8_t size, uint8_t rpt, uint8_t period) {
    bool right = SFI::Right::read(sfiFlags);
    bool bpp2 = (flags.bitsPerPixel() != PixelFormat::BW_1BIT);
    uint8_t stateWidth = bpp2 ? 12 : 8;

#if defined(MAMEFONT_1BPP_ONLY)
    shift_state_t modifier = getRightMaskU8(right ? (8 - size) : size);
#else
    shift_state_t modifier =
        getRightMaskU16(right ? (stateWidth - size) : size);
#endif
    if (right) modifier = ~modifier;

    bool postSet = SFI::PostSet::read(sfiFlags);
    if (!postSet) modifier = ~modifier;

    int8_t timer;
    if (SFI::PreShift::read(sfiFlags)) {
      rpt++;
      timer = 1;
    } else {
      timer = period;
    }

    shift_state_t state;

    if (bpp2) {
      state = encodeShiftState2bpp(lastFragment, right, postSet);
    } else {
      state = lastFragment;
    }

    do {
      if (--timer == 0) {
        rpt--;
        timer = period;
        if (right) {
          state >>= size;
        } else {
          state <<= size;
        }
        if (postSet) {
          state |= modifier;
        } else {
          state &= modifier;
        }
      }

      if (bpp2) {
        write(decodeShiftState2bpp(state));
      } else {
        write(state);
      }

    } while (rpt != 0);
  }

  MAMEFONT_INLINE
  void CPY(uint8_t inst) {
    uint8_t offset = CPY::Offset::read(inst);
    uint8_t length = CPY::Length::read(inst);
    bool byteReverse = CPY::ByteReverse::read(inst);

    MAMEFONT_BEFORE_OP(Operator::CPY, 1, "(ofst=%d, len=%d, rev=%d)",
                       (int)offset, (int)length, (byteReverse ? 1 : 0));

    uint8_t cpxFlags;
    if (byteReverse) {
      cpxFlags = CPX::ByteReverse::MASK;
    } else {
      cpxFlags = 0;
      offset += length;
    }
    copyCore(cpxFlags, -offset, length);

    MAMEFONT_AFTER_OP(length);
  }

#ifndef MAMEFONT_NO_CPX
  MAMEFONT_INLINE
  void CPX(uint8_t inst) {
    uint8_t byte2 = readBlobU8(bytecode + (programCounter + 0));
    uint8_t byte3 = readBlobU8(bytecode + (programCounter + 1));
    uint8_t cpxFlags = byte3 & (CPX::ByteReverse::MASK | CPX::PixelReverse::MASK |
                                CPX::Inverse::MASK);
    uint8_t length = CPX::Length::read(byte3);
    frag_index_t offset =
        CPX::Offset::read((static_cast<uint16_t>(byte3) << 8) | byte2);

    bool byteReverse = CPX::ByteReverse::read(cpxFlags);
    bool pixelReverse = CPX::PixelReverse::read(cpxFlags);
    bool inverse = CPX::Inverse::read(cpxFlags);
    MAMEFONT_BEFORE_OP(Operator::CPX, 3,
                       "(ofst=%d, len=%d, byteRev=%d, bitRev=%d, inv=%d)",
                       (int)offset, (int)length, (int)byteReverse,
                       (int)pixelReverse, (int)inverse);

    if (byteReverse) offset -= length;
    copyCore(cpxFlags, -offset, length);

    programCounter += 2;

    MAMEFONT_AFTER_OP(length);
  }
#endif

#ifndef MAMEFONT_NO_CPX
  MAMEFONT_NOINLINE
#endif
  void copyCore(uint8_t cpxFlags, frag_index_t offset, uint8_t length) {
    Cursor readCursor = writeCursor;
    readCursor.add(rule, offset);
    for (int8_t i = length; i != 0; i--) {
      frag_t frag;
      if (CPX::ByteReverse::read(cpxFlags)) {
        frag = readPreDecr(readCursor);
      } else {
        frag = readPostIncr(readCursor);
      }
#ifndef MAMEFONT_NO_CPX
      if (CPX::PixelReverse::read(cpxFlags))
        frag = reversePixels(frag, flags.bitsPerPixel());
      if (CPX::Inverse::read(cpxFlags)) frag = ~frag;
#endif
      write(frag);
    }
  }

  MAMEFONT_INLINE void LDI(uint8_t inst) {
    frag_t frag = readBlobU8(bytecode + programCounter);
    MAMEFONT_BEFORE_OP(Operator::LDI, 2, "(frag=0x%02X)", frag);
    write(frag);
    programCounter += 1;
    MAMEFONT_AFTER_OP(1);
  }

  MAMEFONT_INLINE
  void RPT(uint8_t inst) {
    uint8_t repeatCount = RPT::RepeatCount::read(inst);
    MAMEFONT_BEFORE_OP(Operator::RPT, 1, "(rpt=%d)", repeatCount);
    for (uint8_t i = repeatCount; i != 0; i--) {
      write(lastFragment);
    }
    MAMEFONT_AFTER_OP(repeatCount);
  }

  MAMEFONT_INLINE
  void XOR(uint8_t inst) {
    uint8_t mask = XOR::Width2Bit::read(inst) ? 0x03 : 0x01;
    mask <<= XOR::Pos::read(inst);
    MAMEFONT_BEFORE_OP(Operator::XOR, 1, "(mask=0x%02X)", mask);
    write(lastFragment ^ mask);
    MAMEFONT_AFTER_OP(1);
  }

  MAMEFONT_NOINLINE
  void write(uint8_t value) {
    buffData[writeCursor.postIncr(rule)] = value;
    lastFragment = value;
    if (writeCursor.fragIndex == 0) {
      numLanesToGlyphEnd--;
    }
  }

  MAMEFONT_NOINLINE
  uint8_t readPostIncr(Cursor &cursor) const {
    frag_index_t laneOfst = cursor.laneOffset;
    frag_index_t ofst = cursor.postIncr(rule);
    if (laneOfst < 0 || ofst < 0) return 0;
    return buffData[ofst];
  }

  MAMEFONT_NOINLINE
  uint8_t readPreDecr(Cursor &cursor) const {
    frag_index_t ofst = cursor.preDecr(rule);
    if (cursor.laneOffset < 0 || ofst < 0) return 0;
    return buffData[ofst];
  }
};

}  // namespace mamefont
