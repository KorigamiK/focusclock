#include "clock.h"
#include "config.h"
#include <cairo/cairo.h>
#include <cairomm/context.h>
#include <cmath>
#include <ctime>
#include <glibmm/main.h>

Clock::Clock() : m_radius(0.42), m_line_width(0.035) {
  set_draw_func(sigc::mem_fun(*this, &Clock::on_draw));

  Glib::signal_timeout().connect(sigc::mem_fun(*this, &Clock::on_timeout),
                                 2000);

  int width = 0, height = 0;
  calculate_window_size(width, height);
  set_size_request(width, height);
}

Clock::~Clock() {}

void Clock::on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int width,
                    int height) {
  cr->set_operator(Cairo::Context::Operator::CLEAR);
  cr->paint();
  cr->set_operator(Cairo::Context::Operator::OVER);

  time_t rawtime;
  time(&rawtime);
  struct tm *timeinfo = localtime(&rawtime);

  char buffer[6];
  strftime(buffer, sizeof(buffer), "%I:%M", timeinfo);
  std::string time_text = buffer;

  cr->select_font_face(FONT_FAMILY, Cairo::ToyFontFace::Slant::NORMAL,
                       Cairo::ToyFontFace::Weight::BOLD);

  double font_size = height * 0.8;
  cr->set_font_size(font_size);

  Cairo::TextExtents extents;
  cr->get_text_extents(time_text, extents);

  if (extents.width > width * 0.9) {
    font_size *= (width * 0.9) / extents.width;
    cr->set_font_size(font_size);
    cr->get_text_extents(time_text, extents);
  }

  cr->set_source_rgba(TEXT_COLOR_R, TEXT_COLOR_G, TEXT_COLOR_B, TEXT_COLOR_A);

  cr->move_to((width - extents.width) / 2 - extents.x_bearing,
              (height - extents.height) / 2 - extents.y_bearing);
  cr->show_text(time_text);
}

void Clock::calculate_window_size(int &width, int &height) {
  auto surface =
      Cairo::ImageSurface::create(Cairo::ImageSurface::Format::ARGB32, 1, 1);
  auto cr = Cairo::Context::create(surface);

  cr->select_font_face(FONT_FAMILY, Cairo::ToyFontFace::Slant::NORMAL,
                       Cairo::ToyFontFace::Weight::BOLD);

  double initial_font_size = BASE_FONT_SIZE * FONT_SIZE_MULTIPLIER;
  cr->set_font_size(initial_font_size);

  Cairo::TextExtents extents;
  cr->get_text_extents("00:00", extents);

  double padding = extents.height * TEXT_PADDING_RATIO;
  width = extents.width + padding * 2;
  height = extents.height + padding * 2;

  width = std::min(std::max((int)width, MIN_WINDOW_WIDTH), MAX_WINDOW_WIDTH);
  height =
      std::min(std::max((int)height, MIN_WINDOW_HEIGHT), MAX_WINDOW_HEIGHT);
}

bool Clock::on_timeout() {
  queue_draw();
  return true;
}
