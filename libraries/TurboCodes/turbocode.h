/// \file turbocode.h
/// \brief Systematic parallel-concatenated (Turbo) code with soft-input iterative MAP decoding.

#ifndef TURBOCODE_H
#define TURBOCODE_H

#include "turbo-codes_global.h"
#include "turbo-codes_types.h"

/// # Systematic Turbo Code class
///
/// A Turbo Code made of \f$N_g\f$ recursive systematic convolutional generators, each fed with an interleaved copy of the
/// \f$N_I\f$-bit input message. The codeword is the input message followed by the punctured redundancy of each generator
/// (see the notation in the TurboCodes page).
///
/// Unlike a textbook Turbo decoder for a fixed channel, the channel model is given per codeword bit: \ref m_symbolIndexes says
/// which received symbol carries each codeword bit, and \ref m_pdfs0 / \ref m_pdfs1 give the pdf of that symbol conditioned on the
/// bit value. This is what lets the fuzzy extractors map codeword bits onto biometric features with different reliabilities
/// and different modulations (PSK for code-offset features, 2-PAM for fuzzy-commitment features).
///
/// Typical use:
/// -# \ref setup (or \ref ::parseTurboCodeConfigFile) to define generators, puncturing patterns and interleavers;
/// -# \ref encode a message;
/// -# \ref setSymbolIndexes, \ref setPdfs0 and \ref setPdfs1 to describe the channel;
/// -# \ref decode the received symbols.
///
/// \author Enrique Argones Rúa
/// \date 2025
class TURBOCODES_EXPORT TurboCode
{
public:
  /// Public constructor.
  TurboCode();
  /// Copy constructor.
  /// @param[in] copyFrom Turbo Code to copy. Dynamically allocated decoding buffers are re-allocated, not shared.
  TurboCode(const TurboCode &copyFrom);
  /// Assignment operator.
  /// @param[in] assignFrom Turbo Code to copy.
  /// @return Reference to this object.
  TurboCode & operator=(const TurboCode &assignFrom);
  /// Destructor. It releases all memory the object may have allocated.
  ~TurboCode();
  /// Equality operator.
  /// @param[in] tc Turbo Code to compare with.
  /// @return true if both codes have the same generators (both polynomials of every generator). Puncturing patterns,
  /// interleavers and channel pdfs are not compared.
  bool operator == (const TurboCode &tc) const;
  /// Inequality operator.
  /// @param[in] tc Turbo Code to compare with.
  /// @return Negation of \ref operator==.
  bool operator != (const TurboCode &tc) const;
  /// Getter of the generators \ref m_generators
  /// @return Vector of polynomial pairs (see \ref binary_polynomial_pair), one per generator.
  const binary_polynomial_pair_vector &generators() const;
  /// Setter of the generators \ref m_generators. It also updates \ref m_codeMemorySize and rebuilds the forward transition matrix.
  /// @param[in] newGenerator Vector of polynomial pairs (see \ref binary_polynomial_pair), one per generator.
  void setGenerators(const binary_polynomial_pair_vector &newGenerator);
  /// Getter of the code memory sizes vector \ref m_codeMemorySize
  /// @return Memory \f$D_g\f$ of each generator.
  const uint_vector &codeMemorySize() const;
  /// Getter of the puncturing patterns \ref m_puncturingPatterns
  /// @return One binary vector of length \f$N_I\f$ per generator.
  const binary_polynomial_vector &puncturingPatterns() const;
  /// Setter of the puncturing patterns \ref m_puncturingPatterns. The length of the patterns sets the input length \f$N_I\f$, and
  /// the output length and redundancy lengths are updated accordingly.
  /// @param[in] newPuncturingPattern One binary vector per generator, all of the same length.
  void setPuncturingPatterns(const binary_polynomial_vector &newPuncturingPattern);
  /// Getter of the interleavers \ref m_interleavers
  /// @return One permutation of \f$\{0,\ldots,N_I-1\}\f$ per generator.
  const uint_vector_vector &interleavers() const;
  /// Setter of the interleavers \ref m_interleavers. The deinterleavers are rebuilt.
  /// @param[in] newInterleaver One permutation of \f$\{0,\ldots,N_I-1\}\f$ per generator.
  void setInterleavers(const uint_vector_vector &newInterleaver);
  /// Setter of the GMM vector conditioned to codeword bits 0 \ref m_pdfs0
  /// @param[in] newPdfs0 One GMM per codeword bit (\f$N_O\f$ elements).
  void setPdfs0(const GMM_vector &newPdfs0);
  /// Getter of the GMM vector conditioned to codeword bits 0 \ref m_pdfs0
  /// @return One GMM per codeword bit.
  const GMM_vector &pdfs0() const;
  /// Getter of the GMM vector conditioned to codeword bits 1 \ref m_pdfs1
  /// @return One GMM per codeword bit.
  const GMM_vector &pdfs1() const;
  /// Setter of the GMM vector conditioned to codeword bits 1 \ref m_pdfs1
  /// @param[in] newPdfs1 One GMM per codeword bit (\f$N_O\f$ elements).
  void setPdfs1(const GMM_vector &newPdfs1);
  /// Getter of the forward transition matrix \ref m_forwardTransitionMatrix
  /// @return Trellis transitions indexed as [generator][state][input bit].
  const code_transition_matrix &fowardTransitionMatrix() const;
  /// Getter of the redundancy indexes \ref m_redundancyIndexes
  /// @return For each generator, a map from input time index to the codeword position of its (non-punctured) redundancy bit.
  const uint_uint_map_vector &redundancyIndexes() const;
  /// This is a convenience code setup method. It is equivalent to invoking setGenerator, setPuncturingPattern, and setInterleaver in this order.
  /// @param[in] generators  vector of binary polynomials, each one being one generator.
  /// @param[in] puncturingPatterns  vector puncturing patterns, one for the redundancy of each generator.
  /// @param[in] interleavers  vector of \f$\{\textnormal{m\_inputLength}\}\f$ permutations, one for each generator.
  void setup(const binary_polynomial_pair_vector &generators,
             const binary_polynomial_vector &puncturingPatterns,
             const uint_vector_vector &interleavers);
  /// Method returning the input and output lengths as a pair of integers. The first one is the input length, and the second one is the output length.
  /// @return (\f$N_I\f$, \f$N_O\f$). The code rate is \f$N_I/N_O\f$.
  /// @exception std::logic_error if the generators or the puncturing patterns are not set.
  const uint_uint_pair inputOutputLengths() const;

