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
  double m_radius;
  double m_line_width;
};
