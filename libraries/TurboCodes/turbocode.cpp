/// \file turbocode.cpp
/// \brief Implementation of the \ref TurboCode class (encoder and iterative MAP/BCJR decoder).

#include "turbocode.h"

#include <iostream>
#include <stdexcept>
#include <thread>
#include <cmath>

TurboCode::TurboCode() :
    m_generators(),
    m_codeMemorySize(),
    m_puncturingPatterns(),
    m_inputLength(0),
    m_outputLength(0),
    m_redundancyLengths(NULL),
    m_interleavers(),
    m_forwardTransitionMatrix(),
    m_symbolIndexes(),
    m_redundancyIndexes(),
    m_pdfs0(),
    m_pdfs1(),
    m_codeGammaMatrix(NULL),
    m_codeAlphaMatrix(NULL),
    m_codeBetaMatrix(NULL),
    m_symbolPdfEvaluations(NULL)
{
}

TurboCode::TurboCode(const TurboCode &copyFrom)  :
    m_redundancyLengths(NULL),
    m_codeGammaMatrix(NULL),
    m_codeAlphaMatrix(NULL),
    m_codeBetaMatrix(NULL),
    m_symbolPdfEvaluations(NULL)
{
    setup(copyFrom.generators(),
          copyFrom.puncturingPatterns(),
          copyFrom.interleavers());
    setSymbolIndexes(copyFrom.symbolIndexes());
    setPdfs0(copyFrom.pdfs0());
    setPdfs1(copyFrom.pdfs1());
}

TurboCode & TurboCode::operator=(const TurboCode &assignFrom) {
    m_redundancyLengths = NULL;
    m_codeGammaMatrix = NULL;
    m_codeAlphaMatrix = NULL;
    m_codeBetaMatrix = NULL;
    m_symbolPdfEvaluations = NULL;
    setup(assignFrom.generators(),
          assignFrom.puncturingPatterns(),
          assignFrom.interleavers());
    setSymbolIndexes(assignFrom.symbolIndexes());
    setPdfs0(assignFrom.pdfs0());
    setPdfs1(assignFrom.pdfs1());
    return *this;
}

TurboCode::~TurboCode()
{
    releaseRedundancyLengths();
    releaseAlphaMatrix();
    releaseBetaMatrix();
    releaseGammaMatrix();
    releaseSymbolPdfEvaluations();
}

bool TurboCode::operator ==(const TurboCode &tc) const
{
    binary_polynomial_pair_vector tc_generators = tc.generators();
    if (m_generators.size() != tc_generators.size()) {
        return false;
    } else {
        for (unsigned polyI = 0; polyI < m_generators.size(); polyI++) {
            if (tc_generators[polyI].first.size() != m_generators[polyI].first.size()) {
                return false;
            } else {
                for (unsigned int coeffI = 0; coeffI < m_generators[polyI].first.size(); coeffI++) {
                    if (tc_generators[polyI].first[coeffI] != m_generators[polyI].first[coeffI]) {
                        return false;
                    }
                }
            }
            if (tc_generators[polyI].second.size() != m_generators[polyI].second.size()) {
                return false;
            } else {
                for (unsigned int coeffI = 0; coeffI < m_generators[polyI].second.size(); coeffI++) {
                    if (tc_generators[polyI].second[coeffI] != m_generators[polyI].second[coeffI]) {
                        return false;
                    }
                }
            }
        }
        return true;
    }
}

bool TurboCode::operator !=(const TurboCode &tc) const
{
    return !(*this == tc);
}

void TurboCode::releaseRedundancyLengths() {
    if (m_redundancyLengths != NULL) {
        delete [] m_redundancyLengths;
        m_redundancyLengths = NULL;
    }
}

void TurboCode::releaseAlphaMatrix()
{
    if (m_codeAlphaMatrix != NULL) {
        for (unsigned int gI = 0; gI < m_generators.size(); gI++) {
            if (*(m_codeAlphaMatrix + gI) != NULL) {
                delete [] *(m_codeAlphaMatrix + gI);
                *(m_codeAlphaMatrix + gI) = NULL;
            }
        }
        delete [] m_codeAlphaMatrix;
        m_codeAlphaMatrix = NULL;
    }
}

void TurboCode::releaseBetaMatrix()
{
    if (m_codeBetaMatrix != NULL) {
        for (unsigned int gI = 0; gI < m_generators.size(); gI++) {
            if (*(m_codeBetaMatrix + gI) != NULL) {
                delete [] *(m_codeBetaMatrix + gI);
                *(m_codeBetaMatrix + gI) = NULL;
            }
        }
        delete [] m_codeBetaMatrix;
        m_codeBetaMatrix = NULL;
    }
}

void TurboCode::releaseGammaMatrix()
{
    if (m_codeGammaMatrix != NULL) {
        for (unsigned int gI = 0; gI < m_generators.size(); gI++) {
            if (*(m_codeGammaMatrix + gI) != NULL) {
                delete [] *(m_codeGammaMatrix + gI);
                *(m_codeGammaMatrix + gI) = NULL;
            }
        }
        delete [] m_codeGammaMatrix;
        m_codeGammaMatrix = NULL;
    }
}

void TurboCode::releaseSymbolPdfEvaluations()
{
    if (m_symbolPdfEvaluations != NULL) {
        delete [] m_symbolPdfEvaluations;
        m_symbolPdfEvaluations = NULL;
    }
}

const binary_polynomial_pair_vector &TurboCode::generators() const
{
    return m_generators;
}

void TurboCode::setGenerators(const binary_polynomial_pair_vector &newGenerator)
{
    m_generators = newGenerator;
    m_codeMemorySize = std::vector<unsigned int>();
#ifdef DEBUG
    std::cout << "m_generator.size() = " << m_generators.size() << std::endl;
#endif
    for (unsigned int generatorI = 0; generatorI < m_generators.size(); generatorI++) {
        m_codeMemorySize.push_back(0);
#ifdef DEBUG
        std::cout << "m_generator[" << generatorI << "].first.size() = " << m_generators[generatorI].first.size() << std::endl;
#endif
        if (m_codeMemorySize[generatorI] < m_generators[generatorI].first.size() - 1) {
            m_codeMemorySize[generatorI] = m_generators[generatorI].first.size() - 1;
        }
#ifdef DEBUG
        std::cout << "m_generator[" << generatorI << "].second.size() = " << m_generators[generatorI].second.size() << std::endl;
#endif
        if (m_codeMemorySize[generatorI] < m_generators[generatorI].second.size() - 1) {
            m_codeMemorySize[generatorI] = m_generators[generatorI].second.size() - 1;
        }
    }
    for (unsigned int generatorI = 0; generatorI < m_generators.size(); generatorI++) {
        if (m_generators[generatorI].first.size() != m_codeMemorySize[generatorI] + 1) {
            m_generators[generatorI].first.insert(m_generators[generatorI].first.end(), m_codeMemorySize[generatorI] + 1 - m_generators[generatorI].first.size(), false);
        }
        if (m_generators[generatorI].second.size() != m_codeMemorySize[generatorI] + 1) {
            m_generators[generatorI].second.insert(m_generators[generatorI].second.end(), m_codeMemorySize[generatorI] + 1 - m_generators[generatorI].second.size(), false);
        }
#ifdef DEBUG
        std::cout << "m_codeMemorySize[generatorI] = " << m_codeMemorySize[generatorI] << std::endl;
#endif
    }
    buildForwardTransitionMatrix();
}

