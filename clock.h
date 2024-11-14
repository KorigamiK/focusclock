#pragma once

#include <cairomm/context.h>
#include <gtkmm/drawingarea.h>

class Clock : public Gtk::DrawingArea {
public:
  Clock();
  virtual ~Clock();

protected:
  void on_draw(const Cairo::RefPtr<Cairo::Context> &cr, int width, int height);
  bool on_timeout();

private:
  void calculate_window_size(int &width, int &height);
  void calculate_next_update();
  
  double m_radius;
  double m_line_width;
  
  // Cache variables
  static Cairo::TextExtents s_text_extents;
  static bool s_size_calculated;
  static int s_cached_width;
  static int s_cached_height;
  static double s_cached_font_size;
  
  time_t m_next_update;
};
