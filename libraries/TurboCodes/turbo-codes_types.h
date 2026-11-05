/// \file turbo-codes_types.h
/// \brief Common type aliases and numeric helper macros shared by the TurboCodes, HSTCFE and MHSTCFE libraries.
///
/// All containers used across the Strengthened Fuzzy Extractor software stack are plain STL containers. These aliases give
/// them names that reflect their role (binary polynomials, real-valued feature vectors, trellis transition matrices, ...).
/// \author Enrique Argones Rúa
/// \date 2025

#ifndef TURBOCODES_TYPES_H
#define TURBOCODES_TYPES_H

#include <vector>
#include <map>
#include <GMMLite/gmm.h> // Needed to define the GMM-dependent types.

/// Gaussian Mixture Model used by the SFE libraries: the minimal gmmlite::GMM class of the GMMLite library. The rest of the code
/// only uses this alias, so it does not depend on the GMM implementation.
typedef gmmlite::GMM GMM;

/// Binary polynomial / binary string. Element \f$i\f$ is the coefficient of \f$x^i\f$ (or bit \f$i\f$ of a message or codeword).
typedef std::vector<bool> binary_polynomial;
/// Vector of \ref binary_polynomial (e.g. one puncturing pattern per generator, or a batch of decoded messages).
typedef std::vector<binary_polynomial> binary_polynomial_vector;
/// Pair of binary polynomials describing a recursive systematic convolutional generator. In \ref TurboCode::buildForwardTransitionMatrix
/// the \c first polynomial gives the feedback taps of the shift register and the \c second one the parity (redundancy) taps.
typedef std::pair<binary_polynomial,binary_polynomial> binary_polynomial_pair;
/// Vector of \ref binary_polynomial_pair, i.e. the full set of generators of a Turbo Code.
typedef std::vector<binary_polynomial_pair> binary_polynomial_pair_vector;
/// Vector of unsigned integers (indexes, permutations, capacities, ...).
typedef std::vector<unsigned int> uint_vector;
/// Vector of \ref uint_vector (e.g. one interleaver permutation per generator).
typedef std::vector<uint_vector> uint_vector_vector;
/// Map from unsigned integer to unsigned integer.
typedef std::map<uint,uint> uint_uint_map;
/// Vector of \ref uint_uint_map (e.g. redundancy-bit to codeword-position maps, one per generator).
typedef std::vector<uint_uint_map> uint_uint_map_vector;
/// Floating point type used for every real-valued quantity (features, symbols, log-likelihoods).
typedef float real_number;
/// Vector of \ref real_number. Typically a biometric sample, a feature vector or a received symbol.
typedef std::vector<real_number> real_number_vector;
/// Vector of \ref real_number_vector. Typically a set of samples (one per row) or a sequence of received symbols.
typedef std::vector<real_number_vector> real_number_vector_vector;
/// (next state, output bit) pair describing one trellis transition.
typedef std::pair<unsigned int, bool> state_bit_pair;
/// Trellis transitions leaving one state, indexed by the input bit value (0 or 1).
typedef std::vector<state_bit_pair> transition_matrix_node;
/// Trellis transitions of one convolutional code, indexed by current state.
typedef std::vector<transition_matrix_node> transition_matrix;
/// Trellis transitions of every generator of a Turbo Code, indexed by generator.
typedef std::vector<transition_matrix> code_transition_matrix;
/// Pair of unsigned integers (e.g. Turbo Code input and output lengths).
typedef std::pair<uint,uint> uint_uint_pair;
/// Vector of Gaussian Mixture Models (gmmlite::GMM, see \ref GMM), e.g. one conditional pdf per codeword bit.
typedef std::vector<GMM> GMM_vector;
/// (index, GMM) pair.
typedef std::pair<unsigned int, GMM> index_GMM_pair;
/// Vector of \ref index_GMM_pair.
typedef std::vector<index_GMM_pair> index_GMM_pair_vector;


/// Maximum of two values.
#define MAX(a,b) ((a<b)?(b):(a))
#ifndef _MIN_MATH_CONSTS
/// Smallest positive value considered non-zero by \ref LOG.
#define MIN_POSITIVE 7.12457640674129e-218
/// Value returned by \ref LOG for arguments below \ref MIN_POSITIVE (floor for log-likelihoods).
#define MIN_LOG -500
/// Include guard for \ref MIN_POSITIVE and \ref MIN_LOG.
#define _MIN_MATH_CONSTS
#endif
#ifndef _LOG_MACRO
/// Include guard for \ref LOG.
#define _LOG_MACRO
#include <cmath>
/// Safe natural logarithm: returns \ref MIN_LOG instead of \f$-\infty\f$ for arguments smaller than \ref MIN_POSITIVE.
#define LOG(a) ((a<MIN_POSITIVE)?(MIN_LOG):(std::log(a)))
#endif

#endif // TURBOCODES_TYPES_H
