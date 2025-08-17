#pragma once

#ifdef MAMEFONT_EXCEPTIONS
#include <stdexcept>
#endif

#include "mamefont/debugger.hpp"
#include "mamefont/decoder_context.hpp"

namespace mamefont {

static MAMEFONT_INLINE void RPT(DecoderContext &ctx, Debugger &dbg,
                                uint8_t byte1) {
  uint8_t repeatCount = RPT::RepeatCount::read(byte1);

  MAMEFONT_BEFORE_OP(dbg, ctx, Operator::RPT, "(rpt=%d)", repeatCount);

  frag_t last = ctx.last;
  for (uint8_t i = repeatCount; i != 0; i--) {
    ctx.data[ctx.cursor++] = last;
  }

  MAMEFONT_AFTER_OP(dbg, ctx, repeatCount);
}

}  // namespace mamefont