#include <mamefont/mamefont.hpp>

namespace mamefont {

Status drawChar(const Font &font, uint8_t c, const GlyphBuffer &buff,
                int8_t *glyphWidth, int8_t *xSpacing) {
  Glyph glyph;
  Status status = font.getGlyph(c, &glyph);
  if (status != Status::SUCCESS) return status;

  if (!glyph.isValid()) return Status::GLYPH_NOT_DEFINED;

  glyph.getDimensions(glyphWidth, xSpacing);

  Renderer renderer(font);
  return renderer.render(glyph, buff);
}

#ifdef MAMEFONT_DEBUG
const char *getMnemonic(Operator op) {
  // clang-format off
  switch (op) {
    case Operator::NONE: return "(None)";
    case Operator::RPT:  return "RPT";
    case Operator::CPY:  return "CPY";
    case Operator::XOR:  return "XOR";
    case Operator::SFT:  return "SFT";
    case Operator::LUP:  return "LUP";
    case Operator::LUD:  return "LUD";
    case Operator::LDI:  return "LDI";
    case Operator::CPX:  return "CPX";
    default:             return "(Unknown)";
  }
  // clang-format on
}
#endif

}  // namespace mamefont
