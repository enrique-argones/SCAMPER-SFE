/// \file hybridstrengthenedturbocodefuzzyextractor.cpp
/// \brief Implementation of the \ref HybridStrengthenedTurboCodeFuzzyExtractor class.

#include "hybridstrengthenedturbocodefuzzyextractor.h"
#include <TurboCodes/vectorfiles.h>
#include <algorithm>
#include <armadillo>
#include <filesystem>
#include <fstream>
#include <iostream>

std::uniform_int_distribution<int> HybridStrengthenedTurboCodeFuzzyExtractor::m_uniform_binary_distribution = std::uniform_int_distribution<int>(0,1);
std::mt19937 HybridStrengthenedTurboCodeFuzzyExtractor::m_twister = std::mt19937(RAND_SEED);
std::mutex HybridStrengthenedTurboCodeFuzzyExtractor::m_rng_mutex;

HybridStrengthenedTurboCodeFuzzyExtractor::HybridStrengthenedTurboCodeFuzzyExtractor() :
    m_uniformNormalizer(),
    m_turboCodeMaxDecodingIterations(0),
    m_inputDimensionality(0),
    m_validForReproduce(false)
{}

void HybridStrengthenedTurboCodeFuzzyExtractor::setInputDimensionality(uint newInputDimensionality)
{
    m_inputDimensionality = newInputDimensionality;
}

void HybridStrengthenedTurboCodeFuzzyExtractor::bitLoading(const real_number_vector &noiseStd)
{
    uint capacity;
    uint_uint_pair codeRates = m_turboCode.inputOutputLengths();
    if ((codeRates.first*3 != codeRates.second)
        && (codeRates.first*4 != codeRates.second)
        && (codeRates.first*5 != codeRates.second)
        && (codeRates.first*6 != codeRates.second)) {
        throw std::logic_error("Only allowed rates are 1/3, 1/4, 1/5, and 1/6");
    }
    uint n_k = codeRates.second/codeRates.first;
    m_codeOffsetFeaturesIndexes.clear();
    m_codeOffsetFeaturesCapacities.clear();
    m_fuzzyCommitmentFeaturesIndexes.clear();
    m_codeOffsetTotalCapacity = 0;
    m_codeOffsetFeaturesCount = 0;
    for (uint index = 0; index < noiseStd.size(); index++) {
        if (n_k == 3) {
            if (m_codeOffsetStdDevFactor*noiseStd[index] + m_codeOffsetStdDevOffset > .99) {
                capacity = 0;
            } else if (m_codeOffsetStdDevFactor*noiseStd[index] + m_codeOffsetStdDevOffset > .69) {
                capacity = 1;
            } else if (m_codeOffsetStdDevFactor*noiseStd[index] + m_codeOffsetStdDevOffset > .47) {
                capacity = 2;
            } else if (m_codeOffsetStdDevFactor*noiseStd[index] + m_codeOffsetStdDevOffset > .32) {
                capacity = 3;
            } else {
                capacity = 4;
            }
        }
        else if (n_k == 4) {
            if (m_codeOffsetStdDevFactor*noiseStd[index] + m_codeOffsetStdDevOffset > 1.04) {
                capacity = 0;
            } else if (m_codeOffsetStdDevFactor*noiseStd[index] + m_codeOffsetStdDevOffset > .74) {
                capacity = 1;
            } else if (m_codeOffsetStdDevFactor*noiseStd[index] + m_codeOffsetStdDevOffset > .51) {
                capacity = 2;
            } else if (m_codeOffsetStdDevFactor*noiseStd[index] + m_codeOffsetStdDevOffset > .35) {
                capacity = 3;
            } else {
                capacity = 4;
            }
        }
        else if (n_k == 5) {
            if (m_codeOffsetStdDevFactor*noiseStd[index] + m_codeOffsetStdDevOffset > 1.06) {
                capacity = 0;
            } else if (m_codeOffsetStdDevFactor*noiseStd[index] + m_codeOffsetStdDevOffset > .76) {
                capacity = 1;
            } else if (m_codeOffsetStdDevFactor*noiseStd[index] + m_codeOffsetStdDevOffset > .53) {
                capacity = 2;
            } else if (m_codeOffsetStdDevFactor*noiseStd[index] + m_codeOffsetStdDevOffset > .37) {
                capacity = 3;
            } else {
                capacity = 4;
            }
        }
        else if (n_k == 6) {
            if (m_codeOffsetStdDevFactor*noiseStd[index] + m_codeOffsetStdDevOffset > 1.08) {
                capacity = 0;
            } else if (m_codeOffsetStdDevFactor*noiseStd[index] + m_codeOffsetStdDevOffset > .78) {
                capacity = 1;
            } else if (m_codeOffsetStdDevFactor*noiseStd[index] + m_codeOffsetStdDevOffset > .54) {
                capacity = 2;
            } else if (m_codeOffsetStdDevFactor*noiseStd[index] + m_codeOffsetStdDevOffset > .38) {
                capacity = 3;
            } else {
                capacity = 4;
            }
        }
        else {
            throw std::logic_error("Only allowed rates are 1/3, 1/4, 1/5, and 1/6");
        }
        if (capacity > MAX_CAPACITY) {
            capacity = MAX_CAPACITY;
        }
        if (capacity > 0) {
            m_codeOffsetFeaturesCount++;
            m_codeOffsetFeaturesIndexes.push_back(index);
            m_codeOffsetTotalCapacity += capacity;
            m_codeOffsetFeaturesCapacities.push_back(capacity);
        } else {
            m_fuzzyCommitmentFeaturesIndexes.push_back(index);
        }
    }

}

void HybridStrengthenedTurboCodeFuzzyExtractor::findOptimalFuzzyCommitmentProjection(
    const real_number_vector &featuresMeans,
    const real_number_vector &featuresStdDevs,
    real_number_vector &featuresWeights,
    real_number &combinedFeatureMean,
    real_number &combinedFeatureStdDev)
{
    if (featuresMeans.size() > 1) {
        arma::Mat<real_number> arma_diagonalCovarianceMatrix = arma::diagmat(arma::square(arma::Col(featuresStdDevs)));
        arma::Col<real_number> arma_featuresMeansCol(featuresMeans);
        arma::Row<real_number> arma_featuresMeansRow(featuresMeans);
        arma::Mat<real_number> arma_featuresMeansProductMatrix = arma_featuresMeansCol*arma_featuresMeansRow;
        arma::Mat<real_number> arma_S_inv;
        try {
            arma_S_inv = arma::inv_sympd(arma_diagonalCovarianceMatrix);
        } catch (std::runtime_error &e) {
            std::cerr << "Runtime error exception thrown while inverting arma_diagonalCovarianceMatrix: " << e.what() << std::endl;
            std::cerr << "arma_diagonalCovarianceMatrix: " << std::endl;
            std::cerr << arma_diagonalCovarianceMatrix << std::endl;
            std::cerr << "arma_featuresMeansRow:" << std::endl;
            std::cerr << arma_featuresMeansRow << std::endl;
            // Throw instead of exit(): this runs inside a thread-pool task, and exit() would destroy objects that other
            // tasks are still using. The exception reaches the caller through the task's future.
            throw std::runtime_error(std::string("HybridStrengthenedTurboCodeFuzzyExtractor::findOptimalFuzzyCommitmentProjection: "
                                                 "cannot invert the diagonal covariance matrix: ") + e.what());
        }
        arma::Mat<real_number> arma_Lambda = arma_S_inv*arma_featuresMeansProductMatrix;
        arma::cx_vec arma_eigenValues;
        arma::cx_mat arma_eigenVectors;
        arma::mat arma_LambdaDoublePrecisionMatrix = arma::conv_to<arma::mat>::from(arma_Lambda);
        // This form of eig_gen reports a failure by returning false (it does not throw).
        if (!arma::eig_gen(arma_eigenValues, arma_eigenVectors, arma_LambdaDoublePrecisionMatrix, "balance")) {
            std::cerr << "Eigenvalue decomposition of Lambda failed." << std::endl;
            std::cerr << "Lambda: " << std::endl;
            std::cerr << arma_LambdaDoublePrecisionMatrix << std::endl;
            throw std::runtime_error("HybridStrengthenedTurboCodeFuzzyExtractor::findOptimalFuzzyCommitmentProjection: "
                                     "eigenvalue decomposition of Lambda failed.");
        }
        std::vector<std::pair<real_number, unsigned int>> eigenValuesMagnitudes_and_indexes;
        for (unsigned int eigenValueI = 0; eigenValueI < arma_eigenValues.n_elem; eigenValueI++) {
            eigenValuesMagnitudes_and_indexes.push_back(std::pair<real_number, unsigned int>(std::abs(arma_eigenValues.at(eigenValueI)), eigenValueI));
        }
        std::sort(eigenValuesMagnitudes_and_indexes.begin(), eigenValuesMagnitudes_and_indexes.end());
        unsigned int maxEigenValueIndex = eigenValuesMagnitudes_and_indexes.rbegin()->second;
        combinedFeatureMean = arma::sum(arma_featuresMeansCol%arma::real(arma_eigenVectors.col(maxEigenValueIndex)));
        unsigned int commitment = +1.0; // m_uniform_binary_distribution(m_twister);
        arma_eigenVectors.col(maxEigenValueIndex) /= ((2.0*commitment - 1.0)*combinedFeatureMean);
        featuresWeights.resize(arma_eigenVectors.n_rows);
        for (unsigned int rowI = 0; rowI < arma_eigenVectors.n_rows; rowI++) {
            featuresWeights[rowI] = arma_eigenVectors.at(rowI,maxEigenValueIndex).real();
        }
        combinedFeatureMean = arma::sum(arma_featuresMeansCol%arma::real(arma_eigenVectors.col(maxEigenValueIndex)));
        combinedFeatureStdDev = std::sqrt(arma::sum(arma::square(arma::Col(featuresStdDevs)%arma::abs(arma_eigenVectors.col(maxEigenValueIndex)))));
    } else {
        unsigned int commitment = +1.0; // m_uniform_binary_distribution(m_twister);
        combinedFeatureMean = 2.0*commitment - 1.0;
        featuresWeights = real_number_vector(1, combinedFeatureMean/featuresMeans[0]);
        combinedFeatureStdDev = std::abs(featuresStdDevs[0]/featuresMeans[0]);
    }
}

