# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.1.0] - 2026-09-24

### Added
- Native macOS build (Cocoa + Core Text): click-through overlay on every
  Space and over fullscreen apps, with the same command-line options
- Universal macOS binary in GitHub releases
- Homebrew formula: `brew install korigamik/tap/focusclock`
- `-H, --24-hour` option for 24-hour time (#3)

### Changed
- Linux build rewritten as a native Wayland `wlr-layer-shell` client drawn
  with Cairo; the only runtime dependencies are `wayland` and `cairo`
- The clock redraws on minute boundaries and after clock changes or resume
  instead of polling every second
- `--layer` accepts 0-3; the GTK-only `4=no_layer` mode is gone

### Removed
- GTK/gtkmm and gtk4-layer-shell dependencies
- Windows build

## [2.0.0] - 2024-01-14

### Added
- Command line option for version information
- Configurable font family
- Configurable font color
- Configurable text opacity
- GTK layer shell configuration
