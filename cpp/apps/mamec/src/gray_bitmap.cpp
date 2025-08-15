#include <stdexcept>

#include "mamec/gray_bitmap.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace mamefont::mamec {

static void rgb2hsv(int r, int g, int b, int *h, int *s, int *v);
static frag_t encodePixel(int16_t value, int iPixel, mf::PixelFormat bpp);

GrayBitmapClass::GrayBitmapClass(std::vector<int16_t> &data, int width,
                                 int height)
    : data(data), width(width), height(height) {}

GrayBitmapClass::GrayBitmapClass(const std::string &path) {
  int channels;
  uint8_t *image = stbi_load(path.c_str(), &width, &height, &channels, 4);
  if (!image) {
    throw std::runtime_error("Failed to load image: " + path);
  }

  data.resize(width * height);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int iDest = y * width + x;
      int iSrc = (y * width + x) * 4;

      int h, s, v;
      rgb2hsv(image[iSrc + 0], image[iSrc + 1], image[iSrc + 2], &h, &s, &v);

      bool accept = false;
      if (s < 64) {
        data[iDest] = v;
        accept = true;
      } else if (s > 128 && v > 128) {
        if (h < 30 || 330 <= h) {
          // red
          data[iDest] = MARKER_GLYPH;
          accept = true;
        } else if (210 < h && h <= 270) {
          // blue
          data[iDest] = MARKER_SPACING;
          accept = true;
        }
      }

      if (!accept) {
        stbi_image_free(image);
        image = nullptr;
        throw std::runtime_error("Unsupported color in image: " + path +
                                 " at (" + std::to_string(x) + ", " +
                                 std::to_string(y) + ")");
      }
    }
  }

  if (image) stbi_image_free(image);
}

int16_t GrayBitmapClass::get(int x, int y, int16_t defaultColor) const {
  if (0 <= x && x < width && 0 <= y && y < height) {
    return data[y * width + x];
  } else if (defaultColor >= 0) {
    return defaultColor;
  } else {
    throw std::out_of_range("Coordinates out of bounds");
  }
}

uint8_t GrayBitmapClass::get(int x, int y, mf::PixelFormat fmt,
                             int16_t defaultColor) const {
  return encodePixel(get(x, y, defaultColor), 0, fmt);
}

std::shared_ptr<GrayBitmapClass> GrayBitmapClass::crop(int x, int y, int w,
                                                       int h) const {
  if (x < 0 || y < 0 || x + w > width || y + h > height) {
    throw std::out_of_range("Crop area out of bounds");
  }

  std::vector<int16_t> cropped;
  cropped.resize(w * h);
  for (int j = 0; j < h; ++j) {
    const int16_t *srcPtr = &data[(y + j) * width + x];
    int16_t *destPtr = &cropped[j * w];
    std::copy(srcPtr, srcPtr + w, destPtr);
  }

  return std::make_shared<GrayBitmapClass>(cropped, w, h);
}

std::vector<frag_t> GrayBitmapClass::toFragments(bool verticalFrag,
                                                 bool farPixelFirst,
                                                 mf::PixelFormat bpp) const {
  int ppf = mf::getPixelsPerFrag(bpp);

  std::vector<frag_t> frags;
  if (verticalFrag) {
    frags.resize(width * ((height + (ppf - 1)) / ppf));

    for (int yCoarse = 0; yCoarse < height; yCoarse += ppf) {
      frag_t *destPtr = &frags[yCoarse / ppf * width];
      for (int x = 0; x < width; ++x) {
        frag_t frag = 0;
        for (int yFine = 0; yFine < ppf; ++yFine) {
          int y = yCoarse + yFine;
          int iPixel = farPixelFirst ? (ppf - 1 - yFine) : yFine;
          frag |= encodePixel(get(x, y, 0), iPixel, bpp);
        }
        *(destPtr++) = frag;
      }
    }
  } else {
    frags.resize(height * ((width + (ppf - 1)) / ppf));
    for (int xCoarse = 0; xCoarse < width; xCoarse += ppf) {
      frag_t *destPtr = &frags[xCoarse / ppf * height];
      for (int y = 0; y < height; ++y) {
        frag_t frag = 0;
        for (int xFine = 0; xFine < ppf; ++xFine) {
          int x = xCoarse + xFine;
          int iPixel = farPixelFirst ? (ppf - 1 - xFine) : xFine;
          frag |= encodePixel(get(x, y, 0), iPixel, bpp);
        }
        *(destPtr++) = frag;
      }
    }
  }
  return std::move(frags);
}

static void rgb2hsv(int r, int g, int b, int *h, int *s, int *v) {
  int mx = std::max(r, std::max(g, b));
  int mn = std::min(r, std::min(g, b));
  int dr = mx - mn;
  if (mx == mn) {
    *h = 0;
  } else if (mn == b) {
    *h = 60 + (g - r) * 60 / dr;
  } else if (mn == r) {
    *h = 180 + (b - g) * 60 / dr;
  } else if (mn == g) {
    *h = 300 + (r - b) * 60 / dr;
  }
  *s = dr;
  *v = mx;
}

static frag_t encodePixel(int16_t value, int iPixel, mf::PixelFormat bpp) {
  if (bpp == mf::PixelFormat::BW_1BIT) {
    return (value >= 128) ? (1 << iPixel) : 0;
  } else if (bpp == mf::PixelFormat::GRAY_2BIT) {
    value = (value < 64) ? 0 : (value < 160) ? 1 : (value < 224) ? 2 : 3;
    return value << (iPixel * 2);
  } else {
    throw std::invalid_argument("Unsupported bits per pixel");
  }
}

}  // namespace mamefont::mamec