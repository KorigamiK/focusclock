#include "clock.h"
#include "utils.h"
#include "version.h"
#include <cairo.h>
#include <glibmm/optioncontext.h>
#include <glibmm/optionentry.h>
#include <glibmm/optiongroup.h>
#include <glibmm/ustring.h>
#include <gtk4-layer-shell/gtk4-layer-shell.h>
#include <gtkmm/application.h>
#include <gtkmm/aspectframe.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/window.h>
#include <iostream>

#define APP_ID "org.korigamik.focusclock"

struct WindowLayerOptions {
  bool anchor_top = false;
  bool anchor_bottom = false;
  bool anchor_left = false;
  bool anchor_right = false;
  int margin_top = 0;
  int margin_bottom = 0;
  int margin_left = 0;
  int margin_right = 0;
  int layer = GTK_LAYER_SHELL_LAYER_OVERLAY;
};

static void setup_window_layer(Gtk::Window *window,
                               const WindowLayerOptions &opts) {
  if (opts.layer != GTK_LAYER_SHELL_LAYER_ENTRY_NUMBER) {
    gtk_layer_init_for_window(window->gobj());
    gtk_layer_set_namespace(window->gobj(), APP_ID);
    gtk_layer_set_layer(window->gobj(), (GtkLayerShellLayer)opts.layer);
    gtk_layer_set_keyboard_mode(window->gobj(),
                                GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);
    gtk_layer_set_exclusive_zone(window->gobj(), -1);

    gtk_layer_set_anchor(window->gobj(), GTK_LAYER_SHELL_EDGE_TOP,
                         opts.anchor_top);
    gtk_layer_set_anchor(window->gobj(), GTK_LAYER_SHELL_EDGE_BOTTOM,
                         opts.anchor_bottom);
    gtk_layer_set_anchor(window->gobj(), GTK_LAYER_SHELL_EDGE_LEFT,
                         opts.anchor_left);
    gtk_layer_set_anchor(window->gobj(), GTK_LAYER_SHELL_EDGE_RIGHT,
                         opts.anchor_right);

    gtk_layer_set_margin(window->gobj(), GTK_LAYER_SHELL_EDGE_TOP,
                         opts.margin_top);
    gtk_layer_set_margin(window->gobj(), GTK_LAYER_SHELL_EDGE_BOTTOM,
                         opts.margin_bottom);
    gtk_layer_set_margin(window->gobj(), GTK_LAYER_SHELL_EDGE_LEFT,
                         opts.margin_left);
    gtk_layer_set_margin(window->gobj(), GTK_LAYER_SHELL_EDGE_RIGHT,
                         opts.margin_right);
  } else {
    window->set_decorated(true);
    window->set_resizable(true);
  }
}

