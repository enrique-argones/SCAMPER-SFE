#pragma once

// C++ side of the cxx bridge declared in rust-binding/lib.rs: adapters between Rust types (slices, Vec) and
// HybridStrengthenedTurboCodeFuzzyExtractor.

#include "rust/cxx.h"
#include <TurboCodes/turbocode.h>
#include <HSTCFE/hybridstrengthenedturbocodefuzzyextractor.h>

// New default-constructed extractor.
std::unique_ptr<HybridStrengthenedTurboCodeFuzzyExtractor> make_extractor();

// Enrollment. The samples are flattened in values; lengths gives the dimensionality of each sample.
void generate_wrapper(
    HybridStrengthenedTurboCodeFuzzyExtractor& extractor,
    rust::Slice<const double> values,
    rust::Slice<const std::size_t> lengths
);

// Reproduction from one probe sample (reproduceBatch with one sample); one bool per key bit.
rust::Vec<bool> reproduce_wrapper(
    HybridStrengthenedTurboCodeFuzzyExtractor& extractor,
    rust::Slice<const double> sample
);

// Enrolled key, one bool per bit.
rust::Vec<bool> turboCodeInputMessage_wrapper(
    HybridStrengthenedTurboCodeFuzzyExtractor& extractor
);

// Copy of an extractor (typically the one holding the general data).
std::unique_ptr<HybridStrengthenedTurboCodeFuzzyExtractor>
    copy_extractor(const HybridStrengthenedTurboCodeFuzzyExtractor &general);

// Loads the template of user_id from the .userSpecificHSTCFE files of folder. Not exposed to Rust at the moment.
std::unique_ptr<HybridStrengthenedTurboCodeFuzzyExtractor>
    load_user_extractor(const HybridStrengthenedTurboCodeFuzzyExtractor &general,
                        rust::Str folder, rust::Str user_id);


// User-specific helper data as bytes (getUserSpecificData).
rust::Vec<uint8_t> get_user_specific_data_wrapper(
    HybridStrengthenedTurboCodeFuzzyExtractor &fe);

// Loads user-specific helper data from bytes (loadUserSpecificDataFromBlob).
void load_user_specific_data_wrapper(
        HybridStrengthenedTurboCodeFuzzyExtractor &fe,
        rust::Vec<uint8_t> data);