  /// Encoding method.
  /// @param[in] input Message \f$\mathbf{b}\f$ of length \f$N_I\f$.
  /// @return Systematic codeword \f$\mathbf{o} = \left(\mathbf{b}, \mathbf{r}_0, \ldots, \mathbf{r}_{N_g-1}\right)\f$ of length \f$N_O\f$.
  /// @exception std::logic_error if the length of \p input differs from \f$N_I\f$.
  const binary_polynomial encode(const binary_polynomial &input);
  /// Method to decode a symbol sequence (noted by \f$\mathbf{R} = \left(R_0, \ldots, R_{C-1}\right)\f$) into its most likely binary input sequence (noted as
  /// \f$\mathbf{\hat{b}} = \left(\hat{b}_0, \ldots, \hat{b}_{N_I-1}\right)\f$
  /// @param[in] R Vector of vectors of real numbers. The \f$c^{\textrm{th}}\f$ element of \p R is \f$\textsc{R}[c] = R_c = \left[R_{c,0}, \ldots, R_{c,D^c-1}\right] \f$, the
  /// \f$c^{\textrm{th}}\f$ received symbol. Dimensionality of this symbol \f$R_c\f$ has to match the dimensionality of the \ref m_pdfs0 and \ref m_pdfs1 at the
  /// positions \f$t\f$ where \ref m_symbolIndexes \f$[t] = c\f$.
  /// @param[in] maxIt is the maximum number of decoding iterations. In each decoding iteration a soft-decoding occurs for each generator.
  /// @param[out] decodedBits is the decoded binary string.
  /// @param[out] it is the number of iterations until the decoder converges.
  void decode(const real_number_vector_vector &R, const unsigned int maxIt, binary_polynomial &decodedBits, unsigned int &it);
  /// Method to decode an array of symbol sequences (noted by \f$\bar{\mathbf{R}} = \left[\mathbf{R}^0, \ldots, \mathbf{R}^{S-1}\right]\f$,
  /// with \f$\mathbf{R}^i = \left(R^i_0, \ldots, R^i_{C-1}\right)\f$, where \f$S\f$ denotes the number of input symbol sequences) into its most likely binary input sequences (noted as
  /// \f$\bar{\mathbf{\hat{b}}} = \left[\mathbf{\hat{b}}^0, \ldots, \mathbf{\hat{b}}^{S-1}\right]\f$, with \f$\mathbf{\hat{b}}^i = \left(\hat{b}^i_0, \ldots, \hat{b}^i_{N_I-1}\right)\f$
  /// @param[in] R Vector of vector of vectors of real numbers. The \f$i^{\textrm{th}}\f$ element of \p R is \f$\textsc{R}[i] = \mathbf{R}^i = \left[R^i_{0}, \ldots, R^i_{c-1}\right] \f$, the
  /// \f$i^{\textrm{th}}\f$ received sequence of symbols. Dimensionality of symbols \f$R^i_c\f$ has to match the dimensionality of the \ref m_pdfs0 and \ref m_pdfs1 at the
  /// positions \f$t\f$ where \ref m_symbolIndexes \f$[t] = c\f$.
  /// @param[in] maxIt is the maximum number of decoding iterations. In each decoding iteration a soft-decoding occurs for each generator.
  /// @param[out] decodedBits is the decoded binary string.
  /// @param[out] it a vector for the number of iterations until the decoder converges for each element or \p R .
  void decode(const std::vector<real_number_vector_vector> &R, const unsigned int maxIt, binary_polynomial_vector &decodedBits, std::vector<unsigned int> &it);
  /// Method to generate the forward transition matrices
  void buildForwardTransitionMatrix();

