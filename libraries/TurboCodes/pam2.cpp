/// \file pam2.cpp
/// \brief Implementation of the \ref PAM2 constellation.

#include "pam2.h"

PAM2::PAM2() {}

void PAM2::encode(const binary_polynomial &input, real_number_vector &output, const bool addNoise) const
{
    if (m_inputBitsCount != 1) {
        std::string exceptionMessage("PAM2::encode: m_inputBitsCount has to be 1 before calling the PAM2::encode method.");
        throw std::logic_error(exceptionMessage);
    }
    if (input.size() != m_inputBitsCount) {
        throw std::logic_error(std::string("PAM2::encode: Inconsistent input length and m_inputBitsCount."));
    }
    if (!input[0]) {
        output = real_number_vector(1, -1.0);
    } else {
        output = real_number_vector(1, 1.0);
    }
    if (addNoise) {
        float noiseSample;
        m_noiseDistribution.generateSample(&noiseSample);
        output[0] += noiseSample;
    }

}

void PAM2::setupConstellation(const unsigned int inputBitsCount, const GMM noiseDistribution)
{
    m_inputBitsCount = inputBitsCount;
    if (m_inputBitsCount != 1) {
        std::string exceptionMessage("PAM2::setupConstellation: inputBitsCount has to be 1 when calling PAM2::setupConstellation method.");
        throw std::logic_error(exceptionMessage.c_str());
    }
    if (inputBitsCount != 1) {
        throw std::logic_error(std::string("PAM2::setupConstellation: 2-PAM constellation has only 1 input bit."));
    }
    if (noiseDistribution.getDimension() != 1) {
        std::string exceptionMessage;
        exceptionMessage =
            std::string("PSK::setupConstellation: noiseDistribution must have dimensionality 1 for a PSK constellation, but it has dimensionality ") + std::to_string(noiseDistribution.getDimension()) +
            std::string(" instead.");
        throw std::logic_error(exceptionMessage.c_str());
    }
    m_pdfs0 = GMM_vector(1, noiseDistribution);
    m_pdfs1 = GMM_vector(1, noiseDistribution);
    double *mu0 = m_pdfs0[0].getMu();
    double *mu1 = m_pdfs1[0].getMu();
    *mu0 = -1.0;
    *mu1 = 1.0;
    m_pdfs0[0].calculateFactors();
    m_pdfs1[0].calculateFactors();
}
