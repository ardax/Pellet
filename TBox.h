#ifndef _TBOX_
#define _TBOX_

#include "ExpressionNode.h"
#include <map>

class TuBox;
class TgBox;
class KnowledgeBase;

class TBox
{
 public:
  TBox(KnowledgeBase* pKB);

  bool addClass(ExprNode* pClassExpr);
  bool addAxiomExplanation(ExprNode* pAxiom, ExprNodeSet* pExplanation);
  bool addAxiom(ExprNode* pAxiom, ExprNodeSet* pExplanation);
  bool addAxiom(ExprNode* pAxiom, ExprNodeSet* pExplanation, bool bForceAddition);

  ExprNodeSet* getAxiomExplanation(ExprNode* pAxiom);
  void getAxioms(ExprNode* pTerm, ExprNodeSet* pAxioms);
  Expr2ExprSetPairList* getUC();
  Expr2ExprSetPairList* unfold(ExprNode* pC);

  void getAllClasses(ExprNodeSet* pSetAllClasses);

  void absorb();
  void normalize();
  void internalize();

  // Return the universal concept
  //void getUC();

  TuBox* m_pTuBox;
  TgBox* m_pTgBox;
  ExprNodeSet m_setClasses;
  ExprNodeSet m_setAllClasses;

  Expr2SetOfExprNodeSets m_mapTBoxAxioms;
  Expr2ExprNodeSet m_mapReverseExplain;
};

#endif
