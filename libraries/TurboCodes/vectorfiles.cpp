/// \file vectorfiles.cpp
/// \brief Implementation of the file parsing and sample statistics helpers declared in vectorfiles.h.

#include "vectorfiles.h"
#include <armadillo>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <stdexcept>

void parseTemplateFile(const std::string templateFileName, real_number_vector &templateVector) {
    std::ifstream templateFile(templateFileName);
    templateFile.exceptions(std::ios_base::failbit | std::ios_base::badbit | std::ios_base::eofbit);
    if (!templateFile) {
        std::string exceptionMessage = std::string("Error opening template file ") + std::string(templateFileName) + std::string(".");
        throw std::logic_error(exceptionMessage.c_str());
    }
    templateFile.exceptions(std::ios_base::failbit | std::ios_base::badbit | std::ios_base::eofbit);
    if (!templateFile) {
        std::string exceptionMessage = std::string("Error opening template file ") + std::string(templateFileName) + std::string(".");
        throw std::logic_error(exceptionMessage.c_str());
    }
    unsigned int templateLength;
    templateFile >> templateLength;
    templateVector.resize(templateLength);
    for (unsigned int k = 0; k < templateLength; k++) {
        templateFile >> templateVector[k];
    }
    templateFile.close();
}

void parseExpectedStdFile(const std::string expectedStdFileName, const unsigned int templateLength, real_number_vector &expectedStdVector) {
    std::ifstream expectedStdFile(expectedStdFileName);
    if (!expectedStdFile) {
        std::string exceptionMessage = std::string("Error opening template file ") + std::string(expectedStdFileName) + std::string(".");
        throw std::logic_error(exceptionMessage.c_str());
    }
    expectedStdFile.exceptions(std::ios_base::failbit | std::ios_base::badbit | std::ios_base::eofbit);
    unsigned int expectedStdVectorLength;
    expectedStdFile >> expectedStdVectorLength;
    if (expectedStdVectorLength != templateLength) {
        std::string exceptionMessage = std::string("Expected genuine standard deviation vector length indicated in first line of ") + std::string(expectedStdFileName) +
                                       std::string(" mismatches template length: ") + std::to_string(expectedStdVectorLength) + std::string(" != ") + std::to_string(templateLength);
        throw std::logic_error(exceptionMessage.c_str());
    }
    expectedStdVector.resize(templateLength);
    for (unsigned int k = 0; k < templateLength; k++) {
        expectedStdFile >> expectedStdVector[k];
    }
    expectedStdFile.close();
}

void parseAttemptsFile(const std::string attemptsFileName, const unsigned int templateLength, real_number_vector_vector &attemptsVectorVector) {
    std::ifstream attemptsFile(attemptsFileName);
    if (!attemptsFile) {
        std::string exceptionMessage = std::string("Error opening attempts file ") + std::string(attemptsFileName) + std::string(".");
        throw std::logic_error(exceptionMessage.c_str());
    }
    attemptsFile.exceptions(std::ios_base::failbit | std::ios_base::badbit | std::ios_base::eofbit);
    unsigned int attemptsCount;
    attemptsFile >> attemptsCount;
    attemptsVectorVector = real_number_vector_vector(attemptsCount);
    unsigned int attemptsLength;
    attemptsFile >> attemptsLength;
    if (attemptsLength != templateLength) {
        std::string exceptionMessage = std::string("Attempts length indicated in first line of ") + std::string(attemptsFileName) +
                                       std::string(" mismatches template length: ") + std::to_string(attemptsLength) + std::string(" != ") + std::to_string(templateLength);
        throw std::logic_error(exceptionMessage.c_str());
    }
    for (unsigned int attemptIndex = 0; attemptIndex < attemptsCount; attemptIndex++) {
        attemptsVectorVector[attemptIndex].resize(attemptsLength);
        for (unsigned int k = 0; k < templateLength; k++) {
            attemptsFile >> attemptsVectorVector[attemptIndex][k];
        }
    }
    attemptsFile.close();
}

