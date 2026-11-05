/// \file psk.cpp
/// \brief Implementation of the \ref PSK constellation.

#include "psk.h"
#include <stdexcept>

#ifdef DEBUG
#include <iostream>
#include <iomanip>
#endif

PSK::PSK() {
  m_outputDimensionsCount = 1;
}

void PSK::encode(const binary_polynomial &input, real_number_vector &output, const bool addNoise) const {
  if (m_inputBitsCount == 0) {
    std::string exceptionMessage("PSK::encode: m_inputBitsCount is 0. The method PSK::setupConstellation must be called in order to setup the constellation before calling the PSK::encode method.");
    throw std::logic_error(exceptionMessage.c_str());
  }
  unsigned int gray = 0;
  for (unsigned int bitI = 0; bitI < m_inputBitsCount; bitI++) {
    if (input[bitI]) {
      gray += (1 << (m_inputBitsCount - 1 - bitI));
    }
  }
  unsigned int num = grayToBinary(gray);
  output = real_number_vector(1, (num*2*M_PI)/(1 << m_inputBitsCount));
  if (addNoise) {
    float noiseSample;
    m_noiseDistribution.generateSample(&noiseSample);
    output[0] += noiseSample;
    /*
    while (output[0] < 0.0) {
        output[0] += 2.0*M_PI;
      }
    while (output[0] >= 2.0*M_PI) {
        output[0] -= 2.0*M_PI;
      }
    */
  }
}

void PSK::setupConstellation(const unsigned int inputBitsCount, const GMM noiseDistribution) {
  m_inputBitsCount = inputBitsCount;
  if (noiseDistribution.getDimension() != 1) {
    std::string exceptionMessage;
    exceptionMessage =
      std::string("PSK::setupConstellation: noiseDistribution must have dimensionality 1 for a PSK constellation, but it has dimensionality ") + std::to_string(noiseDistribution.getDimension()) +
      std::string(" instead.");
    throw std::logic_error(exceptionMessage.c_str());
  }
  m_noiseDistribution = noiseDistribution;
  setupConditionalPdfs();
}

void PSK::setupConditionalPdfs() {
  const unsigned int symbolsCount = (1 << m_inputBitsCount);
  const unsigned int mixturesInNoiseDistribution = m_noiseDistribution.getNMixtures();
  const unsigned int mixturesInConditionalDistributions = 3*(symbolsCount >> 1)*mixturesInNoiseDistribution;
  m_pdfs0 = GMM_vector(m_inputBitsCount, GMM(1, mixturesInConditionalDistributions));
  m_pdfs1 = GMM_vector(m_inputBitsCount, GMM(1, mixturesInConditionalDistributions));
  double *varNoise = m_noiseDistribution.getVar();
  double *muNoise = m_noiseDistribution.getMu();
  for (unsigned int bitI = 0; bitI < m_inputBitsCount; bitI++) {
    unsigned int alreadySet0(0), alreadySet1(0);
    for (unsigned int symbolI = 0; symbolI < symbolsCount; symbolI++) {
      unsigned int gray = binaryToGray(symbolI);
      double symbolEncoding = symbolI*2.0*M_PI/symbolsCount;
      bool bitIValue = ((gray & (symbolsCount >> (bitI + 1))) != 0);
      double *mu;
      double *var;
      if (!bitIValue) {
	// Copy the noise distribution means plus the symbolEncoding + {-2pi, 0, 2pi} to the corresponding m_pdfs
	mu = m_pdfs0[bitI].getMu() + 3*alreadySet0*mixturesInNoiseDistribution;
	var = m_pdfs0[bitI].getVar() + 3*alreadySet0*mixturesInNoiseDistribution;
      } else { // bitIValue == 1
	// Copy the noise distribution means plus the symbolEncoding + {-2pi, 0, 2pi} to the corresponding m_pdfs
	mu = m_pdfs1[bitI].getMu() + 3*alreadySet1*mixturesInNoiseDistribution;
	var = m_pdfs1[bitI].getVar() + 3*alreadySet1*mixturesInNoiseDistribution;
      }
      for (unsigned int mixtureI = 0; mixtureI < mixturesInNoiseDistribution; mixtureI++) {
#ifdef DEBUG
          std::cout << "Setting mixture " << 3*mixtureI << " of bit " << bitI
                    << " conditioned to value " << bitIValue << " mean to "
                    << *(muNoise + mixtureI) + symbolEncoding << std::endl;
#endif
	*(mu + 3*mixtureI) = *(muNoise + mixtureI) + symbolEncoding;
	*(var + 3*mixtureI) = *(varNoise + mixtureI);
#ifdef DEBUG
          std::cout << "Setting mixture " << 3*mixtureI + 1 << " of bit " << bitI
                    << " conditioned to value " << bitIValue << " mean to "
                    << *(muNoise + mixtureI) + symbolEncoding + 2*M_PI << std::endl;
#endif
	*(mu + 3*mixtureI + 1) = *(muNoise + mixtureI) + symbolEncoding + 2*M_PI;
	*(var + 3*mixtureI + 1) = *(varNoise + mixtureI);
#ifdef DEBUG
          std::cout << "Setting mixture " << 3*mixtureI + 2 << " of bit " << bitI
                    << " conditioned to value " << bitIValue << " mean to "
                    << *(muNoise + mixtureI) + symbolEncoding - 2*M_PI << std::endl;
#endif
	*(mu + 3*mixtureI + 2) = *(muNoise + mixtureI) + symbolEncoding - 2*M_PI;
	*(var + 3*mixtureI + 2) = *(varNoise + mixtureI);
      }
      if (bitIValue) {
        alreadySet1++;
      } else {
        alreadySet0++;
      }	
    }
    m_pdfs0[bitI].calculateFactors();
    m_pdfs1[bitI].calculateFactors();
  }
#ifdef DEBUG
  std::cout << symbolsCount << "-PSK conditional PDF means in the main interval:" << std::endl;
  for (unsigned int bitValue = 0; bitValue < 2; bitValue++) {
      std::cout << "\tConditioned to " << bitValue << ":" << std::endl;
      for (unsigned int bitI = 0; bitI < m_inputBitsCount; bitI++) {
          std::cout << "\t\tBit index: " << bitI << std::endl;
          double *mu, *var;
          if (bitValue == 0) {
              mu = m_pdfs0[bitI].getMu();
	      var = m_pdfs0[bitI].getVar();
            } else {
              mu = m_pdfs1[bitI].getMu();
              var = m_pdfs1[bitI].getVar();
            }
          for (unsigned int mixtureI = 0; mixtureI < mixturesInConditionalDistributions/3; mixtureI++) {
              std::cout << "\t\t\tMixture " << std::setw(3) << 3*mixtureI
			<< " mean: " << std::setw(10) << *(mu + 3*mixtureI)
			<< " variance: " << std::setw(10) << *(var + 3*mixtureI) << std::endl;
            }
        }
    }
#endif
}

// This function converts an unsigned binary number to reflected binary Gray code.
uint PSK::binaryToGray(uint num)
{
    return num ^ (num >> 1); // The operator >> is shift right. The operator ^ is exclusive or.
}

uint PSK::grayToBinary(uint num) const
{
    uint mask = num;
    while (mask) {           // Each Gray code bit is exclusive-ored with all more significant bits.
        mask >>= 1;
        num   ^= mask;
    }
    return num;
}