void HybridStrengthenedTurboCodeFuzzyExtractor::findOptimalFuzzyCommitment(real_number_vector featuresMeans,
                                                                           real_number_vector featuresStdDevs,
                                                                           uint_vector_vector &featuresIndexesVectors,
                                                                           real_number_vector_vector &featuresWeightsVectors,
                                                                           real_number_vector &combinedFeaturesReliabilities,
                                                                           real_number_vector &combinedFeaturesMeans,
                                                                           real_number_vector &combinedFeaturesStdDevs)
{
    if (featuresIndexesVectors.empty()) {
        throw std::logic_error("featureIndexesVectors is empty, but it should initially contain as many empty uint_vector as desired combined features.");
    }
    if (featuresWeightsVectors.empty()) {
        throw std::logic_error("featureWeightsVectors is empty, but it should initially contain as many empty real_number_vector as desired combined features.");
    }
    if (featuresIndexesVectors.size() != featuresWeightsVectors.size()) {
        throw std::logic_error("featureWeightsVectors and featureIndexesVectors  must have the same size.");
    }
    for (unsigned int combinedFeatureI = 0; combinedFeatureI < featuresIndexesVectors.size(); combinedFeatureI++) {
        if (!featuresIndexesVectors[combinedFeatureI].empty()) {
            throw std::logic_error(std::string(std::string("featureIndexesVector[") + std::to_string(combinedFeatureI) + std::string("] is not empty.")).c_str());
        }
    }
    uint_vector unreliableFeaturesIndexes;
    for (uint i = 0; i < featuresMeans.size(); i++) {
        unreliableFeaturesIndexes.push_back(i);
    }
    while (featuresMeans.size() > 0) {
        // Find the least reliable feature from the features pool
        std::vector<std::pair<real_number, unsigned int>> reliability_index_vector;
        for (unsigned int featureI = 0; featureI < featuresMeans.size(); featureI++) {
            reliability_index_vector.push_back(std::pair<real_number, unsigned int>(std::abs(featuresMeans[featureI])/featuresStdDevs[featureI], featureI));
        }
        std::sort(reliability_index_vector.begin(), reliability_index_vector.end());
        unsigned int leastReliableFeatureIndex = reliability_index_vector.begin()->second;
        // Add this feature to the least reliable combined feature and find least reliable combined feature
        std::vector<std::pair<real_number, unsigned int>> combinedFeatures_reliability_index_vector;
        for (unsigned int featureI = 0; featureI < combinedFeaturesReliabilities.size(); featureI++) {
            combinedFeatures_reliability_index_vector.push_back(std::pair<real_number, unsigned int>(combinedFeaturesReliabilities[featureI], featureI));
        }
        std::sort(combinedFeatures_reliability_index_vector.begin(), combinedFeatures_reliability_index_vector.end());
        unsigned int leastReliableCombinedFeatureIndex = combinedFeatures_reliability_index_vector.begin()->second;
        real_number_vector combined_and_new_means;
        real_number_vector combined_and_new_stdDevs;
        real_number_vector combined_and_new_weights;
        real_number combined_mean, combined_stdDev;
        if (featuresWeightsVectors[leastReliableCombinedFeatureIndex].size() > 0) {
            combined_and_new_means.push_back(combinedFeaturesMeans[leastReliableCombinedFeatureIndex]);
            combined_and_new_means.push_back(featuresMeans[leastReliableFeatureIndex]);
            combined_and_new_stdDevs.push_back(combinedFeaturesStdDevs[leastReliableCombinedFeatureIndex]);
            combined_and_new_stdDevs.push_back(featuresStdDevs[leastReliableFeatureIndex]);
        } else {
            combined_and_new_means.push_back(featuresMeans[leastReliableFeatureIndex]);
            combined_and_new_stdDevs.push_back(featuresStdDevs[leastReliableFeatureIndex]);
        }
        findOptimalFuzzyCommitmentProjection(combined_and_new_means, combined_and_new_stdDevs,
                                             combined_and_new_weights, combined_mean, combined_stdDev);
        // Normalize previous weights
        for (unsigned int featureI = 0; featureI < featuresWeightsVectors[leastReliableCombinedFeatureIndex].size(); featureI++) {
            featuresWeightsVectors[leastReliableCombinedFeatureIndex][featureI] *= combined_and_new_weights[0];
        }
        // Add new feature to the least reliable combined feature
        featuresIndexesVectors[leastReliableCombinedFeatureIndex].push_back(unreliableFeaturesIndexes[leastReliableFeatureIndex]);
        if (combined_and_new_weights.size() > 1) {
            featuresWeightsVectors[leastReliableCombinedFeatureIndex].push_back(combined_and_new_weights[1]);
        } else {
            featuresWeightsVectors[leastReliableCombinedFeatureIndex].push_back(combined_and_new_weights[0]);
        }
        combinedFeaturesMeans[leastReliableCombinedFeatureIndex] = combined_mean;
        combinedFeaturesStdDevs[leastReliableCombinedFeatureIndex] = combined_stdDev;
        combinedFeaturesReliabilities[leastReliableCombinedFeatureIndex] = std::abs(combined_mean/combined_stdDev);
        // Remove least reliable feature from the feature pool
        featuresMeans.erase(featuresMeans.begin() + leastReliableFeatureIndex);
        featuresStdDevs.erase(featuresStdDevs.begin() + leastReliableFeatureIndex);
        unreliableFeaturesIndexes.erase(unreliableFeaturesIndexes.begin() + leastReliableFeatureIndex);
    }
}

void HybridStrengthenedTurboCodeFuzzyExtractor::setTurboCodeInputMessageLength(unsigned int newInputLength)
{
    m_turboCodeInputMessageLength = newInputLength;
}

void HybridStrengthenedTurboCodeFuzzyExtractor::computeRandomizedInterleaversAndPuncturingPatterns()
{
    uint_vector_vector interleavers;
    if (m_turboCodeInputMessageLength == 0) {
        throw std::logic_error(std::string("HybridStrengthenedTurboCodeFuzzyExtractor::computeRandomizedInterleavers: m_inputMessageLength = 0. It must be set to a non-0 valuebefore calling this method."));
    }
    for (unsigned int generatorI = 0; generatorI < m_turboCode.generators().size(); generatorI++) {
        uint_vector generatorInterleavers(m_turboCodeInputMessageLength, 0);
        for (unsigned int index = 1; index < m_turboCodeInputMessageLength; index++) {
            generatorInterleavers[index] = generatorInterleavers[index - 1] + 1;
        }
        m_rng_mutex.lock();
        std::shuffle(generatorInterleavers.begin(), generatorInterleavers.end(), m_twister);
        m_rng_mutex.unlock();
        interleavers.push_back(generatorInterleavers);
    }
    binary_polynomial_vector puncturingPatterns =
        binary_polynomial_vector(m_turboCode.generators().size(), binary_polynomial(m_turboCodeInputMessageLength, true));

    m_turboCode.setPuncturingPatterns(puncturingPatterns);
    m_turboCode.setInterleavers(interleavers);
}

void HybridStrengthenedTurboCodeFuzzyExtractor::setCodeOffsetFeaturesIndexes(uint_vector newCodeOffsetFeatureIndexes)
{
    m_codeOffsetFeaturesIndexes = newCodeOffsetFeatureIndexes;
    for (unsigned int featureI = 0; featureI < m_codeOffsetFeaturesIndexes.size(); featureI++) {
        if (m_codeOffsetFeaturesIndexes[featureI] >= m_inputDimensionality) {
	  throw std::logic_error(std::string("HybridStrengthenedTurboCodeFuzzyExtractor::setCodeOffsetFeaturesIndexes: Reliable feature ") +
				 std::to_string(featureI) +
				 std::string(" index is ") +
				 std::to_string(m_codeOffsetFeaturesIndexes[featureI]) +
				 std::string(" >= input dimensionality = ") +
				 std::to_string(m_inputDimensionality));
        }
    }
}

void HybridStrengthenedTurboCodeFuzzyExtractor::setFuzzyCommitmentFeaturesIndexes(uint_vector newFuzzyCommitmentFeaturesIndexes)
{
    m_fuzzyCommitmentFeaturesIndexes = newFuzzyCommitmentFeaturesIndexes;
    for (unsigned int featureI = 0; featureI < m_fuzzyCommitmentFeaturesIndexes.size(); featureI++) {
        if (m_fuzzyCommitmentFeaturesIndexes[featureI] >= m_inputDimensionality) {
	  throw std::logic_error(std::string("HybridStrengthenedTurboCodeFuzzyExtractor::setFuzzyCommitmentFeaturesIndexes: Unreliable feature ") +
				 std::to_string(featureI) +
				 std::string(" index is ") + std::to_string(m_fuzzyCommitmentFeaturesIndexes[featureI]) + std::string(" >= input dimensionality = ") +
				 std::to_string(m_inputDimensionality));
        }
    }
}

void HybridStrengthenedTurboCodeFuzzyExtractor::setCombinedFeaturesIndexes(const uint_vector_vector &newCombinedFeaturesIndexes)
{
    m_combinedFeaturesIndexes = newCombinedFeaturesIndexes;
    for (unsigned int combinedFeatureI = 0; combinedFeatureI < m_combinedFeaturesIndexes.size(); combinedFeatureI++) {
        for (unsigned int featureI = 0; featureI < m_combinedFeaturesIndexes[combinedFeatureI].size(); featureI++)
	  if (m_combinedFeaturesIndexes[combinedFeatureI][featureI] >= m_fuzzyCommitmentFeaturesIndexes.size()) {
	      throw std::logic_error(std::string("HybridStrengthenedTurboCodeFuzzyExtractor::setCombinedFeaturesIndexes: Unreliable feature ") +
				     std::to_string(featureI) +
				     std::string(" at combined feature ") +
				     std::to_string(combinedFeatureI) +
				     std::string(" is ") + std::to_string(m_combinedFeaturesIndexes[combinedFeatureI][featureI]) +
				     std::string(" >= the number of input features used for fuzzy commitment = ") +
				     std::to_string(m_fuzzyCommitmentFeaturesIndexes.size()));
            }
    }
}

binary_polynomial &HybridStrengthenedTurboCodeFuzzyExtractor::generateRandomBinaryMessageAndCodeWord()
{
    if (m_turboCodeInputMessageLength == 0) {
        throw std::logic_error(std::string("HybridStrengthenedTurboCodeFuzzyExtractor::generateRandomBinaryMessage: m_inputMessageLength cannot be 0."));
    }
    m_turboCodeInputMessage = binary_polynomial(m_turboCodeInputMessageLength);
    for (uint bitI = 0; bitI < m_turboCodeInputMessage.size(); bitI++) {
        m_rng_mutex.lock();
        m_turboCodeInputMessage[bitI] = m_uniform_binary_distribution(m_twister);
        m_rng_mutex.unlock();
    }
    m_randomCodeWord = m_turboCode.encode(m_turboCodeInputMessage);
    return m_turboCodeInputMessage;
}