void parseSamplesFile(const std::string enrollmentSamplesFileName,
                      real_number_vector_vector &enrollmentSamples,
                      real_number_vector &enrollmentSamplesMeanVector,
                      real_number_vector &enrollmentSamplesVarianceVector,
                      real_number_vector_vector &enrollmentSamplesCovarianceMatrix) {
    std::ifstream enrollmentSamplesFile(enrollmentSamplesFileName);
    if (!enrollmentSamplesFile) {
        std::string exceptionMessage = std::string("Error opening samples file ") + std::string(enrollmentSamplesFileName) + std::string(".");
        throw std::logic_error(exceptionMessage.c_str());
    }
    enrollmentSamplesFile.exceptions(std::ios_base::failbit | std::ios_base::badbit | std::ios_base::eofbit);
    unsigned int dimsCount, samplesCount;
    enrollmentSamplesFile >> samplesCount >> dimsCount;
    enrollmentSamples.clear();
    enrollmentSamples.resize(samplesCount, real_number_vector(dimsCount, 0));
    real_number_vector sample(dimsCount);
    for (unsigned int sampleI = 0; sampleI < samplesCount; sampleI++) {
        for (unsigned int dimI = 0; dimI < dimsCount; dimI++) {
            enrollmentSamplesFile >> enrollmentSamples[sampleI][dimI];
        }
    }
    enrollmentSamplesFile.close();
    computeSampleStatistics(enrollmentSamples, enrollmentSamplesMeanVector, enrollmentSamplesVarianceVector, enrollmentSamplesCovarianceMatrix);
}

void parseSamplesFile(const std::string samplesFileName,
                      real_number_vector_vector &samples) {
    std::ifstream samplesFile(samplesFileName);
    if (!samplesFile) {
        std::string exceptionMessage = std::string("Error opening samples file ") + std::string(samplesFileName) + std::string(".");
        throw std::logic_error(exceptionMessage.c_str());
    }
    samplesFile.exceptions(std::ios_base::failbit | std::ios_base::badbit | std::ios_base::eofbit);
    unsigned int dimsCount, samplesCount;
    samplesFile >> samplesCount >> dimsCount;
    // std::cout << "Samples count: " << samplesCount << std::endl;
    // std::cout << "Samples dimensionality: " << dimsCount << std::endl;
    samples = real_number_vector_vector(samplesCount, real_number_vector(dimsCount));
    for (unsigned int sampleI = 0; sampleI < samplesCount; sampleI++) {
        // std::cout << "Reading sample " << sampleI + 1 << std::endl;
        for (unsigned int dimI = 0; dimI < dimsCount; dimI++) {
            // std::cout << "Reading dimension " << dimI << std::endl;
            samplesFile >> samples[sampleI][dimI];
        }
    }
    samplesFile.close();
}

