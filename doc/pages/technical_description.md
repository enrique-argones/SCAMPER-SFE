Technical description {#technical_description}
=====================

[TOC]

This page describes the system implemented by SCAMPER SFE as a whole. The detailed description of every component, with the
links to the classes and methods that implement each step, is in the module pages (\ref modules_page).

Biometric template protection {#td_btp}
=============================

Biometric references cannot be revoked like passwords: a stolen template permanently exposes the subject, and the same
template stored by several services allows cross-matching. Biometric template protection schemes must therefore make the
stored references *irreversible*, *unlinkable* and *renewable* while keeping the recognition performance \cite iso24745.
*Biometric cryptosystems* \cite rathgeb2011survey bind or derive a cryptographic key from the biometric data, so that the
recognition decision becomes the correct reproduction of a key, which integrates directly with cryptographic protocols. This
is what the SCAMPER project needs to integrate biometrics into self-sovereign identity platforms: the key reproduced from the
biometric sample unlocks the credentials of the user, and no biometric template is stored in the clear.

Fuzzy extractors {#td_fe}
================

A fuzzy extractor \cite dodis2008fuzzy consists of two procedures:

- \f$\mathrm{Gen}(\mathbf{x}) \to (\mathbf{b}, P)\f$ takes a biometric reading and outputs a uniformly random key
  \f$\mathbf{b}\f$ and public helper data \f$P\f$;
- \f$\mathrm{Rep}(\mathbf{x}', P) \to \mathbf{b}\f$ reproduces the key from a new reading \f$\mathbf{x}'\f$ that is close enough
  to \f$\mathbf{x}\f$.

The classical realizations bind the codeword of an error-correcting code to the biometric data: the *fuzzy commitment*
\cite juels1999fuzzy for binary templates, and the *code-offset* or helper-data schemes \cite linnartz2003shielding. Their
helper data leak information about the template \cite ignatenko2012leakage, and hard-decision codes operate far from the
capacity of the biometric channel.

The **Strengthened Fuzzy Extractor** (SFE) \cite argonesrua2019efficient introduces two changes. A model-based *uncoupling*
construction maps every continuous feature to a phase that is uniformly distributed over the population, so that the values
bound to it are independent of the biometric data; and a fixed second authentication factor protects the helper data, which
makes the protected templates of the same subject unlinkable and renewable. The SFE was then realized with soft-decoded Turbo
Codes and user-specific helper data \cite argonesrua2024strengthened, which adapt the code to the reliability of the features
of every subject, and extended into the *hybrid* SFE implemented in this software \cite argonesrua2026exploiting, which also
exploits the features that are too unreliable to carry a bit on their own.

The hybrid SFE {#td_hybrid}
===============

\dot
digraph system {
  rankdir=LR; nodesep=0.25; ranksep=0.35;
  node [shape=box, fontname=Helvetica, fontsize=10, style="rounded,filled", fillcolor="#eef3fb", color="#4a6fa5"];
  edge [fontname=Helvetica, fontsize=9, color="#555555", arrowsize=0.7];
  X  [label="Biometric\nfeatures x"];
  N  [label="Uncoupling\n(empirical CDF)"];
  BL [label="Bit loading"];
  CO [label="Code-offset\nkey binding\n(PSK)"];
  FC [label="Feature fusion +\nfuzzy commitment\n(2-PAM)"];
  TC [label="Turbo Code\n(user-specific\ninterleavers,\npuncturing)"];
  P  [label="Helper data P", shape=note, fillcolor="#fdf3e3", color="#b7791f"];
  K  [label="Key b", shape=ellipse, fillcolor="#e6f4ea", color="#2f855a"];
  X -> N -> BL;
  BL -> CO [label="reliable"];
  BL -> FC [label="unreliable"];
  K -> TC -> CO; TC -> FC;
  CO -> P; FC -> P;
}
\enddot

Enrollment (Gen) runs these steps for every subject (\ref hstcfe_gen):

1. **Uncoupling.** Every feature \f$x_d\f$ is mapped to the phase \f$\theta_d = 2\pi\hat F_d(x_d)\f$, where \f$\hat F_d\f$ is the
   empirical CDF of the feature on a training population \cite argonesrua2019efficient. The mean \f$\mu_d\f$ and the standard
   deviation \f$\sigma_d\f$ of the phase over the enrollment samples describe the subject and its intra-subject noise
   (\ref hstcfe_normalization).
2. **Bit loading.** The number of bits that a feature can carry depends on its noise. The effective noise
   \f$a\sigma_d + b\f$, where \f$a\f$ and \f$b\f$ are safety margins, is compared with thresholds derived for the PSK channel
   and the rate of the code: reliable features carry 1 to 3 bits, unreliable features none \cite argonesrua2026exploiting
   (\ref hstcfe_bitloading).
3. **Feature fusion.** The unreliable features are greedily grouped into linear combinations whose signal-to-noise ratio is
   maximal, until every combination reaches a minimum reliability. Each combination carries one bit
   (\ref hstcfe_fuzzycommitment).
4. **Turbo Code dimensioning.** The capacity \f$C\f$ of the template is the number of bits carried by all the symbols. A Turbo
   Code \cite berrou1993near of rate \f$1/r\f$ with key length \f$N_I = \lceil C/r \rceil\f$ is built with random interleavers
   for this subject, and punctured \cite hagenauer1988rcpc so that its codeword has exactly \f$C\f$ bits
   (\ref hstcfe_dimensioning).
5. **Key binding.** A random key \f$\mathbf{b}\f$ is encoded. The bits of the codeword are assigned to the symbols by a random
   permutation. Reliable features bind their bits with the code-offset construction: the bits are Gray-mapped
   \cite gray1953pulse to a PSK phase \f$\varphi_d\f$ \cite proakis2008digital, and the helper data stores
   \f$c_d = (\varphi_d + \mu_d) \bmod 2\pi\f$. Each combination of unreliable features commits its bit through the sign of its
   weights, so that a genuine probe reproduces the 2-PAM symbol \f$\pm 1\f$ (\ref hstcfe_commitment).

Reproduction (Rep) turns a probe into noisy symbols: \f$(c_d - \theta'_d) \bmod 2\pi\f$ for every reliable feature, which is
\f$\varphi_d\f$ plus a small noise term for a genuine probe, and the committed combination \f$y'_k\f$ for every group of
unreliable features. The soft Turbo decoder uses the conditional pdf of every symbol given every codeword bit, modelled with
Gaussian mixtures \cite reynolds2009gmm (with a wrapped-normal approximation for the phases \cite mardia2000directional), and
runs the iterative MAP algorithm \cite bahl1974optimal, \cite hagenauer1996iterative until its component decoders agree
(\ref hstcfe_rep, \ref turbocodes_decoding). The key is reproduced when the probe is close enough to the enrollment samples.

Because every feature is treated as a channel with its own noise level, reliable features contribute more to the decision than
unreliable ones, and no feature is discarded. Compared with constructions that discard the features with insufficient
capacity, the hybrid SFE obtains longer effective keys, in particular when many features are unreliable
\cite argonesrua2026exploiting. Turbo Codes combined with modulation constellations had already been used for template
protection in \cite maiorana2012turbo.

Multimodal fusion {#td_multimodal}
=================

\ref MultimodalHybridStrengthenedTurboCodeFuzzyExtractor fuses \f$M\f$ modalities at feature level
\cite nagar2012multibiometric. The uncoupled features of all modalities, each with the normalizer of its own modality, are
concatenated, and bit loading, feature fusion and Turbo coding run once on the concatenated vector. One key is bound to all
modalities, its length grows with the total capacity, and the unreliable features of a weak modality can be combined with
those of the others (\ref mhstcfe_fusion).

Evaluation methodology {#td_evaluation}
======================

The two programs implement the experimental protocol of \cite argonesrua2024strengthened and \cite argonesrua2026exploiting, which works as follows.

- every subject of the enrollment set is enrolled, in parallel (\ref threadpool_page);
- every probe is compared with every template; a comparison is accepted when the reproduced message equals the enrolled key;
- the false rejection rate (FRR) is measured on genuine comparisons and the false acceptance rate (FAR) on impostor
  comparisons, and the key length of every template measures its security level;
- for multimodal experiments, *chimeric* subjects are built by pairing subjects of different modalities at random
  (\ref multimodal_program_chimeric).

Implementation {#td_implementation}
==============

The software is written in C++17 and built with CMake. The Gaussian mixture models are provided by the minimal GMMLite library
(\ref gmmlite_page), the linear algebra (statistics, eigen-decomposition of the fusion problem) by Armadillo
\cite sanderson2016armadillo, and the parallel execution by the ThreadPool library \cite williams2019concurrency. The random
keys, interleavers and permutations are drawn from a Mersenne Twister \cite matsumoto1998mersenne.

Security considerations {#td_security}
=======================

\warning SCAMPER SFE is a research implementation, designed to evaluate the recognition performance and the key length of the
SFE. Before it is used in a deployed system:
- the Mersenne Twister with a fixed seed must be replaced by a cryptographically secure random source;
- the key must be removed from the helper data and the \c .Key files, which are written for evaluation purposes;
- the helper data must be protected with the second authentication factor of the SFE \cite argonesrua2019efficient, which is
  not implemented here (the experiments correspond to the scenario in which the second factor has been disclosed
  \cite argonesrua2024strengthened).
