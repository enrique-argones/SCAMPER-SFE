/// \file hybridstrengthenedturbocodefuzzyextractor.h
/// \brief Declaration of the \ref HybridStrengthenedTurboCodeFuzzyExtractor class (HSTCFE), the unimodal Strengthened Fuzzy
/// Extractor built on a soft-decoded Turbo Code.
///
/// The conditional pdfs used by the soft decoder are gmmlite::GMM objects of the GMMLite library (see the \c GMM alias in
/// turbo-codes_types.h).
/// \author Enrique Argones Rúa
/// \date 2025

#ifndef HYBRIDSTRENGTHENEDTURBOCODEFUZZYEXTRACTOR_H
#define HYBRIDSTRENGTHENEDTURBOCODEFUZZYEXTRACTOR_H

#include "HSTCFE_global.h"
#include <TurboCodes/turbocode.h>
#include <TurboCodes/psk.h>
#include <TurboCodes/pam2.h>
#include <random>
#include <mutex>
#include <memory>
#include <string>
#include <stdexcept>

/// Enables the HSTCFE-specific debugging blocks (currently only commented-out traces).
#define HSTCFE_DEBUG 1

/// Upper bound on the number of bits that \ref HybridStrengthenedTurboCodeFuzzyExtractor::bitLoading assigns to a single
/// code-offset feature (i.e. the largest PSK constellation used is \f$2^{\textrm{MAX\_CAPACITY}}\f$-PSK).
#define MAX_CAPACITY 3

/// Seed of the shared Mersenne Twister (\ref HybridStrengthenedTurboCodeFuzzyExtractor::m_twister).
/// @warning The seed is a compile-time constant, so the sequence of secrets, interleavers and permutations is the same on
/// every run of a program. This makes experiments reproducible, but it is not suitable for a deployed system, which needs a
/// cryptographically secure, freshly seeded random source.
#define RAND_SEED -1

/// # Hybrid Strengthened Turbo Code Fuzzy Extractor (HSTCFE)
///
/// A fuzzy extractor binds a uniformly random secret (the \e key) to a biometric template in such a way that the key can only be
/// recovered from a fresh biometric sample of the same subject, while the stored helper data reveals as little as possible about
/// both the key and the biometric template.
///
/// This class implements a \e hybrid construction: the features of a template are split into two groups, and each group is
/// protected with a different scheme. All the bits produced by both schemes form one codeword of a soft-decoded \ref TurboCode.
///
/// - **Code-offset (reliable features).** Features whose intra-user variability is small enough get one or more codeword bits
///   each (see \ref bitLoading). The feature is first mapped to a phase in \f$[0, 2\pi)\f$ with the empirical CDF of a training
///   set (\ref uniformNormalization), the bits are Gray-mapped to a \ref PSK phase \f$\varphi\f$, and the helper data stores
///   \f$c = (\varphi + \mu) \bmod 2\pi\f$, where \f$\mu\f$ is the mean normalized enrollment value of that feature.
/// - **Fuzzy commitment (unreliable features).** The remaining features are greedily grouped into linear combinations
///   (\ref findBestFuzzyCommitment) whose mean is \f$\pm 1\f$ and whose reliability \f$|\mu/\sigma|\f$ is at least
///   \ref m_fuzzyCommitmentMinReliability. Each combined feature carries one codeword bit as a 2-PAM symbol: the bit is committed
///   by flipping the sign of the combination weights, which are stored as helper data.
///
/// At reproduction time, the probe is turned into one noisy symbol per code-offset feature and per combined feature, and the
/// Turbo decoder, which knows the noise pdf of each symbol from enrollment, recovers the most likely input message.
///
/// The user-specific randomness (Turbo Code interleavers, symbol permutation, secret message) is drawn independently for every
/// enrolled template.
///
/// ## Typical usage
/// \code
/// HybridStrengthenedTurboCodeFuzzyExtractor general;
/// general.loadGeneralData("generalConfig.txt");          // Turbo Code generators, thresholds, training set normalizers
///
/// // Enrollment
/// HybridStrengthenedTurboCodeFuzzyExtractor user(general);
/// user.setUserId("subject_001");
/// user.generate(enrollmentSamples);                     // one sample per element
/// if (user.validForReproduce()) {
///     user.saveUserSpecificData("subject_001.userSpecificHSTCFE");   // helper data
///     user.saveKey("subject_001.Key");                               // secret, for evaluation only
/// }
///
/// // Verification
/// HybridStrengthenedTurboCodeFuzzyExtractor stored(general);
/// stored.loadUserSpecificData("subject_001.userSpecificHSTCFE");
/// binary_polynomial key = stored.reproduce(probeSample);
/// \endcode
///
/// @note Copies of an object share the uniform normalizer built by \ref loadGeneralData (a \c std::shared_ptr), so copying
/// is cheap. The normalizer is freed automatically when the last object that refers to it is destroyed.
/// \author Enrique Argones Rúa
/// \date 2025
class HSTCFE_EXPORT HybridStrengthenedTurboCodeFuzzyExtractor
{
public:
    /**
     * @brief Default constructor. The object is unusable until \ref loadGeneralData has been called (or until it is
     * copy-constructed from an object on which it was).
     */
    HybridStrengthenedTurboCodeFuzzyExtractor();

