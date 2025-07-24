#include <mamefont/mamefont.hpp>

namespace mamefont {

Status drawChar(const Font &font, uint8_t c, const GlyphBuffer &buff,
                int8_t *glyphWidth, int8_t *xAdvance) {
  Glyph glyph;
  Status status = font.getGlyph(c, &glyph);
  if (status != Status::SUCCESS) return status;

  if (!glyph.isValid()) return Status::GLYPH_NOT_DEFINED;

  if (glyphWidth) *glyphWidth = glyph.width();
  if (xAdvance) *xAdvance = glyph.xAdvance();

  Renderer renderer(font);
  return renderer.render(glyph, buff);
}

}  // namespace mamefont