void HybridStrengthenedTurboCodeFuzzyExtractor::findBestFuzzyCommitment()
{
#ifdef DEBUG
    std::cout << "Fuzzy-commitment features count: " << m_fuzzyCommitmentEnrollmentSamplesMean.size() << std::endl;
#endif
    unsigned int codeRateInverse = m_turboCode.inputOutputLengths().second/m_turboCode.inputOutputLengths().first;
    m_combinedFeaturesCount = 0;
    m_fuzzyCommitmentTotalCapacity = 0;
    unsigned int fuzzyCommitmentFeaturesCount = m_fuzzyCommitmentEnrollmentSamplesMean.size();
    unsigned int minCombinedFeaturesCount = 0; // codeRateInverse - (m_codeOffsetTotalCapacity % codeRateInverse);
    unsigned int maxCombinedFeaturesCount = fuzzyCommitmentFeaturesCount; // - (m_codeOffsetTotalCapacity % codeRateInverse);
    if (fuzzyCommitmentFeaturesCount < minCombinedFeaturesCount) {
        m_fuzzyCommitmentTotalCapacity = m_combinedFeaturesCount;
        m_combinedFeaturesIndexes = uint_vector_vector(m_combinedFeaturesCount, uint_vector());
        m_combinedFeaturesWeights = real_number_vector_vector(m_combinedFeaturesCount, real_number_vector());
        m_combinedFeaturesReliabilities = real_number_vector(m_combinedFeaturesCount, 0.0);
        m_combinedFeaturesMeans = real_number_vector(m_combinedFeaturesCount, 0.0);
        m_combinedFeaturesStdDevs = real_number_vector(m_combinedFeaturesCount, 1.0);
        m_fuzzyExtractorCapacity = m_codeOffsetTotalCapacity + m_fuzzyCommitmentTotalCapacity;
#ifdef DEBUG
        std::cout << "No features available for fuzzy commitment." << std::endl;
#endif
        return;
    }
    while (minCombinedFeaturesCount < maxCombinedFeaturesCount) {
        m_combinedFeaturesCount = (minCombinedFeaturesCount + maxCombinedFeaturesCount)/2;
        // if (codeRateInverse*(((minCombinedFeaturesCount + maxCombinedFeaturesCount)/2)/codeRateInverse) > (m_codeOffsetTotalCapacity % codeRateInverse)) {
        //     m_combinedFeaturesCount = codeRateInverse*(((minCombinedFeaturesCount + maxCombinedFeaturesCount)/2)/codeRateInverse) - (m_codeOffsetTotalCapacity % codeRateInverse);
        // } else {
        //     if (m_combinedFeaturesCount > codeRateInverse) {
        //         m_combinedFeaturesCount -= codeRateInverse;
        //     } else {
        //         m_combinedFeaturesCount = 0;
        //         m_fuzzyCommitmentTotalCapacity = m_combinedFeaturesCount;
        //         m_combinedFeaturesIndexes = uint_vector_vector(m_combinedFeaturesCount, uint_vector());
        //         m_combinedFeaturesWeights = real_number_vector_vector(m_combinedFeaturesCount, real_number_vector());
        //         m_combinedFeaturesReliabilities = real_number_vector(m_combinedFeaturesCount, 0.0);
        //         m_combinedFeaturesMeans = real_number_vector(m_combinedFeaturesCount, 0.0);
        //         m_combinedFeaturesStdDevs = real_number_vector(m_combinedFeaturesCount, 1.0);
        //         break;
        //     }
        //     maxCombinedFeaturesCount = m_combinedFeaturesCount;
        // }
#ifdef DEBUG
        std::cout << "Trying " << m_combinedFeaturesCount << " combined features" << std::endl;
        std::cout << "Min combined features count: " << minCombinedFeaturesCount << std::endl;
        std::cout << "Max combined features count: " << maxCombinedFeaturesCount << std::endl;
#endif
        m_fuzzyCommitmentTotalCapacity = m_combinedFeaturesCount;
        m_combinedFeaturesIndexes = uint_vector_vector(m_combinedFeaturesCount, uint_vector());
        m_combinedFeaturesWeights = real_number_vector_vector(m_combinedFeaturesCount, real_number_vector());
        m_combinedFeaturesReliabilities = real_number_vector(m_combinedFeaturesCount, 0.0);
        m_combinedFeaturesMeans = real_number_vector(m_combinedFeaturesCount, 0.0);
        m_combinedFeaturesStdDevs = real_number_vector(m_combinedFeaturesCount, 1.0);
        if (m_combinedFeaturesCount == 0) {
            break;
        } else {
            findOptimalFuzzyCommitment(m_fuzzyCommitmentEnrollmentSamplesMean,
                                       m_fuzzyCommitmentEnrollmentSamplesStdDev,
                                       m_combinedFeaturesIndexes,
                                       m_combinedFeaturesWeights,
                                       m_combinedFeaturesReliabilities,
                                       m_combinedFeaturesMeans,
                                       m_combinedFeaturesStdDevs);
            arma::Row<real_number> arma_reliabilities(m_combinedFeaturesReliabilities);
            real_number minReliability = arma::min(arma_reliabilities);
#ifdef DEBUG
            std::cout << "Min reliability: " << minReliability << " (required " << m_fuzzyCommitmentMinReliability << ")" << std::endl;
#endif
            if (minReliability >= m_fuzzyCommitmentMinReliability) {
#ifdef DEBUG
                std::cout << "Min reliability: " << minReliability << " > m_fuzzyCommitmentMinReliability " << m_fuzzyCommitmentMinReliability << std::endl;
#endif
                if (minCombinedFeaturesCount == m_combinedFeaturesCount) {
                    m_fuzzyExtractorCapacity = m_fuzzyCommitmentTotalCapacity + m_codeOffsetTotalCapacity;
                    break;
                } else {
#ifdef DEBUG
                    std::cout << "Setting minCombinedFeaturesCount to " << m_fuzzyCommitmentTotalCapacity << std::endl;
#endif
                    minCombinedFeaturesCount = m_fuzzyCommitmentTotalCapacity;
                }
            } else {
#ifdef DEBUG
                std::cout << "Setting maxCombinedFeaturesCount to " << m_fuzzyCommitmentTotalCapacity << std::endl;
#endif
                maxCombinedFeaturesCount = m_fuzzyCommitmentTotalCapacity;
            }
        }
    }
    m_fuzzyExtractorCapacity = m_codeOffsetTotalCapacity + m_fuzzyCommitmentTotalCapacity;
}

void HybridStrengthenedTurboCodeFuzzyExtractor::randomlyPermute()
{
    uint symbolsCount(m_codeOffsetFeaturesCount + m_combinedFeaturesCount);
    m_symbolsPermutation.resize(symbolsCount);
    for (uint symbI = 0; symbI < symbolsCount; symbI++ ) {
        m_symbolsPermutation[symbI] = symbI;
    }
#ifdef HSTCFE_DEBUG
    // std::cout << "\n\n... Lock ...\n\n";
#endif
    m_rng_mutex.lock();
    std::shuffle(m_symbolsPermutation.begin(), m_symbolsPermutation.end(), m_twister);
    m_rng_mutex.unlock();
#ifdef HSTCFE_DEBUG
    // std::cout << "\n\n... Unlock ...\n\n";
#endif
    m_symbolsBitOffset = uint_vector(symbolsCount, 0);
    uint bitOffset = 0;
    for (uint symbI = 0; symbI < symbolsCount; symbI++) {
        uint originalSymbI(0);
        for (uint symbII = 0; symbII < symbolsCount; symbII++) {
            if (m_symbolsPermutation[symbII] == symbI) {
                originalSymbI = symbII;
                break;
            }
        }
        m_symbolsBitOffset[originalSymbI] = bitOffset;
        if (originalSymbI < m_codeOffsetFeaturesCount) {
            bitOffset += m_codeOffsetFeaturesCapacities[originalSymbI];
        } else {
            bitOffset++;
        }
    }
}

void HybridStrengthenedTurboCodeFuzzyExtractor::enrollCodeOffset()
{
    m_symbolIndexes = uint_vector(m_fuzzyExtractorCapacity);
    m_pdfs0 = GMM_vector(m_fuzzyExtractorCapacity);
    m_pdfs1 = GMM_vector(m_fuzzyExtractorCapacity);
    m_commitment = real_number_vector(m_codeOffsetFeaturesCount + m_fuzzyCommitmentTotalCapacity, 0);
    for (unsigned int symbolIndex = 0; symbolIndex < m_codeOffsetFeaturesIndexes.size(); symbolIndex++) {
        PSK pskConstellation;
        GMM noiseDistribution;
        double mean(0.0);
        double variance = m_codeOffsetEnrollmentSamplesStdDev[symbolIndex]*m_codeOffsetEnrollmentSamplesStdDev[symbolIndex];
        noiseDistribution.createFromOneGaussian(1, &mean, &variance);
        noiseDistribution.calculateFactors();
        pskConstellation.setupConstellation(m_codeOffsetFeaturesCapacities[symbolIndex], noiseDistribution);
        // Commitment operation
        real_number_vector committedSymbol;
        binary_polynomial symbolInput(m_randomCodeWord.begin() + m_symbolsBitOffset[symbolIndex], m_randomCodeWord.begin() + m_symbolsBitOffset[symbolIndex] + m_codeOffsetFeaturesCapacities[symbolIndex]);
        pskConstellation.encode(symbolInput, committedSymbol, false); // set to true if we want noise.
#ifdef DEBUG
        std::cout << "committed constellation symbol for code-offset index "
                  << symbolIndex << ": " << committedSymbol[0] << std::endl;
        std::cout << "biometric value committed for code-offset index "
                  << symbolIndex << ": " << m_codeOffsetEnrollmentSamplesMean[symbolIndex] << std::endl;
#endif
        committedSymbol[0] += m_codeOffsetEnrollmentSamplesMean[symbolIndex];
        while (committedSymbol[0] > 2*M_PI) {
            committedSymbol[0] -= 2*M_PI;
        }
        while (committedSymbol[0] < 0) {
            committedSymbol[0] += 2*M_PI;
        }
#ifdef DEBUG
        std::cout << "Code-offset commitment at index " << symbolIndex << ": " << committedSymbol[0] << std::endl;
#endif
        m_commitment[symbolIndex] = committedSymbol[0];
        // Setting up the distributions and symbol indexes
        for (unsigned int bitI = 0; bitI < m_codeOffsetFeaturesCapacities[symbolIndex]; bitI++) {
            m_symbolIndexes[m_symbolsBitOffset[symbolIndex] + bitI] = symbolIndex;
            m_pdfs0[m_symbolsBitOffset[symbolIndex] + bitI] = pskConstellation.pdfs0()[bitI];
            m_pdfs1[m_symbolsBitOffset[symbolIndex] + bitI] = pskConstellation.pdfs1()[bitI];
        }
    }
}

void HybridStrengthenedTurboCodeFuzzyExtractor::enrollFuzzyCommitment()
{
    if (m_symbolIndexes.size() != m_fuzzyExtractorCapacity) {
        throw std::logic_error(std::string("HybridStrengthenedTurboCodeFuzzyExtractor::enrollFuzzyCommitment: size of m_symbolIndexes is ")
                          + std::to_string(m_symbolIndexes.size()) + std::string(", but it must be equal to Fuzzy Extractor capacity (")
                          + std::to_string(m_fuzzyExtractorCapacity) + std::string(")."));
    }
    for (unsigned int fcFeatI = 0; fcFeatI < m_fuzzyCommitmentTotalCapacity; fcFeatI++) {
        PAM2 pam2Constellation;
        GMM noiseDistribution;
        double mean(0.0);
        double variance = m_combinedFeaturesStdDevs[fcFeatI]*m_combinedFeaturesStdDevs[fcFeatI];
        noiseDistribution.createFromOneGaussian(1, &mean, &variance);
        noiseDistribution.calculateFactors();
        pam2Constellation.setupConstellation(1, noiseDistribution);
        bool committedBit = m_randomCodeWord[m_symbolsBitOffset[m_codeOffsetFeaturesCount + fcFeatI]];
        if (!committedBit) {
#ifdef DEBUG
            std::cout << "Fuzzy-commitment for feature " << fcFeatI << " is: 0" << std::endl;
#endif
            for (unsigned int featI = 0; featI < m_combinedFeaturesWeights[fcFeatI].size(); featI++) {
                m_combinedFeaturesWeights[fcFeatI][featI] = -m_combinedFeaturesWeights[fcFeatI][featI];
            }
        } else {
#ifdef DEBUG
            std::cout << "Fuzzy-commitment for feature " << fcFeatI << " is: 1" << std::endl;
#endif
        }
        m_symbolIndexes[m_symbolsBitOffset[m_codeOffsetFeaturesCount + fcFeatI]] = m_codeOffsetFeaturesCount + fcFeatI;
        m_pdfs0[m_symbolsBitOffset[m_codeOffsetFeaturesCount + fcFeatI]] = pam2Constellation.pdfs0()[0];
        m_pdfs1[m_symbolsBitOffset[m_codeOffsetFeaturesCount + fcFeatI]] = pam2Constellation.pdfs1()[0];
    }
#ifdef DEBUG
    std::cout << "Total number of symbols: " << m_symbolIndexes.size() << std::endl;
    std::cout << "Symbol indexes:" << std::endl;
    printVector(m_symbolIndexes);
    std::cout << std::endl;
#endif
}

