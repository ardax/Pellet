#ifndef _TBOX_TG_
#define _TBOX_TG_

#include "ExpressionNode.h"
#include "TBoxBase.h"

class KnowledgeBase;
class TuBox;
class TBox;

class TgBox : public TBoxBase
{
 public:
  TgBox(KnowledgeBase* pKB);
  ~TgBox();

  void absorb(TuBox* pTuBox, TBox* pTBox);
  void absorbSubClass(ExprNode* pSub, ExprNode* pSup, ExprNodeSet* pAxiomExplanation);
  bool absorbTerm(ExprNodeSet* pSet);
  bool absorbNominal(ExprNodeSet* pSet);
  bool absorbRole(ExprNodeSet* pSet);
  bool absorb2(ExprNodeSet* pSet);
  bool absorb3(ExprNodeSet* pSet);
  bool absorb5(ExprNodeSet* pSet);
  bool absorb6(ExprNodeSet* pSet);
  bool absorb7(ExprNodeSet* pSet);

  void absorbOneOf(ExprNodeList* pExprOneOf, ExprNode* pExprC, ExprNodeSet* pExplanation);
  void absorbOneOf(ExprNode* pExpr, ExprNode* pExprC, ExprNodeSet* pExplanation);

  void internalize();
  void clearUC();
  Expr2ExprSetPairList* getUC() { return &m_UC; }

  Expr2ExprSetPairList m_UC;
  ExprNodeSet m_setAbsorbedOutsideTBox;
  ExprNodeSet m_setExplanation;

  TuBox* m_pTuBox;
  TBox* m_pTBox;
};

#endif