    /**
     * @brief Drops this object's reference to the uniform normalizer built by \ref loadGeneralData.
     *
     * The normalizer is shared with the copies of this object and is freed when the last of them releases it or is destroyed,
     * so calling this method is optional. After it, this object cannot normalize samples until \ref loadGeneralData is called
     * again; its copies are not affected.
     * @exception std::logic_error if the normalizer was never initialized (or was already released by this object).
     */
    void releaseNormalizers();

    /**
     * @brief Maps the code-offset features of a sample to phases in \f$[0, 2\pi)\f$.
     *
     * Only the features listed in \ref m_codeOffsetFeaturesIndexes are kept. Each one is replaced by \f$2\pi\,\hat F_d(x_d)\f$,
     * where \f$\hat F_d\f$ is the empirical CDF of feature \f$d\f$ on the training set.
     * @param[in]  sample Sample of dimensionality \ref m_inputDimensionality
     * @param[out] normalizedSample Normalized sample of dimensionality \ref m_codeOffsetFeaturesCount
     * @exception std::logic_error if the dimensionality of \p sample is not \ref m_inputDimensionality.
     */
    virtual void normalizeCodeOffsetSample(const real_number_vector &sample, real_number_vector &normalizedSample);

    /**
     * @brief Maps every feature of a set of samples to a phase \f$2\pi\,\hat F_d(x_d) \in [0, 2\pi]\f$ using the empirical CDF
     * of the training set.
     * @param[in]  samples Samples of dimensionality \ref m_inputDimensionality, one per element.
     * @param[out] normalizedSamples Normalized samples (resized to match \p samples).
     * @exception std::logic_error if the normalizer is not initialized or a sample has the wrong dimensionality.
     */
    virtual void uniformNormalizationBatch(const real_number_vector_vector &samples, real_number_vector_vector &normalizedSamples);
    /**
     * @brief Maps every feature of one sample to a phase \f$2\pi\,\hat F_d(x_d) \in [0, 2\pi]\f$ using the empirical CDF of the
     * training set (same scaling as \ref uniformNormalizationBatch).
     * @param[in]  sample Sample of dimensionality \ref m_inputDimensionality.
     * @param[out] normalizedSample Normalized sample (resized to \ref m_inputDimensionality if needed).
     * @exception std::logic_error if the normalizer is not initialized or \p sample has the wrong dimensionality.
     */
    virtual void uniformNormalization(const real_number_vector &sample, real_number_vector &normalizedSample);

    /**
     * @brief Enrollment (\e Gen procedure of the fuzzy extractor). Builds the helper data and a fresh random key from the
     * enrollment samples of one subject.
     *
     * Steps:
     * -# Uniform normalization of the samples and per-feature statistics.
     * -# \ref bitLoading splits the features into code-offset and fuzzy-commitment features.
     * -# \ref findBestFuzzyCommitment builds the combined fuzzy-commitment features.
     * -# The total capacity \f$C\f$ (code-offset bits plus combined features) fixes the Turbo Code input length
     *    \f$N_I = \lceil C\,k/n \rceil\f$ for a code of rate \f$k/n\f$. Random interleavers are drawn, and the last bit of the
     *    first generators is punctured so that the codeword has exactly \f$C\f$ bits.
     * -# A random message (the key) is drawn and encoded (\ref generateRandomBinaryMessageAndCodeWord).
     * -# The symbols are randomly permuted (\ref randomlyPermute) and the codeword bits are committed to the features
     *    (\ref enrollCodeOffset, \ref enrollFuzzyCommitment).
     * -# The Turbo Code channel model (symbol indexes and conditional pdfs) is set for decoding.
     *
     * If the capacity is too small to hold a single message bit, \ref m_validForReproduce is set to false and no key is generated.
     * @param[in] enrollmentSamples Enrollment samples of one subject, one per element, each of dimensionality \ref m_inputDimensionality.
     * At least two samples are needed to estimate the per-feature standard deviations.
     * @pre \ref loadGeneralData has been called on this object or on the object it was copied from.
     */
    void generate(const real_number_vector_vector &enrollmentSamples);

    /**
     * @brief Calls \ref reproduce on each probe sample.
     * @param[in] probeSamples Probe samples, one per element.
     * @return \ref binary_polynomial_vector containing the decoded secret input message for each probe sample.
     */
    virtual binary_polynomial_vector reproduceBatch(const real_number_vector_vector &probeSamples);