void HybridStrengthenedTurboCodeFuzzyExtractor::projectFuzzyCommitment(const real_number_vector &sample, real_number_vector &projection)
{
    if (sample.size() != m_inputDimensionality) {
        throw std::logic_error(std::string("HybridStrengthenedTurboCodeFuzzyExtractor::projectFuzzyCommitment: size of sample is ") +
			  std::to_string(sample.size()) +
			  std::string(", but it must be equal to m_inputDimensionaltiy (") +
			  std::to_string(m_inputDimensionality) +
			  std::string(")."));
    }
    real_number_vector fuzzyCommitmentFeatures(m_fuzzyCommitmentFeaturesIndexes.size());
    for (uint fcFeatI = 0; fcFeatI < m_fuzzyCommitmentFeaturesIndexes.size(); fcFeatI++) {
        fuzzyCommitmentFeatures[fcFeatI] = sample[m_fuzzyCommitmentFeaturesIndexes[fcFeatI]];
    }
    projection = real_number_vector(m_combinedFeaturesCount, real_number(0.0));
    for (unsigned int combinedFeatureI = 0; combinedFeatureI < m_combinedFeaturesCount; combinedFeatureI++) {
        for (unsigned int featureI = 0; featureI < m_combinedFeaturesIndexes[combinedFeatureI].size(); featureI++) {
            projection[combinedFeatureI] += fuzzyCommitmentFeatures[m_combinedFeaturesIndexes[combinedFeatureI][featureI]] *
                                            m_combinedFeaturesWeights[combinedFeatureI][featureI];
        }
    }
}

void HybridStrengthenedTurboCodeFuzzyExtractor::computeCodeOffsetEnrollmentSamples(const real_number_vector_vector &normalizedEnrollmentSamples) {
    m_codeOffsetEnrollmentSamples = real_number_vector_vector(normalizedEnrollmentSamples.size(), real_number_vector(m_codeOffsetFeaturesIndexes.size()));
    for (unsigned int sampleI = 0; sampleI < normalizedEnrollmentSamples.size(); sampleI++) {
        for (unsigned int codeOffsetDimI = 0; codeOffsetDimI < m_codeOffsetFeaturesIndexes.size(); codeOffsetDimI++) {
            m_codeOffsetEnrollmentSamples[sampleI][codeOffsetDimI] = normalizedEnrollmentSamples[sampleI][m_codeOffsetFeaturesIndexes[codeOffsetDimI]];
        }
    }
}

void HybridStrengthenedTurboCodeFuzzyExtractor::computeFuzzyCommitmentEnrollmentSamples(const real_number_vector_vector &enrollmentSamples) {
    m_fuzzyCommitmentEnrollmentSamples = real_number_vector_vector(enrollmentSamples.size(), real_number_vector(m_fuzzyCommitmentFeaturesIndexes.size()));
    for (unsigned int sampleI = 0; sampleI < enrollmentSamples.size(); sampleI++) {
        for (unsigned int dimI = 0; dimI < m_fuzzyCommitmentFeaturesIndexes.size(); dimI++) {
            m_fuzzyCommitmentEnrollmentSamples[sampleI][dimI] = enrollmentSamples[sampleI][m_fuzzyCommitmentFeaturesIndexes[dimI]];
        }
    }
}

void HybridStrengthenedTurboCodeFuzzyExtractor::generate(const real_number_vector_vector &enrollmentSamples)
{
#ifdef DEBUG
    std::cout << "Starting generate process for id " << m_userId << std::endl;
    std::cout.flush();
#endif
    // Assertions
    // The first step is to normalize the samples as an uniform
    real_number_vector_vector normalizedEnrollmentSamples;
    uniformNormalizationBatch(enrollmentSamples, normalizedEnrollmentSamples);
    real_number_vector normalizedEnrollmentSamplesMeansVector;
    real_number_vector normalizedEnrollmentSamplesStdDevsVector;
    real_number_vector_vector covarianceMatrix;
#ifdef DEBUG
    std::cout << "Computing normalized enrollment samples statistics... ";
    std::cout.flush();
#endif
    computeSampleStatistics(normalizedEnrollmentSamples, normalizedEnrollmentSamplesMeansVector, normalizedEnrollmentSamplesStdDevsVector, covarianceMatrix);
#ifdef DEBUG
    std::cout << "Done." << std::endl;
    std::cout << "Computing normalized enrollment samples bit-loading... ";
    std::cout.flush();
#endif
    bitLoading(normalizedEnrollmentSamplesStdDevsVector);
#ifdef DEBUG
    std::cout << "Done." << std::endl;
    std::cout << "Building code-offset enrollment samples... ";
    std::cout.flush();
#endif
    computeCodeOffsetEnrollmentSamples(normalizedEnrollmentSamples);
#ifdef DEBUG
    std::cout << "Done." << std::endl;
    std::cout << "Building fuzzy commitment enrollment samples... ";
    std::cout.flush();
#endif
    computeFuzzyCommitmentEnrollmentSamples(enrollmentSamples);
#ifdef DEBUG
    std::cout << "Done." << std::endl;
    std::cout << "Number of features available for fuzzy commitment: " << m_fuzzyCommitmentFeaturesIndexes.size() << std::endl;
    std::cout << "Computing fuzzy commitment enrollment samples statistics... ";
    std::cout.flush();
#endif
    computeSampleStatistics(m_fuzzyCommitmentEnrollmentSamples, m_fuzzyCommitmentEnrollmentSamplesMean, m_fuzzyCommitmentEnrollmentSamplesStdDev, covarianceMatrix);
#ifdef DEBUG
    std::cout << "Done." << std::endl;
    std::cout << "Computing code-offset enrollment samples statistics... ";
    std::cout.flush();
#endif
    computeSampleStatistics(m_codeOffsetEnrollmentSamples, m_codeOffsetEnrollmentSamplesMean, m_codeOffsetEnrollmentSamplesStdDev, covarianceMatrix);
#ifdef DEBUG
    std::cout << "Done." << std::endl;
    std::cout << "Id: " << m_userId << std::endl;
    std::cout << "Finding the best fuzzy commitment scheme... " << std::endl;
    std::cout.flush();
#endif
    findBestFuzzyCommitment();
    /*
    unsigned int minFeatureI = 0;
    while ((m_codeOffsetTotalCapacity + m_fuzzyCommitmentTotalCapacity) % (m_turboCode.inputOutputLengths().second/m_turboCode.inputOutputLengths().first)) {
        if (m_codeOffsetTotalCapacity > 0) {
            for (unsigned int featureI = minFeatureI; featureI < m_codeOffsetFeaturesCapacities.size(); featureI++) {
                if (m_codeOffsetFeaturesCapacities[featureI] >= 2) {
                    m_codeOffsetFeaturesCapacities[featureI]--;
                    m_codeOffsetTotalCapacity--;
                    m_fuzzyExtractorCapacity--;
                    minFeatureI = featureI + 1;
                    break;
                }
            }
        } else {
            throw std::logic_error(std::string("HybridStrengthenedTurboCodeFuzzyExtractor::generate(): total capacity is ") +
                                   std::to_string(m_codeOffsetTotalCapacity + m_fuzzyCommitmentTotalCapacity) +
                                   std::string(", is not a multiple of ") +
                                   std::to_string(m_turboCode.inputOutputLengths().second/m_turboCode.inputOutputLengths().first) +
                                   std::string(", and code offset capacity is 0. Something went wrong."));
        }
    }
    */
#ifdef DEBUG
    std::cout << "Done." << std::endl;
    std::cout << "Number of combined features: " << m_combinedFeaturesCount << std::endl;
    std::cout << "Code-offset capacity: " << m_codeOffsetTotalCapacity << std::endl;
    std::cout << "Fuzzy commitment capacity: " << m_fuzzyCommitmentTotalCapacity << std::endl;
    std::cout << "Total capacity: " << m_fuzzyExtractorCapacity << std::endl;
    std::cout << "Setting input message length... ";
    std::cout.flush();
#endif
    if ((m_fuzzyExtractorCapacity*m_turboCode.inputOutputLengths().first)/m_turboCode.inputOutputLengths().second > 0) {
        unsigned int tcInputMessageLength = (m_fuzzyExtractorCapacity*m_turboCode.inputOutputLengths().first)/m_turboCode.inputOutputLengths().second;
        if (((m_fuzzyExtractorCapacity*m_turboCode.inputOutputLengths().first) % m_turboCode.inputOutputLengths().second) > 0) {
            tcInputMessageLength++;
        }
#ifdef DEBUG
            std::cout << "Turbo code message length: "
                      << tcInputMessageLength << std::endl;
#endif
        setTurboCodeInputMessageLength(tcInputMessageLength);
#ifdef DEBUG
        std::cout << "Done." << std::endl;
        std::cout << "Computting random interleavers and full puncturing patterns... " << std::endl;
        std::cout.flush();
#endif
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
#ifdef DEBUG
        std::cout << "Done." << std::endl;
        std::cout << "Generating randomized input message and codeword... ";
        std::cout.flush();
#endif
        generateRandomBinaryMessageAndCodeWord();
#ifdef DEBUG
        std::cout << "Done." << std::endl;
        // The random permutation of symbols has to be performed now, instead of after committing the code-offset and fuzzy commitment features
        std::cout << "Randomly permuting code-offset and fuzzy commitment... ";
        std::cout.flush();
#endif
        randomlyPermute();
#ifdef DEBUG
        std::cout << "Done." << std::endl;
        std::cout << "Enrolling code-offset... ";
        std::cout.flush();
#endif
        enrollCodeOffset();
#ifdef DEBUG
        std::cout << "Done." << std::endl;
        std::cout << "Enrolling fuzzy commitment... ";
        std::cout.flush();
#endif
        enrollFuzzyCommitment();
#ifdef DEBUG
        std::cout << "Done." << std::endl;
        std::cout << "Number of combined features: " << m_combinedFeaturesCount << std::endl;
        std::cout << "Code-offset capacity: " << m_codeOffsetTotalCapacity << std::endl;
        std::cout << "Fuzzy commitment capacity: " << m_fuzzyCommitmentTotalCapacity << std::endl;
        std::cout << "Total capacity: " << m_fuzzyExtractorCapacity << std::endl;
        std::cout << "Setting TC symbol indexes and PDFs... ";
        std::cout.flush();
#endif
        m_turboCode.setSymbolIndexes(m_symbolIndexes);
        m_turboCode.setPdfs0(m_pdfs0);
        m_turboCode.setPdfs1(m_pdfs1);
#ifdef DEBUG
        std::cout << "Done." << std::endl;
#endif
        m_validForReproduce = true;
    } else {
        m_validForReproduce = false;
    }
}

