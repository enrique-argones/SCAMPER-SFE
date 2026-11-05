/// \file test_gmmlite.cpp
/// \brief Self-contained unit tests of gmmlite::GMM (no dependency other than GMMLite). Run with \c ctest or directly.

#include <GMMLite_global.h>
#include <gmm.h>
#include <cmath>
#include <cstdio>
#include <vector>

namespace {

int failures = 0;

void check(const bool condition, const char *what)
{
    if (!condition) {
        std::printf("FAILED: %s\n", what);
        failures++;
    }
}

bool close(const double a, const double b, const double tolerance = 1e-12)
{
    return std::fabs(a - b) <= tolerance * (1.0 + std::fabs(b));
}

} // namespace

int main()
{
    using gmmlite::GMM;

    // Empty model
    GMM empty;
    check(empty.getDimension() == 0 && empty.getNMixtures() == 0, "empty model has dimension and size 0");
    check(empty.getMu() == nullptr && empty.getVar() == nullptr && empty.getWeights() == nullptr, "empty model has null arrays");

    // Single Gaussian: closed-form log-density
    const double mean[2] = {0.5, -1.0};
    const double variance[2] = {0.25, 4.0};
    GMM single;
    single.createFromOneGaussian(2, mean, variance);
    const float x[2] = {1.0f, 1.0f};
    const double expected = -0.5 * (std::log(std::pow(2.0 * gmmlite::kPi, 2) * 0.25 * 4.0)
                                    + (0.5 * 0.5) / 0.25 + (2.0 * 2.0) / 4.0);
    check(single.getNMixtures() == 1 && single.getDimension() == 2, "single Gaussian sizes");
    check(close(single.logLikelihood(x), expected), "single Gaussian log-likelihood");
    check(close(single.logLikelihoodComponent(x, 0), expected), "single Gaussian component log-likelihood");

    // Flat model: weights 1/M, means 0, variances kFlatModelVariance, factors ready
    GMM flat(1, 4);
    check(flat.getNMixtures() == 4 && flat.getDimension() == 1, "flat model sizes");
    check(close(flat.getWeights()[3], 0.25) && flat.getMu()[2] == 0.0 && flat.getVar()[1] == gmmlite::kFlatModelVariance,
          "flat model parameters");
    const float zero = 0.0f;
    check(close(flat.logLikelihood(&zero), -0.5 * std::log(2.0 * gmmlite::kPi * gmmlite::kFlatModelVariance)),
          "flat model log-likelihood");

    // Two-component mixture filled through the accessors (as PSK and PAM2 do)
    GMM mixture(1, 2);
    mixture.getMu()[0] = -1.0;
    mixture.getMu()[1] = 1.0;
    mixture.getVar()[0] = 0.5;
    mixture.getVar()[1] = 0.5;
    mixture.calculateFactors();
    const float y = 0.3f;
    auto gauss = [](double v, double m, double s2) { return std::exp(-0.5 * (v - m) * (v - m) / s2) / std::sqrt(2 * gmmlite::kPi * s2); };
    const double yd = static_cast<double>(y);
    check(close(mixture.logLikelihood(&y), std::log(0.5 * gauss(yd, -1, 0.5) + 0.5 * gauss(yd, 1, 0.5)), 1e-10),
          "mixture log-likelihood");

    // Flooring of the logarithm far away from the model
    const float far = 1e6f;
    check(mixture.logLikelihood(&far) == gmmlite::kMinLog, "log-likelihood floored at kMinLog");

    // Deep copies
    GMM copy(mixture);
    copy.getMu()[0] = 10.0;
    check(mixture.getMu()[0] == -1.0, "copy constructor is deep");
    GMM assigned;
    assigned = mixture;
    assigned.getVar()[1] = 3.0;
    check(mixture.getVar()[1] == 0.5, "copy assignment is deep");
    std::vector<GMM> models(3, mixture);
    check(close(models[2].logLikelihood(&y), mixture.logLikelihood(&y), 0.0), "copies in a vector evaluate identically");

    // createFromOneGaussian replaces a previous model
    copy.createFromOneGaussian(2, mean, variance);
    check(copy.getNMixtures() == 1 && close(copy.logLikelihood(x), expected), "createFromOneGaussian replaces the model");

    // Sampling: empirical mean and variance of a single Gaussian
    GMM noise;
    const double m0 = 2.0, v0 = 0.09;
    noise.createFromOneGaussian(1, &m0, &v0);
    const unsigned int n = 200000;
    double sum = 0.0, sum2 = 0.0;
    for (unsigned int i = 0; i < n; i++) {
        float s;
        noise.generateSample(&s);
        sum += s;
        sum2 += static_cast<double>(s) * s;
    }
    const double sampleMean = sum / n, sampleVar = sum2 / n - sampleMean * sampleMean;
    check(std::fabs(sampleMean - m0) < 0.005 && std::fabs(sampleVar - v0) < 0.005, "generateSample moments");

    // Sampling from a mixture picks the components according to the weights
    GMM bimodal(1, 2);
    bimodal.getWeights()[0] = 0.25;
    bimodal.getWeights()[1] = 0.75;
    bimodal.getMu()[0] = -100.0;
    bimodal.getMu()[1] = 100.0;
    unsigned int positives = 0;
    for (unsigned int i = 0; i < 40000; i++) {
        float s;
        bimodal.generateSample(&s);
        positives += (s > 0.0f) ? 1 : 0;
    }
    check(std::fabs(positives / 40000.0 - 0.75) < 0.02, "generateSample mixture weights");

    if (failures == 0) {
        std::printf("All GMMLite tests passed.\n");
    }
    return failures == 0 ? 0 : 1;
}
