MHSTCFE library {#mhstcfe_page}
===============

[TOC]

MHSTCFE implements \ref MultimodalHybridStrengthenedTurboCodeFuzzyExtractor, the **multimodal** Hybrid Strengthened Turbo-Code
Fuzzy Extractor. It binds a single key to several biometric modalities of the same subject by **feature-level fusion**
\cite nagar2012multibiometric, \cite argonesrua2026exploiting.

\author Enrique Argones Rúa (COSIC, KU Leuven)
\date 2025

Feature-level fusion {#mhstcfe_fusion}
====================

Multibiometric cryptosystems can fuse modalities at different levels \cite nagar2012multibiometric. Fusing the keys or the
decisions of independent unimodal extractors forces each modality to carry a complete key on its own, so a weak modality
(face in uncontrolled conditions, keystroke dynamics) either needs a very short key or fails often. MHSTCFE fuses at feature
level instead: the samples of the \f$M\f$ modalities are concatenated into one feature vector, and the complete
HybridStrengthenedTurboCodeFuzzyExtractor pipeline runs once on the result:

- **normalization**: each modality keeps its own empirical-CDF normalizer. The class holds one unimodal
  \ref HybridStrengthenedTurboCodeFuzzyExtractor per modality (\ref MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::setHstcfes),
  and \ref MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::uniformNormalizationBatch sends every feature to the
  normalizer of the modality it comes from, writing each modality to its own block of the concatenated vector;
- **bit loading and combined features**: they work on the concatenated vector, so the reliable features of a strong modality
  (finger vein) are bound with the code-offset construction, and the unreliable features of all modalities can be combined
  with each other into fuzzy-commitment features;
- **Turbo Code**: one code protects one key over all modalities. Its generators are reloaded, unpunctured, from the
  configuration file of the first modality, and \ref MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::generate gives each
  multimodal subject its own interleavers and puncturing patterns for its total capacity
  \f$C = \sum_{m} C_m\f$, as in the unimodal case.

The total capacity, and therefore the key length, grows with the number of modalities, and the reliable features of one
modality compensate for the unreliable features of another \cite argonesrua2026exploiting.

Enrollment and reproduction {#mhstcfe_use}
===========================

1. Load the general data of every modality into a unimodal extractor and load the user-specific data of the subject in each
   modality (this provides the per-modality normalizers and dimensionalities).
2. Pass them to \ref MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::setHstcfes, set the multimodal bit-loading
   parameters (\c CodeOffsetStdDevFactor, \c CodeOffsetStdDevOffset, \c FuzzyCommitmentMinReliability) and call
   \ref MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::generate with the enrollment samples of each modality.
3. Save the helper data with \ref HybridStrengthenedTurboCodeFuzzyExtractor::saveUserSpecificData; the format is the same as
   for unimodal templates. \ref MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::saveGeneralData writes the multimodal
   general configuration (Turbo Code file, number of modalities, bit-loading parameters).
4. To verify, load the helper data, attach the per-modality extractors with
   \ref MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::setHstcfesNoFurtherUpdates, and call
   \ref MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::reproduce with one probe sample per modality.

\code{.cpp}
MultimodalHybridStrengthenedTurboCodeFuzzyExtractor mm;
mm.setCodeOffsetStdDevFactor(1.1);
mm.setCodeOffsetStdDevOffset(0.2);
mm.setFuzzyCommitmentMinReliability(3.0);
mm.setHstcfes(hstcfes);                           // one unimodal extractor per modality
mm.setUserId("face_012_keystroke_034");
mm.generate(enrollmentSamplesPerModality);
binary_polynomial key = mm.reproduce(probePerModality);
\endcode

The security notes of the unimodal extractor (fixed-seed random source, key stored in the helper data for evaluation,
helper data not encrypted with a second factor) apply here as well.

Building {#mhstcfe_build}
========

MHSTCFE depends on HSTCFE and on everything HSTCFE depends on. It is built by the top-level CMake project of the repository, or
on its own against the installed dependencies:

    cmake -S libraries/MHSTCFE -B build/MHSTCFE -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_PREFIX_PATH=PREFIX -DCMAKE_INSTALL_PREFIX=PREFIX
    cmake --build build/MHSTCFE -j
    cmake --install build/MHSTCFE

It is installed as \c libMHSTCFE (\c libMHSTCFE_Debug in Debug builds), with its headers in \c PREFIX/include/MHSTCFE.