binary_polynomial_vector HybridStrengthenedTurboCodeFuzzyExtractor::reproduceBatch(const real_number_vector_vector &probeSamples)
{
    unsigned int probeSamplesCount = probeSamples.size();
    binary_polynomial_vector reproducedSecrets(probeSamplesCount);
#ifdef DEBUG
    std::cout << "HybridStrengthenedTurboCodeFuzzyExtractor::reproduceBatch: Sending decoding tasks for " << probeSamplesCount << " samples... " << std::endl;
#endif
    for (uint sampleI = 0; sampleI < probeSamplesCount; sampleI++) {
        reproducedSecrets[sampleI] = reproduce(probeSamples[sampleI]);
    }
#ifdef DEBUG
    std::cout << "Done!" << std::endl;
#endif
    return reproducedSecrets;
}

binary_polynomial HybridStrengthenedTurboCodeFuzzyExtractor::reproduce(const real_number_vector &probeSample)
{
    #ifdef DEBUG
        std::cout << "HybridStrengthenedTurboCodeFuzzyExtractor::reproduce: Starting." << std::endl;
    #endif
    if (m_symbolsPermutation.size() != m_codeOffsetFeaturesCount + m_fuzzyCommitmentTotalCapacity) {
        throw std::logic_error(std::string("HybridStrengthenedTurboCodeFuzzyExtractor::reproduce: Length of symbols permutation (") +
                               std::to_string(m_symbolsPermutation.size()) +
                               std::string(") inconsistent with m_codeOffsetFeaturesCount + m_fuzzyCommitmentTotalCapacity = ") +
                               std::to_string(m_codeOffsetFeaturesCount) + std::string(" + ") + std::to_string(m_fuzzyCommitmentTotalCapacity) +
                               std::string(" = ") + std::to_string(m_codeOffsetFeaturesCount + m_fuzzyCommitmentTotalCapacity));
    }
    binary_polynomial decodedMessage(m_turboCodeInputMessageLength);

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

void HybridStrengthenedTurboCodeFuzzyExtractor::loadUserSpecificData(const std::string &fileName)
{
    std::ifstream file(fileName);
    if (!file) {
        throw std::ifstream::failure(std::string("Unable to open file ") + fileName + std::string(" for reading."));
    }
    readUserSpecificData(file);
    file.close();
}

void HybridStrengthenedTurboCodeFuzzyExtractor::loadUserSpecificDataFromBlob(const std::vector<uint8_t> &blob)
{
    std::istringstream ss(std::string(blob.begin(), blob.end()));
    readUserSpecificData(ss);
}

void HybridStrengthenedTurboCodeFuzzyExtractor::readUserSpecificData(std::istream &file)
{
    std::string label;
    file >> label;
    if (label != std::string("UserId")) {
        throw std::logic_error(std::string("Error processing user specific data file ") +
                               std::string(": expecting label \"UserId\", found label \"") + label + std::string("\""));
    }
    file >> m_userId;
    file >> label;
    if (label != std::string("InputDimensionality")) {
        throw std::logic_error(std::string("Error processing user specific data file ") + 
                               std::string(": expecting label \"InputDimensionality\", found label \"") + label + std::string("\""));
    }
    file >> m_inputDimensionality;
    file >> label;
    if (label != std::string("CodeOffsetStdDevOffset")) {
        throw std::logic_error(std::string("Error processing user specific data file ") +
                               std::string(": expecting label \"CodeOffsetStdDevOffset\", found label \"") + label + std::string("\""));
    }
    file >> m_codeOffsetStdDevOffset;
    file >> label;
    if (label != std::string("CodeOffsetStdDevFactor")) {
        throw std::logic_error(std::string("Error processing user specific data file ") +
                               std::string(": expecting label \"CodeOffsetStdDevFactor\", found label \"") + label + std::string("\""));
    }
    file >> m_codeOffsetStdDevFactor;
    file >> label;
    if (label != std::string("FuzzyCommitmentMinReliability")) {
        throw std::logic_error(std::string("Error processing user specific data file ") + 
                               std::string(": expecting label \"FuzzyCommitmentMinReliability\", found label \"") + label + std::string("\""));
    }
    file >> m_fuzzyCommitmentMinReliability;
    file >> label;
    if (label != std::string("CodeOffsetFeaturesCount")) {
        throw std::logic_error(std::string("Error processing user specific data file ") +
                               std::string(": expecting label \"CodeOffsetFeaturesCount\", found label \"") + label + std::string("\""));
    }
    file >> m_codeOffsetFeaturesCount;
    file >> label;
    if (label != std::string("CodeOffsetFeaturesIndexes")) {
        throw std::logic_error(std::string("Error processing user specific data file ") +
                               std::string(": expecting label \"CodeOffsetFeaturesIndexes\", found label \"") + label + std::string("\""));
    }
    unsigned int vectorSize;
    file >> vectorSize;
    m_codeOffsetFeaturesIndexes.resize(vectorSize);
    for (unsigned int featureI = 0; featureI < vectorSize; featureI++) {
        file >> m_codeOffsetFeaturesIndexes[featureI];
    }
    file >> label;
    if (label != std::string("CodeOffsetTotalCapacity")) {
        throw std::logic_error(std::string("Error processing user specific data file ") +
                               std::string(": expecting label \"CodeOffsetTotalCapacity\", found label \"") + label + std::string("\""));
    }
    file >> m_codeOffsetTotalCapacity;
    file >> label;
    if (label != std::string("CodeOffsetFeaturesCapacities")) {
        throw std::logic_error(std::string("Error processing user specific data file ") +
                               std::string(": expecting label \"CodeOffsetFeaturesCapacities\", found label \"") + label + std::string("\""));
    }
    file >> vectorSize;
    m_codeOffsetFeaturesCapacities.resize(vectorSize);
    for (unsigned int featureI = 0; featureI < vectorSize; featureI++) {
        file >> m_codeOffsetFeaturesCapacities[featureI];
    }

    file >> label;
    if (label != std::string("CodeOffsetEnrollmentSamplesStdDev")) {
        throw std::logic_error(std::string("Error processing user specific data file ") +
                               std::string(": expecting label \"CodeOffsetEnrollmentSamplesStdDev\", found label \"") + label + std::string("\""));
    }
    file >> vectorSize;
    m_codeOffsetEnrollmentSamplesStdDev.resize(vectorSize);
    for (unsigned int featureI = 0; featureI < vectorSize; featureI++) {
        file >> m_codeOffsetEnrollmentSamplesStdDev[featureI];
    }

    file >> label;
    if (label != std::string("Commitment")) {
        throw std::logic_error(std::string("Error processing user specific data file ") +
                               std::string(": expecting label \"Commitment\", found label \"") + label + std::string("\""));
    }
    file >> vectorSize;
    m_commitment.resize(vectorSize);
    for (unsigned int featureI = 0; featureI < vectorSize; featureI++) {
        file >> m_commitment[featureI];
    }
    file >> label;
    if (label != std::string("FuzzyCommitmentTotalCapacity")) {
        throw std::logic_error(std::string("Error processing user specific data file ") +
                               std::string(": expecting label \"FuzzyCommitmentTotalCapacity\", found label \"") + label + std::string("\""));
    }
    file >> m_fuzzyCommitmentTotalCapacity;
    file >> label;
    if (label != std::string("FuzzyCommitmentFeaturesIndexes")) {
        throw std::logic_error(std::string("Error processing user specific data file ") +
                               std::string(": expecting label \"FuzzyCommitmentFeaturesIndexes\", found label \"") + label + std::string("\""));
    }
    file >> vectorSize;
    m_fuzzyCommitmentFeaturesIndexes.resize(vectorSize);
    for (unsigned int featureI = 0; featureI < vectorSize; featureI++) {
        file >> m_fuzzyCommitmentFeaturesIndexes[featureI];
    }
    file >> label;
    if (label != std::string("CombinedFeaturesCount")) {
        throw std::logic_error(std::string("Error processing user specific data file ") +
                               std::string(": expecting label \"CombinedFeaturesCount\", found label \"") + label + std::string("\""));
    }
    file >> m_combinedFeaturesCount;
    file >> label;
    if (label != std::string("CombinedFeaturesIndexes")) {
        throw std::logic_error(std::string("Error processing user specific data file ") +
                               std::string(": expecting label \"CombinedFeaturesIndexes\", found label \"") + label + std::string("\""));
    }
    file >> vectorSize;
    m_combinedFeaturesIndexes.resize(vectorSize);
    for (unsigned int featureI = 0; featureI < vectorSize; featureI++) {
        unsigned int vectorSize2;
        file >> vectorSize2;
        m_combinedFeaturesIndexes[featureI].resize(vectorSize2);
        for (unsigned int featureII = 0; featureII < vectorSize2; featureII++) {
            file >> m_combinedFeaturesIndexes[featureI][featureII];
        }
    }
    file >> label;
    if (label != std::string("CombinedFeaturesWeights")) {
        throw std::logic_error(std::string("Error processing user specific data file ") +
                               std::string(": expecting label \"CombinedFeaturesWeights\", found label \"") + label + std::string("\""));
    }
    file >> vectorSize;
    m_combinedFeaturesWeights.resize(vectorSize);
    for (unsigned int featureI = 0; featureI < vectorSize; featureI++) {
        unsigned int vectorSize2;
        file >> vectorSize2;
        m_combinedFeaturesWeights[featureI].resize(vectorSize2);
        for (unsigned int featureII = 0; featureII < vectorSize2; featureII++) {
            file >> m_combinedFeaturesWeights[featureI][featureII];
        }
    }
    file >> label;
    if (label != std::string("CombinedFeaturesStdDevs")) {
        throw std::logic_error(std::string("Error processing user specific data file ") +
                               std::string(": expecting label \"CombinedFeaturesStdDevs\", found label \"") + label + std::string("\""));
    }
    file >> vectorSize;
    if (vectorSize != m_fuzzyCommitmentTotalCapacity) {
        throw std::logic_error(std::string("HybridStrengthenedTurboCodeFuzzyExtractor::loadUserSpecificData: inconsistent fuzzy commitmen total capacity (") +
                               std::to_string(m_fuzzyCommitmentTotalCapacity) + std::string(") and size of CombinedFeaturesStdDevs (") +
                               std::to_string(vectorSize) + std::string(")."));
    }
    m_combinedFeaturesStdDevs.resize(vectorSize);
    for (unsigned int featureI = 0; featureI < vectorSize; featureI++) {
        file >> m_combinedFeaturesStdDevs[featureI];
    }
    file >> label;
    if (label != std::string("FuzzyExtractorCapacity")) {
        throw std::logic_error(std::string("Error processing user specific data file ") +
                               std::string(": expecting label \"FuzzyExtractorCapacity\", found label \"") + label + std::string("\""));
    }
    file >> m_fuzzyExtractorCapacity;
    file >> label;
    if (label != std::string("ValidForReproduce")) {
        throw std::logic_error(std::string("Error processing user specific data file ") +
                               std::string(": expecting label \"ValidForReproduce\", found label \"") + label + std::string("\""));
    }
    file >> label;
    if (label == std::string("true")) {
        m_validForReproduce = true;
    } else if (label == std::string("false")) {
        m_validForReproduce = false;
    } else {
        throw std::logic_error(std::string("Error processing user specific data file ") +
                               std::string(": expecting value of \"ValidForReproduce\" to be either \"true\" or \"false\", found label \"") +
                               label + std::string("\""));
    }
    if (m_validForReproduce) {
        file >> label;
        if (label != std::string("TurboCodePuncturingPatterns")) {
            throw std::logic_error(std::string("Error processing user specific data file ") +
                                   std::string(": expecting label \"TurboCodePuncturingPatterns\", found label \"") + label + std::string("\""));
        }
        binary_polynomial_vector turboCodePuncturingPatterns;
        file >> vectorSize;
        turboCodePuncturingPatterns.resize(vectorSize);
        for (unsigned int genI = 0; genI < vectorSize; genI++) {
            unsigned int vectorSize2;
            file >> vectorSize2;
            turboCodePuncturingPatterns[genI].resize(vectorSize2);
            for (unsigned int punctI = 0; punctI < vectorSize2; punctI++) {
                unsigned int value;
                bool bit;
                file >> value;
                switch (value) {
                case 0:
                    bit = false;
                    break;
                case 1:
                    bit = true;
                    break;
                default:
                    throw std::logic_error(std::string("Non valid binary value read: ") + std::to_string(value));
                }
                turboCodePuncturingPatterns[genI][punctI] = bit;
            }
        }
        m_turboCode.setPuncturingPatterns(turboCodePuncturingPatterns);
        file >> label;
        if (label != std::string("SymbolsPermutation")) {
            throw std::logic_error(std::string("Error processing user specific data file ") +
                                   std::string(": expecting label \"SymbolsPermutation\", found label \"") + label + std::string("\""));
        }
        file >> vectorSize;
        m_symbolsPermutation.resize(vectorSize);
        for (unsigned int featureI = 0; featureI < vectorSize; featureI++) {
            file >> m_symbolsPermutation[featureI];
        }
        m_symbolsBitOffset = uint_vector(m_codeOffsetFeaturesCount + m_fuzzyCommitmentTotalCapacity, 0);
        uint bitOffset = 0;
        for (uint featureI = 0; featureI < m_codeOffsetFeaturesCount + m_fuzzyCommitmentTotalCapacity; featureI++) {
            uint originalFeatureI(0);
            for (uint featureII = 0; featureII < m_codeOffsetFeaturesCount + m_fuzzyCommitmentTotalCapacity; featureII++) {
                if (m_symbolsPermutation[featureII] == featureI) {
                    originalFeatureI = featureII;
                    break;
                }
            }
            m_symbolsBitOffset[originalFeatureI] = bitOffset;
            if (originalFeatureI < m_codeOffsetFeaturesCount) {
                bitOffset += m_codeOffsetFeaturesCapacities[originalFeatureI];
            } else {
                bitOffset++;
            }
        }
        file >> label;
        if (label != std::string("SymbolIndexes")) {
            throw std::logic_error(std::string("Error processing user specific data file ") +
                                   std::string(": expecting label \"SymbolIndexes\", found label \"") + label + std::string("\""));
        }
        file >> vectorSize;
        m_symbolIndexes.resize(vectorSize);
        for (unsigned int featureI = 0; featureI < vectorSize; featureI++) {
            file >> m_symbolIndexes[featureI];
        }
        file >> label;
        if (label != std::string("TurboCodeInterleavers")) {
            throw std::logic_error(std::string("Error processing user specific data file ") +
                                   std::string(": expecting label \"TurboCodeInterleavers\", found label \"") + label + std::string("\""));
        }
        uint_vector_vector turboCodeInterleavers;
        file >> vectorSize;
        turboCodeInterleavers.resize(vectorSize);
        for (unsigned int featureI = 0; featureI < vectorSize; featureI++) {
            unsigned int vectorSize2;
            file >> vectorSize2;
            turboCodeInterleavers[featureI].resize(vectorSize2);
            for (unsigned int featureII = 0; featureII < vectorSize2; featureII++) {
                file >> turboCodeInterleavers[featureI][featureII];
            }
        }
        m_turboCode.setInterleavers(turboCodeInterleavers);
        file >> label;
        if (label != std::string("TurboCodeInputMessageLength")) {
            throw std::logic_error(std::string("Error processing user specific data file ") +
                                   std::string(": expecting label \"TurboCodeInputMessageLength\", found label \"") + label + std::string("\""));
        }
        file >> m_turboCodeInputMessageLength;
        file >> label;
        if (label != std::string("TurboCodeInputMessage")) {
            throw std::logic_error(std::string("Error processing user specific data file ") +
                                   std::string(": expecting label \"TurboCodeInputMessage\", found label \"") + label + std::string("\""));
        }
        file >> vectorSize;
        m_turboCodeInputMessage.resize(vectorSize);
        for (unsigned int bitI = 0; bitI < vectorSize; bitI++) {
            unsigned int value;
            bool bit;
            file >> value;
            switch (value) {
            case 0:
                bit = false;
                break;
            case 1:
                bit = true;
                break;
            default:
                throw std::logic_error(std::string("Non valid binary value read: ") + std::to_string(value));
            }
            m_turboCodeInputMessage[bitI] = bit;
        }
        file >> label;
        if (label != std::string("TurboCodePuncturingPatterns")) {
            if (label == std::string("TurboCodeInputMessage")) {
                setupTurboCodeNoiseConditionalPDFs();
                return;
            }
            throw std::logic_error(std::string("Error processing user specific data file ") +
                                   std::string(": expecting label \"TurboCodePuncturingPatterns\", found label \"") + label + std::string("\""));
        }
        file >> vectorSize;
        binary_polynomial_vector puncturingPatterns;
        puncturingPatterns.resize(vectorSize);
        for (unsigned ppI = 0; ppI < puncturingPatterns.size(); ppI++) {
            file >> vectorSize;
            puncturingPatterns[ppI].resize(vectorSize);
            for (unsigned int ppbI = 0; ppbI < vectorSize; ppbI++) {
                unsigned int value;
                bool bit;
                file >> value;
                switch (value) {
                case 0:
                    bit = false;
                    break;
                case 1:
                    bit = true;
                    break;
                default:
                    throw std::logic_error(std::string("Non valid binary value read: ") + std::to_string(value));
                }
                puncturingPatterns[ppI][ppbI] = bit;
            }
        }
        m_turboCode.setPuncturingPatterns(puncturingPatterns);
        setupTurboCodeNoiseConditionalPDFs();
    }
}


void HybridStrengthenedTurboCodeFuzzyExtractor::writeUserSpecificData(std::ostream &file)
{
    file << "UserId " << m_userId << std::endl;
    file << "InputDimensionality " << m_inputDimensionality << std::endl;
    file << "CodeOffsetStdDevOffset " << m_codeOffsetStdDevOffset << std::endl;
    file << "CodeOffsetStdDevFactor " << m_codeOffsetStdDevFactor << std::endl;
    file << "FuzzyCommitmentMinReliability " << m_fuzzyCommitmentMinReliability << std::endl;
    file << "CodeOffsetFeaturesCount " << m_codeOffsetFeaturesCount << std::endl;
    file << "CodeOffsetFeaturesIndexes" << std::endl;
    file << m_codeOffsetFeaturesIndexes.size() << std::endl;
    for (unsigned int featureI = 0; featureI < m_codeOffsetFeaturesIndexes.size(); featureI++) {
        file << m_codeOffsetFeaturesIndexes[featureI];
        if (featureI < m_codeOffsetFeaturesIndexes.size() - 1) {
            file << " ";
        } else {
            file << std::endl;
        }
    }
    file << "CodeOffsetTotalCapacity " << m_codeOffsetTotalCapacity << std::endl;
    file << "CodeOffsetFeaturesCapacities" << std::endl;
    file << m_codeOffsetFeaturesCapacities.size() << std::endl;
    for (unsigned int featureI = 0; featureI < m_codeOffsetFeaturesCapacities.size(); featureI++) {
        file << m_codeOffsetFeaturesCapacities[featureI];
        if (featureI < m_codeOffsetFeaturesCapacities.size() - 1) {
            file << " ";
        } else {
            file << std::endl;
        }
    }
    file << "CodeOffsetEnrollmentSamplesStdDev" << std::endl;
    file << m_codeOffsetEnrollmentSamplesStdDev.size() << std::endl;
    for (unsigned int featureI = 0; featureI < m_codeOffsetEnrollmentSamplesStdDev.size(); featureI++) {
        file << m_codeOffsetEnrollmentSamplesStdDev[featureI];
        if (featureI < m_codeOffsetEnrollmentSamplesStdDev.size() - 1) {
            file << " ";
        } else {
            file << std::endl;
        }
    }
    file << "Commitment" << std::endl;
    file << m_commitment.size() << std::endl;
    for (unsigned int comSymI = 0; comSymI < m_commitment.size(); comSymI++) {
        file << m_commitment[comSymI];
        if (comSymI < m_commitment.size() - 1) {
            file << " ";
        } else {
            file << std::endl;
        }
    }
    file << "FuzzyCommitmentTotalCapacity " << m_fuzzyCommitmentTotalCapacity << std::endl;
    file << "FuzzyCommitmentFeaturesIndexes" << std::endl;
    file << m_fuzzyCommitmentFeaturesIndexes.size() << std::endl;
    for (unsigned int featureI = 0; featureI < m_fuzzyCommitmentFeaturesIndexes.size(); featureI++) {
        file << m_fuzzyCommitmentFeaturesIndexes[featureI];
        if (featureI < m_fuzzyCommitmentFeaturesIndexes.size() - 1) {
            file << " ";
        } else {
            file << std::endl;
        }
    }
    file << "CombinedFeaturesCount " << m_combinedFeaturesCount << std::endl;
    file << "CombinedFeaturesIndexes" << std::endl;
    file << m_combinedFeaturesIndexes.size() << std::endl;
    for (unsigned int combFeatI = 0; combFeatI < m_combinedFeaturesIndexes.size(); combFeatI++) {
        file << m_combinedFeaturesIndexes[combFeatI].size() << std::endl;
        for (unsigned int featureI = 0; featureI < m_combinedFeaturesIndexes[combFeatI].size(); featureI++) {
            file << m_combinedFeaturesIndexes[combFeatI][featureI];
            if (featureI < m_combinedFeaturesIndexes[combFeatI].size() - 1) {
                file << " ";
            } else {
                file << std::endl;
            }
        }
    }
    file << "CombinedFeaturesWeights" << std::endl;
    file << m_combinedFeaturesWeights.size() << std::endl;
    for (unsigned int combFeatI = 0; combFeatI < m_combinedFeaturesWeights.size(); combFeatI++) {
        file << m_combinedFeaturesWeights[combFeatI].size() << std::endl;
        for (unsigned int featureI = 0; featureI < m_combinedFeaturesWeights[combFeatI].size(); featureI++) {
            file << m_combinedFeaturesWeights[combFeatI][featureI];
            if (featureI < m_combinedFeaturesWeights[combFeatI].size() - 1) {
                file << " ";
            } else {
                file << std::endl;
            }
        }
    }
    file << "CombinedFeaturesStdDevs" << std::endl;
    file << m_combinedFeaturesStdDevs.size() << std::endl;
    for (unsigned int featureI = 0; featureI < m_combinedFeaturesStdDevs.size(); featureI++) {
        file << m_combinedFeaturesStdDevs[featureI];
        if (featureI < m_combinedFeaturesStdDevs.size() - 1) {
            file << " ";
        } else {
            file << std::endl;
        }
    }
    file << "FuzzyExtractorCapacity " << m_fuzzyExtractorCapacity << std::endl;
    file << "ValidForReproduce ";
    if (m_validForReproduce) {
        file << "true" << std::endl;
        binary_polynomial_vector turboCodePuncturingPatterns = m_turboCode.puncturingPatterns();
        file << "TurboCodePuncturingPatterns" << std::endl;
        file << turboCodePuncturingPatterns.size() << std::endl;
        for (unsigned int punctPatI = 0; punctPatI < turboCodePuncturingPatterns.size(); punctPatI++) {
            file << turboCodePuncturingPatterns[punctPatI].size() << std::endl;
            for (unsigned int bitI = 0; bitI < turboCodePuncturingPatterns[punctPatI].size(); bitI++) {
                if (turboCodePuncturingPatterns[punctPatI][bitI]) {
                    file << "1";
                } else {
                    file << "0";
                }
                if (bitI < turboCodePuncturingPatterns[punctPatI].size() - 1) {
                    file << " ";
                } else {
                    file << std::endl;
                }
            }
        }
        file << "SymbolsPermutation" << std::endl;
        file << m_symbolsPermutation.size() << std::endl;
        for (unsigned int featureI = 0; featureI < m_symbolsPermutation.size(); featureI++) {
            file << m_symbolsPermutation[featureI];
            if (featureI < m_symbolsPermutation.size() - 1) {
                file << " ";
            } else {
                file << std::endl;
            }
        }
        file << "SymbolIndexes" << std::endl;
        file << m_symbolIndexes.size() << std::endl;
        for (unsigned int featureI = 0; featureI < m_symbolIndexes.size(); featureI++) {
            file << m_symbolIndexes[featureI];
            if (featureI < m_symbolIndexes.size() - 1) {
                file << " ";
            } else {
                file << std::endl;
            }
        }
        uint_vector_vector turboCodeInterleavers = m_turboCode.interleavers();
        file << "TurboCodeInterleavers" << std::endl;
        file << turboCodeInterleavers.size() << std::endl;
        for (unsigned int interleaverI = 0; interleaverI < turboCodeInterleavers.size(); interleaverI++) {
            file << turboCodeInterleavers[interleaverI].size() << std::endl;
            for (unsigned int featureI = 0; featureI < turboCodeInterleavers[interleaverI].size(); featureI++) {
                file << turboCodeInterleavers[interleaverI][featureI];
                if (featureI < turboCodeInterleavers[interleaverI].size() - 1) {
                    file << " ";
                } else {
                    file << std::endl;
                }
            }
        }
        file << "TurboCodeInputMessageLength " << m_turboCodeInputMessageLength << std::endl;
        file << "TurboCodeInputMessage" << std::endl;
        file << m_turboCodeInputMessage.size() << std::endl;
        for (unsigned int bitI = 0; bitI < m_turboCodeInputMessage.size(); bitI++) {
            if (m_turboCodeInputMessage[bitI]) {
                file << "1";
            } else {
                file << "0";
            }
            if (bitI < m_turboCodeInputMessage.size() - 1) {
                file << " ";
            } else {
                file << std::endl;
            }
        }
        file << "TurboCodePuncturingPatterns " << std::endl;
        binary_polynomial_vector pps = turboCode().puncturingPatterns();
        file << pps.size() << std::endl;
        for (unsigned int ppI = 0; ppI < pps.size(); ppI++) {
            file << pps[ppI].size() << std::endl;
            for (unsigned int pbI = 0; pbI < pps[ppI].size(); pbI++) {
                file << pps[ppI][pbI];
                if (pbI < pps[ppI].size() - 1) {
                    file << " ";
                } else {
                    file << std::endl;
                }
            }
        }
    } else {
        file << "false" << std::endl;
    }
}

void HybridStrengthenedTurboCodeFuzzyExtractor::saveUserSpecificData(const std::string &fileName)
{
    std::ofstream file(fileName);
    if (!file) {
        throw std::ofstream::failure(std::string("Unable to open file ") + fileName + std::string(" for writing."));
    }
    
    writeUserSpecificData(file);
    file.close();
}

std::vector<uint8_t> HybridStrengthenedTurboCodeFuzzyExtractor::getUserSpecificData()
{
    std::ostringstream ss;
    writeUserSpecificData(ss);
    const std::string s = ss.str();
    return std::vector<uint8_t>(s.begin(), s.end());
}


void HybridStrengthenedTurboCodeFuzzyExtractor::saveKey(const std::string &fileName) {
    std::ofstream file(fileName);
    if (!file) {
        throw std::ofstream::failure(std::string("Unable to open file ") + fileName + std::string(" for writing."));
    }
    file << m_turboCodeInputMessage.size() << std::endl;
    for (unsigned int bitI = 0; bitI < m_turboCodeInputMessage.size(); bitI++) {
        if (m_turboCodeInputMessage[bitI]) {
            file << "1";
        } else {
            file << "0";
        }
        if (bitI < m_turboCodeInputMessage.size() - 1) {
            file << " ";
        } else {
            file << std::endl;
        }
    }
}

void HybridStrengthenedTurboCodeFuzzyExtractor::saveGeneralData(const std::string &fileName)
{
    std::ofstream file(fileName);
    if (!file) {
        throw std::ofstream::failure(std::string("Unable to open file ") + fileName + std::string(" for writing."));
    }
    file << "TurboCodeConfigFile " << m_turboCodeConfigFileName << std::endl;
    file << "TurboCodeMaxDecodingIterations " << m_turboCodeMaxDecodingIterations << std::endl;
    file << "TrainSetSamplesFilesFolder " << m_trainSetSamplesFilesFolderName << std::endl;
    file << "CodeOffsetStdDevFactor " << m_codeOffsetStdDevFactor << std::endl;
    file << "CodeOffsetStdDevOffset " << m_codeOffsetStdDevOffset << std::endl;
    file << "FuzzyCommitmentMinReliability " << m_fuzzyCommitmentMinReliability << std::endl;
    file.close();
}

void HybridStrengthenedTurboCodeFuzzyExtractor::loadGeneralData(const std::string &fileName)
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
    if (label != std::string("TrainSetSamplesFilesFolder")) {
        throw std::logic_error(std::string("HybridStrengthenedTurboCodeFuzzyExtractor::loadGeneralData: Label in  configuration file ")
                               + fileName + std::string(": ") + label + std::string(". TrainSetSamplesFilesFolder was expected."));
    }
    file >> m_trainSetSamplesFilesFolderName;
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
    loadTrainSetSamplesAndSetupNormalizers();
    std::cout << "Done." << std::endl;
}

