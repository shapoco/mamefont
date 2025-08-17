#pragma once

#ifdef MAMEFONT_EXCEPTIONS
#include <stdexcept>
#endif

#include "mamefont/debugger.hpp"
#include "mamefont/decoder_context.hpp"

namespace mamefont {

static MAMEFONT_INLINE void LDI(DecoderContext &ctx, Debugger &dbg,
                                uint8_t byte1) {
  frag_t byte2 = ctx.fetch();

  MAMEFONT_BEFORE_OP(dbg, ctx, Operator::LDI, "(frag=0x%02X)", byte2);

  ctx.last = ctx.data[ctx.cursor++] = byte2;

  MAMEFONT_AFTER_OP(dbg, ctx, 1);
}

}  // namespace mamefont
