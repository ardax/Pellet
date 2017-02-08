#ifndef _SIZE_ESTIMATE_
#define _SIZE_ESTIMATE_

#include "ExpressionNode.h"
class KnowledgeBase;

class SizeEstimate
{
 public:
  SizeEstimate(KnowledgeBase* pKB);

  void init();

  map<ExprNode*, int, strCmpExprNode> m_mSizes;
  map<ExprNode*, double, strCmpExprNode> m_mAvgs;

  KnowledgeBase* m_pKB;
};

#endif
