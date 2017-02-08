#ifndef _TBOX_TU_
#define _TBOX_TU_

#include "ExpressionNode.h"
#include "TBoxBase.h"

class TBox;

typedef pair<ExprNode*, ExprNodeSet*> Expr2ExprSetPair;
typedef list<Expr2ExprSetPair*> Expr2ExprSetPairList;
typedef map<ExprNode*, Expr2ExprSetPairList*, strCmpExprNode> Expr2Expr2ExprSetPairListMap;

class TuBox : public TBoxBase
{
 public:
  TuBox(KnowledgeBase* pKB);

  bool addDef(ExprNode* pDef);
  bool removeDef(ExprNode* pExpr);

  bool addIfUnfoldable(ExprNode* pAxiom);
  Expr2ExprSetPairList* unfold(ExprNode* pC);
  bool findTarget(ExprNode* pCurrent, ExprNode* pName, ExprNodeSet* pSeenSet);

  void normalizeDefinitions(TBox* pTBox);
  void absorbRanges();

  void clearUnfoldingMap();

  ExprNodes* m_paTermsToNormalize;
  Expr2Expr2ExprSetPairListMap m_UnfoldingMap;
};

#endif
