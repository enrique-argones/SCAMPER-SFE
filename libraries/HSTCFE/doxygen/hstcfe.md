HSTCFE library {#hstcfe_page}
==============

[TOC]

HSTCFE implements \ref HybridStrengthenedTurboCodeFuzzyExtractor, the unimodal **Hybrid Strengthened Turbo-Code Fuzzy
Extractor**: the core of the SCAMPER SFE software. It binds a random key to the biometric template of one subject, using the
soft-decoded Turbo Codes and the modulations of the TurboCodes library, and reproduces the key from a fresh sample of the same
subject \cite argonesrua2026exploiting.

\author Enrique Argones Rúa (COSIC, KU Leuven)
\date 2025

Background {#hstcfe_background}
==========

A **fuzzy extractor** \cite dodis2008fuzzy is a pair of procedures. *Gen* takes a noisy source (here, the enrollment samples of
a subject) and outputs a uniformly random key and public helper data. *Rep* takes the helper data and a new reading of the
source, and outputs the same key when the reading is close enough to the enrollment one. Fuzzy extractors are one of the
families of *biometric cryptosystems* \cite rathgeb2011survey used for biometric template protection
\cite iso24745. Their classical realizations, the *fuzzy commitment* \cite juels1999fuzzy and the *code-offset* or
*helper-data* constructions \cite linnartz2003shielding, leak information about the template through the helper data
\cite ignatenko2012leakage.

The **Strengthened Fuzzy Extractor** (SFE) \cite argonesrua2019efficient addresses this leakage. A model-based *uncoupling*
construction turns the continuous features into phases that are independent of the stored values, and a fixed second
authentication factor (a secret that only the user holds) protects the helper data, which makes the protected templates of the
same subject unlinkable and renewable. The SFE was later built on soft-decoded Turbo Codes with user-specific helper data
\cite argonesrua2024strengthened, and extended into the *hybrid* construction implemented here, which also exploits the
features too unreliable to carry a bit on their own \cite argonesrua2026exploiting.

\note This library implements the biometric part of the SFE: normalization, bit loading, the two key-binding constructions
and the Turbo Code. It writes the helper data in plain files, which corresponds to the evaluation scenario in which the second
factor has been disclosed \cite argonesrua2024strengthened. Encrypting the helper data with the second factor is left to the
application.

Overview {#hstcfe_overview}
========

- **Gen** (\ref HybridStrengthenedTurboCodeFuzzyExtractor::generate) takes the enrollment samples of a subject and returns a
  random key \f$\mathbf{b}\f$ and the helper data \f$P\f$.
- **Rep** (\ref HybridStrengthenedTurboCodeFuzzyExtractor::reproduce) takes a probe sample and \f$P\f$, and returns
  \f$\mathbf{b}\f$ if the probe comes from the same subject.

The key is the input message of a Turbo Code. Its codeword is spread over the biometric features: every feature, or
combination of features, acts as a noisy communication channel with its own modulation and noise level, and the soft Turbo
decoder corrects the errors caused by intra-subject variability.

\dot
digraph pipeline {
  rankdir=TB; nodesep=0.3; ranksep=0.35;
  node [shape=box, fontname=Helvetica, fontsize=10, style="rounded,filled", fillcolor="#eef3fb", color="#4a6fa5"];
  edge [fontname=Helvetica, fontsize=9, color="#555555", arrowsize=0.7];
  subgraph cluster_gen { label="Gen (enrollment)"; fontname=Helvetica; fontsize=11; color="#bbbbbb";
    S  [label="Enrollment\nsamples"];
    N  [label="Empirical-CDF\nnormalization"];
    BL [label="Bit loading\n(split features)"];
    FC [label="Combine unreliable\nfeatures"];
    K  [label="Random key b\nTurbo encode"];
    CO [label="Code-offset\n(PSK)"];
    CM [label="Fuzzy commitment\n(2-PAM)"];
    S -> N -> BL; BL -> FC [label="capacity 0"]; BL -> CO [label="capacity >= 1"];
    FC -> CM; K -> CO; K -> CM;
  }
  P [label="Helper data P", shape=note, fillcolor="#fdf3e3", color="#b7791f"];
  CO -> P; CM -> P;
  subgraph cluster_rep { label="Rep (reproduction)"; fontname=Helvetica; fontsize=11; color="#bbbbbb";
    Q  [label="Probe\nsample"];
    SY [label="Noisy symbols\n(PSK phases, 2-PAM values)"];
    D  [label="Soft Turbo\ndecoding"];
    Q -> SY -> D;
  }
  P -> SY;
  B [label="Key b'", shape=ellipse, fillcolor="#e6f4ea", color="#2f855a"];
  D -> B;
}
\enddot

Enrollment (Gen) {#hstcfe_gen}
================

Normalization (uncoupling) {#hstcfe_normalization}
--------------------------

Each feature \f$d\f$ is mapped to a phase with the empirical cumulative distribution function \f$\hat F_d\f$ estimated on a
training set (\ref HybridStrengthenedTurboCodeFuzzyExtractor::uniformNormalizationBatch):
\f[
\theta_d = 2\pi\,\hat F_d(x_d) \in [0, 2\pi].
\f]
Over the population, \f$\theta_d\f$ is uniform on \f$[0, 2\pi]\f$, so a phase bound to it is uniform too
\cite argonesrua2019efficient. The normalizers are built once per modality by
\ref HybridStrengthenedTurboCodeFuzzyExtractor::loadGeneralData from all the samples files of the training folder, and are
shared by all the copies of the extractor. The mean \f$\mu_d\f$ and standard deviation \f$\sigma_d\f$ of \f$\theta_d\f$ over the
enrollment samples of the subject are then computed.

Bit loading {#hstcfe_bitloading}
-----------

\ref HybridStrengthenedTurboCodeFuzzyExtractor::bitLoading assigns to each feature a capacity of 0 to \ref MAX_CAPACITY bits.
It compares the effective noise \f$\tilde\sigma_d = \alpha\,\sigma_d + \beta\f$ with thresholds that depend on the Turbo Code rate (1/3 to
1/6); the thresholds come from the capacity of a PSK channel with Gaussian phase noise \cite argonesrua2026exploiting. The
parameters \f$\alpha\f$ (\c CodeOffsetStdDevFactor) and \f$\beta\f$ (\c CodeOffsetStdDevOffset) are safety margins that account for
deviations from the Gaussian model. More stable features carry more bits:

- features with capacity \f$\geq 1\f$ go to the **code-offset** part;
- features with capacity 0 go to the **fuzzy-commitment** part.

Combined fuzzy-commitment features {#hstcfe_fuzzycommitment}
----------------------------------

Unreliable features cannot carry a bit on their own, but they still carry information about the subject
\cite argonesrua2026exploiting. \ref HybridStrengthenedTurboCodeFuzzyExtractor::findBestFuzzyCommitment groups them into
\f$K\f$ disjoint linear combinations
\f[
y_k = \sum_{j \in G_k} w_{k,j}\, x_j ,
\f]
computed on the raw (not normalized) features. The weights of each group maximize the signal-to-noise ratio
\f$\mu^2/\sigma^2\f$ of the combination under an independence assumption
(\ref HybridStrengthenedTurboCodeFuzzyExtractor::findOptimalFuzzyCommitmentProjection, which solves an eigenvalue problem with
Armadillo \cite sanderson2016armadillo), and are scaled so that \f$\mathrm{E}[y_k] = +1\f$. The grouping is greedy
(\ref HybridStrengthenedTurboCodeFuzzyExtractor::findOptimalFuzzyCommitment). \f$K\f$ is the largest value, found by bisection,
for which every combined feature has a reliability \f$|\mu_k|/\sigma_k \geq\f$ \c FuzzyCommitmentMinReliability.

Turbo Code dimensioning {#hstcfe_dimensioning}
-----------------------

The capacity of the template is
\f[
C = \sum_{d \in \textrm{code-offset}} \textrm{capacity}_d + K .
\f]
For a code of rate \f$1/r\f$, the key length is \f$N_I = \lceil C / r \rceil\f$. The configuration file provides the generators
of the Turbo Code. The interleavers are new random permutations for each user
(\ref HybridStrengthenedTurboCodeFuzzyExtractor::computeRandomizedInterleaversAndPuncturingPatterns). The last redundancy bit of
the first \f$(r - C \bmod r) \bmod r\f$ generators is punctured \cite hagenauer1988rcpc, so the codeword has exactly \f$C\f$ bits.
If \f$N_I = 0\f$, the template is marked as not valid (\ref HybridStrengthenedTurboCodeFuzzyExtractor::validForReproduce).

Key generation and binding {#hstcfe_commitment}
--------------------------

A random key \f$\mathbf{b}\f$ is drawn and encoded into \f$\mathbf{o}\f$
(\ref HybridStrengthenedTurboCodeFuzzyExtractor::generateRandomBinaryMessageAndCodeWord). The symbols (code-offset features and
combined features) are randomly permuted to decide which codeword bits each one carries
(\ref HybridStrengthenedTurboCodeFuzzyExtractor::randomlyPermute). Then:

- **Code-offset key binding** (\ref HybridStrengthenedTurboCodeFuzzyExtractor::enrollCodeOffset): the
  \f$\textrm{capacity}_d\f$ bits of feature \f$d\f$ are Gray-mapped to a \ref PSK phase \f$\varphi_d\f$, and the helper data
  stores \f$c_d = (\varphi_d + \mu_d) \bmod 2\pi\f$ \cite argonesrua2024strengthened.
- **Fuzzy-commitment key generation** (\ref HybridStrengthenedTurboCodeFuzzyExtractor::enrollFuzzyCommitment): the bit of
  combined feature \f$k\f$ is committed through the sign of its weights. A bit 1 keeps \f$\mathbf{w}_k\f$ (so
  \f$\mathrm{E}[y_k] = +1\f$) and a bit 0 negates it (so \f$\mathrm{E}[y_k] = -1\f$): the genuine probe reproduces a \ref PAM2
  symbol.

For every codeword bit, the conditional pdfs \f$f(\cdot \mid o_i = 0)\f$ and \f$f(\cdot \mid o_i = 1)\f$ of the symbol carrying
it are stored in the Turbo Code (\ref HybridStrengthenedTurboCodeFuzzyExtractor::setupTurboCodeNoiseConditionalPDFs). They are
modeled as Gaussian phase noise with standard deviation \f$\sigma_d\f$ (PSK), or as Gaussian noise with standard deviation
\f$\sigma_k\f$ (2-PAM).

The helper data (\ref HybridStrengthenedTurboCodeFuzzyExtractor::saveUserSpecificData) contains the feature split, the
capacities, the noise levels, \f$\mathbf{c}\f$, the committed weights, the interleavers, the puncturing patterns and the symbol
permutation.

The helper data can be stored in a file or kept in memory:

| Destination | Write | Read |
|-------------|-------|------|
| File (\c .userSpecificHSTCFE) | \ref HybridStrengthenedTurboCodeFuzzyExtractor::saveUserSpecificData | \ref HybridStrengthenedTurboCodeFuzzyExtractor::loadUserSpecificData |
| Memory buffer (\c std::vector<uint8_t>) | \ref HybridStrengthenedTurboCodeFuzzyExtractor::getUserSpecificData | \ref HybridStrengthenedTurboCodeFuzzyExtractor::loadUserSpecificDataFromBlob |
| Any stream | \ref HybridStrengthenedTurboCodeFuzzyExtractor::writeUserSpecificData | \ref HybridStrengthenedTurboCodeFuzzyExtractor::readUserSpecificData |

All of them use the same text format, so a buffer holds exactly the bytes of the corresponding file. The in-memory functions
are used by the Rust binding (\c rust-binding), which returns templates as byte buffers.

Reproduction (Rep) {#hstcfe_rep}
==================

For a probe sample \f$\mathbf{x}'\f$ (\ref HybridStrengthenedTurboCodeFuzzyExtractor::reproduce):

1. Each code-offset feature gives the received PSK symbol \f$(c_d - \theta'_d) \bmod 2\pi = \varphi_d + (\mu_d - \theta'_d)\f$:
   the bound phase plus a noise term that is small for a genuine probe.
2. Each combined feature gives the received 2-PAM symbol \f$y'_k = \mathbf{w}_k^{\pm\,T} \mathbf{x}'_{G_k}\f$.
3. \ref TurboCode::decode runs the iterative MAP decoder \cite berrou1993near, \cite bahl1974optimal (up to
   \c TurboCodeMaxDecodingIterations iterations) and uses the per-bit conditional pdfs to compute the channel likelihoods.

The decoded message equals the enrolled key when the probe is close enough to the enrollment samples.
\ref HybridStrengthenedTurboCodeFuzzyExtractor::reproduceBatch decodes several probe samples.

Configuration {#hstcfe_config}
=============

\ref HybridStrengthenedTurboCodeFuzzyExtractor::loadGeneralData reads the general configuration file (an example is installed
to \c PREFIX/share/HSTCFE/configFiles/exampleGeneralConfigFile.txt):

\verbatim
TurboCodeConfigFile            /path/to/best-found-turbo-code_rate_1_3.txt
TurboCodeMaxDecodingIterations 16
TrainSetSamplesFilesFolder     /path/to/trainSet
CodeOffsetStdDevFactor         1.13
CodeOffsetStdDevOffset         0.23
FuzzyCommitmentMinReliability  2.0
\endverbatim

A larger \c CodeOffsetStdDevFactor or \c CodeOffsetStdDevOffset gives fewer code-offset bits (lower false rejection, shorter
key). A larger \c FuzzyCommitmentMinReliability gives fewer, more reliable combined features.

Security notes {#hstcfe_security}
==============

\warning This is a research implementation. Before using it outside experiments, take into account that:
- the random source is a Mersenne Twister \cite matsumoto1998mersenne with a fixed seed (\ref RAND_SEED), which is not
  cryptographically secure. The sequence it produces is always the same, but parallel enrollment tasks draw from it in
  scheduling order, so the keys, and therefore the evaluation results, can vary slightly between runs;
- the user-specific file also stores the key (\c TurboCodeInputMessage), and the \c .Key file is written next to it, so that
  experiments can measure key-recovery rates;
- the helper data is not encrypted with a second factor (see the note in \ref hstcfe_background).

Building {#hstcfe_build}
========

HSTCFE needs a C++17 compiler with \c std::filesystem, GMMLite, TurboCodes, ThreadPool, Armadillo and POSIX threads. It is
built by the top-level CMake project of the repository, or on its own against the installed dependencies:

    cmake -S libraries/HSTCFE -B build/HSTCFE -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_PREFIX_PATH=PREFIX -DCMAKE_INSTALL_PREFIX=PREFIX
    cmake --build build/HSTCFE -j
    cmake --install build/HSTCFE

It is installed as \c libHSTCFE (\c libHSTCFE_Debug in Debug builds), with its headers in \c PREFIX/include/HSTCFE.