int main(int argc, char **argv) {
  WindowLayerOptions opts;
  ClockConfig default_config;
  double font_size = default_config.font_size;
  Glib::ustring font_family;
  Glib::ustring color_str;
  double alpha = 0.5;
  bool show_version = false;

  Glib::OptionContext context;
  Glib::OptionGroup group("options", "Position Options");
  Glib::OptionEntry entry;

  // Add version option before other options
  entry.set_long_name("version");
  entry.set_short_name('v');
  entry.set_description("Show version information");
  group.add_entry(entry, show_version);

  entry.set_long_name("anchor-top");
  entry.set_short_name('t');
  entry.set_description("Anchor to the top edge");
  group.add_entry(entry, opts.anchor_top);

  entry.set_long_name("anchor-bottom");
  entry.set_short_name('b');
  entry.set_description("Anchor to the bottom edge");
  group.add_entry(entry, opts.anchor_bottom);

  entry.set_long_name("anchor-left");
  entry.set_short_name('l');
  entry.set_description("Anchor to the left edge");
  group.add_entry(entry, opts.anchor_left);

  entry.set_long_name("anchor-right");
  entry.set_short_name('r');
  entry.set_description("Anchor to the right edge");
  group.add_entry(entry, opts.anchor_right);

  entry.set_long_name("margin-top");
  entry.set_short_name('T');
  entry.set_description("Margin from the top edge");
  group.add_entry(entry, opts.margin_top);

  entry.set_long_name("margin-bottom");
  entry.set_short_name('B');
  entry.set_description("Margin from the bottom edge");
  group.add_entry(entry, opts.margin_bottom);

  entry.set_long_name("margin-left");
  entry.set_short_name('L');
  entry.set_description("Margin from the left edge");
  group.add_entry(entry, opts.margin_left);

  entry.set_long_name("margin-right");
  entry.set_short_name('R');
  entry.set_description("Margin from the right edge");
  group.add_entry(entry, opts.margin_right);

  entry.set_long_name("font-size");
  entry.set_short_name('f');
  entry.set_description("Base font size");
  group.add_entry(entry, font_size);

  entry.set_long_name("color");
  entry.set_short_name('c');
  entry.set_description(
      "Text color (hex format: RGB, RGBA, RRGGBB, or RRGGBBAA)");
  group.add_entry(entry, color_str);

  Glib::OptionEntry font_entry;
  font_entry.set_long_name("font-family");
  font_entry.set_short_name('F');
  font_entry.set_description("Font family name");
  group.add_entry(font_entry, font_family);

  Glib::OptionEntry color_entry;
  color_entry.set_long_name("color");
  color_entry.set_short_name('c');
  color_entry.set_description(
      "Text color (hex format: RGB, RGBA, RRGGBB, or RRGGBBAA)");
  group.add_entry(color_entry, color_str);

  entry.set_long_name("alpha");
  entry.set_short_name('a');
  entry.set_description("Text opacity (0.0-1.0, overridden by RGBA color)");
  group.add_entry(entry, alpha);

  entry.set_long_name("layer");
  entry.set_short_name('y');
  entry.set_description(
      "GTK shell layer (0=background, 1=bottom, 2=top, 3=overlay, 4=no_layer)");
  group.add_entry(entry, opts.layer);

  context.set_main_group(group);
  context.parse(argc, argv);

  if (show_version) {
    std::cout << "focusclock version " << FOCUSCLOCK_VERSION << std::endl;
    return 0;
  }

  if (opts.layer < GTK_LAYER_SHELL_LAYER_BACKGROUND ||
      opts.layer > GTK_LAYER_SHELL_LAYER_ENTRY_NUMBER) {
    opts.layer = GTK_LAYER_SHELL_LAYER_OVERLAY;
  }

  auto app = Gtk::Application::create(APP_ID);
  Gtk::Window *window = nullptr;

  app->signal_activate().connect([&]() {
    if (window == nullptr) {
      window = Gtk::make_managed<Gtk::Window>();
      app->add_window(*window);

      setup_window_layer(window, opts);

      ClockConfig config;
      if (!font_family.empty()) {
        config.font_family = font_family.raw();
      }
      config.font_size = font_size;

      if (!color_str.empty()) {
        double r, g, b, a;
        if (parse_color(color_str.raw(), r, g, b, a)) {
          config.text_color[0] = r;
          config.text_color[1] = g;
          config.text_color[2] = b;
          config.text_color[3] = a;
        }
      } else if (alpha >= 0.0 && alpha <= 1.0) {
        config.text_color[3] = alpha;
      }

      auto clock = Gtk::make_managed<Clock>(config);
      window->set_decorated(false);
      window->set_resizable(false);
      window->set_size_request(clock->get_width(), clock->get_height());
      window->set_margin(0);
      window->set_child(*clock);

      auto css_provider = Gtk::CssProvider::create();
      css_provider->load_from_data("window { background: transparent; }"
                                   "frame border { border: 0px; }");

      window->get_style_context()->add_provider(
          css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

      if (auto surface = window->get_surface()) {
        surface->set_input_region(Cairo::Region::create());
      }

      window->show();
      if (opts.layer != GTK_LAYER_SHELL_LAYER_ENTRY_NUMBER) {
        auto surface = window->get_surface();
        auto empty_region = Cairo::Region::create();
        surface->set_input_region(empty_region);
      }
    }
  });

  return app->run(argc, argv);
}
