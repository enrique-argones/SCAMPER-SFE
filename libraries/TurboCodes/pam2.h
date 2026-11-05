/// \file pam2.h
/// \brief Binary antipodal (2-PAM) constellation.

#ifndef PAM2_H
#define PAM2_H

#include "constellation.h"

/// 2-PAM (binary antipodal) constellation class.
///
/// Encodes a single bit \f$b\f$ into the one-dimensional symbol \f$s = 2b - 1\f$, i.e. bit 0 \f$\rightarrow -1\f$ and
/// bit 1 \f$\rightarrow +1\f$. This convention is used throughout the software (see \ref setupConstellation).
/// In the Hybrid Strengthened Fuzzy Extractor it models the fuzzy-commitment part: each combined feature carries one
/// codeword bit, encoded in the sign of the projected (and possibly sign-flipped) biometric feature.
/// \author Enrique Argones Rúa
/// \date 2025
class PAM2 : public Constellation
{
public:
    /// Constructor
    PAM2();
    /// Method implementing the pure virtual \ref Constellation::encode method.
    /// @param[in]  input Binary vector with exactly one element.
    /// @param[out] output One-dimensional symbol: \f$-1\f$ for bit 0 and \f$+1\f$ for bit 1.
    /// @param[in]  addNoise If true, a noise realisation drawn from the noise distribution is added to the symbol.
    virtual void encode(const binary_polynomial &input, real_number_vector &output, const bool addNoise=false) const;
    /// Method implementing the pure virtual \ref Constellation::setupConstellation method.
    /// @param[in] inputBitsCount Number of bits per symbol. Must be 1 for 2-PAM.
    /// @param[in] noiseDistribution One-dimensional noise model. The conditional pdfs \ref Constellation::m_pdfs0 and \ref Constellation::m_pdfs1 are this distribution shifted to \f$-1\f$ and \f$+1\f$ respectively.
    virtual void setupConstellation(const unsigned int inputBitsCount, const GMM noiseDistribution);

};

#endif // PAM2_H
