#include "utils.h"
#include <sstream>

bool parse_color(const std::string &color_str, double &r, double &g, double &b,
                 double &a) {
  if (color_str.empty())
    return false;

  std::string hex = color_str;
  if (hex[0] == '#')
    hex = hex.substr(1);

  // Handle different hex formats: RGB, RGBA, RRGGBB, RRGGBBAA
  unsigned int color;
  std::istringstream iss(hex);
  iss >> std::hex >> color;

  switch (hex.length()) {
  case 3: // RGB
    r = ((color >> 8) & 0xF) / 15.0;
    g = ((color >> 4) & 0xF) / 15.0;
    b = (color & 0xF) / 15.0;
    a = 1.0;
    break;
  case 4: // RGBA
    r = ((color >> 12) & 0xF) / 15.0;
    g = ((color >> 8) & 0xF) / 15.0;
    b = ((color >> 4) & 0xF) / 15.0;
    a = (color & 0xF) / 15.0;
    break;
  case 6: // RRGGBB
    r = ((color >> 16) & 0xFF) / 255.0;
    g = ((color >> 8) & 0xFF) / 255.0;
    b = (color & 0xFF) / 255.0;
    a = 1.0;
    break;
  case 8: // RRGGBBAA
    r = ((color >> 24) & 0xFF) / 255.0;
    g = ((color >> 16) & 0xFF) / 255.0;
    b = ((color >> 8) & 0xFF) / 255.0;
    a = (color & 0xFF) / 255.0;
    break;
  default:
    return false;
  }
  return true;
}
