#pragma once

#include "clock_config.h"

// Parses the command line into config/opts. Returns -1 to continue running,
// otherwise the exit code to return from main (after --help, --version or a
// bad option).
int parse_options(int argc, char **argv, ClockConfig &config,
                  WindowLayerOptions &opts);
