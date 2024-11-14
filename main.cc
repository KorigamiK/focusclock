#include "clock.h"
#include <gtk4-layer-shell/gtk4-layer-shell.h>
#include <gtkmm/application.h>
#include <gtkmm/aspectframe.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/window.h>

#define APP_ID "org.korigamik.focusclock"

int main(int argc, char **argv) {
  auto app = Gtk::Application::create(APP_ID);

  app->signal_activate().connect([&]() {
    auto window = new Gtk::Window();
    // gtk_layer_init_for_window(g_window->gobj());
    // gtk_layer_set_layer(g_window->gobj(),
    //                     GtkLayerShellLayer::GTK_LAYER_SHELL_LAYER_TOP
    //                     );
    // gtk_layer_set_exclusive_zone(g_window->gobj(), 0);
    window->set_application(app);

    // Configure the window
    window->set_resizable(false);
    window->set_can_focus(false);
    window->set_decorated(false);
    window->set_sensitive(false);
    window->set_can_target(false);

    // Create and add the clock
    auto clock = Gtk::make_managed<Clock>();
    window->set_child(*clock);

    // Set up CSS
    auto css_provider = Gtk::CssProvider::create();
    css_provider->load_from_data("window { background: transparent; }"
                                "frame border { border: 0px; }");

    window->get_style_context()->add_provider(
        css_provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    window->show();
  });

  return app->run(argc, argv);
}
