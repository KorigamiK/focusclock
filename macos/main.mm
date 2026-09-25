// Native macOS frontend: a borderless, click-through, all-spaces overlay
// window drawn with Core Text.

#import <Cocoa/Cocoa.h>
#import <CoreText/CoreText.h>

#include "options.h"
#include <algorithm>

static NSWindowLevel window_level_for_layer(int layer) {
  switch (layer) {
  case 0:
    return CGWindowLevelForKey(kCGDesktopIconWindowLevelKey) + 1;
  case 1:
    return NSNormalWindowLevel;
  case 2:
    return NSFloatingWindowLevel;
  default:
    return NSScreenSaverWindowLevel;
  }
}

static CTFontRef create_bold_font(const std::string &family, double size) {
  NSString *name = [NSString stringWithUTF8String:family.c_str()];
  NSFont *font = nil;
  if (family != "Sans") {
    font = [[NSFontManager sharedFontManager] fontWithFamily:name
                                                      traits:NSBoldFontMask
                                                      weight:9
                                                        size:size];
  }
  if (font == nil) {
    font = [NSFont boldSystemFontOfSize:size];
  }
  return (CTFontRef)CFBridgingRetain(font);
}

@interface ClockView : NSView
- (instancetype)initWithConfig:(const ClockConfig &)config;
- (NSSize)preferredSize;
@end

@implementation ClockView {
  ClockConfig _config;
  CTFontRef _font;
  CGColorRef _color;
}

- (instancetype)initWithConfig:(const ClockConfig &)config {
  if ((self = [super initWithFrame:NSZeroRect])) {
    _config = config;
    _font = create_bold_font(config.font_family, config.font_size);
    _color = CGColorCreateSRGB(config.text_color[0], config.text_color[1],
                               config.text_color[2], config.text_color[3]);
  }
  return self;
}

- (void)dealloc {
  CFRelease(_font);
  CGColorRelease(_color);
}

