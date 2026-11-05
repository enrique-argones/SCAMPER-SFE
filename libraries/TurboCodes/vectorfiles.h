/// \file vectorfiles.h
/// \brief Text-file readers (samples, templates, Turbo Code configurations) and basic sample statistics.
///
/// All files handled here are whitespace-separated plain text. Biometric samples of one subject are stored in one file, and
/// a folder of such files (one per subject, the file stem being the subject identifier) is read by \ref ::readSamplesFilesFromPath.

#ifndef VECTORFILES_H
#define VECTORFILES_H

#include "turbo-codes_global.h"
#include "turbo-codes_types.h"
#include "turbocode.h"
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>

/// This is a template function to print vectors.
/// @tparam T Class of the elements of the vector to be printed
/// @param[in] p Vector containing elements of class T
/// @param[in] width Width reserved to print each element
///
/// It prints a vector of class T elements, tabulated with a size width
template<typename T> void TURBOCODES_EXPORT printVector(const std::vector<T> &p, const unsigned int width = 0) {
    unsigned int prevPrecision = std::cout.precision();
    std::cout.precision(3);
    for (unsigned int indexI = 0; indexI < p.size(); indexI++) {
        if (width > 0) {
            std::cout << std::setw(width) << p[indexI];
        } else {
            std::cout << p[indexI];
        }
        if (indexI < p.size()-1) {
            std::cout << " ";
        }
    }
    std::cout.precision(prevPrecision);
}
template void TURBOCODES_EXPORT printVector<int>(const std::vector<int> &p, const unsigned int width = 0);
template void TURBOCODES_EXPORT printVector<unsigned int>(const std::vector<unsigned int> &p, const unsigned int width = 0);
template void TURBOCODES_EXPORT printVector<real_number>(const std::vector<real_number> &p, const unsigned int width = 0);
template void TURBOCODES_EXPORT printVector<bool>(const std::vector<bool> &p, const unsigned int width = 0);
/*
/// This is the function @ref printVector spezialized for the class @ref binary_polynomial
/// @param[in] p Vector containing elements of type bool
/// @param[in] width It may be 0 or different than 0. If it is 0, no space between the elements is printed. Otherwise, the elements are separated by a single space.
///
/// It prints a vector of binary values separated by spaces if \p width > 0, or not separated if \p width == 0.
template <> void TURBOCODES_EXPORT printVector(const binary_polynomial &p, const unsigned int width) {
    for (unsigned int indexI = 0; indexI < p.size(); indexI++) {
        // std::cout << "[" << indexI << "]";
        if (width > 0) {
            if (p[indexI]) {
                std::cout << std::setw(width) << 1;
            } else {
                std::cout << std::setw(width) << 0;
            }
        } else {
            if (p[indexI]) {
                std::cout << "1";
            } else {
                std::cout << "0";
            }
        }
    }
}
*/

/// This function parses a template file into a \ref real_number_vector object
/// @param[in] templateFileName name of the template file.
/// @param[out] templateVector \ref real_number_vector object which is setup by this function
///
/// On the template configuration file format:<br>
/// TEMPLATE_SIZE<br>
/// TEMPLATE[0] ... TEMPLATE[TEMPLATE_SIZE - 1]<br>
void TURBOCODES_EXPORT parseTemplateFile(const std::string templateFileName, real_number_vector &templateVector);

/// This function parses an expected standard deviation file into a \ref real_number_vector object
/// @param[in] expectedStdFileName name of the expected standard deviation file.
/// @param[in] templateLength Number of elements in the file \p expectedStdFileName read into the object \p expectedStdVector
/// @param[out] expectedStdVector \ref real_number_vector object which is read by this function
///
/// On the expected standard deviation file format:<br>
/// TEMPLATE_SIZE<br>
/// STD[0] ... STD[TEMPLATE_SIZE - 1]<br>
void TURBOCODES_EXPORT parseExpectedStdFile(const std::string expectedStdFileName, const unsigned int templateLength, real_number_vector &expectedStdVector);

/// This function parses an attempts file into a \ref real_number_vector_vector object
/// @param[in] attemptsFileName name of the attempts file.
/// @param[in] templateLength Number of elements in the file \p expectedStdFileName read into the object \p attemptsFileName
/// @param[out] attemptsVectorVector \ref real_number_vector_vector object which is read by this function
///
/// On the template attempts file format:<br>
/// ATTEMPTS_COUNT ATTEMPTS_SIZE<br>
/// ATTEMPTS[0][0] ... ATTEMPTS[O][TEMPLATE_SIZE - 1]<br>
/// ...<br>
/// ATTEMPTS[ATTEMPTS_COUNT - 1][0] ... ATTEMPTS[ATTEMPTS_COUNT - 1][TEMPLATE_SIZE - 1]<br>
void TURBOCODES_EXPORT parseAttemptsFile(const std::string attemptsFileName, const unsigned int templateLength, real_number_vector_vector &attemptsVectorVector);

/// This function parses a samples file and computes its first and second order statistics.
/// @param[in] samplesFileName name of the samples file.
/// @param[out] samples \ref real_number_vector_vector object containing the samples read from \p samplesFileName
/// @param[out] samplesMeanVector \ref real_number_vector object containing the \p samples mean vector
/// @param[out] samplesVarianceVector \ref real_number_vector object containing the per-dimension (unbiased) standard deviations
/// of \p samples. Despite its name, it holds standard deviations, not variances.
/// @param[out] samplesCovarianceMatrix \ref real_number_vector_vector object containing the \p sample covariance matrix
///
/// On the samples file format:<br>
/// SAMPLES_COUNT SAMPLE_SIZE<br>
/// TEMPLATE[0][0] ... TEMPLATE[0][SAMPLE_SIZE - 1]<br>
/// ... <br>
/// TEMPLATE[SAMPLES_COUNT - 1][0] ... TEMPLATE[SAMPLES_COUNT - 1][SAMPLE_SIZE - 1]<br>
void TURBOCODES_EXPORT parseSamplesFile(const std::string samplesFileName,
                                                  real_number_vector_vector &samples,
                                                  real_number_vector &samplesMeanVector,
                                                  real_number_vector &samplesVarianceVector,
                                                  real_number_vector_vector &samplesCovarianceMatrix);

