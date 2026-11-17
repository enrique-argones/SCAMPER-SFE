#pragma once

#include "rust/cxx.h"
#include <TurboCodes/turbocode.h>

// C++ side of the cxx bridge declared in rust-binding/turbocodes/mod.rs.

struct ParsedSamples;

// Reads a samples file with parseSamplesFile; the samples are flattened in values, with their lengths in lengths.
ParsedSamples parse_samples_file_wrapper(
    const std::string& filename
);