const uint_vector &TurboCode::codeMemorySize() const {
    return m_codeMemorySize;
}

const binary_polynomial TurboCode::encode(const binary_polynomial &input)
{
    const unsigned int generatorSize = m_generators.size();
    const unsigned int inputLength = input.size();
    if (inputLength != m_inputLength) {
        std::string exceptionMessage =
            std::string("TurboCode::encode: input length (") +
            std::to_string(inputLength) + std::string(" must be equal to ") + std::to_string(m_inputLength) + std::string(").");
        throw std::logic_error(exceptionMessage.c_str());
    }
    binary_polynomial output = input;
    for (unsigned int generatorI = 0; generatorI < generatorSize; generatorI++) {
        unsigned int currentState = 0;
        for (unsigned int inputBitI = 0; inputBitI < inputLength; inputBitI++) {
            unsigned int inputBit = input[m_interleavers[generatorI][inputBitI]]?1:0;
            state_bit_pair nextStateAndCurrentRedundancy = m_forwardTransitionMatrix[generatorI][currentState][inputBit];
            currentState = nextStateAndCurrentRedundancy.first;
            const bool redundancyBit = nextStateAndCurrentRedundancy.second;
            if (m_puncturingPatterns[generatorI][inputBitI]) {
                output.push_back(redundancyBit);
            }
        }
    }
    return output;
}

const binary_polynomial_vector &TurboCode::puncturingPatterns() const
{
    return m_puncturingPatterns;
}

void TurboCode::setPuncturingPatterns(const binary_polynomial_vector &newPuncturingPattern)
{
    if (newPuncturingPattern.size() != m_generators.size()) {
        std::string exceptionMessage = std::string("TurboCode::setPuncturingPattern: puncturing pattern contains ") +
                                       std::to_string(newPuncturingPattern.size()) + std::string(" binary vectors, but it must contain as many puncturing vectors as polynomials in m_generator (") +
                                       std::to_string(m_generators.size()) + std::string(").");
        throw std::logic_error(exceptionMessage.c_str());
    }
    unsigned int L = newPuncturingPattern[0].size();
    for (unsigned int generatorI = 1; generatorI < m_generators.size(); generatorI++) {
        if (newPuncturingPattern[generatorI].size() != L) {
            throw std::logic_error("TurboCode::setPuncturingPattern: all puncturing vectors must have equal size.");
        }
    }
    m_puncturingPatterns = newPuncturingPattern;
    m_inputLength = L;
    updateOutputAndRedundancyLengths();
    // The positions of the redundancy bits in the codeword depend on the puncturing patterns: they must be rebuilt
    // whenever the patterns change, not only when the interleavers are set. Otherwise an object punctured after
    // setInterleavers() (as HybridStrengthenedTurboCodeFuzzyExtractor::generate() does) decodes with stale indexes,
    // reading wrong redundancy bits and past the end of m_symbolPdfEvaluations.
    buildRedundancyIndexes();
}

const uint_uint_pair TurboCode::inputOutputLengths() const
{
    if (m_generators.size() == 0) {
        throw std::logic_error("TurboCode::codeRate: Code rate cannot be established since the generators are not initialized.");
    }
    if (m_puncturingPatterns.size() == 0) {
        throw std::logic_error("TurboCode::codeRate: Code rate cannot be established since the puncturing pattern is not initialized.");
    }
    return std::pair<unsigned int, unsigned int>(m_inputLength, m_outputLength);
}

void TurboCode::updateOutputAndRedundancyLengths()
{
    unsigned int generatorRedundancyLength(0), totalRedundancyLength(0);
    releaseRedundancyLengths();
    m_redundancyLengths = new unsigned int[m_generators.size()];
    for (unsigned int generatorI = 0; generatorI < m_puncturingPatterns.size(); generatorI++) {
        generatorRedundancyLength = 0;
        for (unsigned int puncturingPatternBitI = 0; puncturingPatternBitI < m_puncturingPatterns[generatorI].size(); puncturingPatternBitI++) {
            if (m_puncturingPatterns[generatorI][puncturingPatternBitI]) generatorRedundancyLength++;
        }
        *(m_redundancyLengths + generatorI) = generatorRedundancyLength;
        totalRedundancyLength += generatorRedundancyLength;
    }
    m_outputLength = m_inputLength + totalRedundancyLength;
}