    /**
     * @brief Key reproduction (\e Rep procedure of the fuzzy extractor).
     *
     * The code-offset features of the probe are normalized and subtracted from the stored commitment (modulo \f$2\pi\f$),
     * which yields noisy PSK symbols; the fuzzy-commitment features are projected with the stored (sign-committed) weights,
     * which yields noisy 2-PAM symbols. The Turbo Code then soft-decodes the symbols for up to
     * \ref m_turboCodeMaxDecodingIterations iterations.
     * @param[in] probeSample Probe sample of dimensionality \ref m_inputDimensionality.
     * @return The decoded message. It equals \ref turboCodeInputMessage() when the probe is close enough to the enrolled template.
     * @exception std::logic_error if the helper data is inconsistent.
     * @pre \ref validForReproduce() is true.
     */
    virtual binary_polynomial reproduce(const real_number_vector &probeSample);

    /**
     * @brief Loads the user-specific helper data written by \ref saveUserSpecificData and rebuilds the Turbo Code channel model.
     *
     * The object must already hold the general data (Turbo Code generators and uniform normalizer), typically because it was copied from
     * an object on which \ref loadGeneralData was called. The "File formats" page of the complete manual (\c doc/html) describes the file layout.
     * @param[in] fileName Name of the user-specific data file (\c .userSpecificHSTCFE by convention).
     * @exception std::ifstream::failure if the file cannot be opened.
     * @exception std::logic_error if a label or value is not the expected one.
     */
    void loadUserSpecificData(const std::string &fileName);

    /**
     * @brief Writes the user-specific helper data (feature split, commitment, combination weights, Turbo Code interleavers and
     * puncturing patterns, symbol permutation) to a text file.
     * @param[in] fileName Name of the output file (\c .userSpecificHSTCFE by convention).
     * @exception std::ofstream::failure if the file cannot be opened.
     * @warning The file also contains the secret message (\c TurboCodeInputMessage label), so that experiments can check
     * the reproduced keys. A deployed system must not store it.
     */
    void saveUserSpecificData(const std::string &fileName);

    /**
     * @brief Writes the general configuration file, in the format read by \ref loadGeneralData.
     *
     * The six label/value pairs are written in this order: \c TurboCodeConfigFile, \c TurboCodeMaxDecodingIterations,
     * \c TrainSetSamplesFilesFolder, \c CodeOffsetStdDevFactor, \c CodeOffsetStdDevOffset and \c FuzzyCommitmentMinReliability.
     * @param[in] fileName Name of the output file.
     * @exception std::ofstream::failure if the file cannot be opened.
     */
    virtual void saveGeneralData(const std::string &fileName);

    /**
     * @brief Loads the general configuration, parses the Turbo Code configuration file and builds the uniform normalizer from the
     * training set.
     *
     * The file is a sequence of label/value pairs, in this order:
     * \code
     * TurboCodeConfigFile            <path to a Turbo Code configuration file>
     * TurboCodeMaxDecodingIterations <unsigned int>
     * TrainSetSamplesFilesFolder     <folder with one .txt samples file per training subject>
     * CodeOffsetStdDevFactor         <real>
     * CodeOffsetStdDevOffset         <real>
     * FuzzyCommitmentMinReliability  <real>
     * \endcode
     * @param[in] fileName Name of the general configuration file.
     * @exception std::ofstream::failure if the file cannot be opened.
     * @exception std::logic_error if a label is not the expected one.
     */
    virtual void loadGeneralData(const std::string &fileName);


    /**
     * @brief Setter of \ref m_userId.
     * @param[in] newUserId Identifier of the enrolled subject.
     */
    void setUserId(const std::string &newUserId);

    /**
     * @brief Writes the secret message (key) as a single line of space-separated 0/1 values.
     * @param[in] fileName Name of the output file (\c .Key by convention).
     * @exception std::ofstream::failure if the file cannot be opened.
     */
    void saveKey(const std::string &fileName);
    /**
     * @brief Getter of \ref m_validForReproduce.
     * @return true if \ref generate produced (or \ref loadUserSpecificData loaded) usable helper data.
     */
    bool validForReproduce() const;

    /**
     * @brief Getter of \ref m_turboCodeInputMessage.
     * @return The secret message (key) bound to the template.
     */
    binary_polynomial turboCodeInputMessage() const;

    /**
     * @brief Getter of \ref m_turboCode.
     * @return Reference to the user-specific Turbo Code.
     */
    const TurboCode & turboCode() const;

    /**
     * @brief Getter of \ref m_userId.
     * @return Identifier of the enrolled subject.
     */
    std::string userId() const;

    /**
     * @brief Getter of \ref m_turboCodeMaxDecodingIterations.
     * @return Maximum number of Turbo decoding iterations.
     */
    uint turboCodeMaxDecodingIterations() const;
    /**
     * @brief Getter of \ref m_turboCodeConfigFileName.
     * @return Path of the Turbo Code configuration file.
     */
    std::string turboCodeConfigFileName() const;

