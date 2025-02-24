#pragma once

#include <stdint.h>

namespace mamefont {

class Font {
public:
  const uint8_t height;
  const uint8_t numChars;
  const uint8_t codeOffset;
  const uint8_t flags;
  const uint16_t *charTable;
  const uint8_t *segTable;
  const uint8_t *glyphData;

  Font(
    const uint8_t height, 
    const uint8_t numChars,
    const uint8_t codeOffset, 
    const uint8_t flags, 
    const uint16_t *charTable, 
    const uint8_t *segTable, 
    const uint8_t *glyphData
  ) : 
    height(height), 
    numChars(numChars), 
    codeOffset(codeOffset),
    flags(flags),
    charTable(charTable), 
    segTable(segTable), 
    glyphData(glyphData) {}
};

}
