#include "rust/cxx.h"
#include <filesystem>
#include <iostream>
#include <TurboCodes/turbocode.h>
#include <TurboCodes/vectorfiles.h>
#include <HSTCFE/hybridstrengthenedturbocodefuzzyextractor.h>


std::unique_ptr<HybridStrengthenedTurboCodeFuzzyExtractor> make_extractor() {
    return std::make_unique<HybridStrengthenedTurboCodeFuzzyExtractor>();
}

std::unique_ptr<HybridStrengthenedTurboCodeFuzzyExtractor>
    copy_extractor(const HybridStrengthenedTurboCodeFuzzyExtractor &general) 
{
    return std::make_unique<HybridStrengthenedTurboCodeFuzzyExtractor>(general);
}

rust::Vec<bool> reproduce_wrapper(
    HybridStrengthenedTurboCodeFuzzyExtractor& extractor,
    rust::Slice<const double> probe) 
{
    real_number_vector cpp_probe(
        probe.begin(),
        probe.end()
    );

    real_number_vector_vector x;
    x.push_back(cpp_probe);

    binary_polynomial_vector bits = extractor.reproduceBatch(x);

    rust::Vec<bool> result;
    result.reserve(bits[0].size());

    for (std::size_t i = 0; i < bits[0].size(); i++) {
        result.push_back(bits[0][i]);
    }

    return result;
}

void generate_wrapper(
    HybridStrengthenedTurboCodeFuzzyExtractor& extractor,
    rust::Slice<const double> values,
    rust::Slice<const std::size_t> lengths
) {
    real_number_vector_vector samples;
    samples.reserve(lengths.size());

    std::size_t pos = 0;

    for (std::size_t len : lengths) {
        if (len > values.size() - pos) {
            throw std::invalid_argument(
                "Invalid flattened sample lengths"
            );
        }

        samples.emplace_back(
            values.begin() + pos,
            values.begin() + pos + len
        );

        pos += len;
    }

    if (pos != values.size()) {
        throw std::invalid_argument(
            "Sample lengths do not cover all values"
        );
    }

    extractor.generate(samples);
}


rust::Vec<bool> turboCodeInputMessage_wrapper(
    HybridStrengthenedTurboCodeFuzzyExtractor& extractor
){
    binary_polynomial bits = extractor.turboCodeInputMessage();

    rust::Vec<bool> result;
    result.reserve(bits.size());

    for (std::size_t i = 0; i < bits.size(); i++) {
        result.push_back(bits[i]);
    }

    return result;
}


std::unique_ptr<HybridStrengthenedTurboCodeFuzzyExtractor>
    load_user_extractor(const HybridStrengthenedTurboCodeFuzzyExtractor &general,
                        rust::Str folder, rust::Str user_id) 
{
    std::string dir(folder), id(user_id);

    std::vector<std::string> ids;
    std::vector<HybridStrengthenedTurboCodeFuzzyExtractor> loaded;
    readHybridStrengthenedTurboCodeFuzzyExtractorsFromPath(
        dir, ".userSpecificHSTCFE", general, ids, loaded);

    for (std::size_t i = 0; i < ids.size(); i++) {
        if (ids[i] == id) {
            return std::make_unique<HybridStrengthenedTurboCodeFuzzyExtractor>(loaded[i]);
        }
    }
    throw std::runtime_error("load_user_extractor: no template for user " + id + " in " + dir);
}
