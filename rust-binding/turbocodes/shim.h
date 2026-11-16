#pragma once

#include "rust/cxx.h"
#include <TurboCodes/turbocode.h>

struct ParsedSamples;

ParsedSamples parse_samples_file_wrapper(
    const std::string& filename
);
