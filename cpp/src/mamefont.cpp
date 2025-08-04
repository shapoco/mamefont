#include <mamefont/mamefont.hpp>

namespace mamefont {

Status extractGlyph(const Font &font, uint8_t c, const GlyphBuffer &buff,
                GlyphDimensions *dims) {
  Glyph glyph;
  Status status = font.getGlyph(c, &glyph);
  if (status != Status::SUCCESS) return status;

  if (!glyph.isValid()) return Status::GLYPH_NOT_DEFINED;

  if (dims) glyph.getDimensions(dims);

  StateMachine stm(font);
  return stm.run(glyph, buff);
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
    case Operator::SFI:  return "SFI";
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
