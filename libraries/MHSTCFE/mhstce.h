/// \file mhstce.h
/// \brief Declaration of the \ref MultimodalHybridStrengthenedTurboCodeFuzzyExtractor class (MHSTCFE), the multimodal
/// (feature-level fusion) Strengthened Fuzzy Extractor.
///
/// The conditional pdfs used by the soft decoder are gmmlite::GMM objects of the GMMLite library (see the \c GMM alias in
/// turbo-codes_types.h).
/// \author Enrique Argones Rúa
/// \date 2025

#ifndef MHSTCE_H
#define MHSTCE_H

#include "MHSTCFE_global.h"
#include <HSTCFE/hybridstrengthenedturbocodefuzzyextractor.h>
#include <vector>

/// # Multimodal Hybrid Strengthened Turbo Code Fuzzy Extractor (MHSTCFE)
///
/// Binds a single key to several biometric modalities of the same (possibly chimeric) subject. The samples of all modalities
/// are concatenated into one feature vector, and the whole \ref HybridStrengthenedTurboCodeFuzzyExtractor pipeline (bit loading,
/// fuzzy-commitment grouping, Turbo coding) is run once on the concatenated vector, so reliable features of one modality can
/// compensate for unreliable features of another.
///
/// Each modality keeps its own uniform normalizer: the object holds one unimodal \ref HybridStrengthenedTurboCodeFuzzyExtractor per
/// modality (\ref m_hstcfes), and the normalization methods dispatch every feature to the normalizer of the modality it comes from.
///
/// ## Typical usage
/// \code
/// // hstcfes[m]: unimodal extractors (general data loaded) of the same subject, one per modality, same Turbo Code generators
/// MultimodalHybridStrengthenedTurboCodeFuzzyExtractor mm;
/// mm.setCodeOffsetStdDevFactor(1.1);
/// mm.setCodeOffsetStdDevOffset(0.2);
/// mm.setFuzzyCommitmentMinReliability(3.0);
/// mm.setHstcfes(hstcfes);
/// mm.setUserId("face_012_keystroke_034");
/// mm.generate(enrollmentSamplesPerModality);     // enrollmentSamplesPerModality[m] = samples of modality m
/// mm.saveUserSpecificData("face_012_keystroke_034.userSpecificHSTCFE");
///
/// binary_polynomial key = mm.reproduce(probePerModality);   // probePerModality[m] = one sample of modality m
/// \endcode
/// \author Enrique Argones Rúa
/// \date 2025
class MHSTCE_EXPORT MultimodalHybridStrengthenedTurboCodeFuzzyExtractor : public HybridStrengthenedTurboCodeFuzzyExtractor
{
public:
    /**
     * @brief Default constructor. The modality extractors must be set with \ref setHstcfes (enrollment) or
     * \ref setHstcfesNoFurtherUpdates (after \ref HybridStrengthenedTurboCodeFuzzyExtractor::loadUserSpecificData) before use.
     */
    MultimodalHybridStrengthenedTurboCodeFuzzyExtractor();
    /**
     * @brief Getter of \ref m_hstcfes.
     * @return Reference to the per-modality unimodal fuzzy extractors.
     */
    std::vector<HybridStrengthenedTurboCodeFuzzyExtractor> &hstcfes();
    /**
     * @brief Sets the per-modality fuzzy extractors and derives the multimodal configuration from them.
     *
     * Updates \ref m_modalitiesCount and the total input dimensionality (sum over modalities), copies the Turbo Code
     * configuration file name and the maximum number of decoding iterations from the first modality, and reloads the
     * unpunctured Turbo Code from that file. The first modality's Turbo Code object is not copied, because after
     * \ref HybridStrengthenedTurboCodeFuzzyExtractor::loadUserSpecificData it carries that user's interleavers and puncturing.
     * @exception std::logic_error (from ::parseTurboCodeConfigFile) if the Turbo Code configuration file cannot be read.
     * @param[in] newHstcfes One unimodal fuzzy extractor per modality (at least 2), all with the same Turbo Code generators.
     * @exception std::logic_error if fewer than 2 extractors are given or their Turbo Codes differ.
     */
    void setHstcfes(const std::vector<HybridStrengthenedTurboCodeFuzzyExtractor> &newHstcfes);
    /**
     * @brief Sets the per-modality fuzzy extractors without touching any other attribute.
     *
     * Used after \ref HybridStrengthenedTurboCodeFuzzyExtractor::loadUserSpecificData, when the multimodal helper data is already loaded and only the per-modality
     * uniform normalizers are needed.
     * @param[in] newHstcfes One unimodal fuzzy extractor per modality (at least 2), all with the same Turbo Code generators.
     * @exception std::logic_error if fewer than 2 extractors are given or their Turbo Codes differ.
     */
    void setHstcfesNoFurtherUpdates(const std::vector<HybridStrengthenedTurboCodeFuzzyExtractor> &newHstcfes);
    /**
     * @brief Number of modalities.
     * @return Size of \ref m_hstcfes.
     */
    unsigned int modalitiesCount();
    /**
     * @brief Multimodal enrollment.
     *
     * Each modality is normalized with its own uniform normalizer, the per-feature statistics of all modalities are
     * concatenated, and the enrollment then proceeds as in \ref HybridStrengthenedTurboCodeFuzzyExtractor::generate on the
     * concatenated feature vector.
     * @param[in] enrollmentSamplesVector One element per modality; element \f$m\f$ holds the enrollment samples of modality \f$m\f$.
     * @exception std::logic_error if the number of modalities does not match \ref m_modalitiesCount.
     * @pre \ref setHstcfes has been called, and the bit-loading and fuzzy-commitment parameters are set.
     */
    void generate(std::vector<real_number_vector_vector> &enrollmentSamplesVector);

