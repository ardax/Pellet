#include "SHOINStrategy.h"
#include "ABox.h"
#include "Blocking.h"

SHOINStrategy::SHOINStrategy(ABox* pABox) : SHOIQStrategy(pABox)
{

}

void SHOINStrategy::applyChooseRuleOnIndividuals()
{
  // overwrite empty
}

SHONStrategy::SHONStrategy(ABox* pABox) : SHOINStrategy(pABox)
{
  m_pBlocking = new SubsetBlocking();
}

void SHONStrategy::applyGuessingRuleOnIndividuals()
{
  // overwrite empty
}

SHINStrategy::SHINStrategy(ABox* pABox) : SHOINStrategy(pABox)
{

}

void SHINStrategy::applyGuessingRuleOnIndividuals()
{
  // overwrite empty
}

void SHINStrategy::applyNominalRuleOnIndividuals()
{
  // overwrite empty
}

SHNStrategy::SHNStrategy(ABox* pABox) : SHOINStrategy(pABox)
{
  m_pBlocking = new SubsetBlocking();
}

void SHNStrategy::applyNominalRuleOnIndividuals()
{
  // overwrite empty
}
