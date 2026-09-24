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

struct WindowLayerOptions {
  bool anchor_top = false;
  bool anchor_bottom = false;
  bool anchor_left = false;
  bool anchor_right = false;
  int margin_top = 0;
  int margin_bottom = 0;
  int margin_left = 0;
  int margin_right = 0;
  int layer = 3; // 0=background, 1=bottom, 2=top, 3=overlay
};