- (CTLineRef)createLine:(const char *)text CF_RETURNS_RETAINED {
  CFStringRef str = CFStringCreateWithCString(nullptr, text,
                                              kCFStringEncodingASCII);
  CFStringRef keys[] = {kCTFontAttributeName, kCTForegroundColorAttributeName};
  CFTypeRef values[] = {_font, _color};
  CFDictionaryRef attrs = CFDictionaryCreate(
      nullptr, (const void **)keys, (const void **)values, 2,
      &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
  CFAttributedStringRef astr =
      CFAttributedStringCreate(nullptr, str, attrs);
  CTLineRef line = CTLineCreateWithAttributedString(astr);
  CFRelease(astr);
  CFRelease(attrs);
  CFRelease(str);
  return line;
}

- (NSSize)preferredSize {
  CTLineRef line = [self createLine:"00:00"];
  CGRect ink = CTLineGetImageBounds(line, nullptr);
  CFRelease(line);

  double padding = _config.font_size * _config.text_padding_ratio;
  return NSMakeSize(std::max(ink.size.width + padding * 2, 120.0),
                    std::max(ink.size.height + padding * 2, 60.0));
}

- (BOOL)isOpaque {
  return NO;
}

- (void)drawRect:(NSRect)dirtyRect {
  time_t rawtime;
  time(&rawtime);
  char buffer[6];
  strftime(buffer, sizeof(buffer), _config.time_format(),
           localtime(&rawtime));

  CGContextRef cr = [NSGraphicsContext currentContext].CGContext;
  CTLineRef line = [self createLine:buffer];
  CGRect ink = CTLineGetImageBounds(line, nullptr);

  NSRect bounds = self.bounds;
  CGContextSetTextMatrix(cr, CGAffineTransformIdentity);
  CGContextSetTextPosition(
      cr, (bounds.size.width - ink.size.width) / 2 - ink.origin.x,
      (bounds.size.height - ink.size.height) / 2 - ink.origin.y);
  CTLineDraw(line, cr);
  CFRelease(line);
}
@end

@interface AppDelegate : NSObject <NSApplicationDelegate>
- (instancetype)initWithConfig:(const ClockConfig &)config
                       options:(const WindowLayerOptions &)opts;
@end

@implementation AppDelegate {
  ClockConfig _config;
  WindowLayerOptions _opts;
  NSPanel *_window;
  ClockView *_view;
  NSTimer *_timer;
}

- (instancetype)initWithConfig:(const ClockConfig &)config
                       options:(const WindowLayerOptions &)opts {
  if ((self = [super init])) {
    _config = config;
    _opts = opts;
  }
  return self;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
  _view = [[ClockView alloc] initWithConfig:_config];
  NSSize size = [_view preferredSize];

  _window = [[NSPanel alloc]
      initWithContentRect:NSMakeRect(0, 0, size.width, size.height)
                styleMask:NSWindowStyleMaskBorderless |
                          NSWindowStyleMaskNonactivatingPanel
                  backing:NSBackingStoreBuffered
                    defer:NO];
  _window.contentView = _view;
  _window.opaque = NO;
  _window.backgroundColor = NSColor.clearColor;
  _window.hasShadow = NO;
  _window.ignoresMouseEvents = YES;
  _window.level = window_level_for_layer(_opts.layer);
  _window.collectionBehavior = NSWindowCollectionBehaviorCanJoinAllSpaces |
                               NSWindowCollectionBehaviorStationary |
                               NSWindowCollectionBehaviorFullScreenAuxiliary |
                               NSWindowCollectionBehaviorIgnoresCycle;
  [self position];
  [_window orderFrontRegardless];

  NSNotificationCenter *nc = NSNotificationCenter.defaultCenter;
  [nc addObserver:self
         selector:@selector(position)
             name:NSApplicationDidChangeScreenParametersNotification
           object:nil];
  [nc addObserver:self
         selector:@selector(tick)
             name:NSSystemClockDidChangeNotification
           object:nil];
  [nc addObserver:self
         selector:@selector(tick)
             name:NSSystemTimeZoneDidChangeNotification
           object:nil];
  [NSWorkspace.sharedWorkspace.notificationCenter
      addObserver:self
         selector:@selector(tick)
             name:NSWorkspaceDidWakeNotification
           object:nil];

  [self scheduleNextTick];
}

// Place the window like a layer-shell surface: anchoring one edge pins it
// there (plus margin); anchoring both or neither centers on that axis.
- (void)position {
  NSScreen *screen = NSScreen.screens.firstObject;
  if (screen == nil)
    return;

  NSRect area = screen.frame;
  NSSize size = _window.frame.size;
  CGFloat x = NSMidX(area) - size.width / 2;
  CGFloat y = NSMidY(area) - size.height / 2;

  if (_opts.anchor_left && !_opts.anchor_right)
    x = NSMinX(area) + _opts.margin_left;
  else if (_opts.anchor_right && !_opts.anchor_left)
    x = NSMaxX(area) - size.width - _opts.margin_right;

  // Cocoa's y axis points up.
  if (_opts.anchor_bottom && !_opts.anchor_top)
    y = NSMinY(area) + _opts.margin_bottom;
  else if (_opts.anchor_top && !_opts.anchor_bottom)
    y = NSMaxY(area) - size.height - _opts.margin_top;

  [_window setFrameOrigin:NSMakePoint(x, y)];
}

// Wake once per minute on the boundary instead of polling every second.
- (void)scheduleNextTick {
  [_timer invalidate];
  time_t now;
  time(&now);
  NSTimeInterval delay = 60 - (now % 60) + 0.05;
  _timer = [NSTimer scheduledTimerWithTimeInterval:delay
                                            target:self
                                          selector:@selector(tick)
                                          userInfo:nil
                                           repeats:NO];
  _timer.tolerance = 0.5;
}

- (void)tick {
  _view.needsDisplay = YES;
  [self scheduleNextTick];
}
@end

int main(int argc, char **argv) {
  ClockConfig config;
  WindowLayerOptions opts;
  int exit_code = parse_options(argc, argv, config, opts);
  if (exit_code >= 0)
    return exit_code;

  @autoreleasepool {
    NSApplication *app = NSApplication.sharedApplication;
    // No Dock icon, no menu bar, never steals focus.
    app.activationPolicy = NSApplicationActivationPolicyAccessory;
    AppDelegate *delegate = [[AppDelegate alloc] initWithConfig:config
                                                        options:opts];
    app.delegate = delegate;
    [app run];
  }
  return 0;
}