void TurboCode::computeSymbolPdfEvaluations(const real_number_vector_vector &symbols)
{
#ifdef DEBUG
    std::cout << "Evaluating symbol pdfs." << std::endl;
#endif
    releaseSymbolPdfEvaluations();
    m_symbolPdfEvaluations = new real_number[2*m_outputLength];

    for (unsigned int nI = 0; nI < 2*m_outputLength; nI++) {
        *(m_symbolPdfEvaluations + nI) = 0.0;
    }
    for (unsigned int tI = 0; tI < m_outputLength; tI++) {
        unsigned int symbolI = m_symbolIndexes[tI];
        unsigned int symbolLength = m_pdfs0[tI].getDimension();
        // Exception throwing...
        if (symbols[symbolI].size() != symbolLength) {
            std::string exceptionMessage =
                std::string("TurboCode::computeSymbolPdfEvaluations: the dimensionality of symbols[") +
                std::to_string(symbolI) + std::string(") is not the same as the dimensionality m_pdfs0[") +
                std::to_string(tI) + std::string("].getDimension() = ") + std::to_string(symbolLength) + std::string(", where ") +
                std::to_string(symbolI) + std::string(" is m_symbolIndexes[") + std::to_string(tI) + std::string("].");
            throw std::logic_error(exceptionMessage.c_str());
        }
        if (m_pdfs1[tI].getDimension() != symbolLength) {
            std::string exceptionMessage =
                std::string("TurboCode::computeSymbolPdfEvaluations: the dimensionality m_pdfs0[") +
                std::to_string(tI) + std::string("].getDimension() = ") + std::to_string(m_pdfs0[tI].getDimension()) +
                std::to_string(symbolI) + std::string(" is not the same as the dimensionality m_pdfs1[") +
                std::to_string(tI) + std::string("].getDimension() = ") + std::to_string(m_pdfs0[tI].getDimension()) + std::string(".");
            throw std::logic_error(exceptionMessage.c_str());
        }
#ifdef DEBUG
        std::cout << "t = " << tI << "; symbol index = " << symbolI << ": ";
#endif
        // Symbol memory allocation. Alternative to dynamic allocation (a large enough buffer) would speed up this function.
        real_number *symbolNumbers = new real_number[symbolLength];
        for (unsigned int numberI = 0; numberI < symbolLength; numberI++) {
            *(symbolNumbers + numberI) = symbols[symbolI][numberI];
#ifdef DEBUG
            std::cout << *(symbolNumbers + numberI) << "; ";
#endif
        }
        *(m_symbolPdfEvaluations + tI*2) = m_pdfs0[tI].logLikelihood(symbolNumbers);
        *(m_symbolPdfEvaluations + tI*2 + 1) = m_pdfs1[tI].logLikelihood(symbolNumbers);
#ifdef DEBUG
        std::cout << "pdf0 = " << *(m_symbolPdfEvaluations + tI*2) << "; ";
        std::cout << "pdf1 = " << *(m_symbolPdfEvaluations + tI*2 + 1) << std::endl;
#endif
        delete [] symbolNumbers;
    }
}

void TurboCode::buildRedundancyIndexes() {
    m_redundancyIndexes = uint_uint_map_vector(m_generators.size(), uint_uint_map());
    unsigned int offset = m_inputLength;
    for (unsigned int gI = 0; gI < m_generators.size(); gI++) {
        for (unsigned int iI = 0; iI < m_inputLength; iI++) {
            if (m_puncturingPatterns[gI][iI]) {
                m_redundancyIndexes[gI][iI] = offset;
                offset++;
            }
        }
    }
}

void TurboCode::computeGammaMatrix(const unsigned int generatorIndex, const real_number_vector &aPrioriLogLikelihood)
{
    if (m_codeGammaMatrix == NULL) {
        m_codeGammaMatrix = new real_number*[m_generators.size()];
        for (unsigned int i=0; i<m_generators.size(); i++) {
            *(m_codeGammaMatrix + i) = NULL;
        }
    }
    unsigned int statesCount = (1 << m_codeMemorySize[generatorIndex]);
    if (*(m_codeGammaMatrix + generatorIndex) == NULL) {
        *(m_codeGammaMatrix + generatorIndex) = new real_number[m_inputLength*2*statesCount];
    }
#ifdef DEBUG
    std::cout << "Computing Gamma matrix for generator " << generatorIndex << std::endl;
#endif
    for (unsigned int tI = 0; tI < m_inputLength; tI++) {
        const unsigned int inputBitIndexInOutput = m_interleavers[generatorIndex][tI];
        const real_number intrinsicLLK0 = *(m_symbolPdfEvaluations + inputBitIndexInOutput*2);
        const real_number intrinsicLLK1 = *(m_symbolPdfEvaluations + inputBitIndexInOutput*2 + 1);
        real_number aPrioriLLK0, aPrioriLLK1;
        const real_number aPrioriLLKR = aPrioriLogLikelihood[inputBitIndexInOutput];
        if (aPrioriLLKR > 0.0) {
            aPrioriLLK0 = -LOG(1.0 + std::exp(-aPrioriLLKR));
            aPrioriLLK1 = -aPrioriLLKR - LOG(1.0 + std::exp(-aPrioriLLKR));
        } else {
            aPrioriLLK0 = aPrioriLLKR - LOG(1.0 + std::exp(aPrioriLLKR));
            aPrioriLLK1 = -LOG(1.0 + std::exp(aPrioriLLKR));
        }
        m_intrinsicInputLogLikelihoodRatio[generatorIndex][inputBitIndexInOutput] = intrinsicLLK0 - intrinsicLLK1 + aPrioriLLK0;
        for (unsigned int previousState = 0; previousState < statesCount; previousState++) {
            if ((tI == 0) && (previousState != 0)) {
                for (unsigned int previousStateI = 1; previousStateI < statesCount; previousStateI++) {
                    *(*(m_codeGammaMatrix + generatorIndex) + 2*(tI*statesCount + previousStateI)) = 3*MIN_LOG;
                    *(*(m_codeGammaMatrix + generatorIndex) + 2*(tI*statesCount + previousStateI) + 1) = 3*MIN_LOG;
                }
                break;
            }
            *(*(m_codeGammaMatrix + generatorIndex) + 2*(tI*statesCount + previousState)) = intrinsicLLK0 + aPrioriLLK0;
            *(*(m_codeGammaMatrix + generatorIndex) + 2*(tI*statesCount + previousState) + 1) = intrinsicLLK1 + aPrioriLLK1;
#ifdef DEBUG
            std::cout << "tI = " << tI << "; Previous state = " << previousState << "; "
                      << ", index " << inputBitIndexInOutput << "; "
                      << "X = 0 intrinsic: " << intrinsicLLK0 << "; "
                      << "aPriori: " << aPrioriLLK0 << "; "
                      << "X = 1 intrinsic: " << intrinsicLLK1 << "; "
                      << "aPriori: " << aPrioriLLK1;

#endif
            if (m_puncturingPatterns[generatorIndex][tI]) {
                const unsigned int redundancyBitIndexInOutput = m_redundancyIndexes[generatorIndex][tI];
                for (unsigned int bitInput = 0; bitInput < 2; bitInput++) {
                    unsigned int redundancyBitValue =
                        m_forwardTransitionMatrix[generatorIndex][previousState][bitInput].second;
                    *(*(m_codeGammaMatrix + generatorIndex) +
                      2*(tI*statesCount + previousState) +
                      bitInput) += *(m_symbolPdfEvaluations +
                             redundancyBitIndexInOutput*2 +
                             redundancyBitValue);
#ifdef DEBUG
                    std::cout << ", redundancy index " << redundancyBitIndexInOutput
                              << "; Y = " << redundancyBitValue << " intrinsic: "
                              << *(m_symbolPdfEvaluations + redundancyBitIndexInOutput*2 + redundancyBitValue) << "; ";
#endif
                }
#ifdef DEBUG
                std::cout << std::endl;
#endif
            }
        }
    }
}