void HybridStrengthenedTurboCodeFuzzyExtractor::setupNormalizers(const real_number_vector_vector &trainSetSamples) {
    if (trainSetSamples.size() == 0) {
        throw std::logic_error("Train set samples count cannot be 0.");
    }
    unsigned int dimsCount(trainSetSamples.begin()->size()), samplesCount(trainSetSamples.size());
    if (m_inputDimensionality == 0) {
        setInputDimensionality(dimsCount);
    }
    if (m_inputDimensionality != dimsCount) {
        throw std::logic_error(std::string("Train set samples dimensionality (") +
                               std::to_string(dimsCount) +
                               std::string(") must be equal to the Hybrid Strengthened Turbo-Code Fuzzy Extractor input dimensionality (") +
                               std::to_string(m_inputDimensionality) +
                               std::string(")."));
    }
    real_number_vector_vector trainSetSamplesTranspose(dimsCount, real_number_vector(samplesCount, 0));
    m_uniformNormalizer = std::make_shared<std::vector<std::map<real_number,real_number>>>(dimsCount);
    for (unsigned int sampleI = 0; sampleI < samplesCount; sampleI++) {
        if (trainSetSamples[sampleI].size() != dimsCount) {
            throw std::logic_error(std::string("Inconsistent sample size found in train set. Sample ") + std::to_string(sampleI + 1) +
                                   std::string(" has dimensionality ") + std::to_string(trainSetSamples[sampleI].size()) +
                                   std::string(" but all previous samples have dimensionality ") + std::to_string(dimsCount));
        }
        for (unsigned int dimI = 0; dimI < dimsCount; dimI++) {
            trainSetSamplesTranspose[dimI][sampleI] = trainSetSamples[sampleI][dimI];
        }
    }
    for (unsigned int dimI = 0; dimI < dimsCount; dimI++) {
        std::sort(trainSetSamplesTranspose[dimI].begin(), trainSetSamplesTranspose[dimI].end());
        for (unsigned int sampleI = 0; sampleI < samplesCount; sampleI++) {
            real_number cdf = (sampleI*2.0 + 1.0)/(2.0*samplesCount);
            (*m_uniformNormalizer)[dimI].insert(std::pair<real_number,real_number>(trainSetSamplesTranspose[dimI][sampleI], cdf));
        }
    }
}

