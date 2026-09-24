// Native Linux frontend: a wlr-layer-shell surface drawn with Cairo into a
// shared-memory buffer. Works on compositors that implement
// zwlr_layer_shell_v1 (Hyprland, Sway, river, niri, KDE Plasma, ...).

#include "options.h"
#include <wayland-client.h>
// The protocol names an argument `namespace`, a C++ keyword.
#define namespace namespace_
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#undef namespace
#include <algorithm>
#include <cairo.h>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <poll.h>
#include <sys/mman.h>
#include <sys/timerfd.h>
#include <unistd.h>

namespace {

struct Buffer {
  wl_buffer *wl = nullptr;
  void *data = nullptr;
  size_t size = 0;
  int width = 0;
  int height = 0;
  bool busy = false;
};

struct App {
  ClockConfig config;
  WindowLayerOptions opts;

  wl_display *display = nullptr;
  wl_compositor *compositor = nullptr;
  uint32_t compositor_version = 0;
  wl_shm *shm = nullptr;
  zwlr_layer_shell_v1 *layer_shell = nullptr;
  wl_surface *surface = nullptr;
  zwlr_layer_surface_v1 *layer_surface = nullptr;

  Buffer buffers[2];
  int width = 0; // surface-local size
  int height = 0;
  int scale = 1;
  bool configured = false;
  bool running = true;
};

void buffer_release(void *data, wl_buffer *) {
  static_cast<Buffer *>(data)->busy = false;
}

const wl_buffer_listener buffer_listener = {buffer_release};

void destroy_buffer(Buffer &buf) {
  if (buf.wl)
    wl_buffer_destroy(buf.wl);
  if (buf.data)
    munmap(buf.data, buf.size);
  buf = Buffer();
}

bool create_buffer(App &app, Buffer &buf, int width, int height) {
  int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
  size_t size = static_cast<size_t>(stride) * height;

  int fd = memfd_create("focusclock", MFD_CLOEXEC);
  if (fd < 0 || ftruncate(fd, size) < 0) {
    perror("focusclock: shm buffer");
    if (fd >= 0)
      close(fd);
    return false;
  }

  void *data = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
    perror("focusclock: mmap");
    close(fd);
    return false;
  }

  wl_shm_pool *pool = wl_shm_create_pool(app.shm, fd, size);
  buf.wl = wl_shm_pool_create_buffer(pool, 0, width, height, stride,
                                     WL_SHM_FORMAT_ARGB8888);
  wl_shm_pool_destroy(pool);
  close(fd);

  buf.data = data;
  buf.size = size;
  buf.width = width;
  buf.height = height;
  buf.busy = false;
  wl_buffer_add_listener(buf.wl, &buffer_listener, &buf);
  return true;
}

// Returns a free buffer of the given pixel size, (re)allocating as needed.
Buffer *acquire_buffer(App &app, int width, int height) {
  for (Buffer &buf : app.buffers) {
    if (buf.busy)
      continue;
    if (buf.wl && (buf.width != width || buf.height != height))
      destroy_buffer(buf);
    if (!buf.wl && !create_buffer(app, buf, width, height))
      return nullptr;
    return &buf;
  }
  return nullptr;
}