  /// Getter of \ref m_symbolIndexes
  /// @return For each codeword bit, the index of the received symbol that carries it.
  const uint_vector &symbolIndexes() const;
  /// Setter of \ref m_symbolIndexes
  /// @param[in] newSymbolIndexes For each codeword bit (\f$N_O\f$ elements), the index of the received symbol that carries it.
  void setSymbolIndexes(uint_vector newSymbolIndexes);


private:
  /// Method to release the redundancy lengths.
  void releaseRedundancyLengths();
  /// Method to release m_codeAlphaMatrix
  void releaseAlphaMatrix();
  /// Method to release m_codeBetaMatrix
  void releaseBetaMatrix();
  /// Method to release m_codeGammaMatrix
  void releaseGammaMatrix();
  /// Method to release m_symbolPdfEvaluations
  void releaseSymbolPdfEvaluations();
  /// Initializer of \ref m_inputLogLikelihoodRatio
  void initializeInputLogLikelihoodRatio();
  /// Method to update the output lengths and the redundancy lengths.
  void updateOutputAndRedundancyLengths();
  /// Method to compute the deinterleavers permutations.
  void buildDeinterleavers();
  /// Method to compute the redundancy indexes. It is called when the puncturing pattern is set.
  void buildRedundancyIndexes();
  /// Method to compute the evaluation of the conditional symbol PDFs \ref m_symbolPdfEvaluations
  /// @param[in] symbols is an array of real numbers. Its size depends on the dimensionality of the symbols, which is set when the \ref m_pdfs0 and \ref m_pdfs1 are set.
  void computeSymbolPdfEvaluations(const real_number_vector_vector &symbols);
  /// Method to compute the gamma matrix of a generator
  /// @param[in] generatorIndex is the index of the generator polynomial defining the code.
  /// @param[in] aPrioriLogLikelihood is the a-priori information, which is the extrinsic loglikelihood ratio available for the inputs. At the begining, it is
  /// just a vector containing 0s, but after a code's extrinsic loglikelihood ratio is available, it constitutes the a-priori information.
  ///
  /// It is assumed that the symbol PDF have been previously evaluated by calling \ref computeSymbolPdfEvaluations(const real_number_vector_vector &symbols);
  ///
  /// By calling this method, \ref m_codeGammaMatrix[\p generatorIndex] and \ref m_intrinsicInputLogLikelihoodRatio[\p generatorIndex] are computed.
  void computeGammaMatrix(const unsigned int generatorIndex, const real_number_vector &aPrioriLogLikelihood);
  /// Method to compute the alpha matrix of a generator
  /// @param[in] generatorIndex is the index of the generator polynomial defining the code.
  ///
  /// It is assumed that the gamma matrix from this generator has been previously computed by calling \ref computeGammaMatrix;
  void computeAlphaMatrix(const unsigned int generatorIndex);
  /// Method to compute the beta matrix of a generator
  /// @param[in]  generatorIndex is the index of the generator polynomial defining the code.
  ///
  /// It is assumed that the gamma matrix from this generator has been previously computed by calling \ref computeGammaMatrix;
  void computeBetaMatrix(const unsigned int generatorIndex);
  /// Method to compute the input bits loglikelihood for a generator
  /// @param[in]  generatorIndex is the index of the generator polynomial defining the code.
  ///
  /// It is assumed that the \ref m_codeGammaMatrix[\p generatorIndex],  \ref m_codeAlphaMatrix[\p  generatorIndex], and
  /// \ref m_codeBetaMatrix[\p  generatorIndex] matrices from this generator have been previously computed by calling \ref computeGammaMatrix,
  /// \ref computeAlphaMatrix, and \ref computeBetaMatrix.
  ///
  /// By calling this method, \ref m_extrinsicInputLogLikelihoodRatio[\p generatorIndex] and \ref  m_inputLogLikelihoodRatio[\p  generatorIndex] are computed.
  void computeLogLikelihoodVector(const unsigned int generatorIndex);
  /// Method to compare the signs of the log-likelihoods of the input bits stored in \ref m_inputLogLikelihoodRatio at the end of each decoding iteration.
  /// @return true if all signs agree (the decoder has converged), false otherwise.
  bool decodingConverged();

