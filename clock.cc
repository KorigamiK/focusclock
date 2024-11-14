#include "clock.h"
#include "config.h"
#include <cairo/cairo.h>
#include <cairomm/context.h>
#include <cmath>
#include <ctime>
#include <glibmm/main.h>

// static members
Cairo::TextExtents Clock::s_text_extents;
bool Clock::s_size_calculated = false;
int Clock::s_cached_width = 0;
int Clock::s_cached_height = 0;
double Clock::s_cached_font_size = 0;

Clock::Clock() : m_radius(0.42), m_line_width(0.035) {
  set_draw_func(sigc::mem_fun(*this, &Clock::on_draw));

  // Calculate initial window size only once
  if (!s_size_calculated) {
    calculate_window_size(s_cached_width, s_cached_height);
    s_size_calculated = true;
  }
  set_size_request(s_cached_width, s_cached_height);

  // Start timer
  calculate_next_update();
  Glib::signal_timeout().connect(sigc::mem_fun(*this, &Clock::on_timeout), 1000);
}

Clock::~Clock() {}

void Clock::calculate_next_update() {
  time(&m_next_update);
  m_next_update += 60 - (m_next_update % 60); // Next minute
}

void Clock::on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int width,
                    int height) {
  cr->set_operator(Cairo::Context::Operator::CLEAR);
  cr->paint();
  cr->set_operator(Cairo::Context::Operator::OVER);

  time_t rawtime;
  time(&rawtime);
  struct tm *timeinfo = localtime(&rawtime);

  static char buffer[6];
  strftime(buffer, sizeof(buffer), "%I:%M", timeinfo);

  if (s_cached_font_size == 0) {
    // Calculate font size once
    double font_size = height * 0.8;
    cr->set_font_size(font_size);
    cr->get_text_extents(buffer, s_text_extents);

    if (s_text_extents.width > width * 0.9) {
      font_size *= (width * 0.9) / s_text_extents.width;
    }
    s_cached_font_size = font_size;
  }

  cr->set_font_size(s_cached_font_size);
  cr->select_font_face(FONT_FAMILY, Cairo::ToyFontFace::Slant::NORMAL,
                       Cairo::ToyFontFace::Weight::BOLD);
  cr->set_source_rgba(TEXT_COLOR_R, TEXT_COLOR_G, TEXT_COLOR_B, TEXT_COLOR_A);
  cr->move_to((width - s_text_extents.width) / 2 - s_text_extents.x_bearing,
              (height - s_text_extents.height) / 2 - s_text_extents.y_bearing);
  cr->show_text(buffer);
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
  time_t now;
  time(&now);

  if (now >= m_next_update) {
    calculate_next_update();
    queue_draw();
  }

  return true;
}