void TurboCode::computeAlphaMatrix(const unsigned int generatorIndex)
{
    const unsigned int statesCount = (1 << m_codeMemorySize[generatorIndex]);
    if (m_codeAlphaMatrix == NULL) {
        m_codeAlphaMatrix = new real_number*[m_generators.size()];
        for (unsigned int i=0; i<m_generators.size(); i++) {
            *(m_codeAlphaMatrix + i) = NULL;
        }
    }
    if (*(m_codeAlphaMatrix + generatorIndex) == NULL) {
        *(m_codeAlphaMatrix + generatorIndex) = new real_number[m_inputLength*statesCount];
    }
    // The first iteration is done assuming alpha_{-1}(0) = 0, and the rest are MIN_LOG.
    for (unsigned int stateI = 0; stateI < statesCount; stateI++) {
        *(*(m_codeAlphaMatrix + generatorIndex) + stateI) = MIN_LOG;
    }
    real_number maxExponent = -INFINITY;
    for (unsigned int inputBit = 0; inputBit < 2; inputBit++) {
        const unsigned int currentState = m_forwardTransitionMatrix[generatorIndex][0][inputBit].first;
        *(*(m_codeAlphaMatrix + generatorIndex) + currentState) = *(*(m_codeGammaMatrix + generatorIndex) + inputBit);
        if (maxExponent < *(*(m_codeAlphaMatrix + generatorIndex) + currentState)) {
            maxExponent = *(*(m_codeAlphaMatrix + generatorIndex) + currentState);
        }
#ifdef DEBUG
        std::cout << "Alpha at tI = 0; state = 0" << currentState << " = \t"
                  << *(*(m_codeAlphaMatrix + generatorIndex) + currentState) << std::endl;
#endif
    }
#ifdef DEBUG
    std::cout << "tI = 0; \tmaxExponent = " << maxExponent << std::endl;
#endif
    for (unsigned int tI = 1; tI < m_inputLength; tI++) {
        maxExponent = -INFINITY;
        for (unsigned int inputBit = 0; inputBit < 2; inputBit++) {
            for (unsigned int previousState = 0; previousState < statesCount; previousState++) {
                const real_number currentExponent =
                    *(*(m_codeAlphaMatrix + generatorIndex) + (tI - 1)*statesCount + previousState) +
                    *(*(m_codeGammaMatrix + generatorIndex) + 2*(tI*statesCount + previousState) + inputBit);
                if (maxExponent < currentExponent) {
                    maxExponent = currentExponent;
                }
            }
        }
#ifdef DEBUG
        std::cout << "tI = " << tI << "; \tmaxExponent = " << maxExponent << std::endl;
#endif
        for (unsigned int currentState = 0; currentState < statesCount; currentState++) {
            *(*(m_codeAlphaMatrix + generatorIndex) + tI*statesCount + currentState) = 0.0;
        }
        for (unsigned int previousState = 0; previousState < statesCount; previousState++) {
            for (unsigned int inputBit = 0; inputBit < 2; inputBit++) {
                const unsigned int currentState = m_forwardTransitionMatrix[generatorIndex][previousState][inputBit].first;
                *(*(m_codeAlphaMatrix + generatorIndex) + tI*statesCount + currentState)  +=
                    std::exp(*(*(m_codeAlphaMatrix + generatorIndex) + (tI - 1)*statesCount + previousState) +
                             *(*(m_codeGammaMatrix + generatorIndex) + 2*(tI*statesCount + previousState) + inputBit) -
                             maxExponent);
#ifdef DEBUG
                std::cout << "Alpha Exponent for tI = " << tI <<"; currentState = " << currentState << " = \t"
                          << *(*(m_codeAlphaMatrix + generatorIndex) + (tI - 1)*statesCount + previousState) +
                                 *(*(m_codeGammaMatrix + generatorIndex) + 2*(tI*statesCount + previousState) + inputBit)
                          << std::endl;
#endif
            }
        }
        real_number maxExponentPrecursor = -INFINITY;
        for (unsigned int currentState=0; currentState < statesCount; currentState++) {
            *(*(m_codeAlphaMatrix + generatorIndex) + tI*statesCount + currentState) =
                LOG(*(*(m_codeAlphaMatrix + generatorIndex) + tI*statesCount + currentState)) +
                maxExponent;
#ifdef DEBUG
            std::cout << "Alpha at tI = " << tI << "; state = " << currentState << " = \t"
                      << *(*(m_codeAlphaMatrix + generatorIndex) + tI*statesCount + currentState) << std::endl;
#endif
            if (maxExponentPrecursor < *(*(m_codeAlphaMatrix + generatorIndex) + tI*statesCount + currentState)) {
                maxExponentPrecursor = *(*(m_codeAlphaMatrix + generatorIndex) + tI*statesCount + currentState);
            }
        }
        maxExponent = maxExponentPrecursor;
    }
}

