#pragma once

#include <string>

struct ClockConfig {
  // Font settings
  std::string font_family = "Sans";
  double font_size = 32.0;
  double text_padding_ratio = 0.5;

  // Colors
  double text_color[4] = {1.0, 1.0, 1.0, 0.5};
};
