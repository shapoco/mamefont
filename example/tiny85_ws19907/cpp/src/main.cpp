#include <stdint.h>

// clang-format off
//        RESETn dW PCINT5 ADC0 PB5 -|￣￣￣￣|- VCC
// XTAL1 CLKI OC1Bn PCINT3 ADC3 PB3 -|　　　　|- PB2 ADC1      PCINT2 SCK USCK SCL T0 INT0
// XTAL2 CLKO OC1B  PCINT4 ADC2 PB4 -|　　　　|- PB1      AIN1 PCINT1 MISO DO     OC0B OC1A
//                              GND -|＿＿＿＿|- PB0 AREF AIN0 PCINT0 MOSI DI SDA OC0A OC1An
// clang-format on

#ifndef F_CPU
#define F_CPU 20000000UL
#endif

#include <avr/pgmspace.h>

#include <tiny85/ili9488.hpp>
#include <tiny85/spi.hpp>

#include <mamefont/mamefont.hpp>

#include <font/MameSansP_s48c40w08.hpp>

namespace mf = mamefont;

static constexpr uint8_t MAX_GLYPH_COLS = (40 + 7) / 8;
static constexpr uint8_t MAX_GLYPH_HEIGHT = 48;

static constexpr uint16_t DISP_W = 480;
static constexpr uint16_t DISP_H = 320;
static constexpr int8_t PORT_DISP_CS = 3;
static constexpr int8_t PORT_DISP_DC = 4;
static constexpr int8_t PORT_DISP_RST = -1;

using coord_t = ili9488::coord_t;

SPI spi;
ili9488::Display<PORT_DISP_CS, PORT_DISP_DC, PORT_DISP_RST> display(spi);

uint8_t buff[MAX_GLYPH_COLS * MAX_GLYPH_HEIGHT];
mf::GlyphBuffer glyphBuff;

const char MSG_LINE0[] PROGMEM = "8kByte ROM and 512";
const char MSG_LINE1[] PROGMEM = "Byte RAM of ATtiny";
const char MSG_LINE2[] PROGMEM = "85 are enough to fill a";
const char MSG_LINE3[] PROGMEM = "480x320px SPI-LCD";
const char MSG_LINE4[] PROGMEM = "with lots of large 48px";
const char MSG_LINE5[] PROGMEM = "high characters.";
const char* MSG_LINES[] = {
    MSG_LINE0, MSG_LINE1, MSG_LINE2, MSG_LINE3, MSG_LINE4, MSG_LINE5,
};

static void drawString(const mf::Font& font, const char* str, coord_t x,
                       coord_t y, uint16_t fgColor, uint16_t bgColor) {
  int8_t glyphWidth, xSpacing;
  int8_t glyphHeight = font.glyphHeight();
  for (const char* c = str; *c; c++) {
    mf::drawChar(font, *c, glyphBuff, &glyphWidth, &xSpacing);
    xSpacing -= 1;
    display.drawMonoImage(glyphBuff.data, MAX_GLYPH_COLS, x, y, glyphWidth,
                          glyphHeight, fgColor, bgColor);
    display.fillRect(x + glyphWidth, y, xSpacing, glyphHeight, bgColor);
    x += glyphWidth + xSpacing;
  }
}

static void drawNumber(const mf::Font& font, uint16_t number, coord_t x,
                       coord_t y, uint16_t fgColor, uint16_t bgColor) {
  constexpr uint8_t NUM_DIGIT = 5;
  char str[NUM_DIGIT + 1];
  for (int8_t i = 0; i < NUM_DIGIT; i++) {
    str[NUM_DIGIT - i - 1] = '0' + (number % 10);
    number /= 10;
  }
  str[NUM_DIGIT] = '\0';
  drawString(font, str, x, y, fgColor, bgColor);
}

int main() {
  spi.init();
  delayMs(1000);
  display.init();

  // Setup OLED
  display.init();
  display.clear(0xFFFF);

  // Setup GlyphBuffer
  glyphBuff.data = buff;
  glyphBuff.stride = MAX_GLYPH_COLS;

  // Show Message
  for (int8_t i = 0; i < 6; i++) {
    char buff[24];
    char c;
    char* wp = buff;
    const char* rp = MSG_LINES[i];
    while ((c = pgm_read_byte(rp++)) != '\0') {
      *(wp++) = c;
    }
    *wp = '\0';
    drawString(MameSansP_s48c40w08, buff, 8, 8 + i * 52, 0x0000, 0xc618);
  }

  while (1) {
    asm volatile("nop");
  }
}
