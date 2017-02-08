#ifndef _EXPRESSIVITY_
#define _EXPRESSIVITY_

#define NUMOF_EXPRESSIVES 18

#include "ExpressionNode.h"

class KnowledgeBase;

class Expressivity
{
 public:
  Expressivity(KnowledgeBase* pKB);
  Expressivity(Expressivity* pExpressivity);

  enum Expressives
    {
      EX_NEGATION = 0,
      EX_INVERSE,
      EX_FUNCTIONALITY,
      EX_CARDINALITY,
      EX_CARDINALITYQ,
      EX_FUNCTIONALITYD,
      EX_CARDINALITYD,
      EX_TRANSITIVITY,
      EX_ROLEHIERARCHY,
      EX_REFLEXIVITY,
      EX_IRREFLEXIVITY,
      EX_DISJOINTROLES = 11,
      EX_ANTISYMMETRY,
      EX_COMPLEXSUBROLES,
      EX_DATATYPE,
      EX_KEYS,
      EX_DOMAIN,
      EX_RANGE = 17,
    };

  void compute();
  Expressivity* compute(ExprNode* pC);

  void processIndividuals();
  void processIndividual(ExprNode* pI, ExprNode* pConcept);
  void processClasses();
  void processRoles();

  void visit(ExprNode* pExprNode);
  void visitList(ExprNodeList* pExprNodeList);
  void visitRole(ExprNode* pExprNode);
  void visitMin(ExprNode* pExprNode);
  void visitMax(ExprNode* pExprNode);
  
  bool hasNominal();
  bool hasNegation() { return m_bExpressivity[EX_NEGATION]; }
  bool hasInverse() { return m_bExpressivity[EX_INVERSE]; }
  bool hasFunctionality() { return m_bExpressivity[EX_FUNCTIONALITY]; }
  bool hasCardinality() { return m_bExpressivity[EX_CARDINALITY]; }
  bool hasCardinalityQ() { return m_bExpressivity[EX_CARDINALITYQ]; }
  bool hasFunctionalityD() { return m_bExpressivity[EX_FUNCTIONALITYD]; }
  bool hasCardinalityD() { return m_bExpressivity[EX_CARDINALITYD]; }
  bool hasTransitivity() { return m_bExpressivity[EX_TRANSITIVITY]; }
  bool hasRoleHierarchy() { return m_bExpressivity[EX_ROLEHIERARCHY]; }
  bool hasReflexivity() { return m_bExpressivity[EX_REFLEXIVITY]; }
  bool hasIrreflexivity() { return m_bExpressivity[EX_IRREFLEXIVITY]; }
  bool hasDisjointRoles() { return m_bExpressivity[EX_DISJOINTROLES]; }
  bool hasAntiSymmetry() { return m_bExpressivity[EX_ANTISYMMETRY]; }
  bool hasComplexSubRoles() { return m_bExpressivity[EX_COMPLEXSUBROLES]; }
  bool hasDataType() { return m_bExpressivity[EX_DATATYPE]; }
  bool hasKeys() { return m_bExpressivity[EX_KEYS]; }
  bool hasDomain() { return m_bExpressivity[EX_DOMAIN]; }
  bool hasRange() { return m_bExpressivity[EX_RANGE]; }

  void printExpressivity();

  bool m_bExpressivity[NUMOF_EXPRESSIVES];
  ExprNodeSet m_setNominals;
  KnowledgeBase* m_pKB;
};

#endif
