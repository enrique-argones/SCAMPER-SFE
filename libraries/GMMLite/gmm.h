/// \file gmm.h
/// \brief Declaration of gmmlite::GMM, the minimal diagonal-covariance Gaussian Mixture Model used by the Strengthened Fuzzy
/// Extractor (SFE) libraries.
///
/// GMMLite keeps only the part of the CHMM \c GMM class that the SFE software (TurboCodes, HSTCFE, MHSTCFE and the two
/// programs) needs to describe the noise of the PSK and 2-PAM constellations and to evaluate the per-bit conditional pdfs in the
/// soft Turbo decoder:
///
/// | Operation | Members |
/// |-----------|---------|
/// | Construction | GMM(), GMM(unsigned int, unsigned int), createFromOneGaussian(), copy and move |
/// | Access to the parameters | getDimension(), getNMixtures(), getWeights(), getMu(), getVar() |
/// | Evaluation | calculateFactors(), logLikelihood(), logLikelihoodComponent() |
/// | Sampling | generateSample() |
///
/// Training (k-means, EM, MAP, eigen-means), file and binary I/O, HMM support and the POSIX-thread code of CHMM are not part of
/// GMMLite, so the library has no dependency other than the C++ standard library.
///
/// \b Memory \b layout: as in CHMM, the means and variances are row-major <tt>[gaussian][dimension]</tt> arrays, i.e. component
/// \f$d\f$ of Gaussian \f$i\f$ is at index \f$i\cdot D + d\f$.
///
/// \author Enrique Argones Rúa
/// \date 2026

#ifndef GMMLITE_GMM_H
#define GMMLITE_GMM_H

#include "GMMLite_global.h"
#include <cmath>
#include <vector>

/// \brief Namespace of the GMMLite library.
///
/// The namespace keeps gmmlite::GMM apart from the CHMM \c GMM class, so both libraries can be installed in the same prefix.
/// The SFE libraries refer to it through the \c GMM alias defined in their \c turbo-codes_types.h.
namespace gmmlite {

/// \f$\pi\f$, with the same value as the \c M_PI constant of CHMM.
constexpr double kPi = 3.14159265358979323846;
/// Smallest argument for which safeLog() computes a real logarithm (same value as \c MIN_POSITIVE in CHMM).
constexpr double kMinPositive = 7.12457640674129e-218;
/// Value returned by safeLog() for arguments below #kMinPositive (same value as \c MIN_LOG in CHMM); it avoids \f$-\infty\f$.
constexpr double kMinLog = -500.0;
/// Variance given to every component of the flat model built by GMM(unsigned int, unsigned int) (same value as
/// \c DEFAULT_MIN_VAR_FACTOR in CHMM).
constexpr double kFlatModelVariance = 1e-3;

/// \brief Floored natural logarithm: \f$\log(a)\f$, or #kMinLog when \f$a <\f$ #kMinPositive.
///
/// It is the \c LOG() macro of CHMM written as a function, so the log-likelihoods are bit-identical to the CHMM ones.
/// \param a Argument.
/// \return \f$\log(a)\f$ floored at #kMinLog.
inline double safeLog(const double a)
{
    return (a < kMinPositive) ? kMinLog : std::log(a);
}

/*!
 * \brief Gaussian Mixture Model with diagonal covariance matrices.
 *
 * \f[ p(\mathbf{x}) = \sum_{i=0}^{M-1} w_i\,\mathcal{N}(\mathbf{x};\mu_i,\mathrm{diag}(\sigma^2_i)) \f]
 *
 * where \f$\mu_i\f$ and \f$\sigma^2_i\f$ are the mean and variance vectors of Gaussian \f$i\f$.
 *
 * The parameters are owned by the object (\c std::vector members), so copies are deep and independent, and copy and move
 * construction and assignment are the compiler-generated ones.
 *
 * Typical use in the SFE software:
 * \code{.cpp}
 * gmmlite::GMM noise;
 * double mean = 0.0, variance = sigma * sigma;
 * noise.createFromOneGaussian(1, &mean, &variance);  // the factors are computed here
 * gmmlite::GMM conditional(1, 3 * M);                // flat model, then fill its means and variances...
 * conditional.getMu()[0] = ...;
 * conditional.getVar()[0] = ...;
 * conditional.calculateFactors();                    // ...and refresh the factors
 * double llk = conditional.logLikelihood(&symbol);
 * \endcode
 *
 * \note The normalisation factors (\link m_factor \endlink) depend on the variances. The constructors and
 * createFromOneGaussian() compute them, but after the variances are changed through getVar() calculateFactors() must be called
 * before logLikelihood() or logLikelihoodComponent() are used. Changing the means or the weights does not require it.
 *
 * \note The arithmetic of logLikelihood() and calculateFactors() is the one of CHMM (same operations in the same order, same
 * flooring of the logarithms), so the soft decoder produces the same results with GMMLite and with CHMM.
 */
class GMMLITE_EXPORT GMM
{
public:
    /*!
     * \brief Empty model: dimension 0 and no Gaussians.
     *
     * Used for default-constructed elements of containers, which are assigned afterwards. It cannot be evaluated.
     */
    GMM();
    /*!
     * \brief Flat model with \p nMixtures Gaussians of dimension \p dimension.
     *
     * All the weights are \f$1/M\f$, all the means 0 and all the variances #kFlatModelVariance; the factors are computed. The
     * caller usually overwrites the means and variances through getMu() and getVar() and then calls calculateFactors().
     * \param dimension Dimension \f$D\f$ of the samples.
     * \param nMixtures Number of Gaussians \f$M\f$.
     */
    GMM(unsigned int dimension, unsigned int nMixtures);