    /**
     * @brief Writes the multimodal general configuration file.
     *
     * Layout (label/value pairs): \c TurboCodeConfigFile, \c TurboCodeMaxDecodingIterations, \c ModalitiesCount,
     * \c CodeOffsetStdDevFactor, \c CodeOffsetStdDevOffset, \c FuzzyCommitmentMinReliability.
     * @param[in] fileName Name of the output file.
     * @exception std::ofstream::failure if the file cannot be opened.
     */
    virtual void saveGeneralData(const std::string &fileName);

    /**
     * @brief Reads the multimodal general configuration file written by \ref saveGeneralData and parses the Turbo Code
     * configuration file. No normalizers are built: they come from the per-modality extractors.
     * @param[in] fileName Name of the multimodal general configuration file.
     * @exception std::ofstream::failure if the file cannot be opened.
     * @exception std::logic_error if a label is not the expected one.
     */
    virtual void loadGeneralData(const std::string &fileName);

    /**
     * @brief Multimodal key reproduction from separate per-modality samples.
     * @param multiModalProbeSample is here a vector of modality-specific samples, not a vector of different samples. They are
     * concatenated in modality order before reproduction.
     * @return The decoded message.
     * @exception std::logic_error if the helper data is inconsistent.
     */
    binary_polynomial reproduce(const real_number_vector_vector &multiModalProbeSample);

    // reproduceBatch() is inherited from HybridStrengthenedTurboCodeFuzzyExtractor; it calls the virtual reproduce() below.

    /**
     * @brief Multimodal key reproduction from an already concatenated sample.
     * @param probeSample is here a multimodal sample, which has to be already concatenated in the correct order (modality 0
     * features first).
     * @return The decoded message.
     * @exception std::logic_error if the helper data is inconsistent.
     */
    virtual binary_polynomial reproduce(const real_number_vector &probeSample);

    /**
     * @brief Uniform normalization of concatenated multimodal samples, each block of features with the normalizer of its modality.
     *
     * Every feature is mapped to a phase \f$2\pi\,\hat F_d(x_d) \in [0, 2\pi]\f$, as in the base class.
     * @param[in]  samples Concatenated multimodal samples.
     * @param[out] normalizedSamples Normalized samples. It must already have the same shape as \p samples (it is not resized).
     */
    virtual void uniformNormalizationBatch(const real_number_vector_vector &samples, real_number_vector_vector &normalizedSamples);
    /**
     * @brief Uniform normalization of one concatenated multimodal sample, each block of features with the normalizer of its
     * modality.
     *
     * Every feature is mapped to a phase \f$2\pi\,\hat F_d(x_d) \in [0, 2\pi]\f$, as in the base class.
     * @param[in]  sample Concatenated multimodal sample.
     * @param[out] normalizedSample Normalized sample. It must already have \ref HybridStrengthenedTurboCodeFuzzyExtractor::m_inputDimensionality
     * elements (it is not resized).
     */
    virtual void uniformNormalization(const real_number_vector &sample, real_number_vector &normalizedSample);

    /**
     * @brief Maps the code-offset features of a concatenated multimodal sample to phases in \f$[0, 2\pi)\f$, using for each
     * feature the uniform normalizer of the modality it belongs to.
     * @param[in]  sample Concatenated multimodal sample.
     * @param[out] normalizedSample Normalized code-offset features (resized to \ref HybridStrengthenedTurboCodeFuzzyExtractor::m_codeOffsetFeaturesCount).
     */
    virtual void normalizeCodeOffsetSample(const real_number_vector &sample, real_number_vector &normalizedSample);

private:
    /**
     * @brief Selects the fuzzy-commitment features from the concatenated (not normalized) enrollment statistics.
     * @param[in] enrollmentSamplesMeansVector Per-feature means of the concatenated enrollment samples.
     * @param[in] enrollmentSamplesStdDevsVector Per-feature standard deviations of the concatenated enrollment samples.
     */
    void computeFuzzyCommitmentEnrollmentSamplesStatistics(const real_number_vector &enrollmentSamplesMeansVector, const real_number_vector &enrollmentSamplesStdDevsVector);

    /**
     * @brief Selects the code-offset features from the concatenated normalized enrollment statistics.
     * @param[in] normalizedEnrollmentSamplesMeansVector Per-feature means of the concatenated normalized enrollment samples.
     * @param[in] normalizedEnrollmentSamplesStdDevsVector Per-feature standard deviations of the concatenated normalized enrollment samples.
     */
    void computeCodeOffsetEnrollmentSamplesStatistics(const real_number_vector &normalizedEnrollmentSamplesMeansVector, const real_number_vector &normalizedEnrollmentSamplesStdDevsVector);

    /**
     * @brief Per-modality unimodal fuzzy extractors. Only their normalizers, dimensionalities and Turbo Code are used.
     */
    std::vector<HybridStrengthenedTurboCodeFuzzyExtractor> m_hstcfes;
    /**
     * @brief Number of modalities.
     */
    unsigned int m_modalitiesCount;

};

#endif // MHSTCE_H
