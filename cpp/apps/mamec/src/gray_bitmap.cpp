#include <stdexcept>

#include "mamec/gray_bitmap.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace mamefont::mamec {

static void rgb2hsv(int r, int g, int b, int *h, int *s, int *v);

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

std::vector<fragment_t> GrayBitmapClass::toFragments(bool verticalFrag,
                                                     bool msb1st) const {
  std::vector<fragment_t> frags;
  if (verticalFrag) {
    frags.resize(width * ((height + 7) / 8));

    for (int y_coarse = 0; y_coarse < height; y_coarse += 8) {
      fragment_t *destPtr = &frags[y_coarse / 8 * width];
      for (int x = 0; x < width; ++x) {
        fragment_t frag = 0;
        for (int y_fine = 0; y_fine < 8; ++y_fine) {
          int y = y_coarse + y_fine;
          int i_bit = msb1st ? (7 - y_fine) : y_fine;
          if (get(x, y, 0) >= 128) {
            frag |= (1 << i_bit);
          }
        }
        *(destPtr++) = frag;
      }
    }
  } else {
    frags.resize(height * ((width + 7) / 8));
    for (int x_coarse = 0; x_coarse < width; x_coarse += 8) {
      fragment_t *destPtr = &frags[x_coarse / 8 * height];
      for (int y = 0; y < height; ++y) {
        fragment_t frag = 0;
        for (int x_fine = 0; x_fine < 8; ++x_fine) {
          int x = x_coarse + x_fine;
          int i_bit = msb1st ? (7 - x_fine) : x_fine;
          if (get(x, y, 0) >= 128) {
            frag |= (1 << i_bit);
          }
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

}  // namespace mamefont::mamec