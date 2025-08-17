#pragma once

#ifdef MAMEFONT_EXCEPTIONS
#include <stdexcept>
#endif

#include "mamefont/blob_format.hpp"
#include "mamefont/decoder_utils.hpp"
#include "mamefont/font.hpp"
#include "mamefont/glyph.hpp"
#include "mamefont/instruction_set.hpp"
#include "mamefont/mamefont_common.hpp"

namespace mamefont {

struct DecoderContext {
  FontFlags flags;
  uint8_t numTracks;
  uint8_t trackLength;

  const frag_t *fragTable;
  const uint8_t *bytecode;
  uint8_t *data;
  const uint8_t *pc = 0;
  frag_t last = 0;
  frag_index_t cursor;
  frag_index_t endPos;

  DecoderContext(const Font &font, Glyph *glyph) {
    flags = font.header.flags;
    fragTable = font.blob + font.fragmentTableOffset();
    bytecode = font.blob + font.byteCodeOffset();
    glyph->getBufferShape(&numTracks, &trackLength);

    data = glyph->data;
    cursor = 0;
    endPos = cursor + (numTracks * trackLength);
    last = 0x00;
    pc = bytecode + glyph->entryPoint;
  }

  MAMEFONT_INLINE uint8_t fetch() { return readBlobU8(pc++); }
};

}  // namespace mamefont