    /// @brief Getter of \ref m_codeOffsetStdDevFactor. @return The factor \f$a\f$ of the bit-loading rule.
    real_number codeOffsetStdDevFactor() const;
    /// @brief Setter of \ref m_codeOffsetStdDevFactor. @param[in] newCodeOffsetStdDevFactor The factor \f$a\f$ of the bit-loading rule.
    void setCodeOffsetStdDevFactor(real_number newCodeOffsetStdDevFactor);

    /// @brief Getter of \ref m_fuzzyCommitmentMinReliability. @return Minimum reliability required for a combined feature.
    real_number fuzzyCommitmentMinReliability() const;
    /// @brief Setter of \ref m_fuzzyCommitmentMinReliability. @param[in] newFuzzyCommitmentMinReliability Minimum reliability required for a combined feature.
    void setFuzzyCommitmentMinReliability(real_number newFuzzyCommitmentMinReliability);

    /// @brief Getter of \ref m_codeOffsetStdDevOffset. @return The offset \f$b\f$ of the bit-loading rule.
    real_number codeOffsetStdDevOffset() const;
    /// @brief Setter of \ref m_codeOffsetStdDevOffset. @param[in] newCodeOffsetStdDevOffset The offset \f$b\f$ of the bit-loading rule.
    void setCodeOffsetStdDevOffset(real_number newCodeOffsetStdDevOffset);

    /// @brief Getter of \ref m_uniformNormalizer. @return Pointer to the uniform (empirical CDF) normalizer, one map per feature
    /// (null if it was never built). It stays valid while this object keeps its reference.
    std::vector<std::map<real_number, real_number> > *uniformNormalizer() const;

    /// @brief Getter of \ref m_inputDimensionality. @return Number of features of the input samples.
    uint inputDimensionality() const;

protected:
    /**
     * @brief Bit loading: decides how many codeword bits each feature carries in the code-offset part.
     *
     * For each feature with normalized standard deviation \f$\sigma_d\f$, the effective noise
     * \f$\tilde\sigma_d = a\,\sigma_d + b\f$ (with \f$a\f$ = \ref m_codeOffsetStdDevFactor and \f$b\f$ =
     * \ref m_codeOffsetStdDevOffset) is compared with rate-dependent thresholds:
     *
     * | Code rate | 0 bits if \f$\tilde\sigma >\f$ | 1 bit if \f$\tilde\sigma >\f$ | 2 bits if \f$\tilde\sigma >\f$ | 3 bits if \f$\tilde\sigma >\f$ | otherwise |
     * |:---------:|:----:|:----:|:----:|:----:|:----:|
     * | 1/3       | 0.99 | 0.69 | 0.47 | 0.32 | 4    |
     * | 1/4       | 1.04 | 0.74 | 0.51 | 0.35 | 4    |
     * | 1/5       | 1.06 | 0.76 | 0.53 | 0.37 | 4    |
     * | 1/6       | 1.08 | 0.78 | 0.54 | 0.38 | 4    |
     *
     * The capacity is then capped at \ref MAX_CAPACITY. Features with capacity 0 are assigned to the fuzzy commitment.
     *
     * Computes \ref m_codeOffsetFeaturesIndexes, \ref m_fuzzyCommitmentFeaturesIndexes, \ref m_codeOffsetFeaturesCapacities,
     * \ref m_codeOffsetTotalCapacity and \ref m_codeOffsetFeaturesCount.
     * @param[in] noiseStd Standard deviation of each normalized feature over the enrollment samples.
     * @exception std::logic_error if the rate of \ref m_turboCode is not 1/3, 1/4, 1/5, or 1/6.
     */
    void bitLoading(const real_number_vector &noiseStd);

    /**
     * @brief Computes \ref m_fuzzyCommitmentEnrollmentSamples by selecting the features in \ref m_fuzzyCommitmentFeaturesIndexes.
     * @param[in] enrollmentSamples Enrollment samples (not normalized).
     * @pre \ref m_fuzzyCommitmentFeaturesIndexes has been computed (\ref bitLoading).
     */
    void computeFuzzyCommitmentEnrollmentSamples(const real_number_vector_vector &enrollmentSamples);

    /**
     * @brief Computes \ref m_codeOffsetEnrollmentSamples by selecting the features in \ref m_codeOffsetFeaturesIndexes.
     * @param[in] normalizedEnrollmentSamples Enrollment samples after \ref uniformNormalizationBatch.
     * @pre \ref m_codeOffsetFeaturesIndexes has been computed (\ref bitLoading).
     */
    void computeCodeOffsetEnrollmentSamples(const real_number_vector_vector &normalizedEnrollmentSamples);

