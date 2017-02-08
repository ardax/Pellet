#ifndef _KNOWLEDGE_BASE_
#define _KNOWLEDGE_BASE_

#include "ReazienerUtils.h"
#include "ExpressionNode.h"
#include "DependencySet.h"
#include "Node.h"

class TBox;
class ABox;
class RBox;
class Expressivity;
class SizeEstimate;
class DependencyIndex;
class Taxonomy;
class TaxonomyBuilder;
class CompletionStrategy;
class DependencyEntry;
class ClashDependency;
class TypeDependency;
class MergeDependency;
class BranchAddDependency;
class CloseBranchDependency;
class Edge;

enum Status
  {
    KBSTATUS_UNCHANGED = 0x0000,
    KBSTATUS_ABOX_CHANGED = 0x0001,
    KBSTATUS_TBOX_CHANGED = 0x0002,
    KBSTATUS_RBOX_CHANGED = 0x0004,
    KBSTATUS_ALL_CHANGED = 0x0007,
    KBSTATUS_CONSISTENCY = 0x0008,
    KBSTATUS_CLASSIFICATION = 0x0010,
    KBSTATUS_REALIZATION = 0x0020
  };

class KnowledgeBase
{
 public:
  KnowledgeBase();
  ~KnowledgeBase();

  enum AssertionTypes
    {
      ASSERTION_TYPE = 0,
      ASSERTION_OBJ_ROLE,
      ASSERTION_DATA_ROLE
    };

  void prepare();
  void clear();

  void addClass(ExprNode* pExpr);
  void addDisjointClass(ExprNode* pC1, ExprNode* pC2);
  void addDisjointClass(ExprNode* pC1, ExprNode* pC2, ExprNodeSet* pExplaination);
  void addSubClass(ExprNode* pSub, ExprNode* pSup);
  void addSubClass(ExprNode* pSub, ExprNode* pSup, ExprNodeSet* pExplanation);
  void addSameClasses(ExprNode* pC1, ExprNode* pC2);
  void addEquivalentClasses(ExprNode* pC1, ExprNode* pC2);
  void addDisjointClasses(ExprNode* pC1, ExprNode* pC2);
  void addDisjointClasses(ExprNode* pC1, ExprNode* pC2, ExprNodeSet* pExplanation);
  void addComplementClasses(ExprNode* pC1, ExprNode* pC2);

  void addType(ExprNode* pIndvidualExpr, ExprNode* pCExpr);
  void addType(ExprNode* pIndvidualExpr, ExprNode* pCExpr, DependencySet* pDS);
  void addDomain(ExprNode* pExpr, ExprNode* pDomainExpr, DependencySet* pDS = NULL);
  void addRange(ExprNode* pRTerm, ExprNode* pRangeExpr, DependencySet* pDS = NULL);
  Node* addIndividual(ExprNode* pExpr);
  void addDifferent(ExprNode* pExprNode1, ExprNode* pExprNode2);
  void addProperty(ExprNode* pExpr);
  bool addPropertyValue(ExprNode* pR, ExprNode* pX, ExprNode* pY);
  void addSubProperty(ExprNode* pRExpr, ExprNode* pSExpr);
  bool addDatatypeProperty(ExprNode* pExpr);
  bool addObjectProperty(ExprNode* pExpr);
  void addFunctionalProperty(ExprNode* pExpr);
  void addTransitiveProperty(ExprNode* pExpr);
  void addInverseProperty(ExprNode* pExpr, ExprNode* pInverseExpr);
  void addEquivalentProperty(ExprNode* pRExpr, ExprNode* pSExpr);

  Role* getProperty(ExprNode* p);
  Role* getRole(ExprNode* p);
  int getPropertyType(ExprNode* p);
  bool isObjectProperty(ExprNode* p);

  Expressivity* getExpressivity();

  void restoreDependencies();
  void restoreDependency(ExprNode* pAssertion, DependencyEntry* pEntry);
  void restoreEdge(ExprNode* pAssertion, Edge* pEdge);
  void restoreType(ExprNode* pAssertion, TypeDependency* pType);
  void restoreMerge(ExprNode* pAssertion, MergeDependency* pMerge);
  void restoreBranchAdd(ExprNode* pAssertion, BranchAddDependency* pBranchAdd);
  void restoreCloseBranch(ExprNode* pAssertion, CloseBranchDependency* pCloseBranch);
  void restoreClash(ExprNode* pAssertion, ClashDependency* pClash);

  bool canUseIncrementalConsistency();
  void updateExpressivity(ExprNode* pIndividualExpr, ExprNode* pCExpr);

  void checkConsistency();
  bool isConsistent();
  bool isConsistencyDone();

  bool isChanged() { return (m_iKBStatus & KBSTATUS_ALL_CHANGED) != 0; }

  bool isTBoxChanged() { return (m_iKBStatus & KBSTATUS_TBOX_CHANGED) != 0; }
  bool isABoxChanged() { return (m_iKBStatus & KBSTATUS_ABOX_CHANGED) != 0; }
  bool isRBoxChanged() { return (m_iKBStatus & KBSTATUS_RBOX_CHANGED) != 0; }

  CompletionStrategy* chooseStrategy(ABox* pABox);
  CompletionStrategy* chooseStrategy(ABox* pABox, Expressivity* pExpressivity);

  ExprNodeSet* getClasses();
  void getDomains(ExprNode* pName, ExprNodeSet* pDomains);

  void classify();
  void ensureConsistency();
  bool isClassified();

  void realize();
  bool isRealized();
  
  bool isType(ExprNode* pX, ExprNode* pC);
  bool isClass(ExprNode* pC);

  void retrieve(ExprNode* pD, ExprNodeSet* pIndividuals, ExprNodeSet* pSubInstances);
  void linearInstanceRetrieval(ExprNode* pC, ExprNodes* pCandidates, ExprNodes* pResults);
  void binaryInstanceRetrieval(ExprNode* pC, ExprNodes* pCandidates, ExprNodes* pResults);
  void getInstances(ExprNode* pC, ExprNodeSet* pSetInstances);

  void printClassTree();
  void printClassTreeInFile(char* pOutputFile);
  void printExpressivity();
    
  TBox* m_pTBox;
  ABox* m_pABox;
  RBox* m_pRBox;
  Expressivity* m_pExpressivity;
  SizeEstimate* m_pEstimate;
  TaxonomyBuilder* m_pBuilder;

  bool  m_bConsistent;

  int   m_iKBStatus;
  bool  m_bABoxAddition, m_bABoxDeletion;
  map<int, ExprNodeSet*> m_mABoxAssertions;
  ExprNodeSet m_setSyntacticAssertions;
  ExprNodeSet m_setIndividuals;
  ExprNodeSet m_setDeletedAssertions;

  // Index used for abox deletions
  DependencyIndex* m_pDependencyIndex;

  Taxonomy* m_pTaxonomy;
  ExprNode2ExprNodeSetMap m_mapInstances;
};

#endif
