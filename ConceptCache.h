#ifndef _CONCEPT_CACHE_
#define _CONCEPT_CACHE_

#include "CachedNode.h"
#include "ExpressionNode.h"

typedef map<ExprNode*, CachedNode*, strCmpExprNode> ExprNode2CachedNodeMap;

class ConceptCache
{
 public:
  ConceptCache();

  void setMaxSize(int iMax) { m_iMaxSize = iMax; }
  bool putSat(ExprNode* pC, bool bIsSatisfiable);
  bool put(ExprNode* pC, CachedNode* pCachedNode);
  CachedNode* getCached(ExprNode* pC);
  int getSat(ExprNode* pC);

  int m_iMaxSize;
  ExprNode2CachedNodeMap m_mCache;
};

#endif
