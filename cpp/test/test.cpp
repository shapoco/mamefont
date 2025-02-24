#include <stdio.h>

#include "mamefont/mamefont.hpp"
#include "mamefont/shaposans32_fh.hpp"

int main(int argc, char** argv) {
  mfnt::Status ret;
  const auto& font = mfnt::shaposans32_fh;

  printf("font32.height: %d\n", (int)font.height);
  printf("font32.numChars: %d\n", (int)font.numChars);
  printf("font32.codeOffset: %d\n", (int)font.codeOffset);

  uint8_t maxWidth = font.getMaxWidth();
  uint8_t buff[maxWidth * font.rows];

  for (int i = 0; i < font.numChars; i++) {
    char code = font.codeOffset + i;
    mfnt::GlyphInfo glyph;
    ret = font.getGlyph(code, &glyph);
    printf("#%3d 0x%02x\n", (int)i, (int)code);
    if (ret == mfnt::Status::SUCCESS) {
      printf("  glyph.width: %d\n", glyph.width);
      ret = font.extractGlyph(&glyph, buff);
      if (ret != mfnt::Status::SUCCESS) {
        printf("  *ERROR CODE: %02x\n", (int)ret);
        return 1;
      }
      printf("  +");
      for (int x = 0; x < glyph.width * 3; x++) printf("-");
      printf("+\n");
      for (int y = 0; y < font.height; y++) {
        printf("  |");
        for (int x = 0; x < glyph.width; x++) {
          uint8_t row = (y / mfnt::SEGMENT_HEIGHT);
          uint8_t seg = buff[row * glyph.width + x];
          uint8_t bit = (seg << (y % mfnt::SEGMENT_HEIGHT)) & 0x80;
          printf("%s", bit ? "###" : "   ");
        }
        printf("|\n");
      }
      printf("  +");
      for (int x = 0; x < glyph.width * 3; x++) printf("-");
      printf("+\n");
      printf("\n");
    }
    else if (ret == mfnt::Status::GLYPH_NOT_DEFINED) {
      // ignore
    }
    else {
      printf("  *ERROR CODE: %02x\n", (int)ret);
      return 1;
    }
  }

	return 0;
}