/// This function parses a samples file (one sample per row) into a \ref real_number_vector_vector object.
/// @param[in] samplesFileName name of the samples file.
/// @param[out] samples \ref real_number_vector_vector object containing the samples read from \p samplesFileName
///
/// On the samples file format:<br>
/// SAMPLES_COUNT SAMPLE_SIZE<br>
/// TEMPLATE[0][0] ... TEMPLATE[0][SAMPLE_SIZE - 1]<br>
/// ... <br>
/// TEMPLATE[SAMPLES_COUNT - 1][0] ... TEMPLATE[SAMPLES_COUNT - 1][SAMPLE_SIZE - 1]<br>
void TURBOCODES_EXPORT parseSamplesFile(const std::string samplesFileName,
                                                  real_number_vector_vector &samples);

/**
 * @brief computeSampleStatistics computes the per-dimension mean and standard deviation of a set of samples.
 * @param[in] samples \ref real_number_vector_vector containing one sample per element. All samples must have the same dimensionality.
 * @param[out] samplesMeans \ref real_number_vector containing the means vector of the samples
 * @param[out] samplesStdDevs \ref real_number_vector containing the (unbiased) standard deviations vector of the samples
 * @param[out] samplesCovar \ref real_number_vector_vector receiving the unbiased estimator of the covariance matrix of the samples (D x D).
 */
void TURBOCODES_EXPORT computeSampleStatistics(const real_number_vector_vector &samples, real_number_vector &samplesMeans, real_number_vector &samplesStdDevs, real_number_vector_vector &samplesCovar);

/**
 * @brief parseTrainSetSamplesFile reads a training set samples file (same format as \ref parseSamplesFile).
 *
 * The training samples are later used to build the empirical-CDF normalizers of the fuzzy extractors.
 * @param[in] trainSetSamplesFileName Name of the training set samples file.
 * @param[out] trainSetSamples Samples read from the file, one per element.
 */
void TURBOCODES_EXPORT parseTrainSetSamplesFile(const std::string trainSetSamplesFileName,
                                                real_number_vector_vector &trainSetSamples);

/// This function parses a TurboCode configuation file and setups a turbocode.
/// @param[in] configFileName name of the configuration file.
/// @param[in,out] turboCode \ref TurboCode object which is setup by this funciton
///
/// On the TurboCode configuration file format (whitespace separated):<br>
/// GENERATORS_COUNT<br>
/// GENERATOR_1_FEEDFORWARD_POLYNOMIAL<br>
/// GENERATOR_1_FEEDBACKWARD_POLYNOMIAL<br>
/// ...<br>
/// GENERATOR_GENERATORS_COUNT_FEEDFORWARD_POLYNOMIAL<br>
/// GENERATOR_GENERATORS_COUNT_FEEDBACKWARD_POLYNOMIAL<br>
/// PUNCTURING_PATTERN_1<br>
/// ...<br>
/// PUNCTURING_PATTERN_GENERATORS_COUNT<br>
/// INTERLEAVER_1[0] ... INTERLEAVER_1[N_I - 1]<br>
/// ...<br>
/// INTERLEAVER_GENERATORS_COUNT[0] ... INTERLEAVER_GENERATORS_COUNT[N_I - 1]<br>
///
/// Polynomials and puncturing patterns are written as strings of 0s and 1s (lowest degree first). For each generator, the first
/// polynomial read is stored as \c first of the \ref binary_polynomial_pair and is used by \ref TurboCode as the feedback
/// polynomial; the second one is used as the parity (feed-forward) polynomial, regardless of the FEEDFORWARD/FEEDBACK labels above. All puncturing patterns
/// have length \f$N_I\f$ and every interleaver is a permutation of \f$\{0, \ldots, N_I - 1\}\f$. Example (rate 1/3, \f$N_I = 60\f$):
/// \code
/// 2
/// 11111
/// 11001
/// 10001
/// 11011
/// 1111...1111   (60 ones)
/// 1111...1111   (60 ones)
/// 4 51 11 15 ...   (60 indexes)
/// 35 24 53 16 ...  (60 indexes)
/// \endcode
/// The fuzzy extractors only use the generators from this file: they replace the interleavers and puncturing patterns
/// with random, user-specific ones sized to the capacity of each enrolled template.
/// @exception std::logic_error if the file cannot be opened or a value is malformed.
///
void TURBOCODES_EXPORT parseTurboCodeConfigFile(const std::string configFileName, TurboCode &turboCode);

/**
 * @brief readSamplesFilesFromPath Function to read all samples files from a given folder.
 * @param[in] enrollmentSamplesFilesPathName Path to the samples files folder.
 * @param[in] samplesFilesExtension Extension, including the initial ".", of the samples files. All files with a different extension will be ignored.
 * @param[out] samplesFilesNames vector containing all file samples files names without the extension.
 * @param[out] folderSamples std::vector< \ref real_number_vector_vector > containing all read samples
 */
void TURBOCODES_EXPORT readSamplesFilesFromPath(const std::string enrollmentSamplesFilesPathName,
                                                const std::string samplesFilesExtension,
                                                std::vector<std::string> &samplesFilesNames,
                                                std::vector<real_number_vector_vector> &folderSamples);

#endif // VECTORFILES_H