void TurboCode::computeBetaMatrix(const unsigned int generatorIndex)
{
    if (m_codeBetaMatrix == NULL) {
        m_codeBetaMatrix = new real_number*[m_generators.size()];
        for (unsigned int i=0; i<m_generators.size(); i++) {
            *(m_codeBetaMatrix + i) = NULL;
        }
    }
    unsigned int statesCount = (1 << m_codeMemorySize[generatorIndex]);
    if (*(m_codeBetaMatrix + generatorIndex) == NULL) {
        *(m_codeBetaMatrix + generatorIndex) = new real_number[m_inputLength*statesCount];
    }
    // The first iteration (for \f$t = N_I - 1\f$) is done assuming that \f$\beta^g_{N_I}(s) = -\mathrm{ln}\left({N_S\right)\f$, and \f$\gamma_{N_I}(s,s') = 0\f$.
    real_number maxExponent = -INFINITY;
    for (unsigned int currentState = 0; currentState < statesCount; currentState++) {
        *(*(m_codeBetaMatrix + generatorIndex) + (m_inputLength - 1)*statesCount + currentState) =
            -LOG(statesCount);

    }
    for (int tI = m_inputLength - 2; tI >= 0; tI--) {
        maxExponent = -INFINITY;
        for (unsigned int inputBit = 0; inputBit < 2; inputBit++) {
            for (unsigned int currentState = 0; currentState < statesCount; currentState++) {
                unsigned int futureState = m_forwardTransitionMatrix[generatorIndex][currentState][inputBit].first;
                const real_number currentExponent =
                    *(*(m_codeBetaMatrix + generatorIndex) + (tI + 1)*statesCount + futureState) +
                    *(*(m_codeGammaMatrix + generatorIndex) + 2*((tI + 1)*statesCount + currentState) + inputBit);
                if (maxExponent < currentExponent) {
                    maxExponent = currentExponent;
                }
            }
        }
        for (unsigned int currentState = 0; currentState < statesCount; currentState++) {
            *(*(m_codeBetaMatrix + generatorIndex) + tI*statesCount + currentState) = 0.0;
        }

        for (unsigned int inputBit = 0; inputBit < 2; inputBit++) {
            for (unsigned int currentState = 0; currentState < statesCount; currentState++) {
                unsigned int futureState = m_forwardTransitionMatrix[generatorIndex][currentState][inputBit].first;
                *(*(m_codeBetaMatrix + generatorIndex) + tI*statesCount + currentState) +=
                    std::exp(*(*(m_codeBetaMatrix + generatorIndex) + (tI + 1)*statesCount + futureState) +
                             *(*(m_codeGammaMatrix + generatorIndex) + 2*((tI + 1)*statesCount + currentState) + inputBit) -
                             maxExponent);
            }
        }
        real_number maxExponentPrecursor = -INFINITY;
        for (unsigned int currentState = 0; currentState < statesCount; currentState++) {
            *(*(m_codeBetaMatrix + generatorIndex) + tI*statesCount + currentState) =
                LOG(*(*(m_codeBetaMatrix + generatorIndex) + tI*statesCount + currentState)) +
                maxExponent;
            if (maxExponentPrecursor < *(*(m_codeBetaMatrix + generatorIndex) + tI*statesCount + currentState)) {
                maxExponentPrecursor = *(*(m_codeBetaMatrix + generatorIndex) + tI*statesCount + currentState);
            }
        }
        maxExponent = maxExponentPrecursor;
    }
}

void TurboCode::computeLogLikelihoodVector(const unsigned int generatorIndex)
{
    const unsigned int statesCount = (1 << m_codeMemorySize[generatorIndex]);
    for (unsigned int tI=0; tI<m_inputLength; tI++) {
        real_number maxNumerator = -INFINITY;
        real_number maxDenominator = -INFINITY;
        real_number interleavedInputLogLikelihood;
        real_number numerator = 0;
        real_number denominator = 0;
        for (unsigned int inputBit = 0; inputBit<2; inputBit++) {
            for (unsigned int previousState = 0; previousState < statesCount; previousState++) {
                const unsigned int currentState = m_forwardTransitionMatrix[generatorIndex][previousState][inputBit].first;
                real_number currentContribution;
                if (tI == 0) {
                    if (previousState == 0) {
                        if (inputBit == 0) {
                            currentContribution =
                                *(*(m_codeGammaMatrix + generatorIndex)) +
                                *(*(m_codeBetaMatrix + generatorIndex) + currentState);
                            if (maxNumerator < currentContribution) {
                                maxNumerator = currentContribution;
                            }
                        } else {
                            currentContribution =
                                *(*(m_codeGammaMatrix + generatorIndex) + 1) +
                                *(*(m_codeBetaMatrix + generatorIndex) + currentState);
                            if (maxDenominator < currentContribution) {
                                maxDenominator = currentContribution;
                            }
                        }
                    } else {
                        if (inputBit == 0) {
                            currentContribution =
                                MIN_LOG +
                                *(*(m_codeGammaMatrix + generatorIndex) + 2*previousState) +
                                *(*(m_codeBetaMatrix + generatorIndex) + currentState);
                            if (maxNumerator < currentContribution) {
                                maxNumerator = currentContribution;
                            }
                        } else {
                            currentContribution =
                                MIN_LOG +
                                *(*(m_codeGammaMatrix + generatorIndex) + 2*previousState + 1) +
                                *(*(m_codeBetaMatrix + generatorIndex) + currentState);
                            if (maxDenominator < currentContribution) {
                                maxDenominator = currentContribution;
                            }
                        }
                    }
                } else {
                    if (inputBit == 0) {
                        currentContribution =
                            *(*(m_codeAlphaMatrix + generatorIndex) + (tI - 1)*statesCount + previousState) +
                            *(*(m_codeGammaMatrix + generatorIndex) + 2*(tI*statesCount + previousState)) +
                            *(*(m_codeBetaMatrix + generatorIndex) + tI*statesCount + currentState);
                        if (maxNumerator < currentContribution) {
                            maxNumerator = currentContribution;
                        }
                    } else {
                        currentContribution =
                            *(*(m_codeAlphaMatrix + generatorIndex) + (tI - 1)*statesCount + previousState) +
                            *(*(m_codeGammaMatrix + generatorIndex) + 2*(tI*statesCount + previousState) + 1) +
                            *(*(m_codeBetaMatrix + generatorIndex) + tI*statesCount + currentState);
                        if (maxDenominator < currentContribution) {
                            maxDenominator = currentContribution;
                        }
                    }
                }
#ifdef DEBUG
                std::cout << "tI = " << tI
                          << "; input bit = " << inputBit
                          << "; previousState = " << previousState
                          << "; currentState = " << currentState
                          << "; \tCurrent contribution: " << currentContribution;
                if (inputBit == 0) {
                    std::cout << "; \tMaximum numerator = " << maxNumerator << std::endl;
                } else {
                    std::cout << "; \tMaximum denominator = " << maxDenominator << std::endl;
                }
#endif
            }
        }
#ifdef DEBUG
        std::cout << "tI = " << tI << "; Final Maximum numerator = " << maxNumerator
                  << "; \tFinal Maximum denominator = " << maxDenominator << std::endl;
#endif
        for (unsigned int inputBit = 0; inputBit<2; inputBit++) {
            for (unsigned int previousState = 0; previousState < statesCount; previousState++) {
                real_number currentContribution;
                const unsigned int currentState = m_forwardTransitionMatrix[generatorIndex][previousState][inputBit].first;
                if (tI == 0) { // Special case: alpha_{-1} = [1, 0, ..., 0]
                    if (previousState == 0) {
                        if (inputBit == 0) {
                            currentContribution = *(*(m_codeGammaMatrix + generatorIndex)) +
                                                  *(*(m_codeBetaMatrix + generatorIndex) + currentState) - maxNumerator;
                        } else {
                            currentContribution = *(*(m_codeGammaMatrix + generatorIndex) + 1) +
                                                  *(*(m_codeBetaMatrix + generatorIndex) + currentState) - maxDenominator;
                        }
                    } else {
                        if (inputBit == 0) {
                            currentContribution = MIN_LOG +
                                                  *(*(m_codeGammaMatrix + generatorIndex) + 2*(tI*statesCount + previousState)) +
                                                  *(*(m_codeBetaMatrix + generatorIndex) + tI*statesCount + currentState) - maxNumerator;
                        } else {
                            currentContribution = MIN_LOG +
                                                  *(*(m_codeGammaMatrix + generatorIndex) + 2*(tI*statesCount + previousState) + 1) +
                                                  *(*(m_codeBetaMatrix + generatorIndex) + currentState) - maxDenominator;
                        }
                    }
                } else {
                    if (inputBit == 0) {
                        currentContribution = *(*(m_codeAlphaMatrix + generatorIndex) + (tI - 1)*statesCount + previousState) +
                                              *(*(m_codeGammaMatrix + generatorIndex) + 2*(tI*statesCount + previousState)) +
                                              *(*(m_codeBetaMatrix + generatorIndex) + tI*statesCount + currentState) - maxNumerator;
                    } else {
                        currentContribution = *(*(m_codeAlphaMatrix + generatorIndex) + (tI - 1)*statesCount + previousState) +
                                              *(*(m_codeGammaMatrix + generatorIndex) + 2*(tI*statesCount + previousState) + 1) +
                                              *(*(m_codeBetaMatrix + generatorIndex) + tI*statesCount + currentState) - maxDenominator;
                    }
                }
                if (inputBit == 0)
                    numerator += std::exp(currentContribution);
                else
                    denominator += std::exp(currentContribution);
#ifdef DEBUG
                std::cout << "tI = " << tI
                          << "; input bit = " << inputBit
                          << "; previousState = " << previousState
                          << "; currentState = " << currentState;
                if (inputBit == 0) {
                    std::cout << "; \tContribution to numerator = " << currentContribution << std::endl;
                } else {
                    std::cout << "; \tContribution to denominator = " << currentContribution << std::endl;
                }
#endif
            }
        }
#ifdef DEBUG
        std::cout << "tI = " << tI << "; Numerator = " << LOG(numerator) + maxNumerator
                  << "; \tDenominator = " << LOG(denominator) + maxDenominator << std::endl;
#endif
        interleavedInputLogLikelihood =
            (LOG(numerator) + maxNumerator) -
            (LOG(denominator)  + maxDenominator);
        m_inputLogLikelihoodRatio[generatorIndex][m_interleavers[generatorIndex][tI]] =
            interleavedInputLogLikelihood;
        m_extrinsicInputLogLikelihoodRatio[generatorIndex][m_interleavers[generatorIndex][tI]] =
            interleavedInputLogLikelihood -
            m_intrinsicInputLogLikelihoodRatio[generatorIndex][m_interleavers[generatorIndex][tI]];
    }
}

