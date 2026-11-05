/// \file mhstce.cpp
/// \brief Implementation of the \ref MultimodalHybridStrengthenedTurboCodeFuzzyExtractor class.

#include "mhstce.h"
#include <TurboCodes/vectorfiles.h>
#include <fstream>
#include <iostream>
#include <stdexcept>

MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::MultimodalHybridStrengthenedTurboCodeFuzzyExtractor() {}

std::vector<HybridStrengthenedTurboCodeFuzzyExtractor> &MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::hstcfes()
{
    return m_hstcfes;
}

void MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::setHstcfes(const std::vector<HybridStrengthenedTurboCodeFuzzyExtractor> &newHstcfes)
{
    if (newHstcfes.size() < 2) {
        throw std::logic_error("MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::setHstcfes: size of HybridStrengthenedTurboCodeFuzzyExtractor vector should be 2 or more.");
    }
    for (unsigned int hstcfeI = 1; hstcfeI < newHstcfes.size(); hstcfeI++) {
        if (newHstcfes[0].turboCode() != newHstcfes[hstcfeI].turboCode()) {
            throw std::logic_error("MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::setHstcfes: incompatible TurboCodes.");
        }
    }
    m_hstcfes = newHstcfes;
    m_modalitiesCount = m_hstcfes.size();
    m_inputDimensionality = 0;
    for (unsigned int modalityI = 0; modalityI < m_modalitiesCount; modalityI++) {
        m_inputDimensionality += m_hstcfes[modalityI].inputDimensionality();
    }
    // Do not copy newHstcfes[0].turboCode(): once its user-specific data is loaded, it carries that user's interleavers and
    // puncturing (codeword length = that user's capacity), so its rate is no longer 1/3..1/6 and bitLoading() rejects it.
    // Start from the unpunctured mother code in the configuration file instead; generate() sets this subject's own
    // interleavers and puncturing.
    m_turboCodeConfigFileName = newHstcfes[0].turboCodeConfigFileName();
    m_turboCodeMaxDecodingIterations = newHstcfes[0].turboCodeMaxDecodingIterations();
    parseTurboCodeConfigFile();
}

void MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::setHstcfesNoFurtherUpdates(const std::vector<HybridStrengthenedTurboCodeFuzzyExtractor> &newHstcfes)
{
    if (newHstcfes.size() < 2) {
        throw std::logic_error("MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::setHstcfes: size of HybridStrengthenedTurboCodeFuzzyExtractor vector should be 2 or more.");
    }
    for (unsigned int hstcfeI = 1; hstcfeI < newHstcfes.size(); hstcfeI++) {
        if (newHstcfes[0].turboCode() != newHstcfes[hstcfeI].turboCode()) {
            throw std::logic_error("MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::setHstcfes: incompatible TurboCodes.");
        }
    }
    m_hstcfes = newHstcfes;
}

unsigned int MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::modalitiesCount()
{
    return m_hstcfes.size();
}

void MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::generate(std::vector<real_number_vector_vector> &enrollmentSamplesVector)
{
    if (enrollmentSamplesVector.size() != m_modalitiesCount) {
        throw std::logic_error("Inconsistent number of modalities.");
    }
    real_number_vector enrollmentSamplesMeansVector;
    real_number_vector enrollmentSamplesStdDevsVector;
    real_number_vector normalizedEnrollmentSamplesMeansVector;
    real_number_vector normalizedEnrollmentSamplesStdDevsVector;
    std::vector<real_number_vector_vector> normalizedEnrollmentSamples;
    //#ifdef DEBUG
    std::cout << "\t" << "Computing normalized enrollment samples statistics... ";
    std::cout.flush();
    //#endif
    for (unsigned int modalityI = 0; modalityI < m_modalitiesCount; modalityI++) {
        real_number_vector_vector modalityNormalizedEnrollmentSamples;
        real_number_vector modalityNormalizedEnrollmentSamplesMeansVector;
        real_number_vector modalityNormalizedEnrollmentSamplesStdDevsVector;
        real_number_vector modalityEnrollmentSamplesMeansVector;
        real_number_vector modalityEnrollmentSamplesStdDevsVector;
        real_number_vector_vector covarianceMatrix;
        m_hstcfes[modalityI].uniformNormalizationBatch(enrollmentSamplesVector[modalityI], modalityNormalizedEnrollmentSamples);
        computeSampleStatistics(modalityNormalizedEnrollmentSamples,
                                modalityNormalizedEnrollmentSamplesMeansVector,
                                modalityNormalizedEnrollmentSamplesStdDevsVector,
                                covarianceMatrix);
        normalizedEnrollmentSamplesMeansVector.insert(normalizedEnrollmentSamplesMeansVector.end(),
                                                      modalityNormalizedEnrollmentSamplesMeansVector.begin(),
                                                      modalityNormalizedEnrollmentSamplesMeansVector.end());
        normalizedEnrollmentSamplesStdDevsVector.insert(normalizedEnrollmentSamplesStdDevsVector.end(),
                                                        modalityNormalizedEnrollmentSamplesStdDevsVector.begin(),
                                                        modalityNormalizedEnrollmentSamplesStdDevsVector.end());
        computeSampleStatistics(enrollmentSamplesVector[modalityI],
                                modalityEnrollmentSamplesMeansVector,
                                modalityEnrollmentSamplesStdDevsVector,
                                covarianceMatrix);
        enrollmentSamplesMeansVector.insert(enrollmentSamplesMeansVector.end(),
                                            modalityEnrollmentSamplesMeansVector.begin(),
                                            modalityEnrollmentSamplesMeansVector.end());
        enrollmentSamplesStdDevsVector.insert(enrollmentSamplesStdDevsVector.end(),
                                              modalityEnrollmentSamplesStdDevsVector.begin(),
                                              modalityEnrollmentSamplesStdDevsVector.end());
        normalizedEnrollmentSamples.push_back(modalityNormalizedEnrollmentSamples);
    }
    setInputDimensionality(enrollmentSamplesMeansVector.size());
    //#ifdef DEBUG
    std::cout << "\t" << "Input dimensionality set to " << inputDimensionality() << std::endl;
    std::cout << "\t" << "Done." << std::endl;
    std::cout << "\t" << "Computing normalized enrollment samples bit-loading... ";
    std::cout.flush();
    //#endif
    bitLoading(normalizedEnrollmentSamplesStdDevsVector);
    //#ifdef DEBUG
    std::cout << "\t" << "Done." << std::endl;
    std::cout << "\t" << "Code-offset capacity after bit-loading: " << m_codeOffsetTotalCapacity << std::endl;
    std::cout << "\t" << "Number of fuzzy commitment features: "
              << m_codeOffsetFeaturesIndexes.size() << std::endl;
    std::cout << "\t" << "Number of code offset features: "
              << m_fuzzyCommitmentFeaturesIndexes.size() << std::endl;
    std::cout << "\t" << "Building fuzzy commitment enrollment samples... ";
    std::cout.flush();
    //#endif
    computeFuzzyCommitmentEnrollmentSamplesStatistics(enrollmentSamplesMeansVector, enrollmentSamplesStdDevsVector);
    //#ifdef DEBUG
    std::cout << "\t" << "Done." << std::endl;
    std::cout << "\t" << "Building code-offset enrollment samples... " << std::endl;
    std::cout << "\t" << "Size of normalizedEnrollmentSamplesMeansVector: "
              << normalizedEnrollmentSamplesMeansVector.size() << std::endl;
    std::cout << "\t" << "Size of normalizedEnrollmentSamplesStdDevsVector: "
              << normalizedEnrollmentSamplesStdDevsVector.size() << std::endl;
    std::cout.flush();
    //#endif
    computeCodeOffsetEnrollmentSamplesStatistics(normalizedEnrollmentSamplesMeansVector, normalizedEnrollmentSamplesStdDevsVector);
    //#ifdef DEBUG
    std::cout << "\t" << "Done." << std::endl;
    std::cout << "\t" << "Id: " << m_userId << std::endl;
    std::cout << "\t" << "Finding the best fuzzy commitment scheme... " << std::endl;
    std::cout.flush();
    //#endif
    findBestFuzzyCommitment();
    //#ifdef DEBUG
    std::cout << "\t" << "Done." << std::endl;
    //#endif
    // unsigned int minFeatureI = 0;
    // while ((m_codeOffsetTotalCapacity + m_fuzzyCommitmentTotalCapacity) % (m_turboCode.inputOutputLengths().second/m_turboCode.inputOutputLengths().first)) {
    //     if (m_codeOffsetTotalCapacity > 0) {
    //         for (unsigned int featureI = minFeatureI; featureI < m_codeOffsetFeaturesCapacities.size(); featureI++) {
    //             if (m_codeOffsetFeaturesCapacities[featureI] >= 2) {
    //                 m_codeOffsetFeaturesCapacities[featureI]--;
    //                 m_codeOffsetTotalCapacity--;
    //                 m_fuzzyExtractorCapacity--;
    //                 minFeatureI = featureI + 1;
    //                 break;
    //             }
    //         }
    //     } else {
    //         throw std::logic_error(std::string("HybridStrengthenedTurboCodeFuzzyExtractor::generate(): total capacity is ") +
    //                                std::to_string(m_codeOffsetTotalCapacity + m_fuzzyCommitmentTotalCapacity) +
    //                                std::string(", is not a multiple of ") +
    //                                std::to_string(m_turboCode.inputOutputLengths().second/m_turboCode.inputOutputLengths().first) +
    //                                std::string(", and code offset capacity is 0. Something went wrong."));
    //     }
    // }
    //#ifdef DEBUG
    std::cout << "\t" << "Done." << std::endl;
    std::cout << "\t" << "Number of combined features: " << m_combinedFeaturesCount << std::endl;
    std::cout << "\t" << "Code-offset capacity: " << m_codeOffsetTotalCapacity << std::endl;
    std::cout << "\t" << "Fuzzy commitment capacity: " << m_fuzzyCommitmentTotalCapacity << std::endl;
    std::cout << "\t" << "Total capacity: " << m_fuzzyExtractorCapacity << std::endl;
    std::cout << "\t" << "Setting input message length... ";
    std::cout.flush();
    //#endif
    if ((m_fuzzyExtractorCapacity*m_turboCode.inputOutputLengths().first)/m_turboCode.inputOutputLengths().second > 0) {
        unsigned int tcInputMessageLength = (m_fuzzyExtractorCapacity*m_turboCode.inputOutputLengths().first)/m_turboCode.inputOutputLengths().second;
        if (((m_fuzzyExtractorCapacity*m_turboCode.inputOutputLengths().first) % m_turboCode.inputOutputLengths().second) > 0) {
            tcInputMessageLength++;
        }
        //#ifdef DEBUG
        std::cout << "\t" << "Turbo code message length: "
                  << tcInputMessageLength << std::endl;
        //#endif
        setTurboCodeInputMessageLength(tcInputMessageLength);        //#ifdef DEBUG
        std::cout << "\t" << "Done." << std::endl;
        std::cout << "\t" << "Computting random interleavers and full puncturing patterns... " << std::endl;
        std::cout.flush();
        //#endif
        computeRandomizedInterleaversAndPuncturingPatterns();
        // Check whether it is necessary to puncture the TurboCode to handle the final output length
        unsigned int bitsToPunctureCount = ((m_turboCode.inputOutputLengths().second/m_turboCode.inputOutputLengths().first) -
                (m_codeOffsetTotalCapacity + m_fuzzyCommitmentTotalCapacity) % (m_turboCode.inputOutputLengths().second/m_turboCode.inputOutputLengths().first)) % (m_turboCode.inputOutputLengths().second/m_turboCode.inputOutputLengths().first);
        binary_polynomial_vector puncturingPatterns = turboCode().puncturingPatterns();
        while (bitsToPunctureCount > 0 ) {
            int generatorToBePuncturedIndex = bitsToPunctureCount - 1;
            int puncturingPatternLength = puncturingPatterns[generatorToBePuncturedIndex].size();
            int puncturedBitIndex = puncturingPatternLength - 1;
            puncturingPatterns[generatorToBePuncturedIndex][puncturedBitIndex] = 0;
            bitsToPunctureCount--;
        }
        m_turboCode.setPuncturingPatterns(puncturingPatterns);
        //#ifdef DEBUG
        std::cout << "\t" << "Done." << std::endl;
        std::cout << "\t" << "Generating randomized input message and codeword... ";
        std::cout.flush();
        //#endif
        generateRandomBinaryMessageAndCodeWord();
        //#ifdef DEBUG
        std::cout << "\t" << "Done." << std::endl;
        // The random permutation of symbols has to be performed now, instead of after committing the code-offset and fuzzy commitment features
        std::cout << "\t" << "Randomly permuting code-offset and fuzzy commitment... ";
        std::cout.flush();
        //#endif
        randomlyPermute();
        //#ifdef DEBUG
        std::cout << "\t" << "Done." << std::endl;
        std::cout << "\t" << "Enrolling code-offset... ";
        std::cout.flush();
        //#endif
        enrollCodeOffset();
        //#ifdef DEBUG
        std::cout << "\t" << "Done." << std::endl;
        std::cout << "\t" << "Enrolling fuzzy commitment... ";
        std::cout.flush();
        //#endif
        enrollFuzzyCommitment();
        //#ifdef DEBUG
        std::cout << "\t" << "Done." << std::endl;
        std::cout << "\t" << "Number of combined features: " << m_combinedFeaturesCount << std::endl;
        std::cout << "\t" << "Code-offset capacity: " << m_codeOffsetTotalCapacity << std::endl;
        std::cout << "\t" << "Fuzzy commitment capacity: " << m_fuzzyCommitmentTotalCapacity << std::endl;
        std::cout << "\t" << "Total capacity: " << m_fuzzyExtractorCapacity << std::endl;
        std::cout << "\t" << "Setting TC symbol indexes and PDFs... ";
        std::cout.flush();
        //#endif
        m_turboCode.setSymbolIndexes(m_symbolIndexes);
        m_turboCode.setPdfs0(m_pdfs0);
        m_turboCode.setPdfs1(m_pdfs1);
        //#ifdef DEBUG
        std::cout << "\t" << "Done." << std::endl;
        //#endif
        m_validForReproduce = true;
    } else {
        m_validForReproduce = false;
    }
}

