/// \file constellation.h
/// \brief Abstract base class for the digital modulations (constellations) used to embed codeword bits into biometric features.

#ifndef CONSTELLATION_H
#define CONSTELLATION_H

#include "turbo-codes_global.h"
#include "turbo-codes_types.h"
#include <string>
#include <stdexcept>

/// Pure virtual class, base for all constellations
/// The constellations encode binary vectors \f$ \mathbf{B} = \left[b_1, \ldots, b_{D_{\mathbf{B}}}\right] \f$ into
/// symbols \f$ \mathbf{S} = \left[s_1, \ldots, s_{D_{\mathbf{S}}}\right] \f$.
/// \author Enrique Argones Rúa
/// \date 2025
class TURBOCODES_EXPORT Constellation {
 public:
  /// Constructor.
  Constellation();
 protected:
  /// Number of bits per symbol. Noted as \f$ D_{\mathbf{B}} = \f$ \ref m_inputBitsCount
  unsigned int m_inputBitsCount;
  /// Dimensionality of the symbols. Noted as \f$ D_{\mathbf{S}} = \f$ \ref m_outputDimensionsCount
  unsigned int m_outputDimensionsCount;
  /// Distribution of the noise. It needs to have a dimensionality equal to \ref m_outputDimensionsCount
  GMM m_noiseDistribution;
  /// Vector of the probability density functions of the conditional random variables \f$ \mathbf{S} \mid b_i = 0\f$.
  /// Noted as \f$ \mathbf{f}^0 = \left[f^0_1(\mathbf{S}), \ldots, f^0_n(\mathbf{S})\right] = \left[f_{\mathbf{S} \mid b_1 = 0}(\mathbf{S}), \ldots, f_{\mathbf{S} \mid b_n = 0}(\mathbf{S})\right] \f$.
  GMM_vector m_pdfs0;
  /// Vector of the probability density functions of the conditional random variables \f$ \mathbf{S} \mid b_i = 1\f$.
  /// Noted as \f$ \mathbf{f}^1 = \left[f^1_1(\mathbf{S}), \ldots, f^1_n(\mathbf{S})\right] = \left[f_{\mathbf{S} \mid b_1 = 1}(\mathbf{S}), \ldots, f_{\mathbf{S} \mid b_n = 1}(\mathbf{S})\right] \f$.
  GMM_vector m_pdfs1;
 public:
  /// Pure virtual method, to be implemented by all derived classes.
  /// @param[in]  input Binary vector. It needs to contain exactly \ref m_inputBitsCount elements.
  /// @param[out] output Real number vector. It will contain exactly \ref m_outputDimensionsCount elements.
  /// @param[in]  addNoise Binary flag. If true, the output will be noisy. Otherwise, it will be the exact encoding of the \p input binary vector.
  virtual void encode(const binary_polynomial &input, real_number_vector &output, const bool addNoise=false) const = 0;
  /// Pure virtual method, to be implemented by all derived classes.
  /// @param[in] inputBitsCount \ref m_inputBitsCount will be set to this value by the derived class implementation.
  /// @param[in] noiseDistribution \ref m_noiseDistribution will be set to this value by the derived class implementation.
  virtual void setupConstellation(const unsigned int inputBitsCount, const GMM noiseDistribution) = 0;
  /// Getter of \ref m_pdfs0 attribute
  /// @return Vector of \ref m_inputBitsCount GMMs, the pdf of the symbol conditioned to each input bit being 0.
  const GMM_vector &pdfs0() const;
  /// Getter of \ref m_pdfs1 attribute
  /// @return Vector of \ref m_inputBitsCount GMMs, the pdf of the symbol conditioned to each input bit being 1.
  const GMM_vector &pdfs1() const;
};

#endif // CONSTELLATION_H