void TurboCode::initializeInputLogLikelihoodRatio()
{
    m_inputLogLikelihoodRatio = real_number_vector_vector(m_generators.size(), real_number_vector(m_inputLength, 0.0));
    m_intrinsicInputLogLikelihoodRatio = real_number_vector_vector(m_generators.size(), real_number_vector(m_inputLength, 0.0));
    m_extrinsicInputLogLikelihoodRatio = real_number_vector_vector(m_generators.size(), real_number_vector(m_inputLength, 0.0));
}

void TurboCode::setSymbolIndexes(uint_vector newSymbolIndexes)
{
    if (newSymbolIndexes.size() == 0) {
#ifdef DEBUG
        std::cerr << "TurboCode::setSymbolIndexes: WARNING: incomplete TurboCode object, not ready to encode/decode: empty symbol indexes." << std::endl;
#endif
    } else {
        if (newSymbolIndexes.size() != m_outputLength) {
            std::string exceptionMessage =
                std::string("TurboCode::setSymbolIndexes: the size of the input argument newSymbolIndexes (") +
                std::to_string(newSymbolIndexes.size()) + std::string(") is not the same as the attribute m_outputLength (") +
                std::to_string(m_outputLength) + std::string(").");
            throw std::logic_error(exceptionMessage.c_str());
        }
    }
    m_symbolIndexes = newSymbolIndexes;
}

const GMM_vector &TurboCode::pdfs1() const
{
    return m_pdfs1;
}

void TurboCode::setPdfs1(const GMM_vector &newPdfs1)
{    if (newPdfs1.size() == 0) {
#ifdef DEBUG
        std::cerr << "TurboCode::setPdfs0: WARNING: incomplete TurboCode object, not ready for encode/decode: empty Pdfs0" << std::endl;
#endif
    } else  {
        if (newPdfs1.size() != m_outputLength) {
            std::string exceptionMessage =
                std::string("TurboCode::setPdfs1: the size of the input argument newPdfs1 (") +
                std::to_string(newPdfs1.size()) + std::string(") is not the same as the attribute m_outputLength (") +
                std::to_string(m_outputLength) + std::string(").");
            throw std::logic_error(exceptionMessage.c_str());
        }
    }
    m_pdfs1 = newPdfs1;
    for (unsigned int i = 0; i < m_pdfs1.size(); i++) {
        m_pdfs1[i].calculateFactors();
    }
}

const GMM_vector &TurboCode::pdfs0() const
{
    return m_pdfs0;
}

void TurboCode::setPdfs0(const GMM_vector &newPdfs0)
{
    if (newPdfs0.size() == 0) {
#ifdef DEBUG
        std::cerr << "TurboCode::setPdfs0: WARNING: incomplete TurboCode object, not ready for encode/decode: empty Pdfs0" << std::endl;
#endif
    } else  {
        if (newPdfs0.size() != m_outputLength) {
            std::string exceptionMessage =
                std::string("TurboCode::setPdfs0: the size of the input argument newPdfs0 (") +
                std::to_string(newPdfs0.size()) + std::string(") is not the same as the attribute m_outputLength (") +
                std::to_string(m_outputLength) + std::string(").");
            throw std::logic_error(exceptionMessage.c_str());
        }
    }
    m_pdfs0 = newPdfs0;
    for (unsigned int i = 0; i < m_pdfs0.size(); i++) {
        m_pdfs0[i].calculateFactors();
    }
}

