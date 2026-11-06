File formats {#fileformats}
============

[TOC]

All files are plain text. Values are separated by whitespace and line breaks only help readability. Labels must appear exactly
as shown and in the order shown.

Samples files {#ff_samples}
=============

There is one file per subject, with extension \c .txt. The file stem is the subject identifier: for example,
\c subject_001.txt holds the samples of \c subject_001. A folder of such files is read by \ref ::readSamplesFilesFromPath, which
calls \ref parseSamplesFile for each file.

\verbatim
SAMPLES_COUNT SAMPLE_SIZE
x[0][0]               ... x[0][SAMPLE_SIZE-1]
...
x[SAMPLES_COUNT-1][0] ... x[SAMPLES_COUNT-1][SAMPLE_SIZE-1]
\endverbatim

The same format is used for enrollment, probe and training-set folders. For the unimodal program, the enrollment and probe
files of a subject must have the same stem, because genuine comparisons are detected by comparing stems.

Turbo Code configuration file {#ff_turbocode}
=============================

Parsed by \ref ::parseTurboCodeConfigFile. It contains the number of generators, then two polynomials per generator, one
puncturing pattern per generator and one interleaver per generator. The reference documentation of that function describes it
in detail. The folder \c libraries/TurboCodes/configFiles contains the best codes found for rates 1/3 to 1/6
(\c best-found-turbo-code_rate_1_R.txt). They are also installed to \c PREFIX/share/TurboCodes/configFiles.

The fuzzy extractors only use the generators from this file. The interleavers and puncturing patterns are replaced with
user-specific ones whose length depends on the capacity of each template.

HSTCFE general configuration file {#ff_general}
=================================

An example is installed to \c PREFIX/share/HSTCFE/configFiles/exampleGeneralConfigFile.txt. Written by \ref HybridStrengthenedTurboCodeFuzzyExtractor::saveGeneralData, read by
\ref HybridStrengthenedTurboCodeFuzzyExtractor::loadGeneralData, and passed to the unimodal program with \c -G.

\verbatim
TurboCodeConfigFile            /path/to/best-found-turbo-code_rate_1_3.txt
TurboCodeMaxDecodingIterations 16
TrainSetSamplesFilesFolder     /path/to/trainSet
CodeOffsetStdDevFactor         1.13
CodeOffsetStdDevOffset         0.23
FuzzyCommitmentMinReliability  2.0
\endverbatim

| Label | Meaning |
|-------|---------|
| \c TurboCodeConfigFile | Turbo Code configuration file (\ref ff_turbocode). Its rate must be 1/3, 1/4, 1/5 or 1/6. |
| \c TurboCodeMaxDecodingIterations | Maximum number of Turbo decoding iterations during reproduction. |
| \c TrainSetSamplesFilesFolder | Folder of samples files used to estimate the empirical CDF of each feature. |
| \c CodeOffsetStdDevFactor | Factor \f$\alpha\f$ of the bit-loading rule \f$\tilde\sigma = \alpha\sigma + \beta\f$. A larger value gives fewer code-offset bits. |
| \c CodeOffsetStdDevOffset | Offset \f$\beta\f$ of the bit-loading rule. |
| \c FuzzyCommitmentMinReliability | Minimum \f$|\mu|/\sigma\f$ of every combined fuzzy-commitment feature. A larger value gives fewer, more reliable combined features. |

Multimodal general configuration file {#ff_mgeneral}
=====================================

The multimodal program writes this file as \c generalConfigFile.generalHSTCFE in the multimodal output folder
(\ref MultimodalHybridStrengthenedTurboCodeFuzzyExtractor::saveGeneralData), and reads it in test mode.

\verbatim
TurboCodeConfigFile            <path>
TurboCodeMaxDecodingIterations <unsigned int>
ModalitiesCount                <unsigned int>
CodeOffsetStdDevFactor         <real>
CodeOffsetStdDevOffset         <real>
FuzzyCommitmentMinReliability  <real>
\endverbatim

Multimodal experiment configuration file {#ff_mexperiment}
========================================

Passed to the multimodal program with \c -c. The description of
MultimodalHybridStrengthenedTurboCodeFuzzyExtractor/main.cpp gives the full layout.

The example \c applications/MultimodalHybridStrengthenedTurboCodeFuzzyExtractor/configFiles/exampleConfigFile.txt (installed to
\c PREFIX/share/MultimodalHybridStrengthenedTurboCodeFuzzyExtractor/configFiles) has this layout; its paths point to the
original experiment folders and must be adapted.

User-specific helper data (.userSpecificHSTCFE) {#ff_userspecific}
===============================================

Written by \ref HybridStrengthenedTurboCodeFuzzyExtractor::saveUserSpecificData and read by
\ref HybridStrengthenedTurboCodeFuzzyExtractor::loadUserSpecificData. Unimodal and multimodal templates use the same format.
A vector is written as its length followed by its elements. A vector of vectors is written as its length followed by each inner
vector (length, then elements).

| # | Label | Value |
|---|-------|-------|
| 1 | \c UserId | Subject identifier |
| 2 | \c InputDimensionality | Number of features \f$D\f$ |
| 3 | \c CodeOffsetStdDevOffset | \f$\beta\f$ |
| 4 | \c CodeOffsetStdDevFactor | \f$\alpha\f$ |
| 5 | \c FuzzyCommitmentMinReliability | Reliability threshold |
| 6 | \c CodeOffsetFeaturesCount | Number of code-offset features |
| 7 | \c CodeOffsetFeaturesIndexes | Vector: indexes of the code-offset features |
| 8 | \c CodeOffsetTotalCapacity | Number of code-offset bits |
| 9 | \c CodeOffsetFeaturesCapacities | Vector: bits per code-offset feature |
| 10 | \c CodeOffsetEnrollmentSamplesStdDev | Vector: PSK noise level of each code-offset feature |
| 11 | \c Commitment | Vector: code-offset commitment phases \f$c_d\f$ |
| 12 | \c FuzzyCommitmentTotalCapacity | Number of combined features \f$K\f$ |
| 13 | \c FuzzyCommitmentFeaturesIndexes | Vector: indexes of the fuzzy-commitment features |
| 14 | \c CombinedFeaturesCount | \f$K\f$ |
| 15 | \c CombinedFeaturesIndexes | Vector of vectors: members of each combined feature |
| 16 | \c CombinedFeaturesWeights | Vector of vectors: committed (signed) weights |
| 17 | \c CombinedFeaturesStdDevs | Vector: 2-PAM noise level of each combined feature |
| 18 | \c FuzzyExtractorCapacity | Codeword length \f$C\f$ |
| 19 | \c ValidForReproduce | \c true or \c false. The file ends here if \c false. |
| 20 | \c TurboCodePuncturingPatterns | Vector of 0/1 vectors |
| 21 | \c SymbolsPermutation | Vector: symbol permutation |
| 22 | \c SymbolIndexes | Vector: symbol carrying each codeword bit |
| 23 | \c TurboCodeInterleavers | Vector of vectors: one permutation per generator |
| 24 | \c TurboCodeInputMessageLength | Key length \f$N_I\f$ |
| 25 | \c TurboCodeInputMessage | Vector of 0/1: the key |
| 26 | \c TurboCodePuncturingPatterns | Vector of 0/1 vectors (repeated) |

\warning Entry 25 stores the key in the helper data. This is useful for evaluation, but it must be removed before the format is
used in a deployed system.

Key file (.Key) {#ff_key}
===============

Written by \ref HybridStrengthenedTurboCodeFuzzyExtractor::saveKey. It contains the \f$N_I\f$ key bits as 0/1 values separated by spaces,
on one line.

Evaluation outputs {#ff_results}
==================

In test mode, both programs write to the recovered-messages folder:

- \c authenticationResults.txt: one line per comparison of a probe sample with a template.
  \verbatim
  PROBE_ID TEMPLATE_ID AUTHENTICATED GENUINE
  \endverbatim
  \c AUTHENTICATED is 1 if the decoded message equals the key, and \c GENUINE is 1 for same-subject comparisons. The false
  rejection rate is the fraction of \c GENUINE=1 lines with \c AUTHENTICATED=0. The false acceptance rate is the fraction of
  \c GENUINE=0 lines with \c AUTHENTICATED=1.
- \c probe_PROBE_hstcfe_TEMPLATE.recovered (unimodal) or one \c .recovered file per comparison (multimodal): one decoded message
  per line. These files are not written when the program runs with \c -R.
