#ifndef _TAXONOMY_BUILDER_
#define _TAXONOMY_BUILDER_

#include "ExpressionNode.h"
#include "TaxonomyNode.h"

class KnowledgeBase;
class Taxonomy;
class TaxonomyNode;

typedef map<ExprNode*, int, strCmpExprNode> ExprNode2IntMap;

class TaxonomyBuilder
{
 public:
  TaxonomyBuilder();

  enum ConceptFlags
    {
      FLAG_COMPLETELY_DEFINED = 0,
      FLAG_PRIMITIVE,
      FLAG_NONPRIMITIVE,
      FLAG_NONPRIMITIVE_TA,
      FLAG_OTHER,
    };

  enum
    {
      PROPAGAET_DOWN = -1,
      NO_PROPAGATE = 0,
      PROPAGATE_UP = 1,
    };

  Taxonomy* classify();
  TaxonomyNode* classify(ExprNode* pC, bool bRequireTopSearch = TRUE);
  TaxonomyNode* checkSatisfiability(ExprNode* pC);
  void prepare();
  void reset();
  void computeToldInformation();
  void createDefinitionOrder();
  void computeConceptFlags();
  void clearMarks();

  void doTopSearch(ExprNode* pC, TaxonomyNodes* paSupers);
  void doBottomSearch(ExprNode* pC, TaxonomyNodes* paSupers, TaxonomyNodes* paSubs);
  void collectLeafs(TaxonomyNode* pNode, TaxonomyNodeSet* leafs);
  void getCDSupers(ExprNode* pC, TaxonomyNodes* paSupers);

  bool subsumes(TaxonomyNode* pNode, ExprNode* pC);
  bool subsumes(ExprNode* pSup, ExprNode* pSub);
  bool subsumed(TaxonomyNode* pNode, ExprNode* pC);
  bool subCheckWithCache(TaxonomyNode* pNode, ExprNode* pC, bool bTopDown);
  
  void search(bool bTopSearch, ExprNode* pC, TaxonomyNode* pX, TaxonomyNodeSet* pVisited, TaxonomyNodes* pResult);
  bool mark(TaxonomyNode* pNode, int bMarkValue, int iPropagate);
  void mark(ExprNodeSet* pSet, ExprNode2Int* pMarked, int bMarkValue);
  
  void markToldSubsumers(ExprNode* pC);
  void markToldDisjoints(ExprNodes* pInputC, bool bTopSearch);
  void markToldSubsumeds(ExprNode* pD, int iMark);

  void addToldDisjoint(ExprNode* pC, ExprNode* pD);  
  void addToldEquivalent(ExprNode* pC, ExprNode* pD);  
  void addToldRelation(ExprNode* pC, ExprNode* pD, bool bEquivalent, ExprNodeSet* pExplanation);
  void addToldSubsumer(ExprNode* pC, ExprNode* pD, ExprNodeSet* pExplanation);

  bool isCDDesc(Expr2ExprSetPairList* pDesc);
  bool isCDDesc(ExprNode* pDesc);

  Taxonomy* realize();
  Taxonomy* realizeByIndividuals();
  Taxonomy* realizeByConcepts();
  bool realizeByIndividual(ExprNode* pN, ExprNode* pC, ExprNode2Int* pMarked);
  void realizeByConcept(ExprNode* pC, ExprNodeSet* pIndividuals, ExprNodeSet* pSubInstances);

  ExprNode2IntMap m_mConceptFlags;
  ExprNodeSet* m_pSetClasses;
  ExprNode2ExprNodeSetMap m_mapToldDisjoints;
  ExprNodes m_aOrderedClasses;
  bool m_bUseCD;

  TaxonomyNodes m_aMarkedNodes;
  ExprNodeSet m_setCyclicConcepts;

  int m_iCount;
  ExprNode2ExprNodeListMap m_mapUnionClasses;

  Taxonomy* m_pTaxonomy;
  Taxonomy* m_pToldTaxonomy;
  KnowledgeBase* m_pKB;
};


#endif
