#pragma once

#include "mamefont/font.hpp"
#include "mamefont/glyph_buffer.hpp"
#include "mamefont/state_machine.hpp"

namespace mamefont {

Status decodeGlyph(const Font &font, uint8_t c, const GlyphBuffer &buff,
                   Glyph *glyph);

}
