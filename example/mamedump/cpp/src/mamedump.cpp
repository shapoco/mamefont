#include <stdio.h>

#define MAMEFONT_INCLUDE_IMPL
#define MAMEFONT_INCLUDE_AS_STATIC
#include "mamefont/mamefont.hpp"
#undef MAMEFONT_INCLUDE_IMPL

#include "target_font.hpp"

int main(int argc, char** argv) {
  mamefont::Status ret;
  const auto& font = TARGET_FONT_NAME;

  int glyphHeight = font.glyphHeight();
  int numChars = font.numGlyphs();
  int codeOffset = font.firstCode();
  bool verticalFragment = font.verticalFragment();

  printf("font32.glyphHeight()      : %d\n", glyphHeight);
  printf("font32.glyphTableLen()    : %d\n", numChars);
  printf("font32.firstCode()        : %d\n", codeOffset);
  printf("font32.verticalFragment() : %d\n", verticalFragment);
  fflush(stdout);

  int lutSize = font.lutSize();
  printf("LUT (size=%1d):\n", lutSize);
  for (int i = 0; i < lutSize; i++) {
    mamefont::fragment_t frag = font.getLutEntry(i);
    printf("  %2d: 0x%02X ", i, (int)frag);
    uint8_t tmp = frag;
    for (int ibit = 0; ibit < 8; ibit++) {
      printf((tmp & 0x80) ? " ##" : " ..");
      tmp <<= 1;
    }
    printf("\n");
  }

  uint8_t stride;
  uint8_t buff[font.getRequiredGlyphBufferSize(&stride) * 2];

  constexpr int X_ZOOM = 3;

  for (int i = 0; i < numChars; i++) {
    char code = codeOffset + i;
    mamefont::Glyph glyph;
    ret = font.getGlyph(code, &glyph);
    printf("#%-3d", (int)i);
    if (0x20 <= code && code <= 0x7e) {
      printf(" '%c'", code);
    }
    printf(" (0x%02X)\n", (int)code);
    if (ret == mamefont::Status::SUCCESS) {
      int glyphWidth = glyph.width();

      mamefont::GlyphBuffer glyphBuff;
      glyphBuff.data = buff;
      glyphBuff.stride = stride;

      mamefont::StateMachine stm(font);
      ret = stm.run(glyph, glyphBuff);
      if (ret != mamefont::Status::SUCCESS) {
        printf("  *ERROR CODE: 0x%02X (lastInstByte1=0x%02X)\n", (int)ret,
               (int)stm.lastInstByte1);
        return 1;
      }

      printf(" _|");
      for (int x = 0; x < glyphWidth * X_ZOOM; x++) printf("_");
      printf("|_\n");
      for (int y = 0; y < glyphHeight; y++) {
        printf(((y % 8) == 7) ? " _|" : "  |");
        for (int x = 0; x < glyphWidth; x++) {
          uint8_t bit;

          int iBit;
          int iBuff;
          bool firstOfFragment = 0;

          if (verticalFragment) {
            uint8_t row = (y / mamefont::FRAGMENT_SIZE);
            iBuff = row * stride + x;
            iBit = y % mamefont::FRAGMENT_SIZE;
            bit = (buff[iBuff] >> iBit) & 0x1;
          } else {
            uint8_t col = (x / mamefont::FRAGMENT_SIZE);
            iBuff = y * stride + col;
            iBit = x % mamefont::FRAGMENT_SIZE;
            bit = (buff[iBuff] >> iBit) & 0x1;
          }

          auto opr = stm.dbgOpLog[iBuff];
          int nw = X_ZOOM;
          if (iBit == 0 && opr != mamefont::Operator::NONE) {
            nw -= printf("%-3s", mamefont::getMnemonic(opr));
          }

          for (int z = 0; z < nw; z++) {
            printf("%s", (bit && z < nw - 1) ? "#" : " ");
          }
        }
        printf(((y % 8) == 7) ? "|_\n" : "|\n");
      }
      printf("  +");
      for (int x = 0; x < glyphWidth * X_ZOOM; x++) printf("-");
      printf("+\n");
      printf("\n");
    } else if (ret == mamefont::Status::GLYPH_NOT_DEFINED) {
      printf("  GLYPH_NOT_DEFINED\n");
      printf("\n");
      // ignore
    } else {
      printf("  *ERROR CODE: 0x%02X\n", (int)ret);
      return 1;
    }
  }

  return 0;
}
