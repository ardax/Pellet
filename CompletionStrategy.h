#ifndef _COMPLETION_STRATEGY_
#define _COMPLETION_STRATEGY_

#include "Utils.h"
#include "NodeMerge.h"
#include "Blocking.h"

class ABox;
class Individual;
class Edge;
class DependencySet;
class Literal;
class Role;
class Branch;
class Blocking;
class QueueElement;

class CompletionStrategy
{
 public:
  CompletionStrategy(ABox* pABox, Blocking* pBlocking);

  virtual void initialize();
  virtual bool supportsPseudoModelCompletion() = 0;
  virtual void printStrategyType() = 0;
  virtual ABox* complete() = 0;
  virtual void restore(Branch* pBranch);

  virtual void applyUniversalRestrictions(Individual* pNode);
  virtual void applyUnfoldingRule(Individual* pNode);
  virtual void applyUnfoldingRule(QueueElement* pElement);
  virtual void applyUnfoldingRuleOnIndividuals();
  virtual void applyUnfoldingRule(Individual* pNode, ExprNode* pC);
  virtual void applySelfRule(Individual* pNode);
  virtual void applyNominalRule(Individual* pNode);
  virtual void applyNominalRule(QueueElement* pElement);
  virtual void applyNominalRule(Individual* pY, ExprNode* pNC, DependencySet* pDS);
  virtual void applyNominalRuleOnIndividuals();
  virtual void applyAllValues(Individual* pNode);
  virtual void applyAllValues(QueueElement* pElement);
  virtual void applyAllValues(Individual* pX, ExprNode* pAV, DependencySet* pDS);
  virtual void applyAllValues(Individual* pSubj, Role* pPred, Node* pObj, ExprNode* pC, DependencySet* pDS);
  virtual void applyAllValues(Individual* pSubj, Role* pPred, Node* pObj, DependencySet* pDS);
  virtual void applyGuessingRule(QueueElement* pElement);
  virtual void applyGuessingRule(Individual* pX, ExprNode* pMC);
  virtual void applyGuessingRuleOnIndividuals();
  virtual void applyPropertyRestrictions(Edge* pEdge);
  virtual void applyDomainRange(Individual* pSubj, Role* pPred, Node* pObj, DependencySet* pDS);
  virtual void applyFunctionality(Individual* pSubj, Role* pPred, Node* pObj);
  virtual void applyDisjointness(Individual* pSubj, Role* pPred, Node* pObj, DependencySet* pDS);
  virtual void applyFunctionalMaxRule(Individual* pX, Role* pS, ExprNode* pC, DependencySet* pDS);
  virtual void applyFunctionalMaxRule(Literal *pX, Role* pR, DependencySet* pDS);
  virtual void checkReflexivitySymmetry(Individual* pSubj, Role* pPred, Individual* pObj, DependencySet* pDS);
  virtual void applyChooseRuleOnIndividuals();
  virtual void applyChooseRule(QueueElement* pElement);
  virtual void applyChooseRule(Individual* pX);
  virtual void applyChooseRule(Individual* pX, ExprNode* pMaxCard);
  virtual void applySomeValuesRuleOnIndividuals();
  virtual void applySomeValuesRule(QueueElement* pElement);
  virtual void applySomeValuesRule(Individual* pX);
  virtual void applySomeValuesRule(Individual* pX, ExprNode* pSV);
  virtual void applyMinRuleOnIndividuals();
  virtual void applyMinRule(QueueElement* pElement);
  virtual void applyMinRule(Individual* pX);
  virtual void applyMinRule(Individual* pX, ExprNode* pMC);
  virtual void applyMaxRuleOnIndividuals();
  virtual void applyMaxRule(QueueElement* pElement);
  virtual void applyMaxRule(Individual* pX);
  virtual void applyMaxRule(Individual* pX, ExprNode* pMC);
  virtual bool applyMaxRule(Individual* pX, Role* pRole, ExprNode* pC, int iN, DependencySet* pDS);
  virtual void applyDisjunctionRuleOnIndividuals();
  virtual void applyDisjunctionRule(QueueElement* pElement);
  virtual void applyDisjunctionRule(Individual* pX);
  virtual void applyLiteralRuleOnIndividuals();
  virtual void applyLiteralRule(QueueElement* pElement);
  virtual void checkDatatypeCountOnIndividuals();
  virtual void checkDatatypeCount(QueueElement* pElement);
  virtual void checkDatatypeCount(Individual* pX);

  Individual* createFreshIndividual(bool bIsNominal);
  DependencySet* findMergeNodes(NodeSet* pSetNeighbors, Individual* pNode, NodeMerges* pPairs);

  void mergeFirst();
  void mergeLater(Node* pY, Node* pZ, DependencySet* pDS);
  void mergeTo(Node* pY, Node* pZ, DependencySet* pDS);
  void mergeLiterals(Literal* pY, Literal* pZ, DependencySet* pDS);
  void mergeIndividuals(Individual* pY, Individual* pZ, DependencySet* pDS);
  
  void addEdge(Individual* pSubj, Role* pPred, Node* pObj, DependencySet* pDS);
  void addType(Node* pNode, ExprNode* pC, DependencySet* pDS);
  virtual void addBranch(Branch* pNewBranch);

  void updateQueueAddEdge(Individual* pSubj, Role* pPred, Node *pObj);

  ABox* m_pABox;
  NodeMerges m_aMergeList;
  Blocking* m_pBlocking;

  // Flag to indicate that a merge operation is going on
  bool m_bMerging;
};

#endif
