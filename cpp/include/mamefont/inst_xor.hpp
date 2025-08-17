#pragma once

#ifdef MAMEFONT_EXCEPTIONS
#include <stdexcept>
#endif

#include "mamefont/debugger.hpp"
#include "mamefont/decoder_context.hpp"

namespace mamefont {

static MAMEFONT_INLINE void XOR(DecoderContext &ctx, Debugger &dbg,
                                uint8_t byte1) {
  uint8_t mask = XOR::Width2Bit::read(byte1) ? 0x03 : 0x01;
  mask <<= XOR::Pos::read(byte1);

  MAMEFONT_BEFORE_OP(dbg, ctx, Operator::XOR, "(mask=0x%02X)", mask);

  ctx.last = ctx.data[ctx.cursor++] = ctx.last ^ mask;

  MAMEFONT_AFTER_OP(dbg, ctx, 1);
}

}  // namespace mamefont