  /// @name Debugging helpers
  /// Print the internal decoder state to standard output.
  /// @{

  /// Prints \ref m_codeGammaMatrix for one generator.
  /// @param[in] generatorIndex Index of the generator.
  void printGammaMatrix(const unsigned int generatorIndex) const;
  /// Prints \ref m_codeAlphaMatrix for one generator.
  /// @param[in] generatorIndex Index of the generator.
  void printAlphaMatrix(const unsigned int generatorIndex) const;
  /// Prints \ref m_codeBetaMatrix for one generator.
  /// @param[in] generatorIndex Index of the generator.
  void printBetaMatrix(const unsigned int generatorIndex) const;
  /// Prints the a-posteriori state transition log-likelihoods (\f$\lambda\f$) for one generator.
  /// @param[in] generatorIndex Index of the generator.
  void printLambdaMatrix(const unsigned int generatorIndex) const;
  /// Prints \ref m_symbolPdfEvaluations.
  void printSymbolPdfEvaluations() const;
  /// Prints the input log-likelihood ratios for one generator.
  /// @param[in] generatorIndex Index of the generator.
  void printLogLikelihoodVector(const unsigned int generatorIndex);
  /// @}

private:
  /// Generator matrix as a vector of polynomials
  binary_polynomial_pair_vector m_generators;
  /// Memory of the code, which for each code equals the maximum degree of the feedforward and feedback polynomials. Updated when the generators are set.
  /// The total number of states is then \f$N_S = 2^D\f$.
  uint_vector m_codeMemorySize;
  /// Puncturing patterns, with as many elements (binary vectors) as generator polynomials. The length of each puncturing vector
  /// is the length of the input message of the TurboCode.
  binary_polynomial_vector m_puncturingPatterns;
  /// Input length
  /// Both input length and output length are updated when the puncturing patterns are set.
  unsigned int m_inputLength;
  /// Output length. It is equal to sum of the Hamming weights of the puncturing pattern plus the length of the input.
  /// Both input length and output length are updated when the puncturing patterns are set.
  unsigned int m_outputLength;
  /// Length of the redundancy generated by each of the generator. It is computed when the puncturing patterns are set.
  unsigned int *m_redundancyLengths;
  /// Interleavers. There are as many interleavers as generators (the input to the first generator is also permutted for avoiding corner cases).
  /// \f[
  /// X^g_t = b_{\textsc{m\_interleaver}s[g][t-1]}
  /// \f]
  /// Notice that since m_interleaver is a std::vector, it is indexed from 0 instead of 1, but time index in \f$X\f$ starts at \f$t=1\f$.
  uint_vector_vector m_interleavers;
  /// The forward transition matrix is used to get the future state and redundancy bit for all the generators, given the current input bit and state
  /// \f[
  ///    F^g_{j,s'} = \left(s, r\right)
  /// \f]
  /// Where \f$ s = S_{t} \mid S_{t-1} = s', X_t = j \f$, and \f$r = Y^g_t \mid S_{t-1} = s', X_t = j \f$
  /// where \f$ Y^g_t \f$ is the redundancy bit generated by generator \f$g\f$ at time \f$t\f$ when \f$S_{t-1} = s'\f$ and \f$S_{t-1} = s'\f$.
  ///
  /// The forward transition matrix is structured as a vector (indexing the generators) of vectors (indexing the current state) of vectors (indexing
  /// the input bit values) of pairs containing the next state and redundancy bit. Therefore, we have:
  /// \f[ F^g_{j,s'} = \textnormal{m\_forwardTransitionMatrix}[g][s'][j] = \textnormal{state\_bit\_pair}\left(s, r\right) \f]
  code_transition_matrix m_forwardTransitionMatrix;
  /// The symbol indexes \ref m_symbolIndexes is a vector of length \ref m_outputLength where \f$\textsf{m\_symbolIndexes}[i]\f$ contains the index of the symbol where the bit
  /// \f$o_{i}\f$ is encoded.
  uint_vector m_symbolIndexes;
  /// This encodes the indexes in the output of the Turbo codeword related to each redundancy bit for each generator. Punturing makes not all redundancy bits have an associated index.
  uint_uint_map_vector m_redundancyIndexes;
  /// Symbol probability density functions of the received symbols conditioned to \f$o_t = 0\f$, where \f$o_t\f$ is the \f$t\textnormal{-th}\f$ bit in \f$\mathbf{o}\f$, the transmitted codeword.
  ///
  /// If we denote the received symbol sequence as \f$R_1, \ldots, R_C\f$, then \f$\textsf{m\_pdfs0}[i] = \left(k,f_{R_k \mid o_t=0}(R)\right)\f$ is a
  /// pair of unsigned int and GMM, where \f$k \in \{1, \ldots, C\}\f$ is the received symbol index where \f$o_t\f$ is encoded, and \f$f_{R_k \mid o_t=0}(R)\f$ is the
  /// Gaussian Mixture Model fitting the probability density function of \f$R_k \mid o_t = 0\f$.
  ///
  /// The number of elements in this vector must be the same as the number of bits in a codeword.
  GMM_vector m_pdfs0;
  /// Symbol probability density functions conditioned to input \f$o_t = 1\f$, where \f$o_t\f$ is the \f$t\textnormal{-th}\f$ bit in \f$\mathbf{o}\f$, the transmitted codeword.
  ///
  /// If we denote the received symbol sequence as \f$R_1, \ldots, R_C\f$, then \f$\textsf{m\_pdfs1}[i] = \left(k,f_{R_k \mid o_t=1}(R)\right)\f$ is a
  /// pair of unsigned int and GMM, where \f$k \in \{1, \ldots, C\}\f$ is the received symbol index where \f$o_t\f$ is encoded, and \f$f_{R_k \mid o_t=1}(R)\f$ is the
  /// Gaussian Mixture Model fitting the probability density function of \f$R_k \mid o_t = 1\f$.
  ///
  /// The number of elements in this vector must be the same as the number of bits in a codeword.
  GMM_vector m_pdfs1;
  /// The Gamma matrices encode the log-likelihood of the symbols involved in the encoding of the input and redundancy bit for a given generator,
  /// time \f$t\f$, input bit \f$X_t = x\f$, current state \f$S_t = s\f$ and previous state \f$S_{t-1} = s'\f$. It can be noteis noted as \f${\gamma'}^g_{t,x}(s',s)\f$
  ///
  /// The size of the gamma matrix is not \f$2N_g N_I N_S N_S\f$, since there is only one valid \f$S_t\f$ given \f$S_{t-1}\f$ and \f$X_t\f$. Therefore we
  /// have a vector-like structure \f$\textrm{m\_codeGammaMatrix}[g][t][s'][x]\f$. The value of \f$S_t\f$ is easily derived from \f$s'\f$ and \f$x\f$, as
  /// \f$s = \left(S_{t} \mid S_{t-1} = s', X_{t} = x\right) = F^g_{x,s}[1] = x  \cdot 2^{D-1} + \lfloor\frac{s'}{2}\rfloor\f$.
  ///
  /// m_codeGammaMatrix is a pointer to pointers to real_number.
  ///
  /// Once computed, we have: \f[\textsf{m\_codeGammaMatrix}[g][t][s'][x] = *(*(\textsf{m\_codeGammaMatrix} + g) + t*(2*N_S) + 2*s' + x)\f]
  /// 
  /// The size of each of the \f$N_g\f$ real_number matrices pointed by \f$*(\textsf{m\_codeGammaMatrix} + g)\f$ is \f$N_I \times N_S \times 2\f$.
  real_number **m_codeGammaMatrix;
  /// The Alpha matrices encode the log-likelihood \f$\mathrm{ln}\left[\alpha^g_t(s)\right] = \mathrm{L}(S^g_t=s,\mathbf{d}^g_{1,t})\f$.
  ///
  /// The vector-like structure of m_codeAlphaMatrix is \f$\textsf{m\_codeAlphaMatrix}[g][t][s]\f$.
  ///
  /// m_codeAlphaMatrix is a pointer to \f$N_g\f$ pointers to real_number, one for each generator.
  /// Once computed, we have: \f[\textsf{m\_codeAlphaMatrix}[g][t][s] = *(*(\textsf{m\_codeAlphaMatrix} + g) + t*N_S + s)\f]
  ///
  /// The size of each of the \f$N_g\f$ real_number matrices pointed by \f$\textsf{m\_codeAlphaMatrix}\f$ is \f$N_S N_I\f$.
  ///
  real_number **m_codeAlphaMatrix;
  /// The Beta matrices encode the log-likelihood \f$\mathrm{ln}\left[\beta^g_t(s)\right] = \mathrm{L}(\mathbf{d}^g_{t+1,N_I} \mid S^g_t=s)\f$.
  ///
  /// The vector-like structure of m_codeAlphaMatrix is \f$\textsf{m\_codeBetaMatrix}[g][t][s]\f$.
  ///
  /// m_codeBetaMatrix is a pointer to \f$N_g\f$ pointers to real_number, one for each generator.
  /// Once computed, we have: \f[\textsf{m\_codeBetaMatrix}[g][t][s] = *(*(\textsf{m\_codeBetaMatrix} + g) + t*N_S + s)\f]
  ///
  /// The size of each of the \f$N_g\f$ real_number matrices pointed by \f$\textsf{m\_codeBetaMatrix}\f$ is \f$N_S N_I\f$.
  ///
  real_number **m_codeBetaMatrix;
  /// The symbol pdf evaluations encode the values of the pdf evaluations for each output bit value.
  ///
  /// The vector-like structure of m_symbolPdfEvals is \f$\textsf{m\_symbolPdfEvaluations}[i][x]\f$
  ///
  /// m_symbolPdfEvaluations is a pointer to real_number. Once computed, we have:
  /// \f[
  /// \textsf{m\_symbolPdfEvaluations}[i][x] = *(\textsf{m\_symbolPdfEvaluations} + 2i + x)\,\textrm{,}
  /// \f]
  /// with \f$i \in \left\{0, N_O-1\right\}\f$ the index of the output bit \f$o_{i+1}\f$, and \f$x \in \{0,1\}\f$ the value of the bit.
  real_number *m_symbolPdfEvaluations;
  /// Log-Likelihood ratio vector of the input.
  real_number_vector_vector m_inputLogLikelihoodRatio;
  /// Intrinsic Log-Likelihood ratio vector of the input.
  real_number_vector_vector m_intrinsicInputLogLikelihoodRatio;
  /// Extrinsic Log-Likelihood ratio vector of the input.
  real_number_vector_vector m_extrinsicInputLogLikelihoodRatio;
};

#endif // TURBOCODE_H
