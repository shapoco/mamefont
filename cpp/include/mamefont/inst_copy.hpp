#pragma once

#ifdef MAMEFONT_EXCEPTIONS
#include <stdexcept>
#endif

#include "mamefont/debugger.hpp"
#include "mamefont/decoder_context.hpp"

namespace mamefont {

#ifdef MAMEFONT_NO_CPX
#define MAMEFONT_COPY_CORE_CLASS static MAMEFONT_INLINE
#else
#define MAMEFONT_COPY_CORE_CLASS MAMEFONT_NOINLINE
#endif

MAMEFONT_COPY_CORE_CLASS void copyCore(DecoderContext &ctx, Debugger &dbg,
                                       uint8_t cpxFlags, frag_index_t offset,
                                       uint8_t length)
#if defined(MAMEFONT_INCLUDE_IMPL) || defined(MAMEFONT_NO_CPX)
{
  frag_index_t readCursor = ctx.cursor;
  readCursor += offset;

  frag_t frag;
  for (int8_t i = length; i != 0; i--) {
    frag_index_t ri;
    if (CPX::ByteReverse::read(cpxFlags)) {
      ri = --readCursor;
    } else {
      ri = readCursor++;
    }

    frag = (ri >= 0) ? ctx.data[ri] : 0x00;

#ifndef MAMEFONT_NO_CPX
    if (CPX::PixelReverse::read(cpxFlags)) {
      frag = reversePixels(frag, ctx.flags.fragFormat());
    }
    if (CPX::Inverse::read(cpxFlags)) {
      frag = ~frag;
    }
#endif

    ctx.data[ctx.cursor++] = frag;
  }
  ctx.last = frag;
}
#else
    ;
#endif

static MAMEFONT_INLINE void CPY(DecoderContext &ctx, Debugger &dbg,
                                uint8_t byte1) {
  uint8_t offset = CPY::Offset::read(byte1);
  uint8_t length = CPY::Length::read(byte1);
  bool byteReverse = CPY::ByteReverse::read(byte1);

  MAMEFONT_BEFORE_OP(dbg, ctx, Operator::CPY, "(ofst=%d, len=%d, rev=%d)",
                     (int)offset, (int)length, (byteReverse ? 1 : 0));

  uint8_t cpxFlags;
  if (byteReverse) {
    cpxFlags = CPX::ByteReverse::MASK;
  } else {
    cpxFlags = 0;
    offset += length;
  }
  copyCore(ctx, dbg, cpxFlags, -offset, length);

  MAMEFONT_AFTER_OP(dbg, ctx, length);
}

#ifndef MAMEFONT_NO_CPX
static MAMEFONT_INLINE void CPX(DecoderContext &ctx, Debugger &dbg,
                                uint8_t byte1) {
  uint8_t byte2 = ctx.fetch();
  uint8_t byte3 = ctx.fetch();
  uint8_t cpxFlags = byte3 & (CPX::ByteReverse::MASK | CPX::PixelReverse::MASK |
                              CPX::Inverse::MASK);
  uint8_t length = CPX::Length::read(byte3);
  frag_index_t offset =
      CPX::Offset::read((static_cast<uint16_t>(byte3) << 8) | byte2);

  bool byteReverse = CPX::ByteReverse::read(cpxFlags);
  bool pixelReverse = CPX::PixelReverse::read(cpxFlags);
  bool inverse = CPX::Inverse::read(cpxFlags);
  MAMEFONT_BEFORE_OP(dbg, ctx, Operator::CPX,
                     "(ofst=%d, len=%d, byteRev=%d, bitRev=%d, inv=%d)",
                     (int)offset, (int)length, (int)byteReverse,
                     (int)pixelReverse, (int)inverse);

  if (byteReverse) offset -= length;
  copyCore(ctx, dbg, cpxFlags, -offset, length);

  MAMEFONT_AFTER_OP(dbg, ctx, length);
}
#endif

}  // namespace mamefont
