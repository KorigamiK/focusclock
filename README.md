# Focus Clock

Bigass clock that sits on top of all windows to help you focus.

![Screenshot](./.github/screenshot.png)

## Building

Requires Gtkmm4, gtk4-layer-shell, CMake, pkg-config.

```
mkdir build
cd build
cmake ..
make
```

You might also wanna add the following or Hyprland window rules to your wm or similar:

```
windowrule = noblur, wlclock
windowrule = noborder, wlclock
windowrule = pin, wlclock
windowrule = noshadow, wlclock
```

# References

- https://github.com/nwg-piotr/nwg-wrapper/
- https://github.com/wmww/gtk4-layer-shell/
- https://www.gtk.org/docs/language-bindings/cpp

## License

GNU GPL3+
