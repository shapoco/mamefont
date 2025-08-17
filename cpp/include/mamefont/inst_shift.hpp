#pragma once

#ifdef MAMEFONT_EXCEPTIONS
#include <stdexcept>
#endif

#include "mamefont/debugger.hpp"
#include "mamefont/decoder_context.hpp"

namespace mamefont {

#ifdef MAMEFONT_NO_SFI
#define MAMEFONT_SHIFT_CORE_CLASS static MAMEFONT_INLINE
#else
#define MAMEFONT_SHIFT_CORE_CLASS MAMEFONT_NOINLINE
#endif

#if defined(MAMEFONT_1BPP_ONLY)
using shift_state_t = frag_t;
#else
using shift_state_t = uint16_t;
#endif

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

MAMEFONT_SHIFT_CORE_CLASS void shiftCore(DecoderContext &ctx, Debugger &dbg,
                                         uint8_t sfiFlags, uint8_t size,
                                         uint8_t rpt, uint8_t period)
#if defined(MAMEFONT_INCLUDE_IMPL) || defined(MAMEFONT_NO_SFI)
{
  bool right = SFI::Right::read(sfiFlags);
  bool bpp2 = (ctx.flags.fragFormat() != PixelFormat::BW_1BIT);
  uint8_t stateWidth = bpp2 ? 12 : 8;

#if defined(MAMEFONT_1BPP_ONLY)
  shift_state_t modifier = getRightMaskU8(right ? (8 - size) : size);
#else
  shift_state_t modifier = getRightMaskU16(right ? (stateWidth - size) : size);
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

  frag_t last = ctx.last;

  shift_state_t state;
  if (bpp2) {
    state = encodeShiftState2bpp(last, right, postSet);
  } else {
    state = last;
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
      last = decodeShiftState2bpp(state);
    } else {
      last = state;
    }
    ctx.data[ctx.cursor++] = last;

  } while (rpt != 0);

  ctx.last = last;
}
#else
    ;
#endif

static MAMEFONT_INLINE void SFT(DecoderContext &ctx, Debugger &dbg,
                                uint8_t byte1) {
  uint8_t size = SFT::Size::read(byte1);
  uint8_t rpt = SFT::RepeatCount::read(byte1);
  uint8_t sfiFlags = byte1 & (SFI::Right::MASK | SFI::PostSet::MASK);
  static_assert(SFT::Right::MASK == SFI::Right::MASK &&
                    SFT::PostSet::MASK == SFI::PostSet::MASK,
                "SFT and SFI flags must match");
  bool right = SFI::Right::read(sfiFlags);
  bool postSet = SFI::PostSet::read(sfiFlags);

  MAMEFONT_BEFORE_OP(
      dbg, ctx, Operator::SFT, "(dir=%c, postOp=%c, size=%d, rpt=%d)",
      (right ? 'R' : 'L'), (postSet ? 'S' : 'C'), (int)size, (int)rpt);

  shiftCore(ctx, dbg, sfiFlags, size, rpt, 1);

  MAMEFONT_AFTER_OP(dbg, ctx, rpt);
}

#ifndef MAMEFONT_NO_SFI
static MAMEFONT_INLINE void SFI(DecoderContext &ctx, Debugger &dbg,
                                uint8_t byte1) {
  uint8_t byte2 = ctx.fetch();
  uint8_t rpt = SFI::RepeatCount::read(byte2);
  uint8_t period = SFI::Period::read(byte2);
  uint8_t sfiFlags =
      byte2 & (SFI::PreShift::MASK | SFI::Right::MASK | SFI::PostSet::MASK);
  bool right = SFI::Right::read(sfiFlags);
  bool postSet = SFI::PostSet::read(sfiFlags);
  bool preShift = SFI::PreShift::read(sfiFlags);

  MAMEFONT_BEFORE_OP(dbg, ctx, Operator::SFI,
                     "(dir=%c, period=%d, shift1st=%d, postOp=%c, rpt=%d)",
                     (right ? 'R' : 'L'), (int)period, (preShift ? 1 : 0),
                     (postSet ? 'S' : 'C'), (int)rpt);

  shiftCore(ctx, dbg, sfiFlags, 1, rpt, period);

  MAMEFONT_AFTER_OP(dbg, ctx, rpt * period + (preShift ? 1 : 0));
}
#endif
}  // namespace mamefont