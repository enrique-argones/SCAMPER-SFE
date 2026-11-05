/// \file constellation.cpp
/// \brief Implementation of the \ref Constellation base class.

#include "constellation.h"

Constellation::Constellation():
  m_inputBitsCount(0),
  m_outputDimensionsCount(0)
{

}

const GMM_vector &Constellation::pdfs1() const
{
  return m_pdfs1;
}

const GMM_vector &Constellation::pdfs0() const
{
  return m_pdfs0;
}