void select_font(cairo_t *cr, const ClockConfig &config) {
  cairo_select_font_face(cr, config.font_family.c_str(),
                         CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(cr, config.font_size);
}

// Surface size: ink extents of "00:00" plus padding, like the macOS build.
void measure(const ClockConfig &config, int &width, int &height) {
  cairo_surface_t *surface =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
  cairo_t *cr = cairo_create(surface);
  select_font(cr, config);

  cairo_text_extents_t ext;
  cairo_text_extents(cr, "00:00", &ext);
  cairo_destroy(cr);
  cairo_surface_destroy(surface);

  double padding = config.font_size * config.text_padding_ratio;
  width = std::max(static_cast<int>(ext.width + padding * 2), 120);
  height = std::max(static_cast<int>(ext.height + padding * 2), 60);
}

void draw(App &app) {
  if (!app.configured)
    return;

  int pw = app.width * app.scale;
  int ph = app.height * app.scale;
  Buffer *buf = acquire_buffer(app, pw, ph);
  if (!buf)
    return;

  time_t now = time(nullptr);
  char text[6];
  strftime(text, sizeof(text), "%I:%M", localtime(&now));

  cairo_surface_t *surface = cairo_image_surface_create_for_data(
      static_cast<unsigned char *>(buf->data), CAIRO_FORMAT_ARGB32, pw, ph,
      cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, pw));
  cairo_t *cr = cairo_create(surface);

  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_scale(cr, app.scale, app.scale);

  select_font(cr, app.config);
  cairo_text_extents_t ext;
  cairo_text_extents(cr, text, &ext);
  cairo_move_to(cr, (app.width - ext.width) / 2 - ext.x_bearing,
                (app.height - ext.height) / 2 - ext.y_bearing);

  const double *c = app.config.text_color;
  cairo_set_source_rgba(cr, c[0], c[1], c[2], c[3]);
  cairo_show_text(cr, text);

  cairo_destroy(cr);
  cairo_surface_flush(surface);
  cairo_surface_destroy(surface);

  if (app.compositor_version >= WL_SURFACE_SET_BUFFER_SCALE_SINCE_VERSION)
    wl_surface_set_buffer_scale(app.surface, app.scale);
  wl_surface_attach(app.surface, buf->wl, 0, 0);
  if (app.compositor_version >= WL_SURFACE_DAMAGE_BUFFER_SINCE_VERSION)
    wl_surface_damage_buffer(app.surface, 0, 0, pw, ph);
  else
    wl_surface_damage(app.surface, 0, 0, app.width, app.height);
  buf->busy = true;
  wl_surface_commit(app.surface);
}

void surface_enter(void *, wl_surface *, wl_output *) {}
void surface_leave(void *, wl_surface *, wl_output *) {}

#ifdef WL_SURFACE_PREFERRED_BUFFER_SCALE_SINCE_VERSION
void surface_preferred_buffer_scale(void *data, wl_surface *, int32_t factor) {
  App &app = *static_cast<App *>(data);
  if (factor > 0 && factor != app.scale) {
    app.scale = factor;
    draw(app);
  }
}

void surface_preferred_buffer_transform(void *, wl_surface *, uint32_t) {}
#endif

const wl_surface_listener surface_listener = {
    surface_enter,
    surface_leave,
#ifdef WL_SURFACE_PREFERRED_BUFFER_SCALE_SINCE_VERSION
    surface_preferred_buffer_scale,
    surface_preferred_buffer_transform,
#endif
};

void layer_surface_configure(void *data, zwlr_layer_surface_v1 *layer_surface,
                             uint32_t serial, uint32_t width,
                             uint32_t height) {
  App &app = *static_cast<App *>(data);
  zwlr_layer_surface_v1_ack_configure(layer_surface, serial);
  if (width > 0)
    app.width = width;
  if (height > 0)
    app.height = height;
  app.configured = true;
  draw(app);
}

void layer_surface_closed(void *data, zwlr_layer_surface_v1 *) {
  static_cast<App *>(data)->running = false;
}

const zwlr_layer_surface_v1_listener layer_surface_listener = {
    layer_surface_configure,
    layer_surface_closed,
};

void registry_global(void *data, wl_registry *registry, uint32_t name,
                     const char *interface, uint32_t version) {
  App &app = *static_cast<App *>(data);
  std::string iface = interface;
  if (iface == wl_compositor_interface.name) {
    app.compositor_version =
        std::min<uint32_t>(version, wl_compositor_interface.version);
    app.compositor = static_cast<wl_compositor *>(wl_registry_bind(
        registry, name, &wl_compositor_interface, app.compositor_version));
  } else if (iface == wl_shm_interface.name) {
    app.shm = static_cast<wl_shm *>(
        wl_registry_bind(registry, name, &wl_shm_interface, 1));
  } else if (iface == zwlr_layer_shell_v1_interface.name) {
    app.layer_shell = static_cast<zwlr_layer_shell_v1 *>(wl_registry_bind(
        registry, name, &zwlr_layer_shell_v1_interface, std::min(version, 4u)));
  }
}

void registry_global_remove(void *, wl_registry *, uint32_t) {}

const wl_registry_listener registry_listener = {
    registry_global,
    registry_global_remove,
};

