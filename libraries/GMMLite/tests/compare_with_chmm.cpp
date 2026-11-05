/// \file compare_with_chmm.cpp
/// \brief Checks that gmmlite::GMM gives bit-identical log-likelihoods to the CHMM \c GMM class for the operations used by the
/// SFE software (flat models filled through the accessors, single Gaussians, copies). Built only when CHMM is found.

#include <chmm-library/gmm_pthreaded.hh>
#include <GMMLite_global.h>
#include <gmm.h>
#include <cstdio>
#include <cstring>
#include <random>
#include <vector>

namespace {

/// Number of evaluations whose results differ in at least one bit.
unsigned long long mismatches = 0;
/// Number of evaluations.
unsigned long long evaluations = 0;

void compare(const GMM &reference, const gmmlite::GMM &lite, const std::vector<float> &samples, const unsigned int dimension)
{
    for (std::size_t s = 0; s + dimension <= samples.size(); s += dimension) {
        const double a = reference.logLikelihood(samples.data() + s);
        const double b = lite.logLikelihood(samples.data() + s);
        evaluations++;
        if (std::memcmp(&a, &b, sizeof(double)) != 0) {
            if (mismatches < 10) {
                std::printf("mismatch: CHMM %.17g GMMLite %.17g\n", a, b);
            }
            mismatches++;
        }
    }
}

} // namespace

int main()
{
    std::mt19937 engine(12345);
    std::uniform_real_distribution<double> uniform(-8.0, 8.0);
    std::uniform_real_distribution<double> varianceDist(1e-4, 3.0);

    for (unsigned int trial = 0; trial < 400; trial++) {
        const unsigned int dimension = 1 + trial % 4;
        const unsigned int mixtures = 1 + (trial * 7) % 48;
        // Flat model filled through the accessors, as PSK::setupConditionalPdfs does.
        GMM reference(static_cast<int>(dimension), static_cast<int>(mixtures));
        gmmlite::GMM lite(dimension, mixtures);
        double weightSum = 0.0;
        std::vector<double> weights(mixtures);
        for (unsigned int g = 0; g < mixtures; g++) {
            weights[g] = (trial % 3 == 0) ? 1.0 : std::uniform_real_distribution<double>(1e-6, 1.0)(engine);
            weightSum += weights[g];
        }
        for (unsigned int g = 0; g < mixtures; g++) {
            reference.getWeights()[g] = lite.getWeights()[g] = weights[g] / weightSum;
            for (unsigned int d = 0; d < dimension; d++) {
                reference.getMu()[g * dimension + d] = lite.getMu()[g * dimension + d] = uniform(engine);
                reference.getVar()[g * dimension + d] = lite.getVar()[g * dimension + d] = varianceDist(engine);
            }
        }
        reference.calculateFactors();
        lite.calculateFactors();
        std::vector<float> samples(dimension * 500);
        for (float &v : samples) {
            v = static_cast<float>(uniform(engine) * 2.0);
        }
        compare(reference, lite, samples, dimension);
        // Copies (std::vector<GMM> elements are copies, as in TurboCode::setPdfs0/1)
        std::vector<GMM> referenceCopies(2, reference);
        std::vector<gmmlite::GMM> liteCopies(2, lite);
        compare(referenceCopies[1], liteCopies[1], samples, dimension);

        // Single Gaussian, as the noise distributions of HSTCFE.
        std::vector<double> mean(dimension), variance(dimension);
        for (unsigned int d = 0; d < dimension; d++) {
            mean[d] = uniform(engine);
            variance[d] = varianceDist(engine);
        }
        GMM referenceSingle;
        referenceSingle.createFromOneGaussian(dimension, mean.data(), variance.data());
        referenceSingle.calculateFactors();
        gmmlite::GMM liteSingle;
        liteSingle.createFromOneGaussian(dimension, mean.data(), variance.data());
        compare(referenceSingle, liteSingle, samples, dimension);
    }

    std::printf("%llu evaluations, %llu bit mismatches\n", evaluations, mismatches);
    return mismatches == 0 ? 0 : 1;
}
