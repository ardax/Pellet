#ifndef _DEPENDENCY_SET_
#define _DEPENDENCY_SET_

#include "ExpressionNode.h"
#include <vector>
typedef vector<bool> dynamic_bitset;


class DependencySet
{
 public:
  DependencySet(ExprNode* pExpr = NULL);
  DependencySet(ExprNodeSet* pExprSet);
  DependencySet(DependencySet* pDS);
  DependencySet(int iBranch);

  enum DSBranchTypes
    {
      NO_BRANCH = -1
    };

  int getMax();

  DependencySet* addExplain(ExprNodeSet* pExplain, bool bDoExplanation);
  void removeExplain(ExprNode* pExplain);

  bool contains(int b);
  void add(int b);
  void remove(int b);
  int size() { return m_bitsetDepends.size(); }

  bool isIndependent();
  DependencySet* unionNew(DependencySet* pSource, bool bDoExplanation);
  DependencySet* unionDS(DependencySet* pSource, bool bDoExplanation);

  int m_iBranch;
  ExprNodeSet m_setExplain;
  dynamic_bitset m_bitsetDepends;
} ;

typedef map<ExprNode*, DependencySet*, strCmpExprNode> Expr2DependencySetMap;
typedef vector<DependencySet*> DependencySets;

#endif
