#include "options.h"
#include "utils.h"
#include "version.h"
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <vector>

static std::string config_path() {
  const char *xdg = getenv("XDG_CONFIG_HOME");
  if (xdg && *xdg)
    return std::string(xdg) + "/focusclock/config";
  const char *home = getenv("HOME");
  if (home && *home)
    return std::string(home) + "/.config/focusclock/config";
  return "";
}

// Reads the config file as command-line options: words are separated by
// whitespace, quotes group a word, and lines starting with '#' are comments.
static std::vector<std::string> read_config_args() {
  std::vector<std::string> args;
  std::string path = config_path();
  if (path.empty())
    return args;
  std::ifstream file(path);
  std::string line;
  while (std::getline(file, line)) {
    size_t i = line.find_first_not_of(" \t\r");
    if (i == std::string::npos || line[i] == '#')
      continue;
    std::string word;
    bool in_word = false;
    char quote = 0;
    for (; i < line.size(); i++) {
      char c = line[i];
      if (quote) {
        if (c == quote)
          quote = 0;
        else
          word += c;
      } else if (c == '\'' || c == '"') {
        quote = c;
        in_word = true;
      } else if (isspace(static_cast<unsigned char>(c))) {
        if (in_word)
          args.push_back(word);
        word.clear();
        in_word = false;
      } else {
        word += c;
        in_word = true;
      }
    }
    if (in_word)
      args.push_back(word);
  }
  return args;
}

static void print_usage(const char *prog) {
  std::cout
      << "Usage:\n  " << prog << " [OPTION?]\n\n"
      << "Options:\n"
         "  -h, --help              Show help options\n"
         "  -v, --version           Show version information\n"
         "  -t, --anchor-top        Anchor to the top edge\n"
         "  -b, --anchor-bottom     Anchor to the bottom edge\n"
         "  -l, --anchor-left       Anchor to the left edge\n"
         "  -r, --anchor-right      Anchor to the right edge\n"
         "  -T, --margin-top        Margin from the top edge\n"
         "  -B, --margin-bottom     Margin from the bottom edge\n"
         "  -L, --margin-left       Margin from the left edge\n"
         "  -R, --margin-right      Margin from the right edge\n"
         "  -f, --font-size         Base font size\n"
         "  -c, --color             Text color (hex format: RGB, RGBA, "
         "RRGGBB, or RRGGBBAA)\n"
         "  -F, --font-family       Font family name\n"
         "  -a, --alpha             Text opacity (0.0-1.0, overridden by "
         "RGBA color)\n"
         "  -y, --layer             Layer (0=background, 1=bottom, 2=top, "
         "3=overlay)\n"
         "  -H, --24-hour           Use 24-hour time\n";
#ifdef __APPLE__
  std::cout
      << "      --monitor          Display: primary or secondary (default: primary)\n";
#endif
  std::cout << "\nWhen run without options, options are read from\n"
               "$XDG_CONFIG_HOME/focusclock/config (default: "
               "~/.config/focusclock/config).\n";
}

int parse_options(int argc, char **argv, ClockConfig &config,
                  WindowLayerOptions &opts) {
  std::string color_str;
  double alpha = config.text_color[3];

  // A bare `focusclock` (a login item, a service) takes its options from the
  // config file. Any command-line option bypasses the file entirely.
  std::vector<std::string> file_args;
  std::vector<char *> file_argv;
  if (argc == 1) {
    file_args = read_config_args();
    if (!file_args.empty()) {
      file_argv.push_back(argv[0]);
      for (std::string &arg : file_args)
        file_argv.push_back(arg.data());
      file_argv.push_back(nullptr);
      argc = static_cast<int>(file_args.size()) + 1;
      argv = file_argv.data();
    }
  }

  static const struct option long_options[] = {
      {"help", no_argument, nullptr, 'h'},
      {"version", no_argument, nullptr, 'v'},
      {"anchor-top", no_argument, nullptr, 't'},
      {"anchor-bottom", no_argument, nullptr, 'b'},
      {"anchor-left", no_argument, nullptr, 'l'},
      {"anchor-right", no_argument, nullptr, 'r'},
      {"margin-top", required_argument, nullptr, 'T'},
      {"margin-bottom", required_argument, nullptr, 'B'},
      {"margin-left", required_argument, nullptr, 'L'},
      {"margin-right", required_argument, nullptr, 'R'},
      {"font-size", required_argument, nullptr, 'f'},
      {"color", required_argument, nullptr, 'c'},
      {"font-family", required_argument, nullptr, 'F'},
      {"alpha", required_argument, nullptr, 'a'},
      {"layer", required_argument, nullptr, 'y'},
      {"24-hour", no_argument, nullptr, 'H'},
#ifdef __APPLE__
      {"monitor", required_argument, nullptr, 1000},
#endif
      {nullptr, 0, nullptr, 0},
  };

  int opt;
  while ((opt = getopt_long(argc, argv, "hvtblrT:B:L:R:f:c:F:a:y:H",
                            long_options, nullptr)) != -1) {
    switch (opt) {
    case 'h':
      print_usage(argv[0]);
      return 0;
    case 'v':
      std::cout << "focusclock version " << FOCUSCLOCK_VERSION << std::endl;
      return 0;
    case 't': opts.anchor_top = true; break;
    case 'b': opts.anchor_bottom = true; break;
    case 'l': opts.anchor_left = true; break;
    case 'r': opts.anchor_right = true; break;
    case 'T': opts.margin_top = atoi(optarg); break;
    case 'B': opts.margin_bottom = atoi(optarg); break;
    case 'L': opts.margin_left = atoi(optarg); break;
    case 'R': opts.margin_right = atoi(optarg); break;
    case 'f': config.font_size = atof(optarg); break;
    case 'c': color_str = optarg; break;
    case 'F': config.font_family = optarg; break;
    case 'a': alpha = atof(optarg); break;
    case 'y': opts.layer = atoi(optarg); break;
    case 'H': config.twenty_four_hour = true; break;
#ifdef __APPLE__
    case 1000: {
      std::string monitor = optarg;
      if (monitor != "primary" && monitor != "secondary") {
        std::cerr << "Invalid monitor: " << monitor
                  << ". Use primary or secondary.\n";
        return 1;
      }
      opts.secondary_monitor = monitor == "secondary";
      break;
    }
#endif
    default:
      print_usage(argv[0]);
      return 1;
    }
  }

  if (opts.layer < 0 || opts.layer > 3)
    opts.layer = 3;

  if (!color_str.empty()) {
    double r, g, b, a;
    if (parse_color(color_str, r, g, b, a)) {
      config.text_color[0] = r;
      config.text_color[1] = g;
      config.text_color[2] = b;
      config.text_color[3] = a;
    }
  } else if (alpha >= 0.0 && alpha <= 1.0) {
    config.text_color[3] = alpha;
  }

  return -1;
}