void HybridStrengthenedTurboCodeFuzzyExtractor::releaseNormalizers()
{
    if (!m_uniformNormalizer) {
        throw std::logic_error(std::string("Uniform normalizer is not initialized."));
    } else {
        m_uniformNormalizer.reset();
    }
}

void HybridStrengthenedTurboCodeFuzzyExtractor::uniformNormalizationBatch(const real_number_vector_vector &samples, real_number_vector_vector &normalizedSamples)
{
    if (!m_uniformNormalizer) {
        throw std::logic_error(std::string("Uniform normalizer is not initialized."));
    }
    if (m_uniformNormalizer->size() == 0) {
        throw std::logic_error(std::string("Uniform normalizer is not initialized."));
    }
    if (m_uniformNormalizer->size() != m_inputDimensionality) {
        throw std::logic_error(std::string("Uniform normalizer size (") + std::to_string(m_uniformNormalizer->size()) +
                          std::string(") not consistent with input dimensionality (") + std::to_string(m_inputDimensionality) +
                          std::string(")."));
    }
    normalizedSamples.resize(samples.size());
    for (unsigned int sampleI = 0; sampleI < samples.size(); sampleI++) {
        if (samples[sampleI].size() != m_inputDimensionality) {
            throw std::logic_error(std::string("Sample number ") + std::to_string(sampleI + 1) + std::string(" has dimensionality ") +
                              std::to_string(samples[sampleI].size()) + std::string(", but input dimensionality is ") + std::to_string(m_inputDimensionality) + std::string("."));
        }
        normalizedSamples[sampleI].resize(samples[sampleI].size());
        for (unsigned int dimI = 0; dimI < m_inputDimensionality; dimI++) {
            std::_Rb_tree_const_iterator<std::pair<const real_number, real_number>> upperBound = (*m_uniformNormalizer)[dimI].upper_bound(samples[sampleI][dimI]);
            if (upperBound != (*m_uniformNormalizer)[dimI].end()) {
                normalizedSamples[sampleI][dimI] = upperBound->second;
            } else {
                normalizedSamples[sampleI][dimI] = 1.0;
            }
            normalizedSamples[sampleI][dimI] *= 2.0*M_PI;
        }
    }
}