void parseTurboCodeConfigFile(const std::string configFileName, TurboCode &turboCode) {
    binary_polynomial_pair_vector generators;
    binary_polynomial_vector puncturingPatterns;
    uint_vector_vector interleavers;
    std::ifstream configFile(configFileName);
    if (!configFile) {
        std::string exceptionMessage = std::string("Error opening config file ") + std::string(configFileName) + std::string(".");
        throw std::logic_error(exceptionMessage.c_str());
    }
    configFile.exceptions(std::ios_base::failbit | std::ios_base::badbit | std::ios_base::eofbit);
    unsigned int generatorsCount;
#ifdef DEBUG
    std::cout << "Parsing "
              << configFileName << std::endl;
    std::cout << "\tReading GENERATORS_COUNT ... ";
#endif
    configFile >> generatorsCount;
#ifdef DEBUG
    std::cout << generatorsCount << std::endl;
#endif
    for (unsigned int generatorI = 0; generatorI < generatorsCount; generatorI++) {
#ifdef DEBUG
        std::cout << "\tReading GENERATOR[" << generatorI << "].FEEDFORWARD ... ";
#endif
        std::string generatorFeedForwardString;
        configFile >> generatorFeedForwardString;
        binary_polynomial feedForward, feedBack;
        for (unsigned int bitI = 0; bitI < generatorFeedForwardString.size(); bitI++) {
            if (generatorFeedForwardString.at(bitI) == '0') {
                feedForward.push_back(false);
            } else if (generatorFeedForwardString.at(bitI) == '1') {
                feedForward.push_back(true);
            } else {
                std::string exceptionMessage = std::string("Error parsing GENERATOR[") + std::to_string(generatorI) +
                                               std::string("].FEEDFORWARD: element ") + std::to_string(bitI) + std::string(" is '") + generatorFeedForwardString.at(bitI) +
                                               std::string("', but only 0 or 1 are allowed.");
                throw std::logic_error(exceptionMessage.c_str());
            }
        }
#ifdef DEBUG
        std::cout << "Done: ";
        printVector(feedForward);
        std::cout << std::endl;
        std::cout << "\tReading GENERATOR[" << generatorI << "].FEEDBACK ... ";
#endif
        std::string generatorFeedBackString;
        configFile >> generatorFeedBackString;
        for (unsigned int bitI = 0; bitI < generatorFeedBackString.size(); bitI++) {
            if (generatorFeedBackString.at(bitI) == '0') {
                feedBack.push_back(false);
            } else if (generatorFeedBackString.at(bitI) == '1') {
                feedBack.push_back(true);
            } else {
                std::string exceptionMessage = std::string("Error parsing GENERATOR[") + std::to_string(generatorI) +
                                               std::string("].FEEDBACK: element ") + std::to_string(bitI) + std::string(" is '") + generatorFeedBackString.at(bitI) +
                                               std::string("', but only 0 or 1 are allowed.");
                throw std::logic_error(exceptionMessage.c_str());
            }
        }
#ifdef DEBUG
        std::cout << "Done: ";
        printVector(feedBack);
        std::cout << std::endl;
#endif
        binary_polynomial_pair generator(feedForward, feedBack);
        generators.push_back(generator);
    }
    for (unsigned int generatorI = 0; generatorI < generatorsCount; generatorI++) {
#ifdef DEBUG
        std::cout << "\tReading PUNCTURING[" << generatorI << "] ... ";
#endif
        std::string generatorPuncturingPatternString;
        configFile >> generatorPuncturingPatternString;
        binary_polynomial puncturingPattern;
        for (unsigned int bitI = 0; bitI < generatorPuncturingPatternString.size(); bitI++) {
            if (generatorPuncturingPatternString.at(bitI) == '0') {
                puncturingPattern.push_back(false);
            } else if (generatorPuncturingPatternString.at(bitI) == '1') {
                puncturingPattern.push_back(true);
            } else {
                std::string exceptionMessage = std::string("Error parsing PUNCTURING[") + std::to_string(generatorI) +
                                               std::string("]: element ") + std::to_string(bitI) + std::string(" is '") + generatorPuncturingPatternString.at(bitI) +
                                               std::string("', but only 0 or 1 are allowed.");
                throw std::logic_error(exceptionMessage.c_str());
            }
        }
#ifdef DEBUG
        std::cout << "Done: ";
        printVector(puncturingPattern);
        std::cout << std::endl;
#endif
        puncturingPatterns.push_back(puncturingPattern);
        if (puncturingPattern.size() != puncturingPatterns.front().size()) {
            std::string exceptionMessage = std::string("Error parsing PUNCTURING[") + std::to_string(generatorI) +
                                           std::string("]: all puncturing patterns must have equal size.");
            throw std::logic_error(exceptionMessage.c_str());
        }
    }
    for (unsigned int generatorI = 0; generatorI < generatorsCount; generatorI++) {
#ifdef DEBUG
        std::cout << "\tReading INTERLEAVER[" << generatorI << "] ... ";
#endif
        uint_vector interleaver;
        for (unsigned int numberI = 0; numberI < puncturingPatterns.front().size(); numberI++) {
            uint number;
            configFile >> number;
            if (number < puncturingPatterns.front().size()) {
                interleaver.push_back(number);
            } else {
                std::string exceptionMessage = std::string("Error parsing PUNCTURING[") + std::to_string(generatorI) +
                                               std::string("]: element ") + std::to_string(numberI) + std::string(" is '") + std::to_string(number) +
                                               std::string("', but only the interval [0, ") + std::to_string(puncturingPatterns.front().size() - 1) + std::string("] is allowed.");
                throw std::logic_error(exceptionMessage.c_str());
            }
        }
#ifdef DEBUG
        std::cout << "Done: ";
        unsigned int maxWidthInterleaverElements = std::ceil(std::log10(interleaver.size()));
        printVector(interleaver, maxWidthInterleaverElements);
        std::cout << std::endl;
#endif
        interleavers.push_back(interleaver);
        if (interleaver.size() != puncturingPatterns.front().size()) {
            std::string exceptionMessage = std::string("Error parsing INTERLEAVERS[") + std::to_string(generatorI) +
                                           std::string("], with size ") + std::to_string(interleaver.size()) + std::string(", while ") + std::to_string(puncturingPatterns.front().size()) +
                                           std::string(" was expected.");
            throw std::logic_error(exceptionMessage.c_str());
        }
    }
#ifdef DEBUG
    std::cout << "Setting up TurboCode object... ";
    std::cout.flush();
#endif
    turboCode.setup(generators, puncturingPatterns, interleavers);
#ifdef DEBUG
    std::cout << "Done." << std::endl;
#endif
    configFile.close();
}

