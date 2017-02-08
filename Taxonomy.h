#ifndef _TAXONOMY_
#define _TAXONOMY_

#include "TaxonomyNode.h"

class TaxonomyPrinter;

class Taxonomy
{
 public:
  Taxonomy();
  Taxonomy(ExprNodes* paClasses, bool bHideTopBottom);
  Taxonomy(ExprNodeSet* paClasses, bool bHideTopBottom);

  void assertValid();

  TaxonomyNode* addNode(ExprNode* pC);
  TaxonomyNode* getNode(ExprNode* pC);
  void removeNode(TaxonomyNode*);
  
  void addEquivalentNode(ExprNode* pC, TaxonomyNode* pNode);
  void getAllEquivalents(ExprNode* pC, ExprNodeSet* pEquivalents);
  void getEquivalents(ExprNode*pC, ExprNodeSet* pEquivalents);

  void getSubSupers(ExprNode* pC, bool bDirect, bool bSupers, SetOfExprNodeSet* pSet);
  void getFlattenedSubSupers(ExprNode* pC, bool bDirect, bool bSupers, ExprNodeSet* pSet);

  void getInstances(ExprNode* pC, ExprNodeSet* pInstances, bool bDirect = FALSE);
  void getInstancesHelper(TaxonomyNode* pNode, ExprNodeSet* pInstances);

  void removeAnonTerms(ExprNodeSet* pTerms);

  void computeLCA(ExprNodeList* pList, ExprNodes* pLCA);

  void merge(TaxonomyNode* pNode1, TaxonomyNode* pNode2);
  TaxonomyNode* mergeNodes(TaxonomyNodes* pMergeList);
  void removeCycles(TaxonomyNode* pNode);
  bool removeCycles(TaxonomyNode* pNode, TaxonomyNodes* pPath);
  void topologicalSort(bool bIncludeEquivalents, ExprNodes* pSorted);

  TaxonomyNode* getBottom();
  TaxonomyNode* getTop();

  bool contains(ExprNode* pC);
  bool isType(ExprNode* pInd, ExprNode* pC);
  bool isType(TaxonomyNode* pNode, ExprNode* pInd);

  void print();
  void printInFile(char* pFileName);

  TaxonomyNode* m_pTOP_NODE;
  TaxonomyNode* m_pBOTTOM_NODE;

  TaxonomyPrinter* m_pPrinter;

  ExprNode2TaxonomyNodeMap m_mNodes;
  bool m_bHideAnonTerms;
};

#endif