const uint_vector_vector &TurboCode::interleavers() const {
    return m_interleavers;
}

void TurboCode::setInterleavers(const uint_vector_vector &newInterleaver) {
    if (newInterleaver.size() != m_generators.size()) {
        std::string exceptionMessage =
            std::string("TurboCode::setInterleaver: the size of the interleaver must match the size of the generator (") +
            std::to_string(m_generators.size()) + std::string(").");
        throw std::logic_error(exceptionMessage.c_str());
    }
    for (unsigned int generatorI = 0; generatorI < m_generators.size(); generatorI++) {
        if (newInterleaver[generatorI].size() != m_inputLength) {
            std::string exceptionMessage = std::string("TurboCode::setInterleaver: the size of interleaver ") +
                                           std::to_string(generatorI) + std::string(" must be equal to ") +
                                           std::to_string(m_inputLength) + std::string(", which is set after the puncturing patterns.");
            std::logic_error(exceptionMessage.c_str());
        }
    }
    m_interleavers = newInterleaver;
    buildRedundancyIndexes();
}

void TurboCode::setup(const binary_polynomial_pair_vector &generators,
                      const binary_polynomial_vector &puncturingPatterns,
                      const uint_vector_vector &interleavers) {
    setGenerators(generators);
    setPuncturingPatterns(puncturingPatterns);
    setInterleavers(interleavers);
}

void TurboCode::buildForwardTransitionMatrix() {
    unsigned int generatorsCount = m_generators.size();
    m_forwardTransitionMatrix = code_transition_matrix(generatorsCount);
    for (unsigned int generatorI = 0; generatorI < generatorsCount; generatorI++) {
        unsigned int statesCount = (1 << m_codeMemorySize[generatorI]);
        m_forwardTransitionMatrix[generatorI] = transition_matrix(statesCount);
        for (unsigned int stateI = 0; stateI < statesCount; stateI++) {
            m_forwardTransitionMatrix[generatorI][stateI] = transition_matrix_node(2);
            for (unsigned int inputBit = 0; inputBit < 2; inputBit++) {
                // #ifdef DEBUG
                // 	std::cout << "Building forward transition matrix for g = "
                // 		  << generatorI << ", s = " << stateI << ", j = " << inputBit
                // 		  << std::endl;
                // #endif
                bool feedbackBit = ((inputBit == 1) && m_generators[generatorI].first[0]); // This has to be 1. Otherwise, the input bit is not fed into the shift register.
                bool redundancyBit = ((inputBit == 1) && m_generators[generatorI].second[0]);
                // #ifdef DEBUG
                // 	std::cout << "redundancyBit = " << redundancyBit << std::endl;
                // #endif
                for (unsigned int memoryBitI = 1; memoryBitI < m_codeMemorySize[generatorI] + 1; memoryBitI++) {
                    unsigned int bitIMask = (1 << (m_codeMemorySize[generatorI] - memoryBitI));
                    // #ifdef DEBUG
                    // 	  std::cout << "bitIMask = " << bitIMask << std::endl;
                    // #endif
                    bool stateIBitI = ((stateI & bitIMask) != 0);
                    // #ifdef DEBUG
                    // 	  std::cout << "stateIBitI = " << stateIBitI << std::endl;
                    // #endif
                    redundancyBit = (redundancyBit ^ (m_generators[generatorI].second[memoryBitI] && stateIBitI));
                    // #ifdef DEBUG
                    // 	  std::cout << "redundancyBit = " << redundancyBit << std::endl;
                    // #endif
                    feedbackBit = (feedbackBit ^ (m_generators[generatorI].first[memoryBitI] && stateIBitI));
                }
                unsigned int nextState = (stateI >> 1);
                if (feedbackBit) {
                    nextState += (1 << (m_codeMemorySize[generatorI] - 1));
                }
                m_forwardTransitionMatrix[generatorI][stateI][inputBit] = state_bit_pair(nextState, redundancyBit);
#ifdef DEBUG
                std::cout << "m_forwardTransitionMatrix[" << generatorI << "][" << stateI << "][" << inputBit << "] = "
                          << "(" << m_forwardTransitionMatrix[generatorI][stateI][inputBit].first
                          << ", " << m_forwardTransitionMatrix[generatorI][stateI][inputBit].second
                          << ")" << std::endl;
#endif
            }
        }
    }
}

void TurboCode::decode(const real_number_vector_vector &R, const unsigned int maxIt, binary_polynomial &decodedBits, unsigned int &it) {
    const unsigned int generatorsCount = m_generators.size();
    computeSymbolPdfEvaluations(R);
#ifdef DEBUG
    printSymbolPdfEvaluations();
#endif
    initializeInputLogLikelihoodRatio();
    it = 0;
    do {
        for (unsigned int generatorIndex = 0; generatorIndex < generatorsCount; generatorIndex++) {
            //      real_number_vector feedbackInformation = m_extrinsicInputLogLikelihoodRatio[(generatorIndex - 1 + generatorsCount) % generatorsCount];
            //#ifdef DEBUG
            //      std::cout << "Feedback log-likelihood ratio at encoder " << generatorIndex << ":" << std::endl;
            //#endif
            //      for (unsigned int bitI = 0; bitI < feedbackInformation.size(); bitI++) {
            //	feedbackInformation[bitI] -= m_extrinsicInputLogLikelihoodRatio[generatorIndex][bitI];
            //#ifdef DEBUG
            //	std::cout << "Bit " << bitI << ": " << feedbackInformation[bitI] << std::endl;
            //#endif
            //      }
            computeGammaMatrix(generatorIndex, m_extrinsicInputLogLikelihoodRatio[(generatorIndex - 1 + generatorsCount) % generatorsCount]);
#ifdef DEBUG
            printGammaMatrix(generatorIndex);
#endif
            std::thread alphaThread(&TurboCode::computeAlphaMatrix, this, generatorIndex);
            std::thread betaThread(&TurboCode::computeBetaMatrix, this, generatorIndex);
            alphaThread.join();
#ifdef DEBUG
            printAlphaMatrix(generatorIndex);
#endif
            betaThread.join();
#ifdef DEBUG
            printBetaMatrix(generatorIndex);
            printLambdaMatrix(generatorIndex);
#endif
            computeLogLikelihoodVector(generatorIndex);
#ifdef DEBUG
            printLogLikelihoodVector(generatorIndex);
#endif
        }
        it++;
    } while ((it < maxIt) && !decodingConverged());
    decodedBits = binary_polynomial(m_inputLength, false);
    for (unsigned int decodedBitI = 0; decodedBitI < m_inputLength; decodedBitI++) {
        decodedBits[decodedBitI] = std::signbit(m_inputLogLikelihoodRatio[0][decodedBitI]);
    }
}

