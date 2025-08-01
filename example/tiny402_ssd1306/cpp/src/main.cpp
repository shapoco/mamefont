#include <avr/io.h>
#include <stdint.h>

#include <mamefont/mamefont.hpp>
#include <shapofont/ShapoSansDigitP_s16c14w02.hpp>
#include <shapofont/ShapoSansP_s12c09a01w02.hpp>
#include <tiny402/i2c.hpp>
#include <tiny402/ssd1306.hpp>

namespace mf = mamefont;

static constexpr uint8_t MAX_GLYPH_WIDTH = 32;
static constexpr uint8_t MAX_GLYPH_ROWS = 2;

static constexpr uint8_t DISP_W = 128;
static constexpr uint8_t DISP_H = 64;

I2C i2c;
ssd1306::Lcd<DISP_W, DISP_H, 0x78> display(i2c);

uint8_t buff[MAX_GLYPH_WIDTH * MAX_GLYPH_ROWS];
mf::GlyphBuffer glyphBuff;

static void drawString(const mf::Font& font, const char* str, int16_t x,
                       int8_t row) {
  int8_t glyphWidth, xSpacing;
  int8_t rows = (font.glyphHeight() + 7) / 8;
  for (const char* c = str; *c; c++) {
    mf::drawChar(font, *c, glyphBuff, &glyphWidth, &xSpacing);
    display.drawImage(glyphBuff.data, x, row, MAX_GLYPH_WIDTH, glyphWidth,
                      rows);
    display.fillRect(x + glyphWidth, row, xSpacing, rows, 0x00);
    x += glyphWidth + xSpacing;
  }
}

static void drawNumber(const mf::Font& font, uint16_t number, int16_t x,
                       int8_t row) {
  constexpr uint8_t NUM_DIGIT = 5;
  char str[NUM_DIGIT + 1];
  for (int8_t i = 0; i < NUM_DIGIT; i++) {
    str[NUM_DIGIT - i - 1] = '0' + (number % 10);
    number /= 10;
  }
  str[NUM_DIGIT] = '\0';
  drawString(font, str, x, row);
}

int main() {
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, !CLKCTRL_PEN_bm);
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKSEL_OSC20M_gc);
  _delay_ms(50);  // > 768 / 32k = 24ms

  // Setup I2C
  i2c.beginWithBaud(I2C::getBaud(F_CPU, 1000000ul, 0));

  // Setup OLED
  display.begin();
  display.fillRect(0, 0, DISP_W, DISP_H / 8, 0x00);

  // Setup GlyphBuffer
  glyphBuff.data = buff;
  glyphBuff.stride = MAX_GLYPH_WIDTH;

  const char* text1 = "The quick brown fox jumps over the lazy dog.";
  int16_t x1 = DISP_W;

  const char* text2 = "FURUIKEYA KAWAZU TOBIKOMU MIZUNOOTO";
  int16_t x2 = (int16_t)DISP_W - 200;

  uint32_t number = 0;

  // Animation
  while (1) {
    drawString(ShapoSansP_s12c09a01w02, text1, x1, 0);
    x1 -= 1;
    if (x1 < -300) x1 = DISP_W;

    drawString(ShapoSansP_s12c09a01w02, text2, x2, 2);
    x2 -= 1;
    if (x2 < -300) x2 = DISP_W;

    drawNumber(ShapoSansDigitP_s16c14w02, number++, 0, 5);
    _delay_ms(1);
  }
}
