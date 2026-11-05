/// \file psk.h
/// \brief Phase Shift Keying (\f$2^{D_{\mathbf{B}}}\f$-PSK) constellation with Gray mapping.

#ifndef PSK_H
#define PSK_H

#include "turbo-codes_global.h"
#include "constellation.h"

/// PSK constellation class
/// The number of input bits determines the conventional name of the constellation: \f$2^D_{\mathbf{B}} \textrm{-PSK} \f$.
/// \author Enrique Argones Rúa
/// \date 2025
class TURBOCODES_EXPORT PSK: public Constellation {
 public:
  /// Constructor
  PSK();
  /// Method implementing the pure virtual \ref Constellation::encode method.
  /// @param[in]  input Binary vector with exactly \ref Constellation::m_inputBitsCount elements.
  /// @param[out] output One-dimensional symbol: the phase \f$S(i) \in [0, 2\pi)\f$ of the Gray-mapped constellation point.
  /// @param[in]  addNoise If true, a noise realisation drawn from the noise distribution is added to the phase.
  virtual void encode(const binary_polynomial &input, real_number_vector &output, const bool addNoise=false) const;
  /// Method implementing the pure virtual \ref Constellation::setupConstellation method.
  /// @param[in] inputBitsCount Number of bits per symbol \f$D_{\mathbf{B}}\f$. The constellation has \f$2^{D_{\mathbf{B}}}\f$ points.
  /// @param[in] noiseDistribution One-dimensional (phase) noise model used to build the per-bit conditional pdfs.
  virtual void setupConstellation(const unsigned int inputBitsCount, const GMM noiseDistribution);

 private:
  /// This method computes the symbol index for a given binary vector
  /// @param[in] input Binary vector. It must have exactly \ref Constellation::m_inputBitsCount elements.
  /// @return The index of the output symbol in the PSK constellation. This index can be noted by \f$ i \in \{0, \ldots, 2^{D_{\mathbf{B}}} \} \f$.
  /// The corresponding symbol would be \f$ S(i) = 2^{i - D_{\mathbf{B}}}2\pi \f$
  unsigned int symbolIndex(const binary_polynomial &input) const;
  /// This method initializes the conditional pdf vectors \ref Constellation::m_pdfs0 and \ref Constellation::m_pdfs1
  void setupConditionalPdfs();
  /// This method converts an unsigned binary number to reflected binary Gray code.
  /// @param[in] num Natural binary number.
  /// @return Gray-coded value of \p num.
  uint binaryToGray(uint num);
  /// This method converts a reflected binary Gray code number to a binary number.
  /// @param[in] num Gray-coded number.
  /// @return Natural binary value of \p num.
  uint grayToBinary(uint num) const;
};

#endif // PSK_H