    /**
     * @brief Finds the largest number \f$K\f$ of combined fuzzy-commitment features whose reliabilities are all at least
     * \ref m_fuzzyCommitmentMinReliability.
     *
     * \f$K\f$ is found by bisection between 0 and the number of fuzzy-commitment features; for each candidate \f$K\f$ the
     * grouping is computed by \ref findOptimalFuzzyCommitment. Sets \ref m_combinedFeaturesCount,
     * \ref m_fuzzyCommitmentTotalCapacity (\f$= K\f$), the combined features (indexes, weights, means, standard deviations,
     * reliabilities) and \ref m_fuzzyExtractorCapacity.
     * @pre \ref m_fuzzyCommitmentEnrollmentSamplesMean, \ref m_fuzzyCommitmentEnrollmentSamplesStdDev and
     * \ref m_codeOffsetTotalCapacity are set.
     */
    void findBestFuzzyCommitment();

    /**
     * @brief Setter of \ref m_turboCodeInputMessageLength.
     * @param[in] newInputLength Length \f$N_I\f$ of the secret message.
     */
    void setTurboCodeInputMessageLength(unsigned int newInputLength);

    /**
     * @brief Draws one random interleaver (a uniform permutation of \f$\{0,\ldots,N_I-1\}\f$) per generator and sets all-ones
     * puncturing patterns on \ref m_turboCode.
     * @exception std::logic_error if \ref m_turboCodeInputMessageLength is 0.
     */
    void computeRandomizedInterleaversAndPuncturingPatterns();

    /**
     * @brief Randomly generates the attribute \ref m_turboCodeInputMessage and encodes it into \ref m_randomCodeWord using \ref TurboCode::encode
     * @return Reference to \ref m_turboCodeInputMessage.
     * @exception std::logic_error if \ref m_turboCodeInputMessageLength is 0.
     */
    binary_polynomial &generateRandomBinaryMessageAndCodeWord();

    /**
     * @brief Draws a random permutation of the symbols (code-offset features followed by combined features) and computes the
     * offset of the first codeword bit carried by each symbol.
     *
     * Sets \ref m_symbolsPermutation and \ref m_symbolsBitOffset. Code-offset symbol \f$i\f$ carries
     * \ref m_codeOffsetFeaturesCapacities[i] consecutive codeword bits; every combined feature carries one bit.
     */
    void randomlyPermute();

    /**
     * @brief Commits the codeword bits assigned to the code-offset features.
     *
     * For each code-offset feature, its bits are Gray-mapped to a PSK phase \f$\varphi\f$ and the commitment
     * \f$(\varphi + \mu) \bmod 2\pi\f$ is stored in \ref m_commitment. The PSK conditional pdfs (with a Gaussian phase noise of
     * the feature's enrollment standard deviation) are stored in \ref m_pdfs0 and \ref m_pdfs1, and \ref m_symbolIndexes is filled.
     */
    void enrollCodeOffset();

    /**
     * @brief Commits the codeword bits assigned to the combined fuzzy-commitment features.
     *
     * A bit 1 keeps the combination weights (mean \f$+1\f$); a bit 0 negates them (mean \f$-1\f$). The 2-PAM conditional pdfs
     * of each combined feature are stored in \ref m_pdfs0 and \ref m_pdfs1, and \ref m_symbolIndexes is completed.
     * @exception std::logic_error if \ref m_symbolIndexes does not have \ref m_fuzzyExtractorCapacity elements.
     * @pre \ref enrollCodeOffset has been called.
     */
    void enrollFuzzyCommitment();

    /**
     * @brief Setter of \ref m_inputDimensionality.
     * @param[in] newInputDimensionality Number of features of the input samples.
     */
    void setInputDimensionality(uint newInputDimensionality);

    /**
     * @brief Computes the projection of the fuzzy commitment part of the fuzzy extractor, i.e. the value of each combined feature.
     *
     * It requires the attributes \ref m_inputDimensionality, \ref m_fuzzyCommitmentFeaturesIndexes, \ref m_combinedFeaturesCount,
     * \ref m_combinedFeaturesIndexes, and \ref m_combinedFeaturesWeights to be previously computed.
     * @param[in]  sample Sample (not normalized) of dimensionality \ref m_inputDimensionality.
     * @param[out] projection Vector of \ref m_combinedFeaturesCount combined features.
     * @exception std::logic_error if the dimensionality of \p sample is not \ref m_inputDimensionality.
     */
    void projectFuzzyCommitment(const real_number_vector &sample, real_number_vector &projection);


    /**
     * @brief Parses the Turbo Code Configuration file previously set on \ref m_turboCodeConfigFileName into the attribute
     * \ref m_turboCode (see \ref ::parseTurboCodeConfigFile).
     */
    void parseTurboCodeConfigFile();

private:

    /**
     * @brief Builds the uniform normalizer from a pool of training samples.
     *
     * For each feature \f$d\f$ the training values are sorted, and the \f$i\f$-th smallest value (of \f$n\f$) is mapped to the
     * empirical CDF value \f$F_i = (2i + 1)/(2n)\f$ in \ref m_uniformNormalizer. If \ref m_inputDimensionality is 0, it is set
     * from the training samples.
     * @param[in] trainSetSamples Training samples, one per element.
     * @exception std::logic_error if the set is empty or the dimensionalities are inconsistent.
     */
    void setupNormalizers(const real_number_vector_vector &trainSetSamples);