    /*!
     * \brief Make this GMM a single Gaussian (weight 1) with the given mean and diagonal variance.
     *
     * The previous parameters are discarded and the factors are computed.
     * \param dimension Dimension \f$D\f$ of the samples.
     * \param mean Mean vector (\p dimension values).
     * \param variance Diagonal of the covariance matrix (\p dimension positive values).
     */
    void createFromOneGaussian(unsigned int dimension, const double *mean, const double *variance);

    /// \brief Getter of \link m_dimension \endlink. \return the dimension \f$D\f$ of the samples.
    unsigned int getDimension() const { return m_dimension; }
    /// \brief Getter of \link m_nMixtures \endlink. \return the number of Gaussians \f$M\f$.
    unsigned int getNMixtures() const { return m_nMixtures; }

    /// \brief Access to the mixture weights. \return pointer to the \f$M\f$ weights (owned by the GMM, \c nullptr if empty).
    double *getWeights() { return dataOrNull(m_weights); }
    /// \brief Read-only access to the mixture weights. \return pointer to the \f$M\f$ weights (\c nullptr if empty).
    const double *getWeights() const { return dataOrNull(m_weights); }
    /// \brief Access to the means. \return pointer to the row-major <tt>[gaussian][dimension]</tt> means (\c nullptr if empty).
    double *getMu() { return dataOrNull(m_mu); }
    /// \brief Read-only access to the means. \return pointer to the row-major <tt>[gaussian][dimension]</tt> means.
    const double *getMu() const { return dataOrNull(m_mu); }
    /// \brief Access to the variances. Call calculateFactors() after changing them.
    /// \return pointer to the row-major <tt>[gaussian][dimension]</tt> variances (\c nullptr if empty).
    double *getVar() { return dataOrNull(m_var); }
    /// \brief Read-only access to the variances. \return pointer to the row-major <tt>[gaussian][dimension]</tt> variances.
    const double *getVar() const { return dataOrNull(m_var); }