binary_polynomial MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::reproduce(const real_number_vector_vector &multiModalProbeSample)
{
    std::cout << "Starting MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::reproduce(const real_number_vector_vector &multiModalProbeSample) ..." << std::endl;
    if (m_symbolsPermutation.size() != m_codeOffsetFeaturesCount + m_fuzzyCommitmentTotalCapacity) {
        throw std::logic_error(std::string("MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::reproduce: Length of symbols permutation (") +
                               std::to_string(m_symbolsPermutation.size()) +
                               std::string(") inconsistent with m_codeOffsetFeaturesCount + m_fuzzyCommitmentTotalCapacity = ") +
                               std::to_string(m_codeOffsetFeaturesCount) + std::string(" + ") + std::to_string(m_fuzzyCommitmentTotalCapacity) +
                               std::string(" = ") + std::to_string(m_codeOffsetFeaturesCount + m_fuzzyCommitmentTotalCapacity));
    }
    binary_polynomial decodedMessage(m_turboCodeInputMessageLength);
    real_number_vector probeSample;
    for (unsigned int modalityI = 0; modalityI < m_modalitiesCount; modalityI++) {
        probeSample.insert(probeSample.end(), multiModalProbeSample[modalityI].begin(), multiModalProbeSample[modalityI].end());
    }

    real_number_vector codeOffsetProbeFeatures(m_codeOffsetFeaturesCount);
    real_number_vector fuzzyCommitmentProbeFeatures(m_fuzzyCommitmentTotalCapacity);
    normalizeCodeOffsetSample(probeSample, codeOffsetProbeFeatures);
    projectFuzzyCommitment(probeSample, fuzzyCommitmentProbeFeatures);
    real_number_vector_vector turboCodeInputSymbols(m_codeOffsetFeaturesCount + m_fuzzyCommitmentTotalCapacity, real_number_vector(1, 0.0));
#ifdef DEBUG
    std::cout << "\nDecommitting code-offset features... " << std::endl;
    std::cout << std::endl;
#endif
    if (m_commitment.size() != m_codeOffsetFeaturesCount + m_fuzzyCommitmentTotalCapacity) {
        throw std::logic_error(std::string("HybridStrengthenedTurboCodeFuzzyExtractor::reproduce: Length of code-offset commitment (") +
                               std::to_string(m_commitment.size()) +
                               std::string(") differs from code-offset features count (") + std::to_string(m_codeOffsetFeaturesCount) +
                               std::string(") plus the number of fuzzy-committed combined features (") +
                               std::to_string(m_fuzzyCommitmentTotalCapacity) + std::string(")."));
    }
    for (unsigned int codeOffsetFeatureI = 0; codeOffsetFeatureI < m_codeOffsetFeaturesCount; codeOffsetFeatureI++) {
#ifdef DEBUG
        std::cout << "Decommitting code-offset feature " << codeOffsetFeatureI << " with biometric value: " << codeOffsetProbeFeatures[codeOffsetFeatureI] << std::endl;
#endif
        turboCodeInputSymbols[codeOffsetFeatureI][0] =
                m_commitment[codeOffsetFeatureI] - codeOffsetProbeFeatures[codeOffsetFeatureI];
        while (turboCodeInputSymbols[codeOffsetFeatureI][0] < 0) {
            turboCodeInputSymbols[codeOffsetFeatureI][0] += 2.0*M_PI;
        }
        while (turboCodeInputSymbols[codeOffsetFeatureI][0] > 2.0*M_PI) {
            turboCodeInputSymbols[codeOffsetFeatureI][0] -= 2.0*M_PI;
        }
#ifdef DEBUG
        std::cout << "Decommitted code-offset feature " << codeOffsetFeatureI << ": "
                  << turboCodeInputSymbols[codeOffsetFeatureI][0] << std::endl;
#endif
    }
#ifdef DEBUG
    std::cout << "Done!" << std::endl;
    std::cout << "Computing fuzzy commitment features... ";
    std::cout << std::endl;
#endif
    for (unsigned int fuzzyCommitmentFeatureI = 0; fuzzyCommitmentFeatureI < m_fuzzyCommitmentTotalCapacity; fuzzyCommitmentFeatureI++) {
#ifdef DEBUG
        std::cout << "Value for fuzzy-commitment feature " << fuzzyCommitmentFeatureI << " decommitment: "
                  << m_fuzzyCommitmentFeaturesIndexes[fuzzyCommitmentFeatureI] << std::endl;
#endif
        turboCodeInputSymbols[fuzzyCommitmentFeatureI + m_codeOffsetFeaturesCount][0] = fuzzyCommitmentProbeFeatures[fuzzyCommitmentFeatureI];
    }
#ifdef DEBUG
    std::cout << "Done!" << std::endl;
    std::cout << std::endl;
    std::cout << "Done!" << std::endl;
    std::cout << "Performing Turbo Code decoding... ";
    std::cout << std::endl;
#endif
    unsigned int totalIterations;
    m_turboCode.decode(turboCodeInputSymbols, m_turboCodeMaxDecodingIterations, decodedMessage, totalIterations);
#ifdef DEBUG
    std::cout << "Done!" << std::endl;
#endif
    return decodedMessage;
}