    /**
     * @brief Setter of \ref m_codeOffsetFeaturesIndexes.
     * @param[in] newCodeOffsetFeatureIndexes Indexes (in the input sample) of the code-offset features.
     * @exception std::logic_error if an index is greater than or equal to \ref m_inputDimensionality.
     */
    void setCodeOffsetFeaturesIndexes(uint_vector newCodeOffsetFeatureIndexes);

    /**
     * @brief Setter of \ref m_fuzzyCommitmentFeaturesIndexes.
     * @param[in] newFuzzyCommitmentFeaturesIndexes Indexes (in the input sample) of the fuzzy-commitment features.
     * @exception std::logic_error if an index is greater than or equal to \ref m_inputDimensionality.
     */
    void setFuzzyCommitmentFeaturesIndexes(uint_vector newFuzzyCommitmentFeaturesIndexes);

    /**
     * @brief Setter of \ref m_combinedFeaturesIndexes.
     * @param[in] newCombinedFeaturesIndexes For each combined feature, the positions (within \ref m_fuzzyCommitmentFeaturesIndexes)
     * of the fuzzy-commitment features it combines.
     * @exception std::logic_error if a position is greater than or equal to the size of \ref m_fuzzyCommitmentFeaturesIndexes.
     * @pre \ref m_fuzzyCommitmentFeaturesIndexes has been set.
     */
    void setCombinedFeaturesIndexes(const uint_vector_vector &newCombinedFeaturesIndexes);

    /**
     * @brief Loads the training set samples and sets up the uniform normalizer.
     *
     * All \c .txt samples files in \ref m_trainSetSamplesFilesFolderName are read and pooled before calling \ref setupNormalizers.
     * @pre \ref m_trainSetSamplesFilesFolderName is set.
     */
    void loadTrainSetSamplesAndSetupNormalizers();


    /**
     * @brief Computes the optimal weights for combining multiple features, described by their mean and standard deviation
     * (uncorrelation is assumed), into a single combined feature for a Fuzzy-Commitment scheme.
     *
     * The weights are the dominant eigenvector of \f$\Sigma^{-1}\mu\mu^T\f$ (with \f$\Sigma\f$ the
     * diagonal covariance), which maximizes the ratio between the squared mean and the variance of the combination. The
     * eigenvector is scaled so that the combined mean is \f$+1\f$.
     * @param[in] featuresMeans \ref real_number_vector containing the mean values of the features to be combined.
     * @param[in] featuresStdDevs \ref real_number_vector containing the standard deviations of the features to be combined.
     * @param[out] featuresWeights \ref real_number_vector that will contain the weights of the different features.
     * @param[out] combinedFeatureMean \ref real_number that will contain the mean of the combined feature. This function is designed in such a way that this value should either be +1.0F or -1.0F
     * @param[out] combinedFeatureStdDev \ref real_number that will contain the standard deviation of the combined feature.
     * @exception std::runtime_error if Armadillo cannot invert the covariance matrix or compute the eigenvalue decomposition
     * (for instance, a feature with zero standard deviation). The matrices involved are also printed to \c std::cerr.
     */
    void findOptimalFuzzyCommitmentProjection(const real_number_vector &featuresMeans, const real_number_vector &featuresStdDevs,
                                              real_number_vector &featuresWeights, real_number &combinedFeatureMean, real_number &combinedFeatureStdDev);

    /**
     * @brief Computes an (almost) optimal set of features, obtained as linear combinations of disjoint subsets of features
     * described by their means and standard deviations (uncorrelation is assumed).
     *
     * Greedy procedure: repeatedly take the least reliable remaining feature and merge it into the currently least reliable
     * combined feature, re-optimizing the weights of that combination with \ref findOptimalFuzzyCommitmentProjection, until no
     * feature remains. The reliability of a (combined) feature is \f$|\mu|/\sigma\f$.
     * @param[in] featuresMeans \ref real_number_vector containing the mean values of the features to be combined.
     * @param[in] featuresStdDevs \ref real_number_vector containing the standard deviations of the features to be combined.
     * @param[out] featuresIndexesVectors \ref uint_vector_vector that will contain the indexes of the features combined into each combined feature. Initially it should be a vector of as many empty \ref uint_vector as desired combined features.
     * @param[out] featuresWeightsVectors \ref real_number_vector_vector that will contain the combination weight vectors. Initially it should be a vector of as many empty \ref real_number_vector as desired combined features.
     * @param[out] combinedFeaturesReliabilities \ref real_number_vector that will contain the combined features reliabilities.
     * @param[out] combinedFeaturesMeans \ref real_number_vector that will contain the combined features means.
     * @param[out] combinedFeaturesStdDevs \ref real_number_vector that will contain the combined features standard deviations.
     * @exception std::logic_error if the output vectors are empty, have different sizes, or are not initially empty.
     */
    void findOptimalFuzzyCommitment(real_number_vector featuresMeans, real_number_vector featuresStdDevs,
                                    uint_vector_vector &featuresIndexesVectors, real_number_vector_vector &featuresWeightsVectors,
                                    real_number_vector &combinedFeaturesReliabilities, real_number_vector &combinedFeaturesMeans, real_number_vector &combinedFeaturesStdDevs);


