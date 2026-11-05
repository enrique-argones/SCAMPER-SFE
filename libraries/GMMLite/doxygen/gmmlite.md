GMMLite library {#gmmlite_page}
===============

[TOC]

GMMLite is a minimal library of diagonal-covariance **Gaussian Mixture Models** (GMMs) \cite reynolds2009gmm for the
Strengthened Fuzzy Extractor (SFE) software. It provides the class gmmlite::GMM, which contains only what the SFE libraries
need: building fixed models, evaluating their log-likelihood and drawing samples from them. It depends only on the C++11
standard library.

\author Enrique Argones Rúa (COSIC, KU Leuven)
\date 2026

Role in the SFE software {#gmmlite_role}
========================

In the SFE, every biometric feature acts as a noisy communication channel that carries one or several bits of a Turbo
codeword \cite argonesrua2024strengthened. The soft Turbo decoder needs, for every codeword bit \f$o_i\f$ and every received
symbol \f$R\f$, the two conditional likelihoods \f$f(R \mid o_i = 0)\f$ and \f$f(R \mid o_i = 1)\f$. GMMLite represents these
densities:

- the **noise model** of a feature is a single Gaussian \f$\mathcal{N}(0, \sigma^2)\f$, built with
  gmmlite::GMM::createFromOneGaussian() from the intra-subject standard deviation measured at enrollment;
- the **conditional pdf of a bit** is a mixture: for a \f$2^{m}\f$-PSK symbol carrying \f$m\f$ bits, it has one Gaussian
  component per constellation point whose Gray label has the required value at that bit position (and replicas shifted by
  \f$\pm 2\pi\f$, which approximate the wrapped normal distribution of a phase \cite mardia2000directional); for a 2-PAM
  symbol it is a single Gaussian centred at \f$\pm 1\f$;
- the soft decoder evaluates these mixtures with gmmlite::GMM::logLikelihood().

GMMs are never trained in the SFE software, so GMMLite has no estimation algorithms (no k-means, EM or MAP adaptation).

Model {#gmmlite_model}
=====

A model with \f$M\f$ components in dimension \f$D\f$ is
\f[
  f(\mathbf{x}) = \sum_{m=0}^{M-1} w_m \prod_{d=0}^{D-1} \frac{1}{\sqrt{2\pi\sigma^2_{m,d}}}
                  \exp\!\left(-\frac{(x_d-\mu_{m,d})^2}{2\sigma^2_{m,d}}\right).
\f]
gmmlite::GMM::calculateFactors() precomputes, for every component, \f$\log\big((2\pi)^D\prod_d\sigma^2_{m,d}\big)\f$.
gmmlite::GMM::logLikelihoodComponent() returns
\f$\log f_m(\mathbf{x}) = -\tfrac{1}{2}\big(\sum_d (x_d-\mu_{m,d})^2/\sigma^2_{m,d} + \log((2\pi)^D\prod_d\sigma^2_{m,d})\big)\f$,
and gmmlite::GMM::logLikelihood() computes
\f$\log \sum_m \exp\big(\log w_m + \log f_m(\mathbf{x})\big)\f$, where logarithms of arguments below
gmmlite::kMinPositive are floored to gmmlite::kMinLog (gmmlite::safeLog()), so that \f$-\infty\f$ never reaches the decoder.
The means and variances are stored as row-major
<tt>[component][dimension]</tt> arrays.

API summary {#gmmlite_api}
===========

| Member | Purpose |
|--------|---------|
| \ref gmmlite::GMM::GMM() "GMM()" | Empty model. |
| \ref gmmlite::GMM::GMM(unsigned int, unsigned int) "GMM(D, M)" | Flat model (\f$M\f$ components, zero means, variances gmmlite::kFlatModelVariance, equal weights), to be filled through the accessors. |
| \ref gmmlite::GMM::createFromOneGaussian "createFromOneGaussian()" | Single-Gaussian model from a mean and a variance vector. |
| \ref gmmlite::GMM::getDimension "getDimension()", \ref gmmlite::GMM::getNMixtures "getNMixtures()" | Size of the model. |
| \ref gmmlite::GMM::getWeights "getWeights()", \ref gmmlite::GMM::getMu "getMu()", \ref gmmlite::GMM::getVar "getVar()" | Direct access to the parameters. |
| \ref gmmlite::GMM::calculateFactors "calculateFactors()" | Must be called after the parameters are changed through the accessors. |
| \ref gmmlite::GMM::logLikelihood "logLikelihood()", \ref gmmlite::GMM::logLikelihoodComponent "logLikelihoodComponent()" | Log-likelihood of a sample (whole model or one component). |
| \ref gmmlite::GMM::generateSample "generateSample()" | Draws a sample (used to simulate noisy channels in tests). |

The parameters are held in \c std::vector members, so copies are deep and exception safe. The class lives in the
\c gmmlite namespace and its header defines no macros. In the SFE libraries, \c turbo-codes_types.h of TurboCodes defines
<tt>typedef gmmlite::GMM GMM;</tt>, and the rest of the code only uses that alias.

\code{.cpp}
#include <GMMLite/gmm.h>

gmmlite::GMM noise;
double mean = 0.0, variance = 0.04;
noise.createFromOneGaussian(1, &mean, &variance);   // N(0, 0.04); factors computed

gmmlite::GMM pdf(1, 2);                             // flat model with 2 Gaussians
pdf.getMu()[0] = -1.0;  pdf.getVar()[0] = variance;
pdf.getMu()[1] = +1.0;  pdf.getVar()[1] = variance;
pdf.calculateFactors();                             // variances changed: refresh the factors

float y = 0.3f;
double llk = pdf.logLikelihood(&y);
float noisy;
noise.generateSample(&noisy);
\endcode

Random numbers {#gmmlite_random}
--------------

gmmlite::GMM::generateSample() draws from a per-thread Mersenne Twister engine (\c std::mt19937
\cite matsumoto1998mersenne) seeded from \c std::random_device. It is thread safe and does not use the C library \c rand().
It is only used to simulate noisy symbols; it is not a cryptographic random source.

Origin and equivalence {#gmmlite_equivalence}
======================

GMMLite replaces the \c GMM class of the CHMM library, which the earlier versions of the SFE software used. CHMM is a much
larger library (GMM and coupled-HMM training, file I/O) that depends on Armadillo and POSIX threads and terminates the process
on errors. gmmlite::GMM::logLikelihood() and gmmlite::GMM::calculateFactors() perform the same floating-point operations in
the same order as CHMM, so the log-likelihoods, and therefore every decoded key and evaluation result of the SFE programs, are
**bit-identical**. The helper data files produced with both versions are interchangeable.

The optional test \c compare_with_chmm (tests/compare_with_chmm.cpp) checks the bit-exact agreement on 600,000 evaluations of
random flat, copied and single-Gaussian models. CHMM is not part of SCAMPER SFE, so this test is only built when an installed
CHMM is found.

Building and testing {#gmmlite_build}
====================

GMMLite is built as the shared library \c GMMLite (\c GMMLite_Debug in Debug builds) by the top-level CMake project of the
repository. On its own:

    cmake -S libraries/GMMLite -B build/GMMLite -DCMAKE_BUILD_TYPE=Release \
          -DGMMLITE_BUILD_TESTS=ON [-DCMAKE_INSTALL_PREFIX=PREFIX]
    cmake --build build/GMMLite -j
    (cd build/GMMLite && ctest)
    cmake --install build/GMMLite

The library is installed to \c PREFIX/lib and the headers to \c PREFIX/include/GMMLite, so users include
<tt>\<GMMLite/gmm.h\></tt>.

- \c test_gmmlite (tests/test_gmmlite.cpp) checks closed-form log-likelihoods, the flat model, the flooring, deep copies and
  the moments of the sampler.
- \c compare_with_chmm (tests/compare_with_chmm.cpp) is the optional bit-exactness test described above.
