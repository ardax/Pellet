#include "SizeEstimate.h"
#include "KnowledgeBase.h"

extern ExprNode* EXPRNODE_TOP;
extern ExprNode* EXPRNODE_BOTTOM;

SizeEstimate::SizeEstimate(KnowledgeBase* pKB)
{
  m_pKB = pKB;
  init();
}

void SizeEstimate::init()
{
  m_mSizes.clear();
  m_mAvgs.clear();

  m_mSizes[EXPRNODE_TOP] = m_pKB->m_setIndividuals.size();
  m_mSizes[EXPRNODE_BOTTOM] = 0;
}