    /**
     * @brief Rebuilds the per-bit conditional pdfs (PSK for code-offset bits, 2-PAM for fuzzy-commitment bits) from the loaded
     * helper data and sets the channel model of \ref m_turboCode. Called by \ref loadUserSpecificData.
     */
    void setupTurboCodeNoiseConditionalPDFs();

protected:
    /**
     * @brief Uniform normalizer: for each feature, a map from sorted training value to its empirical CDF value.
     * A feature value \f$x\f$ is normalized to the CDF value of the smallest training value greater than \f$x\f$ (1.0 if none).
     */
    std::shared_ptr<std::vector<std::map<real_number,real_number>>> m_uniformNormalizer;

    /**
     * @brief Maximum number of Turbo decoding iterations used by \ref reproduce.
     */
    uint m_turboCodeMaxDecodingIterations;
    /**
     * @brief Number of features of the input samples.
     */
    uint m_inputDimensionality;
    /**
     * @brief Offset \f$b\f$ of the bit-loading rule \f$\tilde\sigma = a\,\sigma + b\f$ (see \ref bitLoading).
     */
    real_number m_codeOffsetStdDevOffset;
    /**
     * @brief Factor \f$a\f$ of the bit-loading rule \f$\tilde\sigma = a\,\sigma + b\f$ (see \ref bitLoading).
     */
    real_number m_codeOffsetStdDevFactor;
    /**
     * @brief Minimum reliability \f$|\mu|/\sigma\f$ required for every combined fuzzy-commitment feature.
     */
    real_number m_fuzzyCommitmentMinReliability;
    /**
     * @brief Length \f$N_I\f$ of the secret message (the Turbo Code input).
     */
    unsigned int m_turboCodeInputMessageLength;
    /**
     * @brief Indexes (in the input sample) of the features used in the code-offset part.
     */
    uint_vector m_codeOffsetFeaturesIndexes;
    /**
     * @brief Indexes (in the input sample) of the features available to the fuzzy-commitment part.
     */
    uint_vector m_fuzzyCommitmentFeaturesIndexes;
    /**
     * @brief Number \f$K\f$ of combined fuzzy-commitment features.
     */
    unsigned int m_combinedFeaturesCount;
    /**
     * @brief For each combined feature, the positions (within \ref m_fuzzyCommitmentFeaturesIndexes) of the features it combines.
     */
    uint_vector_vector m_combinedFeaturesIndexes;
    /**
     * @brief For each combined feature, the combination weights. After enrollment, their sign encodes the committed bit.
     */
    real_number_vector_vector m_combinedFeaturesWeights;
    /**
     * @brief Mean of each combined feature over the enrollment samples (\f$\pm 1\f$ by construction).
     */
    real_number_vector m_combinedFeaturesMeans;
    /**
     * @brief Standard deviation of each combined feature over the enrollment samples. Used as the 2-PAM noise level.
     */
    real_number_vector m_combinedFeaturesStdDevs;
    /**
     * @brief Reliability \f$|\mu|/\sigma\f$ of each combined feature.
     */
    real_number_vector m_combinedFeaturesReliabilities;
    /**
     * @brief Secret message (the key) bound to the template.
     */
    binary_polynomial m_turboCodeInputMessage;
    /**
     * @brief Turbo codeword of \ref m_turboCodeInputMessage, committed to the features during enrollment.
     */
    binary_polynomial m_randomCodeWord;
    /**
     * @brief Helper data of the code-offset part: one phase \f$(\varphi + \mu) \bmod 2\pi\f$ per code-offset feature.
     * Its size is \ref m_codeOffsetFeaturesCount + \ref m_fuzzyCommitmentTotalCapacity; the last entries are unused.
     */
    real_number_vector m_commitment;
    /**
     * @brief Normalized enrollment samples restricted to the code-offset features.
     */
    real_number_vector_vector m_codeOffsetEnrollmentSamples;
    /**
     * @brief Enrollment samples (not normalized) restricted to the fuzzy-commitment features.
     */
    real_number_vector_vector m_fuzzyCommitmentEnrollmentSamples;
    /**
     * @brief Mean of each fuzzy-commitment feature over the enrollment samples.
     */
    real_number_vector m_fuzzyCommitmentEnrollmentSamplesMean;
    /**
     * @brief Standard deviation of each fuzzy-commitment feature over the enrollment samples.
     */
    real_number_vector m_fuzzyCommitmentEnrollmentSamplesStdDev;
    /**
     * @brief Number of code-offset features.
     */
    real_number m_codeOffsetFeaturesCount;
    /**
     * @brief Mean of each normalized code-offset feature over the enrollment samples (\f$\mu\f$ in the commitment).
     */
    real_number_vector m_codeOffsetEnrollmentSamplesMean;
    /**
     * @brief Standard deviation of each normalized code-offset feature over the enrollment samples. Used as the PSK phase noise level.
     */
    real_number_vector m_codeOffsetEnrollmentSamplesStdDev;
    /**
     * @brief Number of codeword bits carried by each code-offset feature (output of \ref bitLoading).
     */
    uint_vector m_codeOffsetFeaturesCapacities;
    /**
     * @brief Total number of codeword bits carried by the code-offset part.
     */
    uint m_codeOffsetTotalCapacity;
    /**
     * @brief Total number of codeword bits carried by the fuzzy-commitment part (one per combined feature).
     */
    uint m_fuzzyCommitmentTotalCapacity;
    /**
     * @brief Total capacity \f$C\f$ = \ref m_codeOffsetTotalCapacity + \ref m_fuzzyCommitmentTotalCapacity, equal to the codeword length.
     */
    uint m_fuzzyExtractorCapacity;
    /**
     * @brief For each codeword bit, the index of the symbol (code-offset feature or combined feature) that carries it.
     */
    uint_vector m_symbolIndexes;
    /**
     * @brief Random permutation of the symbols; it decides which codeword bits each symbol carries.
     */
    uint_vector m_symbolsPermutation;
    /**
     * @brief For each symbol, the position in the codeword of the first bit it carries.
     */
    uint_vector m_symbolsBitOffset;