void HybridStrengthenedTurboCodeFuzzyExtractor::uniformNormalization(const real_number_vector &sample, real_number_vector &normalizedSample)
{
    if (!m_uniformNormalizer) {
        throw std::logic_error(std::string("Uniform normalizer is not initialized."));
    }
    if (m_uniformNormalizer->size() == 0) {
        throw std::logic_error(std::string("Uniform normalizer is not initialized."));
    }
    if (m_uniformNormalizer->size() != m_inputDimensionality) {
        throw std::logic_error(std::string("Uniform normalizer size (") + std::to_string(m_uniformNormalizer->size()) +
                          std::string(") not consistent with input dimensionality (") + std::to_string(m_inputDimensionality) +
                          std::string(")."));
    }
    if (sample.size() != m_inputDimensionality) {
        throw std::logic_error(std::string("Sample has dimensionality ") + std::to_string(sample.size()) +
                          std::string(", but input dimensionality is ") + std::to_string(m_inputDimensionality) +
                          std::string("."));
    }
    if (normalizedSample.size() != m_inputDimensionality) {
        normalizedSample.resize(m_inputDimensionality);
    }
    for (unsigned int dimI = 0; dimI < m_inputDimensionality; dimI++) {
        std::_Rb_tree_const_iterator<std::pair<const real_number, real_number>> upperBound = (*m_uniformNormalizer)[dimI].upper_bound(sample[dimI]);
        if (upperBound != (*m_uniformNormalizer)[dimI].end()) {
            normalizedSample[dimI] = upperBound->second;
        } else {
            normalizedSample[dimI] = 1.0;
        }
	normalizedSample[dimI] *= 2.0*M_PI;
    }
}

void HybridStrengthenedTurboCodeFuzzyExtractor::normalizeCodeOffsetSample(const real_number_vector &sample, real_number_vector &normalizedSample)
{
    if (sample.size() != m_inputDimensionality) {
        throw std::logic_error(std::string("Sample size (") + std::to_string(sample.size()) +
                          std::string(") not consistent with input dimensionality (") + std::to_string(m_inputDimensionality) +
                          std::string(")."));
    }
    normalizedSample.resize(m_codeOffsetFeaturesCount);
    for (unsigned int codeOffsetDimI = 0; codeOffsetDimI < m_codeOffsetFeaturesCount; codeOffsetDimI++) {
        unsigned int inputDimI = m_codeOffsetFeaturesIndexes[codeOffsetDimI];
        std::_Rb_tree_const_iterator<std::pair<const real_number, real_number>> upperBound = (*m_uniformNormalizer)[inputDimI].upper_bound(sample[inputDimI]);
        if (upperBound != (*m_uniformNormalizer)[inputDimI].end()) {
            normalizedSample[codeOffsetDimI] = upperBound->second;
        } else {
            normalizedSample[codeOffsetDimI] = 1.0;
        }
        normalizedSample[codeOffsetDimI] *= 2.0*M_PI;
    }
}

void HybridStrengthenedTurboCodeFuzzyExtractor::parseTurboCodeConfigFile() {
    ::parseTurboCodeConfigFile(m_turboCodeConfigFileName, m_turboCode);
}

void HybridStrengthenedTurboCodeFuzzyExtractor::loadTrainSetSamplesAndSetupNormalizers()
{
    std::vector<std::string> trainSetSamplesFilesNames;
    std::vector<real_number_vector_vector> trainSetSamples;
    readSamplesFilesFromPath(m_trainSetSamplesFilesFolderName,
                             std::string(".txt"),
                             trainSetSamplesFilesNames,
                             trainSetSamples);
    unsigned int trainSetSamplesCount(0);
    for (unsigned int subjectI = 0; subjectI < trainSetSamples.size(); subjectI++) {
        trainSetSamplesCount += trainSetSamples[subjectI].size();
    }
    real_number_vector_vector trainSetSamplesPool(trainSetSamplesCount, real_number_vector(trainSetSamples.begin()->size()));
    unsigned int samplesOffset(0);
    for (unsigned int subjectI = 0; subjectI < trainSetSamples.size(); subjectI++) {
        for (unsigned int sampI = 0; sampI < trainSetSamples[subjectI].size(); sampI++) {
            trainSetSamplesPool[samplesOffset + sampI] = trainSetSamples[subjectI][sampI];
        }
        samplesOffset += trainSetSamples[subjectI].size();
    }
    setupNormalizers(trainSetSamplesPool);
}

void HybridStrengthenedTurboCodeFuzzyExtractor::setUserId(const std::string &newUserId)
{
    m_userId = newUserId;
}

void HybridStrengthenedTurboCodeFuzzyExtractor::setupTurboCodeNoiseConditionalPDFs()
{
    m_pdfs0 = GMM_vector(m_fuzzyExtractorCapacity);
    m_pdfs1 = GMM_vector(m_fuzzyExtractorCapacity);

    for (unsigned int codeOffestFeatureI = 0; codeOffestFeatureI < m_codeOffsetFeaturesCount; codeOffestFeatureI++) {
        PSK pskConstellation;
        GMM noiseDistribution;
        double mean(0.0);
        double variance = m_codeOffsetEnrollmentSamplesStdDev[codeOffestFeatureI]*m_codeOffsetEnrollmentSamplesStdDev[codeOffestFeatureI];
        noiseDistribution.createFromOneGaussian(1, &mean, &variance);
        noiseDistribution.calculateFactors();
        pskConstellation.setupConstellation(m_codeOffsetFeaturesCapacities[codeOffestFeatureI], noiseDistribution);
        for (unsigned int bitI = 0; bitI < m_codeOffsetFeaturesCapacities[codeOffestFeatureI]; bitI++) {
            uint pdfIndex = m_symbolsBitOffset[codeOffestFeatureI] + bitI;
            m_pdfs0[pdfIndex] = pskConstellation.pdfs0()[bitI];
            m_pdfs1[pdfIndex] = pskConstellation.pdfs1()[bitI];
        }
    }
    for (unsigned int fuzzyCommitmentFeatureI = 0; fuzzyCommitmentFeatureI < m_fuzzyCommitmentTotalCapacity; fuzzyCommitmentFeatureI++) {
        PAM2 pam2Constellation;
        GMM noiseDistribution;
        double mean(0.0);
        double variance =
            m_combinedFeaturesStdDevs[fuzzyCommitmentFeatureI] *
            m_combinedFeaturesStdDevs[fuzzyCommitmentFeatureI];
        // std::cout << "Variance: " << variance << std::endl;
        noiseDistribution.createFromOneGaussian(1, &mean, &variance);
        noiseDistribution.calculateFactors();
        // std::cout << "Setting up 2-PAM costellation" << std::endl;
        pam2Constellation.setupConstellation(1, noiseDistribution);
        uint pdfIndex = m_symbolsBitOffset[m_codeOffsetFeaturesCount + fuzzyCommitmentFeatureI];
        m_pdfs0[pdfIndex] = pam2Constellation.pdfs0()[0];
        m_pdfs1[pdfIndex] = pam2Constellation.pdfs1()[0];
    }

    m_turboCode.setSymbolIndexes(m_symbolIndexes);
    m_turboCode.setPdfs0(m_pdfs0);
    m_turboCode.setPdfs1(m_pdfs1);
}

uint HybridStrengthenedTurboCodeFuzzyExtractor::inputDimensionality() const
{
    return m_inputDimensionality;
}

std::vector<std::map<real_number, real_number> > *HybridStrengthenedTurboCodeFuzzyExtractor::uniformNormalizer() const
{
    return m_uniformNormalizer.get();
}

real_number HybridStrengthenedTurboCodeFuzzyExtractor::codeOffsetStdDevOffset() const
{
    return m_codeOffsetStdDevOffset;
}

void HybridStrengthenedTurboCodeFuzzyExtractor::setCodeOffsetStdDevOffset(real_number newCodeOffsetStdDevOffset)
{
    m_codeOffsetStdDevOffset = newCodeOffsetStdDevOffset;
}

real_number HybridStrengthenedTurboCodeFuzzyExtractor::fuzzyCommitmentMinReliability() const
{
    return m_fuzzyCommitmentMinReliability;
}

void HybridStrengthenedTurboCodeFuzzyExtractor::setFuzzyCommitmentMinReliability(real_number newFuzzyCommitmentMinReliability)
{
    m_fuzzyCommitmentMinReliability = newFuzzyCommitmentMinReliability;
}

real_number HybridStrengthenedTurboCodeFuzzyExtractor::codeOffsetStdDevFactor() const
{
    return m_codeOffsetStdDevFactor;
}

void HybridStrengthenedTurboCodeFuzzyExtractor::setCodeOffsetStdDevFactor(real_number newCodeOffsetStdDevFactor)
{
    m_codeOffsetStdDevFactor = newCodeOffsetStdDevFactor;
}

std::string HybridStrengthenedTurboCodeFuzzyExtractor::turboCodeConfigFileName() const
{
    return m_turboCodeConfigFileName;
}

uint HybridStrengthenedTurboCodeFuzzyExtractor::turboCodeMaxDecodingIterations() const
{
    return m_turboCodeMaxDecodingIterations;
}

std::string HybridStrengthenedTurboCodeFuzzyExtractor::userId() const
{
    return m_userId;
}

const TurboCode &HybridStrengthenedTurboCodeFuzzyExtractor::turboCode() const
{
    return m_turboCode;
}

binary_polynomial HybridStrengthenedTurboCodeFuzzyExtractor::turboCodeInputMessage() const
{
    return m_turboCodeInputMessage;
}

bool HybridStrengthenedTurboCodeFuzzyExtractor::validForReproduce() const
{
    return m_validForReproduce;
}

void readHybridStrengthenedTurboCodeFuzzyExtractorsFromPath(const std::string userSpecificHSTEFEFilesPathName,
                                                            const std::string userSpecificHSTEFEFilesExtension,
                                                            const HybridStrengthenedTurboCodeFuzzyExtractor &generalHSTCFE,
                                                            std::vector<std::string> &userSpecificHSTEFEFilesNames,
                                                            std::vector<HybridStrengthenedTurboCodeFuzzyExtractor> &hstcfeVector) {
    std::filesystem::path  userSpecificHSTEFEFilesPath(userSpecificHSTEFEFilesPathName);
    if (!std::filesystem::exists(userSpecificHSTEFEFilesPath)) {
        throw std::ifstream::failure(std::string("User-specific HybridStrengthenedTurboCodeFuzzyExtractor files path ") + std::string(userSpecificHSTEFEFilesPathName) + std::string(" does not exist."));
    }
    if (!std::filesystem::is_directory(userSpecificHSTEFEFilesPath)) {
        throw std::ifstream::failure(std::string("Enrollment samples files path ") + std::string(userSpecificHSTEFEFilesPathName) + std::string(" is not a directory."));
    }
    userSpecificHSTEFEFilesNames.clear();
    for (auto const& dir_entry : std::filesystem::directory_iterator{userSpecificHSTEFEFilesPath}) {
        if (dir_entry.is_regular_file()) {
            std::string extension = dir_entry.path().extension();
            std::string stem = dir_entry.path().stem();
            if ((extension == userSpecificHSTEFEFilesExtension) && (dir_entry.path().has_stem())) {
                hstcfeVector.push_back(generalHSTCFE);
                auto last = hstcfeVector.rbegin();
                try {
#ifdef DEBUG
                    std::cout << "Reading user-specific HSTCFE configuration file from " << stem << " ... ";
                    std::cout.flush();
#endif
                    last->loadUserSpecificData(dir_entry.path());
#ifdef DEBUG
                    std::cout << std::endl;
#endif
                } catch (std::exception &e) {
                    throw std::logic_error(std::string("Exception caught while doing HybridStrengthenedTurboCodeFuzzyExtractor::loadUserSpecificData: ") + e.what());
                }
                userSpecificHSTEFEFilesNames.push_back(stem);
            }
        }
    }
}
