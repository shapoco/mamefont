#pragma once

#ifdef MAMEFONT_EXCEPTIONS
#include <stdexcept>
#endif

#include "mamefont/blob_format.hpp"
#include "mamefont/debugger.hpp"
#include "mamefont/decoder_context.hpp"
#include "mamefont/decoder_utils.hpp"
#include "mamefont/font.hpp"
#include "mamefont/glyph.hpp"
#include "mamefont/inst_copy.hpp"
#include "mamefont/inst_immediate.hpp"
#include "mamefont/inst_lookup.hpp"
#include "mamefont/inst_repeat.hpp"
#include "mamefont/inst_shift.hpp"
#include "mamefont/inst_xor.hpp"
#include "mamefont/instruction_set.hpp"

namespace mamefont {

Status decodeGlyph(const Font &font, Glyph *glyph);

#ifdef MAMEFONT_DEBUG
Status decodeGlyph(const Font &font, Glyph *glyph, Debugger &dbg);
#endif

#ifdef MAMEFONT_INCLUDE_IMPL

#ifdef MAMEFONT_DEBUG
Status decodeGlyph(const Font &font, Glyph *glyph) {
  Debugger dbg;
  return decodeGlyph(font, glyph, dbg);
}

Status decodeGlyph(const Font &font, Glyph *glyph, Debugger &dbg)
#else
Status decodeGlyph(const Font &font, Glyph *glyph)
#endif
{
  if (!glyph || !(glyph->data)) {
    MAMEFONT_THROW_OR_RETURN(Status::NULL_POINTER);
  }
  if (!glyph->isValid()) {
    MAMEFONT_THROW_OR_RETURN(Status::GLYPH_NOT_DEFINED);
  }

  DecoderContext ctx(font, glyph);
#ifdef MAMEFONT_DEBUG
  dbg.init(glyph, ctx);
#else
  Debugger dbg = 0;
#endif

  constexpr uint8_t ALT_TOP_BOTTOM_MASK =
      Glyph::UseAltTop::MASK | Glyph::UseAltBottom::MASK;
  if (glyph->flags & ALT_TOP_BOTTOM_MASK) {
    frag_t *wrptr = glyph->data;
    for (uint8_t j = 0; j < ctx.numTracks; j++) {
      for (uint8_t i = 0; i < ctx.trackLength; i++) {
        *(wrptr++) = 0x00;
      }
    }
  }

  while (ctx.cursor < ctx.endPos) {
    uint8_t inst = ctx.fetch();

    if ((inst & 0x80) == 0) {
      if ((inst & 0x40) == 0) {
        // 0x00-3F
        SFT(ctx, dbg, inst);
      } else if (((inst & 0x20) == 0) || (((inst & 0x07) != 0))) {
        // 0x40-7F except 0x60, 0x68, 0x70, 0x78
        if (inst == 0x40) {
#ifdef MAMEFONT_NO_CPX
          MAMEFONT_THROW_OR_RETURN(Status::UNKNOWN_OPCODE);
#else
          CPX(ctx, dbg, inst);
#endif
        } else {
          CPY(ctx, dbg, inst);
        }
      } else if ((inst & 0x10) == 0) {
        // 0x60, 0x68
        if ((inst & 0x08) == 0) {
          LDI(ctx, dbg, inst);
        } else {
#ifdef MAMEFONT_NO_SFI
          MAMEFONT_THROW_OR_RETURN(Status::UNKNOWN_OPCODE);
#else
          SFI(ctx, dbg, inst);
#endif
        }
      } else {
        // 0x70, 0x78
        MAMEFONT_THROW_OR_RETURN(Status::UNKNOWN_OPCODE);
      }
    } else {
      if ((inst & 0x40) == 0) {
        // 0x80-BF
        LUP(ctx, dbg, inst);
      } else if ((inst & 0x20) == 0) {
        // 0xC0-DF
        LUD(ctx, dbg, inst);
      } else if ((inst & 0x10) == 0) {
        // 0xE0-EF
        RPT(ctx, dbg, inst);
      } else {
        // 0xF0-FE
        if (inst == 0xFF) {
          MAMEFONT_THROW_OR_RETURN(Status::ABORTED_BY_ABO);
        } else {
          XOR(ctx, dbg, inst);
        }
      }
    }
  }
  return Status::SUCCESS;
}

#endif

}  // namespace mamefont
