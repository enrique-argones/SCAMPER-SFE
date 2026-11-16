#include "rust/cxx.h"
#include "./shim.h"
#include <TurboCodes/turbocode.h>
#include <TurboCodes/vectorfiles.h>
#include "sfe/rust-binding/turbocodes/mod.rs.h"

ParsedSamples parse_samples_file_wrapper(
    const std::string& filename
){
    real_number_vector_vector samples;

    parseSamplesFile(filename, samples);

    ParsedSamples out;

    for (const auto& row : samples) {
        out.lengths.push_back(row.size());

        for (double x : row) {
            out.values.push_back(x);
        }
    }

    return out;
}
