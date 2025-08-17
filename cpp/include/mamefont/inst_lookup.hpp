#pragma once

#ifdef MAMEFONT_EXCEPTIONS
#include <stdexcept>
#endif

#include "mamefont/debugger.hpp"
#include "mamefont/decoder_context.hpp"

namespace mamefont {

static MAMEFONT_INLINE void LUP(DecoderContext &ctx, Debugger &dbg,
                                uint8_t byte1) {
  uint8_t index = LUP::Index::read(byte1);

  MAMEFONT_BEFORE_OP(dbg, ctx, Operator::LUP, "(idx=%d)", (int)index);

  frag_t frag = readBlobU8(ctx.fragTable + index);
  ctx.last = ctx.data[ctx.cursor++] = frag;

  MAMEFONT_AFTER_OP(dbg, ctx, 1);
}

static MAMEFONT_INLINE void LUD(DecoderContext &ctx, Debugger &dbg,
                                uint8_t byte1) {
  uint8_t index = LUD::Index::read(byte1);
  bool step = LUD::Step::read(byte1);

  MAMEFONT_BEFORE_OP(dbg, ctx, Operator::LUD, "(idx=%d, step=%d)", (int)index,
                     (step ? 1 : 0));

  const frag_t *ptr = ctx.fragTable + index;
  frag_t frag = readBlobU8(ptr);
  ctx.data[ctx.cursor++] = frag;
  if (step) frag = readBlobU8(ptr + 1);
  ctx.last = ctx.data[ctx.cursor++] = frag;

  MAMEFONT_AFTER_OP(dbg, ctx, 2);
}

}  // namespace mamefont