void TurboCode::decode(const std::vector<real_number_vector_vector> &R, const unsigned int maxIt, binary_polynomial_vector &decodedBits, std::vector<unsigned int> &it)
{
    for (unsigned int i = 0; i < R.size(); i++) {
        decode(R[i], maxIt, decodedBits[i], it[i]);
    }
}

bool TurboCode::decodingConverged() {
    const unsigned int generatorsCount = m_generators.size();
    unsigned int mismatchesCount(0);
    for (unsigned int decodedBitI = 0; decodedBitI < m_inputLength; decodedBitI++) {
        for (unsigned int generatorIndex = 0; generatorIndex < generatorsCount - 1; generatorIndex++) {
            if (std::signbit(m_inputLogLikelihoodRatio[generatorIndex][decodedBitI]) != std::signbit(m_inputLogLikelihoodRatio[generatorIndex + 1][decodedBitI])) {
                mismatchesCount++;
                //	return false;
            }
        }
    }
#ifdef DEBUG
    if (mismatchesCount > 0) {
        std::cout.flush();
        std::cout << std::endl << "Number of mismatches: " << mismatchesCount << std::endl;
        for (unsigned int generatorIndex = 0; generatorIndex < m_generators.size(); generatorIndex++) {
            printLogLikelihoodVector(generatorIndex);
        }
    }
#endif
    return (mismatchesCount == 0);
    //  return true;
}

void TurboCode::printLambdaMatrix(const unsigned int generatorIndex) const {
    std::cout << "Lambda matrix for generator " << generatorIndex << ":" << std::endl;
    for (unsigned int tI = 0; tI < m_inputLength; tI++) {
        std::cout << "t = " << tI << std::endl;
        const unsigned int statesCount = (1 << m_codeMemorySize[generatorIndex]);
        for (unsigned int stateI = 0; stateI < statesCount; stateI++) {
            std::cout << "state = " << stateI << " -> "
                      << *(*(m_codeAlphaMatrix + generatorIndex) + tI*statesCount + stateI) +
                             *(*(m_codeBetaMatrix + generatorIndex) + tI*statesCount + stateI)
                      << "; ";
            std::cout << std::endl;
        }
    }}

void TurboCode::printGammaMatrix(const unsigned int generatorIndex) const {
    std::cout << "Gamma matrix for generator " << generatorIndex << ":" << std::endl;
    for (unsigned int tI = 0; tI < m_inputLength; tI++) {
        std::cout << "t = " << tI << std::endl;
        const unsigned int statesCount = (1 << m_codeMemorySize[generatorIndex]);
        for (unsigned int stateI = 0; stateI < statesCount; stateI++) {
            std::cout << "state = " << stateI << "; ";
            for (unsigned int bitI = 0; bitI < 2; bitI++) {
                std::cout << "X = " << bitI << " -> " << *(*(m_codeGammaMatrix + generatorIndex) + tI*2*statesCount + 2*stateI + bitI) << "; ";
            }
            std::cout << std::endl;
        }
    }
}

void TurboCode::printAlphaMatrix(const unsigned int generatorIndex) const {
    std::cout << "Alpha matrix for generator " << generatorIndex << ":" << std::endl;
    for (unsigned int tI = 0; tI < m_inputLength; tI++) {
        std::cout << "t = " << tI << std::endl;
        const unsigned int statesCount = (1 << m_codeMemorySize[generatorIndex]);
        for (unsigned int stateI = 0; stateI < statesCount; stateI++) {
            std::cout << "state = " << stateI << " -> " << *(*(m_codeAlphaMatrix + generatorIndex) + tI*statesCount + stateI) << "; ";
            std::cout << std::endl;
        }
    }
}

void TurboCode::printBetaMatrix(const unsigned int generatorIndex) const {
    std::cout << "Beta matrix for generator " << generatorIndex << ":" << std::endl;
    for (unsigned int tI = 0; tI < m_inputLength; tI++) {
        std::cout << "t = " << tI << std::endl;
        const unsigned int statesCount = (1 << m_codeMemorySize[generatorIndex]);
        for (unsigned int stateI = 0; stateI < statesCount; stateI++) {
            std::cout << "state = " << stateI << " -> " << *(*(m_codeBetaMatrix + generatorIndex) + tI*statesCount + stateI) << "; ";
            std::cout << std::endl;
        }
    }
}

void TurboCode::printSymbolPdfEvaluations() const {
    std::cout << "Symbol PDF evaluations:" << std::endl;
    for (unsigned int tI = 0; tI < m_outputLength; tI++) {
        std::cout << "t = " << tI << std::endl;
        for (unsigned int bitI = 0; bitI < 2; bitI++) {
            std::cout << "X = " << bitI << " -> " << *(m_symbolPdfEvaluations + 2*tI + bitI) << "; ";
        }
        std::cout << std::endl;
    }
}

void TurboCode::printLogLikelihoodVector(const unsigned int generatorIndex) {
    std::cout << "Input LogLikelihoodsVector for generator " << generatorIndex << std::endl;
    for (unsigned int bitI = 0; bitI < m_inputLength; bitI++) {
        std::cout << "Bit " << bitI << ": " << m_inputLogLikelihoodRatio[generatorIndex][bitI] << std::endl;
    }
    std::cout << "Intrinsic LogLikelihoodsVector for generator " << generatorIndex << std::endl;
    for (unsigned int bitI = 0; bitI < m_inputLength; bitI++) {
        std::cout << "Bit " << bitI << ": " << m_intrinsicInputLogLikelihoodRatio[generatorIndex][bitI] << std::endl;
    }
    std::cout << "Extrinsic LogLikelihoodsVector for generator " << generatorIndex << std::endl;
    for (unsigned int bitI = 0; bitI < m_inputLength; bitI++) {
        std::cout << "Bit " << bitI << ": " << m_extrinsicInputLogLikelihoodRatio[generatorIndex][bitI] << std::endl;
    }
}

const code_transition_matrix &TurboCode::fowardTransitionMatrix() const {
    return m_forwardTransitionMatrix;
}

const uint_uint_map_vector &TurboCode::redundancyIndexes() const {
    return m_redundancyIndexes;
}

const uint_vector &TurboCode::symbolIndexes() const {
    return m_symbolIndexes;
}