    /*!
     * \brief Computes the normalisation factors \f$\log\left((2\pi)^D \prod_d \sigma^2_{i,d}\right)\f$ of every Gaussian.
     *
     * Must be called after the variances are changed and before logLikelihood() or logLikelihoodComponent() are used.
     */
    inline void calculateFactors()
    {
        for (unsigned int gI = 0; gI < m_nMixtures; gI++) {
            double f = std::pow(2.0 * kPi, static_cast<double>(m_dimension));
            const unsigned int offset = gI * m_dimension;
            for (unsigned int dI = 0; dI < m_dimension; dI++) {
                f *= m_var[offset + dI];
            }
            m_factor[gI] = safeLog(f);
        }
    }

    /*!
     * \brief Log-density of one (unweighted) Gaussian of the mixture.
     * \param sample Input sample (\f$D\f$ values).
     * \param gaussianIndex Index \f$i\f$ of the Gaussian, \f$0 \le i < M\f$.
     * \return \f$\log \mathcal{N}(\mathbf{x};\mu_i,\mathrm{diag}(\sigma^2_i))\f$.
     * \pre The factors are up to date (see calculateFactors()).
     */
    inline double logLikelihoodComponent(const float *sample, const unsigned int gaussianIndex) const
    {
        double normInput = 0.0;
        const unsigned int offset = gaussianIndex * m_dimension;
        for (unsigned int i = 0; i < m_dimension; i++) {
            normInput += (sample[i] - m_mu[offset + i]) * (static_cast<double>(sample[i]) - m_mu[offset + i]) / m_var[offset + i];
        }
        return -0.5 * (normInput + m_factor[gaussianIndex]);
    }

    /*!
     * \brief Log-likelihood of a sample.
     * \param sample Input sample (\f$D\f$ values).
     * \return \f$\log p(\mathbf{x}) = \log \sum_i \exp\left(\log w_i + \log \mathcal{N}(\mathbf{x};\mu_i,
     * \mathrm{diag}(\sigma^2_i))\right)\f$, with both logarithms floored by safeLog().
     * \pre The factors are up to date (see calculateFactors()).
     */
    inline double logLikelihood(const float *sample) const
    {
        double lk = 0.0;
        for (unsigned int gI = 0; gI < m_nMixtures; gI++) {
            lk += std::exp(safeLog(m_weights[gI]) + logLikelihoodComponent(sample, gI));
        }
        return safeLog(lk);
    }

    /*!
     * \brief Draws a random sample from the mixture.
     *
     * A Gaussian is chosen according to the weights and a sample of it is drawn. The random numbers come from a
     * \c std::mt19937 engine owned by the calling thread and seeded from \c std::random_device, so the method is thread safe,
     * does not use or reseed the C library \c rand(), and gives different samples on every run (as the CHMM version, which
     * seeds \c rand() with the time).
     * \param[out] sample Address where the \f$D\f$ values of the sample are written.
     */
    void generateSample(float *sample) const;

private:
    /// Pointer to the first element of \p v, or \c nullptr when it is empty (CHMM returns \c NULL for an empty model).
    /// \param v Vector. \return \c v.data() or \c nullptr.
    static double *dataOrNull(std::vector<double> &v) { return v.empty() ? nullptr : v.data(); }
    /// Read-only version of dataOrNull(std::vector<double>&).
    /// \param v Vector. \return \c v.data() or \c nullptr.
    static const double *dataOrNull(const std::vector<double> &v) { return v.empty() ? nullptr : v.data(); }

    unsigned int m_dimension;       ///< Dimension \f$D\f$ of the samples.
    unsigned int m_nMixtures;       ///< Number of Gaussians \f$M\f$.
    std::vector<double> m_weights;  ///< Mixture weights \f$w_i\f$ (\f$M\f$ values).
    std::vector<double> m_mu;       ///< Means, row-major <tt>[gaussian][dimension]</tt> (\f$M \cdot D\f$ values).
    std::vector<double> m_var;      ///< Diagonal variances, row-major <tt>[gaussian][dimension]</tt> (\f$M \cdot D\f$ values).
    std::vector<double> m_factor;   ///< Normalisation factors \f$\log\left((2\pi)^D \prod_d \sigma^2_{i,d}\right)\f$ (\f$M\f$ values).
};

} // namespace gmmlite

#endif // GMMLITE_GMM_H
