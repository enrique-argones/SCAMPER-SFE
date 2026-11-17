/// \file test_inmemory_reproduce.cpp
/// \brief Regression test: reproducing with the object that ran generate() must give the same result as reproducing
/// with a template rebuilt from its user-specific data (in memory or through a copy).
///
/// Usage: test_inmemory_reproduce WORK_DIR TURBO_CODE_CONFIG_FILE
///
/// Before the fix, TurboCode::setPuncturingPatterns() did not rebuild the redundancy indexes. generate() sets the
/// interleavers on the unpunctured code and punctures it afterwards, so the enrolling object decoded with stale indexes
/// (wrong redundancy bits and an out-of-bounds read) whenever the capacity was not a multiple of n/k, while templates
/// loaded from files were correct. The test therefore also checks that at least one enrolled code is punctured.

#include <HSTCFE/hybridstrengthenedturbocodefuzzyextractor.h>
#include <TurboCodes/vectorfiles.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>

namespace {

constexpr unsigned int kDimensionality = 24;
constexpr unsigned int kTrainSubjects = 40;
constexpr unsigned int kSubjects = 12;
constexpr unsigned int kEnrollmentSamples = 8;
constexpr unsigned int kProbeSamples = 3;

real_number_vector_vector subjectSamples(std::mt19937 &rng, const real_number_vector &mean,
                                         const real_number_vector &noise, unsigned int count)
{
    real_number_vector_vector samples(count, real_number_vector(mean.size()));
    for (auto &sample : samples) {
        for (std::size_t i = 0; i < mean.size(); i++) {
            sample[i] = mean[i] + std::normal_distribution<real_number>(0.0f, noise[i])(rng);
        }
    }
    return samples;
}

void writeSamples(const std::filesystem::path &path, const real_number_vector_vector &samples)
{
    std::ofstream file(path);
    file << samples.size() << " " << samples[0].size() << "\n";
    for (const auto &sample : samples) {
        for (std::size_t i = 0; i < sample.size(); i++) {
            file << sample[i] << (i + 1 < sample.size() ? " " : "\n");
        }
    }
}

real_number_vector randomMean(std::mt19937 &rng)
{
    real_number_vector mean(kDimensionality);
    for (auto &m : mean) {
        m = std::normal_distribution<real_number>(0.0f, 1.0f)(rng);
    }
    return mean;
}

bool isPunctured(const TurboCode &code)
{
    for (const auto &pattern : code.puncturingPatterns()) {
        for (bool bit : pattern) {
            if (!bit) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

int main(int argc, char **argv)
{
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " WORK_DIR TURBO_CODE_CONFIG_FILE" << std::endl;
        return EXIT_FAILURE;
    }
    const std::filesystem::path work(argv[1]);
    const std::filesystem::path train = work / "train";
    std::filesystem::remove_all(work);
    std::filesystem::create_directories(train);

    std::mt19937 rng(20261117);
    real_number_vector noise(kDimensionality);
    for (auto &n : noise) {
        n = std::uniform_real_distribution<real_number>(0.05f, 0.45f)(rng);
    }
    for (unsigned int i = 0; i < kTrainSubjects; i++) {
        char name[32];
        std::snprintf(name, sizeof name, "train_%03u.txt", i);
        writeSamples(train / name, subjectSamples(rng, randomMean(rng), noise, 4));
    }
    const std::filesystem::path generalConfig = work / "generalConfigFile.txt";
    {
        std::ofstream file(generalConfig);
        file << "TurboCodeConfigFile " << argv[2] << "\n"
             << "TurboCodeMaxDecodingIterations 16\n"
             << "TrainSetSamplesFilesFolder " << train.string() << "\n"
             << "CodeOffsetStdDevFactor 1.1\n"
             << "CodeOffsetStdDevOffset 0.2\n"
             << "FuzzyCommitmentMinReliability 2.0\n";
    }

    HybridStrengthenedTurboCodeFuzzyExtractor general;
    general.loadGeneralData(generalConfig.string());

    unsigned int attempts = 0, mismatches = 0, puncturedSubjects = 0, sameObjectSuccesses = 0;
    for (unsigned int s = 0; s < kSubjects; s++) {
        const real_number_vector mean = randomMean(rng);
        const real_number_vector_vector enrollment = subjectSamples(rng, mean, noise, kEnrollmentSamples);
        const real_number_vector_vector probes = subjectSamples(rng, mean, noise, kProbeSamples);

        HybridStrengthenedTurboCodeFuzzyExtractor enrolled(general);
        enrolled.setUserId("subject_" + std::to_string(s));
        enrolled.generate(enrollment);
        if (!enrolled.validForReproduce()) {
            continue;
        }
        puncturedSubjects += isPunctured(enrolled.turboCode());
        const binary_polynomial key = enrolled.turboCodeInputMessage();

        HybridStrengthenedTurboCodeFuzzyExtractor reloaded(general);
        reloaded.loadUserSpecificDataFromBlob(enrolled.getUserSpecificData());
        HybridStrengthenedTurboCodeFuzzyExtractor copied(enrolled);

        for (const auto &probe : probes) {
            attempts++;
            const binary_polynomial fromEnrolled = enrolled.reproduce(probe);
            const binary_polynomial fromReloaded = reloaded.reproduce(probe);
            const binary_polynomial fromCopy = copied.reproduce(probe);
            sameObjectSuccesses += (fromEnrolled == key);
            if (fromEnrolled != fromReloaded || fromEnrolled != fromCopy) {
                mismatches++;
                std::cerr << "subject " << s << ": reproduce() differs between the enrolling object, its copy and the "
                          << "template reloaded from its user-specific data" << std::endl;
            }
        }
    }
    std::cout << attempts << " genuine attempts, " << sameObjectSuccesses << " keys reproduced by the enrolling object, "
              << puncturedSubjects << " punctured codes, " << mismatches << " mismatches" << std::endl;
    if (puncturedSubjects == 0) {
        std::cerr << "No enrolled code is punctured: the test does not exercise the regression" << std::endl;
        return EXIT_FAILURE;
    }
    return (attempts > 0 && mismatches == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
