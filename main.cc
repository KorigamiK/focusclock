#include "clock.h"
#include <gtk4-layer-shell/gtk4-layer-shell.h>
#include <gtkmm/application.h>
#include <gtkmm/aspectframe.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/window.h>
#include <glibmm/optioncontext.h>
#include <glibmm/optiongroup.h>
#include <glibmm/optionentry.h>

#define APP_ID "org.korigamik.focusclock"

int main(int argc, char **argv) {
  bool anchor_top = false;
  bool anchor_bottom = false;
  bool anchor_left = false;
  bool anchor_right = false;
  int margin_top = 0;
  int margin_bottom = 0;
  int margin_left = 0;
  int margin_right = 0;

  Glib::OptionContext context;
  Glib::OptionGroup group("options", "Position Options");
  Glib::OptionEntry entry;

  entry.set_long_name("anchor-top");
  entry.set_short_name('t');
  entry.set_description("Anchor to the top edge");
  group.add_entry(entry, anchor_top);

  entry.set_long_name("anchor-bottom");
  entry.set_short_name('b');
  entry.set_description("Anchor to the bottom edge");
  group.add_entry(entry, anchor_bottom);

  entry.set_long_name("anchor-left");
  entry.set_short_name('l');
  entry.set_description("Anchor to the left edge");
  group.add_entry(entry, anchor_left);

  entry.set_long_name("anchor-right");
  entry.set_short_name('r');
  entry.set_description("Anchor to the right edge");
  group.add_entry(entry, anchor_right);

  entry.set_long_name("margin-top");
  entry.set_short_name('T');
  entry.set_description("Margin from the top edge");
  group.add_entry(entry, margin_top);

  entry.set_long_name("margin-bottom");
  entry.set_short_name('B');
  entry.set_description("Margin from the bottom edge");
  group.add_entry(entry, margin_bottom);

  entry.set_long_name("margin-left");
  entry.set_short_name('L');
  entry.set_description("Margin from the left edge");
  group.add_entry(entry, margin_left);

  entry.set_long_name("margin-right");
  entry.set_short_name('R');
  entry.set_description("Margin from the right edge");
  group.add_entry(entry, margin_right);

  context.set_main_group(group);
  context.parse(argc, argv);

  auto app = Gtk::Application::create(APP_ID);

  app->signal_activate().connect([&]() {
    auto window = new Gtk::Window();
    gtk_layer_init_for_window(window->gobj());
    gtk_layer_set_namespace(window->gobj(), APP_ID);
    gtk_layer_set_layer(window->gobj(), GTK_LAYER_SHELL_LAYER_OVERLAY);
    gtk_layer_set_keyboard_mode(
        window->gobj(),
        GtkLayerShellKeyboardMode::GTK_LAYER_SHELL_KEYBOARD_MODE_NONE);
    gtk_layer_set_exclusive_zone(window->gobj(), -1);

    gtk_layer_set_anchor(window->gobj(), GTK_LAYER_SHELL_EDGE_TOP, anchor_top);
    gtk_layer_set_anchor(window->gobj(), GTK_LAYER_SHELL_EDGE_BOTTOM, anchor_bottom);
    gtk_layer_set_anchor(window->gobj(), GTK_LAYER_SHELL_EDGE_LEFT, anchor_left);
    gtk_layer_set_anchor(window->gobj(), GTK_LAYER_SHELL_EDGE_RIGHT, anchor_right);

    gtk_layer_set_margin(window->gobj(), GTK_LAYER_SHELL_EDGE_TOP, margin_top);
    gtk_layer_set_margin(window->gobj(), GTK_LAYER_SHELL_EDGE_BOTTOM, margin_bottom);
    gtk_layer_set_margin(window->gobj(), GTK_LAYER_SHELL_EDGE_LEFT, margin_left);
    gtk_layer_set_margin(window->gobj(), GTK_LAYER_SHELL_EDGE_RIGHT, margin_right);

    window->set_application(app);

    auto clock = Gtk::make_managed<Clock>();
    window->set_child(*clock);

    auto css_provider = Gtk::CssProvider::create();
    css_provider->load_from_data("window { background: transparent; }"
                                 "frame border { border: 0px; }");

    window->get_style_context()->add_provider(
        css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    window->show();
  });

  return app->run(argc, argv);
}