binary_polynomial MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::reproduce(const real_number_vector &probeSample)
{
    // std::cout << "Starting MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::reproduce(const real_number_vector &probeSample) ..." << std::endl;
    if (m_symbolsPermutation.size() != m_codeOffsetFeaturesCount + m_fuzzyCommitmentTotalCapacity) {
        throw std::logic_error(std::string("HybridStrengthenedTurboCodeFuzzyExtractor::reproduce: Length of symbols permutation (") +
                               std::to_string(m_symbolsPermutation.size()) +
                               std::string(") inconsistent with m_codeOffsetFeaturesCount + m_fuzzyCommitmentTotalCapacity = ") +
                               std::to_string(m_codeOffsetFeaturesCount) + std::string(" + ") + std::to_string(m_fuzzyCommitmentTotalCapacity) +
                               std::string(" = ") + std::to_string(m_codeOffsetFeaturesCount + m_fuzzyCommitmentTotalCapacity));
    }
    // std::cout << "Building decodedMessage, codeOffsetProbeFeatures, and fuzzyCommitmentProbeFeatures... ";
    // std::cout.flush();
    binary_polynomial decodedMessage(m_turboCodeInputMessageLength);
    real_number_vector codeOffsetProbeFeatures(m_codeOffsetFeaturesCount);
    real_number_vector fuzzyCommitmentProbeFeatures(m_fuzzyCommitmentTotalCapacity);
    // std::cout << "Done." << std::endl;
    // std::cout << "Normalizing code offset features... ";
    // for (unsigned int modalityI = 0; modalityI < m_modalitiesCount; modalityI++) {
    //     std::cout << "Size of normalizer for modality " << modalityI + 1
    //               << ": ";
    //     std::cout.flush();
    //     std::cout << m_hstcfes[modalityI].uniformNormalizer()->size() << std::endl;
    //     if (m_hstcfes[modalityI].uniformNormalizer()->size() > 0) {
    //         std::cout << "\tSize of first dimension tree: ";
    //         std::cout.flush();
    //         std::cout << m_hstcfes[modalityI].uniformNormalizer()->begin()->size() << std::endl;
    //         std::cout << "\tSize of last dimension tree: ";
    //         std::cout.flush();
    //         std::cout << m_hstcfes[modalityI].uniformNormalizer()->rbegin()->size() << std::endl;
    //     }
    // }
    // std::cout << "Size of m_codeOffsetFeaturesIndexes: " << m_codeOffsetFeaturesIndexes.size()
    //           << "\tVector:";
    // for (unsigned int dimI = 0; dimI < m_codeOffsetFeaturesIndexes.size(); dimI++) {
    //     std::cout << " " << m_codeOffsetFeaturesIndexes[dimI];
    // }
    // std::cout << std::endl;
    normalizeCodeOffsetSample(probeSample, codeOffsetProbeFeatures);
    // std::cout << "Done." << std::endl;
    // std::cout << "Projecting fuzzy commitment features... ";
    projectFuzzyCommitment(probeSample, fuzzyCommitmentProbeFeatures);
    // std::cout << "Done." << std::endl;
    real_number_vector_vector turboCodeInputSymbols(m_codeOffsetFeaturesCount + m_fuzzyCommitmentTotalCapacity, real_number_vector(1, 0.0));
    //#ifdef DEBUG
    // std::cout << "\nDecommitting code-offset features... " << std::endl;
    // std::cout << std::endl;
    //#endif
    if (m_commitment.size() != m_codeOffsetFeaturesCount + m_fuzzyCommitmentTotalCapacity) {
        throw std::logic_error(std::string("HybridStrengthenedTurboCodeFuzzyExtractor::reproduce: Length of code-offset commitment (") +
                               std::to_string(m_commitment.size()) +
                               std::string(") differs from code-offset features count (") + std::to_string(m_codeOffsetFeaturesCount) +
                               std::string(") plus the number of fuzzy-committed combined features (") +
                               std::to_string(m_fuzzyCommitmentTotalCapacity) + std::string(")."));
    }
    for (unsigned int codeOffsetFeatureI = 0; codeOffsetFeatureI < m_codeOffsetFeaturesCount; codeOffsetFeatureI++) {
        //#ifdef DEBUG
        // std::cout << "Decommitting code-offset feature " << codeOffsetFeatureI << " with biometric value: " << codeOffsetProbeFeatures[codeOffsetFeatureI] << std::endl;
        //#endif
        turboCodeInputSymbols[codeOffsetFeatureI][0] =
                m_commitment[codeOffsetFeatureI] - codeOffsetProbeFeatures[codeOffsetFeatureI];
        while (turboCodeInputSymbols[codeOffsetFeatureI][0] < 0) {
            turboCodeInputSymbols[codeOffsetFeatureI][0] += 2.0*M_PI;
        }
        while (turboCodeInputSymbols[codeOffsetFeatureI][0] > 2.0*M_PI) {
            turboCodeInputSymbols[codeOffsetFeatureI][0] -= 2.0*M_PI;
        }
        //#ifdef DEBUG
        // std::cout << "Decommitted code-offset feature " << codeOffsetFeatureI << ": "
        //           << turboCodeInputSymbols[codeOffsetFeatureI][0] << std::endl;
        //#endif
    }
    //#ifdef DEBUG
    // std::cout << "Done!" << std::endl;
    // std::cout << "Computing fuzzy commitment features... ";
    // std::cout << std::endl;
    //#endif
    for (unsigned int fuzzyCommitmentFeatureI = 0; fuzzyCommitmentFeatureI < m_fuzzyCommitmentTotalCapacity; fuzzyCommitmentFeatureI++) {
        //#ifdef DEBUG
        // std::cout << "Value for fuzzy-commitment feature " << fuzzyCommitmentFeatureI << " decommitment: "
        //           << m_fuzzyCommitmentFeaturesIndexes[fuzzyCommitmentFeatureI] << std::endl;
        //#endif
        turboCodeInputSymbols[fuzzyCommitmentFeatureI + m_codeOffsetFeaturesCount][0] = fuzzyCommitmentProbeFeatures[fuzzyCommitmentFeatureI];
    }
    //#ifdef DEBUG
    // std::cout << "Done!" << std::endl;
    // std::cout << std::endl;
    // std::cout << "Done!" << std::endl;
    // std::cout << "Performing Turbo Code decoding... ";
    // std::cout << std::endl;
    //#endif
    unsigned int totalIterations;
    m_turboCode.decode(turboCodeInputSymbols, m_turboCodeMaxDecodingIterations, decodedMessage, totalIterations);
    // std::cout << "User id " << m_userId
    //           << " \tinput message length (in symbols): " << turboCodeInputSymbols.size()
    //           << " \tmessage length (in bits): " << m_turboCodeInputMessageLength
    //           << " \tmessage length (in bits): " << m_turboCodeInputMessage.size()
    //    << " \tturbo code input output lengths: " << m_turboCode.inputOutputLengths().first << ", " << m_turboCode.inputOutputLengths().second
    //           << " \tdecoded message length (in bits): " << decodedMessage.size() << std::endl;
    //#ifdef DEBUG
    // std::cout << "Done!" << std::endl;
    //#endif
    return decodedMessage;
}

void MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::uniformNormalizationBatch(const real_number_vector_vector &samples, real_number_vector_vector &normalizedSamples)
{
    for (unsigned int sampleI = 0; sampleI < samples.size(); sampleI++) {
        unsigned int dimOffset(0);
        for (unsigned int modalityI = 0; modalityI < m_modalitiesCount; modalityI++) {
            std::vector<std::map<real_number,real_number>> *uniformNormalizer = m_hstcfes[modalityI].uniformNormalizer();
            for (unsigned int dimI = 0; dimI < uniformNormalizer->size(); dimI++) {
                std::_Rb_tree_const_iterator<std::pair<const real_number, real_number>> upperBound = (*uniformNormalizer)[dimI].upper_bound(samples[sampleI][dimI + dimOffset]);
                if (upperBound != (*uniformNormalizer)[dimI].end()) {
                    normalizedSamples[sampleI][dimI + dimOffset] = upperBound->second;
                } else {
                    normalizedSamples[sampleI][dimI + dimOffset] = 1.0;
                }
		normalizedSamples[sampleI][dimI + dimOffset] *= 2*M_PI;
            }
            dimOffset += uniformNormalizer->size();
        }
    }
}

void MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::uniformNormalization(const real_number_vector &sample, real_number_vector &normalizedSample)
{
    unsigned int dimOffset(0);
    for (unsigned int modalityI = 0; modalityI < m_modalitiesCount; modalityI++) {
        std::vector<std::map<real_number,real_number>> *uniformNormalizer = m_hstcfes[modalityI].uniformNormalizer();
        for (unsigned int dimI = 0; dimI < uniformNormalizer->size(); dimI++) {
            std::_Rb_tree_const_iterator<std::pair<const real_number, real_number>> upperBound = (*uniformNormalizer)[dimI].upper_bound(sample[dimI + dimOffset]);
            if (upperBound != (*uniformNormalizer)[dimI].end()) {
                normalizedSample[dimI + dimOffset] = upperBound->second;
            } else {
                normalizedSample[dimI + dimOffset] = 1.0;
            }
	    normalizedSample[dimI + dimOffset] *= 2*M_PI;
        }
        dimOffset += uniformNormalizer->size();
    }
}

void MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::normalizeCodeOffsetSample(const real_number_vector &sample, real_number_vector &normalizedSample)
{
    normalizedSample.resize(m_codeOffsetFeaturesCount);
    for (unsigned int codeOffsetDimI = 0; codeOffsetDimI < m_codeOffsetFeaturesCount; codeOffsetDimI++) {
        unsigned int inputDimI = m_codeOffsetFeaturesIndexes[codeOffsetDimI];
        std::vector<std::map<real_number,real_number>> *uniformNormalizer;
        unsigned int dimOffset = 0;
        unsigned int actualModality = 0;
        for (unsigned int modalityI = 0; modalityI < m_modalitiesCount; modalityI++) {
            if (inputDimI < dimOffset + m_hstcfes[modalityI].uniformNormalizer()->size()) {
                actualModality = modalityI;
                break;
            }
            dimOffset += m_hstcfes[modalityI].uniformNormalizer()->size();
        }
        // std::cout << "Code offset sample index = " << codeOffsetDimI
        //           << "; dimOffset = " << dimOffset
        //           << "; actualModality = " << actualModality << std::endl;
        std::_Rb_tree_const_iterator<std::pair<const real_number, real_number>> upperBound = m_hstcfes[actualModality].uniformNormalizer()->at(inputDimI - dimOffset).upper_bound(sample[inputDimI]);
        if (upperBound != m_hstcfes[actualModality].uniformNormalizer()->at(inputDimI - dimOffset).end()) {
            normalizedSample[codeOffsetDimI] = upperBound->second;
        } else {
            normalizedSample[codeOffsetDimI] = 1.0;
        }
        normalizedSample[codeOffsetDimI] *= 2.0*M_PI;
    }

}

void MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::saveGeneralData(const std::string &fileName)
{
    std::ofstream file(fileName);
    if (!file) {
        throw std::ofstream::failure(std::string("Unable to open file ") + fileName + std::string(" for writing."));
    }
    file << "TurboCodeConfigFile " << m_turboCodeConfigFileName << std::endl;
    file << "TurboCodeMaxDecodingIterations " << m_turboCodeMaxDecodingIterations << std::endl;
    file << "ModalitiesCount " << m_modalitiesCount << std::endl;
    file << "CodeOffsetStdDevFactor " << m_codeOffsetStdDevFactor << std::endl;
    file << "CodeOffsetStdDevOffset " << m_codeOffsetStdDevOffset << std::endl;
    file << "FuzzyCommitmentMinReliability " << m_fuzzyCommitmentMinReliability << std::endl;
    file.close();
}

void MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::loadGeneralData(const std::string &fileName)
{
    std::ifstream file(fileName);
    if (!file) {
        throw std::ifstream::failure(std::string("Unable to open file ") + fileName + std::string(" for reading."));
    }
    std::string label;
    file >> label;
    if (label != std::string("TurboCodeConfigFile")) {
        throw std::logic_error(std::string("HybridStrengthenedTurboCodeFuzzyExtractor::loadGeneralData: Label in  configuration file ")
                               + fileName + std::string(": ") + label + std::string(". TurboCodeConfigFile was expected."));
    }
    file >> m_turboCodeConfigFileName;
    file >> label;
    if (label != std::string("TurboCodeMaxDecodingIterations")) {
        throw std::logic_error(std::string("HybridStrengthenedTurboCodeFuzzyExtractor::loadGeneralData: Label in  configuration file ")
                               + fileName + std::string(": ") + label + std::string(". TurboCodeConfigFile was expected."));
    }
    file >> m_turboCodeMaxDecodingIterations;
    file >> label;
    if (label != std::string("ModalitiesCount")) {
        throw std::logic_error(std::string("HybridStrengthenedTurboCodeFuzzyExtractor::loadGeneralData: Label in  configuration file ")
                               + fileName + std::string(": ") + label + std::string(". ModalitiesCount was expected."));
    }
    file >> m_modalitiesCount;
    file >> label;
    if (label != std::string("CodeOffsetStdDevFactor")) {
        throw std::logic_error(std::string("HybridStrengthenedTurboCodeFuzzyExtractor::loadGeneralData: Label in  configuration file ")
                               + fileName + std::string(": ") + label + std::string(". CodeOffsetStdDevFactor was expected."));
    }
    file >> m_codeOffsetStdDevFactor;
    file >> label;
    if (label != std::string("CodeOffsetStdDevOffset")) {
        throw std::logic_error(std::string("HybridStrengthenedTurboCodeFuzzyExtractor::loadGeneralData: Label in  configuration file ")
                               + fileName + std::string(": ") + label + std::string(". CodeOffsetStdDevOffset was expected."));
    }
    file >> m_codeOffsetStdDevOffset;
    file >> label;
    if (label != std::string("FuzzyCommitmentMinReliability")) {
        throw std::logic_error(std::string("HybridStrengthenedTurboCodeFuzzyExtractor::loadGeneralData: Label in  configuration file ")
                               + fileName + std::string(": ") + label + std::string(". FuzzyCommitmentMinReliability was expected."));
    }
    file >> m_fuzzyCommitmentMinReliability;
    file.close();
    std::cout << "Parsing TC config file... ";
    std::cout.flush();
    parseTurboCodeConfigFile();
    std::cout << "Done." << std::endl;
    std::cout << "Loading train set samples and setting up normalizers ... ";
    std::cout.flush();
    std::cout << "Done." << std::endl;
}


void MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::computeFuzzyCommitmentEnrollmentSamplesStatistics(const real_number_vector &enrollmentSamplesMeansVector, const real_number_vector &enrollmentSamplesStdDevsVector)
{
    m_fuzzyCommitmentEnrollmentSamplesMean = real_number_vector(m_fuzzyCommitmentFeaturesIndexes.size());
    m_fuzzyCommitmentEnrollmentSamplesStdDev = real_number_vector(m_fuzzyCommitmentFeaturesIndexes.size());
    for (unsigned int featureI = 0; featureI < m_fuzzyCommitmentFeaturesIndexes.size(); featureI++) {
        m_fuzzyCommitmentEnrollmentSamplesMean[featureI] = enrollmentSamplesMeansVector[m_fuzzyCommitmentFeaturesIndexes[featureI]];
        m_fuzzyCommitmentEnrollmentSamplesStdDev[featureI] = enrollmentSamplesStdDevsVector[m_fuzzyCommitmentFeaturesIndexes[featureI]];
    }
}

void MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::computeCodeOffsetEnrollmentSamplesStatistics(const real_number_vector &normalizedEnrollmentSamplesMeansVector, const real_number_vector &normalizedEnrollmentSamplesStdDevsVector)
{
    m_codeOffsetEnrollmentSamplesMean = real_number_vector(m_codeOffsetFeaturesCount);
    m_codeOffsetEnrollmentSamplesStdDev = real_number_vector(m_codeOffsetFeaturesCount);
    for (unsigned int featureI = 0; featureI < m_codeOffsetFeaturesIndexes.size(); featureI++) {
        m_codeOffsetEnrollmentSamplesMean[featureI] = normalizedEnrollmentSamplesMeansVector[m_codeOffsetFeaturesIndexes[featureI]];
        m_codeOffsetEnrollmentSamplesStdDev[featureI] = normalizedEnrollmentSamplesStdDevsVector[m_codeOffsetFeaturesIndexes[featureI]];
    }
}

