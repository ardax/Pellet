#ifndef _TAXONOMY_NODE_
#define _TAXONOMY_NODE_

#include "ExpressionNode.h"
#include <vector>
#include <map>
using namespace std;

class TaxonomyNode;
typedef vector<TaxonomyNode*> TaxonomyNodes;
typedef map<ExprNode*, TaxonomyNode*, strCmpExprNode> ExprNode2TaxonomyNodeMap;

int isEqualTaxonomyNode(const TaxonomyNode* pTaxonomyNode1, const TaxonomyNode* pTaxonomyNode2);
struct strCmpTaxonomyNode
{
  bool operator()( const TaxonomyNode* pTN1, const TaxonomyNode* pTN2 ) const {
    return isEqualTaxonomyNode( pTN1, pTN2 ) < 0;
  }
};
typedef set<TaxonomyNode*, strCmpTaxonomyNode> TaxonomyNodeSet;
typedef map<TaxonomyNode*, SetOfExprNodeSet*, strCmpTaxonomyNode> TaxonomyNode2SetOfExprNodeSet;
typedef map<TaxonomyNode*, TaxonomyNode*, strCmpTaxonomyNode> TaxonomyNode2TaxonomyNode;
typedef map<TaxonomyNode*, int, strCmpTaxonomyNode> TaxonomyNode2Int;

bool contains(TaxonomyNodes* pNodeList, TaxonomyNode* pNode);
bool remove(TaxonomyNodes* pList, TaxonomyNode* pNode);

class TaxonomyNode
{
 public:
  TaxonomyNode(ExprNode* pName, bool bHidden);
  
  enum MARKS
    {
      NOT_MARKED = -1,
      MARKED_FALSE = 0,
      MARKED_TRUE = 1
    };

  void addSupers(TaxonomyNodes* paSupers);
  void addSubs(TaxonomyNodes* paSubs);
  void addSub(TaxonomyNode* pNode);
  void addEquivalent(ExprNode* pC);
  void addSuperExplanation(TaxonomyNode* pSup, ExprNodeSet* pExplanation);
  SetOfExprNodeSet* getSuperExplanations(TaxonomyNode* pNode);
  void addInstance(ExprNode* pInd);
  void setInstances(ExprNodeSet* pSet);

  void removeSub(TaxonomyNode* pNode);

  void setSubs(TaxonomyNodes* pSubs);
  void setSupers(TaxonomyNodes* pSupers);

  void removeMultiplePaths();

  void disconnect();

  bool isLeaf();
  bool isBottom();

  ExprNode* m_pName;
  bool m_bHidden;
  int m_bMark;
  ExprNodeSet m_setEquivalents, m_setInstances;
  TaxonomyNodes m_aSupers, m_aSubs;
  TaxonomyNode2SetOfExprNodeSet m_mSuperExplanations;
};

#endif