// Fire on the next wall-clock minute. CANCEL_ON_SET also wakes us when the
// system clock is changed; an absolute realtime timer fires after resume.
void arm_timer(int fd) {
  timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  itimerspec spec = {};
  spec.it_value.tv_sec = now.tv_sec - (now.tv_sec % 60) + 60;
  timerfd_settime(fd, TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET, &spec,
                  nullptr);
}

} // namespace

int main(int argc, char **argv) {
  App app;
  int exit_code = parse_options(argc, argv, app.config, app.opts);
  if (exit_code >= 0)
    return exit_code;

  app.display = wl_display_connect(nullptr);
  if (!app.display) {
    fprintf(stderr, "focusclock: cannot connect to a Wayland display\n");
    return 1;
  }

  wl_registry *registry = wl_display_get_registry(app.display);
  wl_registry_add_listener(registry, &registry_listener, &app);
  wl_display_roundtrip(app.display);

  if (!app.compositor || !app.shm) {
    fprintf(stderr, "focusclock: compositor is missing wl_compositor/wl_shm\n");
    return 1;
  }
  if (!app.layer_shell) {
    fprintf(stderr,
            "focusclock: compositor does not support wlr-layer-shell\n");
    return 1;
  }

  measure(app.config, app.width, app.height);

  app.surface = wl_compositor_create_surface(app.compositor);
  wl_surface_add_listener(app.surface, &surface_listener, &app);

  // Empty input region: clicks pass through to whatever is underneath.
  wl_region *region = wl_compositor_create_region(app.compositor);
  wl_surface_set_input_region(app.surface, region);
  wl_region_destroy(region);

  app.layer_surface = zwlr_layer_shell_v1_get_layer_surface(
      app.layer_shell, app.surface, nullptr, app.opts.layer, "focusclock");
  zwlr_layer_surface_v1_add_listener(app.layer_surface,
                                     &layer_surface_listener, &app);

  const WindowLayerOptions &o = app.opts;
  uint32_t anchor = 0;
  if (o.anchor_top)
    anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
  if (o.anchor_bottom)
    anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
  if (o.anchor_left)
    anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
  if (o.anchor_right)
    anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;

  zwlr_layer_surface_v1_set_size(app.layer_surface, app.width, app.height);
  zwlr_layer_surface_v1_set_anchor(app.layer_surface, anchor);
  zwlr_layer_surface_v1_set_margin(app.layer_surface, o.margin_top,
                                   o.margin_right, o.margin_bottom,
                                   o.margin_left);
  zwlr_layer_surface_v1_set_exclusive_zone(app.layer_surface, -1);
  zwlr_layer_surface_v1_set_keyboard_interactivity(
      app.layer_surface, ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE);
  wl_surface_commit(app.surface);

  int timer_fd = timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC | TFD_NONBLOCK);
  arm_timer(timer_fd);

  while (app.running) {
    while (wl_display_prepare_read(app.display) != 0)
      wl_display_dispatch_pending(app.display);
    wl_display_flush(app.display);

    pollfd fds[2] = {{wl_display_get_fd(app.display), POLLIN, 0},
                     {timer_fd, POLLIN, 0}};
    if (poll(fds, 2, -1) < 0) {
      wl_display_cancel_read(app.display);
      if (errno == EINTR)
        continue;
      break;
    }

    if (fds[0].revents & POLLIN) {
      if (wl_display_read_events(app.display) < 0)
        break;
    } else {
      wl_display_cancel_read(app.display);
    }
    if (fds[0].revents & (POLLERR | POLLHUP))
      break;
    if (wl_display_dispatch_pending(app.display) < 0)
      break;

    if (fds[1].revents & POLLIN) {
      uint64_t expirations;
      // Fails with ECANCELED after a clock change; redraw either way.
      (void)!read(timer_fd, &expirations, sizeof(expirations));
      draw(app);
      arm_timer(timer_fd);
    }
  }

  close(timer_fd);
  for (Buffer &buf : app.buffers)
    destroy_buffer(buf);
  zwlr_layer_surface_v1_destroy(app.layer_surface);
  wl_surface_destroy(app.surface);
  wl_display_disconnect(app.display);
  return 0;
}
