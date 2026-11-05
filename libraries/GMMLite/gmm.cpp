/// \file gmm.cpp
/// \brief Implementation of the non-inline members of gmmlite::GMM.

#include "gmm.h"
#include <random>

namespace gmmlite {

namespace {

/// Random engine of the calling thread, seeded once from \c std::random_device.
/// \return Reference to the engine of the calling thread.
std::mt19937 &threadEngine()
{
    thread_local std::mt19937 engine{std::random_device{}()};
    return engine;
}

} // namespace

GMM::GMM() : m_dimension(0), m_nMixtures(0)
{
}

GMM::GMM(const unsigned int dimension, const unsigned int nMixtures)
    : m_dimension(dimension),
      m_nMixtures(nMixtures),
      m_weights(nMixtures, nMixtures > 0 ? 1.0 / nMixtures : 0.0),
      m_mu(static_cast<std::size_t>(dimension) * nMixtures, 0.0),
      m_var(static_cast<std::size_t>(dimension) * nMixtures, kFlatModelVariance),
      m_factor(nMixtures, 0.0)
{
    calculateFactors();
}

void GMM::createFromOneGaussian(const unsigned int dimension, const double *mean, const double *variance)
{
    m_dimension = dimension;
    m_nMixtures = 1;
    m_weights.assign(1, 1.0);
    m_mu.assign(mean, mean + dimension);
    m_var.assign(variance, variance + dimension);
    m_factor.assign(1, 0.0);
    calculateFactors();
}

void GMM::generateSample(float *sample) const
{
    if (m_nMixtures == 0) {
        return;
    }
    std::mt19937 &engine = threadEngine();
    // Choice of the Gaussian by inversion of the cumulative weights (the last one absorbs rounding errors).
    std::uniform_real_distribution<double> uniform(0.0, 1.0);
    const double u = uniform(engine);
    unsigned int mixture = m_nMixtures - 1;
    double cumulative = 0.0;
    for (unsigned int gI = 0; gI < m_nMixtures; gI++) {
        cumulative += m_weights[gI];
        if (cumulative >= u) {
            mixture = gI;
            break;
        }
    }
    std::normal_distribution<double> standardNormal(0.0, 1.0);
    const unsigned int offset = mixture * m_dimension;
    for (unsigned int dI = 0; dI < m_dimension; dI++) {
        sample[dI] = static_cast<float>(m_mu[offset + dI] + std::sqrt(m_var[offset + dI]) * standardNormal(engine));
    }
}

} // namespace gmmlite