void parseTrainSetSamplesFile(const std::string trainSetSamplesFileName,
                              real_number_vector_vector &trainSetSamples) {
    std::ifstream trainSetSamplesFile(trainSetSamplesFileName);
    if (!trainSetSamplesFile) {
        std::string exceptionMessage = std::string("Error opening template file ") + std::string(trainSetSamplesFileName) + std::string(".");
        throw std::logic_error(exceptionMessage.c_str());
    }
    trainSetSamplesFile.exceptions(std::ios_base::failbit | std::ios_base::badbit | std::ios_base::eofbit);
    unsigned int dimsCount, samplesCount;
    trainSetSamplesFile >> samplesCount >> dimsCount;
    trainSetSamples.clear();
    trainSetSamples.resize(samplesCount, real_number_vector(dimsCount, 0));
    for (unsigned int sampleI = 0; sampleI < samplesCount; sampleI++) {
        for (unsigned int dimI = 0; dimI < dimsCount; dimI++) {
            trainSetSamplesFile >> trainSetSamples[sampleI][dimI];
        }
    }
    trainSetSamplesFile.close();
}

void computeSampleStatistics(const real_number_vector_vector &samples, real_number_vector &samplesMeans, real_number_vector &samplesStdDevs, real_number_vector_vector &samplesCovar)
{
    unsigned int samplesCount(samples.size()), samplesDimensionality(samples[0].size());
    arma::Mat<real_number> arma_samples(samplesCount, samplesDimensionality);
    for (unsigned int sampleI = 0; sampleI < samplesCount; sampleI++) {
        arma::Row<real_number> row(samples[sampleI]);
        arma_samples.row(sampleI) = row;
    }
    samplesMeans.clear();
    samplesMeans.resize(samplesDimensionality);
    samplesStdDevs.clear();
    samplesStdDevs.resize(samplesDimensionality);
    samplesCovar.clear();
    samplesCovar.resize(samplesDimensionality);
    arma::Row<real_number> arma_mean = arma::mean(arma_samples, 0);
    arma::Row<real_number> arma_std_dev = arma::stddev(arma_samples, 0);
    arma::Mat<real_number> arma_covar = arma::cov(arma_samples, 0);
    for (unsigned int dimI = 0; dimI < samplesDimensionality; dimI++) {
        samplesMeans[dimI] = arma_mean.at(dimI);
        samplesStdDevs[dimI] = arma_std_dev.at(dimI);
        samplesCovar[dimI] = real_number_vector(samplesDimensionality);
        for (unsigned int dimII = 0; dimII < samplesDimensionality; dimII++) {
            samplesCovar[dimI][dimII] = arma_covar.at(dimI, dimII);
        }
    }
}


void readSamplesFilesFromPath(const std::string samplesFilesPathName,
                              const std::string samplesFilesExtension,
                              std::vector<std::string> &samplesFilesNames,
                              std::vector<real_number_vector_vector> &folderSamples)
{
    std::filesystem::path  samplesFilesPath(samplesFilesPathName);
    if (!std::filesystem::exists(samplesFilesPath)) {
        throw std::ifstream::failure(std::string("Enrollment samples files path ") + std::string(samplesFilesPathName) + std::string(" does not exist."));
    }
    if (!std::filesystem::is_directory(samplesFilesPath)) {
        throw std::ifstream::failure(std::string("Enrollment samples files path ") + std::string(samplesFilesPathName) + std::string(" is not a directory."));
    }
    samplesFilesNames.clear();
    folderSamples.clear();
    bool samplesLengthInitialized(false);
    unsigned int samplesLength(0);
    for (auto const& dir_entry : std::filesystem::directory_iterator{samplesFilesPath}) {
        if (dir_entry.is_regular_file()) {
            std::string extension = dir_entry.path().extension();
            std::string stem = dir_entry.path().stem();
            if ((extension == samplesFilesExtension) && (dir_entry.path().has_stem())) {
                real_number_vector_vector fileSamples;
                std::cout << "\rReading samples from file " << dir_entry.path() << " ... ";
                parseSamplesFile(dir_entry.path(), fileSamples);
                if (!samplesLengthInitialized) {
                    samplesLengthInitialized = true;
                    samplesLength = fileSamples.begin()->size();
                } else {
                    if (samplesLength != fileSamples.begin()->size()) {
                        throw std::logic_error(std::string("Samples length in samples file ") +
                                               dir_entry.path().string() + std::string(" (") +
                                               std::to_string(folderSamples.begin()->size()) +
                                               std::string(") is different than the previously found in samples folder (") +
                                               std::to_string(samplesLength) + std::string(")."));
                    }
                }
                // std::cout << fileSamples.size() << " samples of dimensionality " << fileSamples.begin()->size() << " read from file " << dir_entry.path() << std::endl;
                folderSamples.push_back(fileSamples);
                samplesFilesNames.push_back(dir_entry.path().stem());
            } else {
                std::cout << "Discarding regular file " << dir_entry.path() << " with extension " << extension << ", different than " << samplesFilesExtension << std::endl;
            }
        } else {
            std::cout << "Discarding non-regular file " << dir_entry.path() << std::endl;
        }
    }
    std::cout << "Done." << std::endl;
}