    /**
     * @brief User-specific Turbo Code (shared generators, user-specific interleavers, puncturing patterns and channel model).
     */
    TurboCode m_turboCode;

    /**
     * @brief For each codeword bit, the pdf of its symbol conditioned on the bit being 0.
     */
    GMM_vector m_pdfs0;
    /**
     * @brief For each codeword bit, the pdf of its symbol conditioned on the bit being 1.
     */
    GMM_vector m_pdfs1;

    /**
     * @brief Path of the Turbo Code configuration file (see \ref ::parseTurboCodeConfigFile).
     */
    std::string m_turboCodeConfigFileName;
    /**
     * @brief Folder containing the training set samples files used to build the uniform normalizer.
     */
    std::string m_trainSetSamplesFilesFolderName;

    /**
     * @brief Uniform distribution over \f$\{0, 1\}\f$ used to draw the secret message bits.
     */
    static std::uniform_int_distribution<int> m_uniform_binary_distribution;
    /**
     * @brief Mersenne Twister shared by all instances, seeded with \ref RAND_SEED.
     */
    static std::mt19937 m_twister;
    /**
     * @brief Mutex protecting \ref m_twister when several templates are enrolled in parallel.
     */
    static std::mutex m_rng_mutex;

    /**
     * @brief Identifier of the enrolled subject.
     */
    std::string m_userId;
    /**
     * @brief true if the object holds usable helper data, i.e. the capacity allowed a non-empty secret message.
     */
    bool m_validForReproduce;
};

/**
 * @brief Loads all the user-specific HSTCFE files found in a folder.
 *
 * For every regular file with extension \p userSpecificHSTEFEFilesExtension, a copy of \p generalHSTCFE is appended to
 * \p hstcfeVector and its user-specific data is loaded from the file.
 * @param[in]  userSpecificHSTEFEFilesPathName Folder containing the user-specific files.
 * @param[in]  userSpecificHSTEFEFilesExtension Extension of the files to load, including the leading dot (e.g. \c ".userSpecificHSTCFE").
 * @param[in]  generalHSTCFE Object holding the general data (on which \ref HybridStrengthenedTurboCodeFuzzyExtractor::loadGeneralData was called).
 * @param[out] userSpecificHSTEFEFilesNames Stems of the loaded files (the user identifiers).
 * @param[out] hstcfeVector Loaded fuzzy extractors (appended).
 * @exception std::ifstream::failure if the folder does not exist or is not a directory.
 * @exception std::logic_error if a file cannot be loaded.
 */
void readHybridStrengthenedTurboCodeFuzzyExtractorsFromPath(const std::string userSpecificHSTEFEFilesPathName,
                                                            const std::string userSpecificHSTEFEFilesExtension,
                                                            const HybridStrengthenedTurboCodeFuzzyExtractor &generalHSTCFE,
                                                            std::vector<std::string> &userSpecificHSTEFEFilesNames,
                                                            std::vector<HybridStrengthenedTurboCodeFuzzyExtractor> &hstcfeVector);

#endif // HYBRIDSTRENGTHENEDTURBOCODEFUZZYEXTRACTOR_H
