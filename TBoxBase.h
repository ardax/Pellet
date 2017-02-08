#ifndef _TBOX_BASE_
#define _TBOX_BASE_

#include "ExpressionNode.h"
#include "TermDefinition.h"

class KnowledgeBase;

class TBoxBase
{
 public:
  TBoxBase(KnowledgeBase* pKB);

  virtual bool addDef(ExprNode* pExpr);
  virtual bool removeDef(ExprNode* pExpr);

  TermDefinition* getTermDef(ExprNode* pExpr);

  KnowledgeBase* m_pKB;
  ExprNode2TermDefinition m_mapTermHash;
};

#endif
