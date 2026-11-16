#pragma once

#include "rust/cxx.h"
#include <TurboCodes/turbocode.h>
#include <HSTCFE/hybridstrengthenedturbocodefuzzyextractor.h>

std::unique_ptr<HybridStrengthenedTurboCodeFuzzyExtractor> make_extractor();

void generate_wrapper(
    HybridStrengthenedTurboCodeFuzzyExtractor& extractor,
    rust::Slice<const double> values,
    rust::Slice<const std::size_t> lengths
);

rust::Vec<bool> reproduce_wrapper(
    HybridStrengthenedTurboCodeFuzzyExtractor& extractor,
    rust::Slice<const double> sample
);

rust::Vec<bool> turboCodeInputMessage_wrapper(
    HybridStrengthenedTurboCodeFuzzyExtractor& extractor
);

std::unique_ptr<HybridStrengthenedTurboCodeFuzzyExtractor>
    copy_extractor(const HybridStrengthenedTurboCodeFuzzyExtractor &general);

std::unique_ptr<HybridStrengthenedTurboCodeFuzzyExtractor>
    load_user_extractor(const HybridStrengthenedTurboCodeFuzzyExtractor &general,
                        rust::Str folder, rust::Str user_id);


rust::Vec<uint8_t> get_user_specific_data_wrapper(
    HybridStrengthenedTurboCodeFuzzyExtractor &fe);

void load_user_specific_data_wrapper(
        HybridStrengthenedTurboCodeFuzzyExtractor &fe,
        rust::Vec<uint8_t> data);

