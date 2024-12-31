#include "clock.h"
#include <ctime>
#include <glibmm/main.h>
#include <iostream>

Cairo::TextExtents Clock::s_text_extents;
bool Clock::s_size_calculated = false;
int Clock::s_cached_width = 0;
int Clock::s_cached_height = 0;
double Clock::s_cached_font_size = 0;

Clock::Clock(const ClockConfig &config) : m_config(config) {
  set_draw_func(sigc::mem_fun(*this, &Clock::on_draw));

  if (!s_size_calculated) {
    calculate_window_size(s_cached_width, s_cached_height);
    s_size_calculated = true;
  }
  set_size_request(s_cached_width, s_cached_height);

  calculate_next_update();
  Glib::signal_timeout().connect(sigc::mem_fun(*this, &Clock::on_timeout),
                                 1000);
}

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

  cr->select_font_face(m_config.font_family, Cairo::ToyFontFace::Slant::NORMAL,
                       Cairo::ToyFontFace::Weight::BOLD);

  if (s_cached_font_size == 0) {
    // Calculate the ideal font size based on window dimensions
    double font_size = std::min(width * 0.4, height * 0.6);
    cr->set_font_size(font_size);
    cr->get_text_extents(buffer, s_text_extents);

    // Adjust font size if text is too large
    double scale_w = (width * 0.8) / s_text_extents.width;
    double scale_h = (height * 0.8) / s_text_extents.height;
    double scale = std::min(scale_w, scale_h);

    if (scale < 1.0) {
      font_size *= scale;
    }

    s_cached_font_size = font_size;
  }

  cr->set_font_size(s_cached_font_size);
  cr->get_text_extents(buffer, s_text_extents);

  double x = (width - s_text_extents.width) / 2;
  double y = height / 2 + (s_text_extents.height / 2);

  cr->set_source_rgba(m_config.text_color[0], m_config.text_color[1],
                      m_config.text_color[2], m_config.text_color[3]);
  cr->move_to(x, y);
  cr->show_text(buffer);
}

void Clock::calculate_window_size(int &width, int &height) {
  auto surface =
      Cairo::ImageSurface::create(Cairo::ImageSurface::Format::ARGB32, 1, 1);
  auto cr = Cairo::Context::create(surface);

  cr->select_font_face(m_config.font_family, Cairo::ToyFontFace::Slant::NORMAL,
                       Cairo::ToyFontFace::Weight::BOLD);
  cr->set_font_size(m_config.font_size);

  Cairo::TextExtents extents;
  cr->get_text_extents("00:00", extents);

  // Calculate base dimensions with padding
  double padding = m_config.font_size * m_config.text_padding_ratio;
  width = extents.width + padding * 2;
  height = extents.height + padding * 2;

  // Ensure minimum size
  width = std::max(width, 120);
  height = std::max(height, 60);
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

void Clock::set_font_family(const std::string &family) {
  if (m_config.font_family != family) {
    m_config.font_family = family;
    invalidate_cache();
  }
}

void Clock::set_font_size(double size) {
  if (m_config.font_size != size) {
    m_config.font_size = size;
    invalidate_cache();
  }
}

void Clock::set_text_color(double r, double g, double b, double a) {
  if (m_config.text_color[0] != r || m_config.text_color[1] != g ||
      m_config.text_color[2] != b || m_config.text_color[3] != a) {
    m_config.text_color[0] = r;
    m_config.text_color[1] = g;
    m_config.text_color[2] = b;
    m_config.text_color[3] = a;
    queue_draw();
  }
}

void Clock::invalidate_cache() {
  s_size_calculated = false;
  s_cached_font_size = 0;
  queue_draw();
}
