#ifndef _DEPENDENCY_INDEX_
#define _DEPENDENCY_INDEX_

#include "Dependency.h"
#include "ExpressionNode.h"

class Clash;
class KnowledgeBase;
class DependencyEntry;
class Edge;
class DependencySet;
class Branch;

typedef map<ExprNode*, DependencyEntry*, strCmpExprNode> Expr2DependencyEntry;
typedef map<Branch*, BranchDependencySet*> Branch2BranchDependencySetMap;

class DependencyIndex
{
 public:
  DependencyIndex(KnowledgeBase* pKB);

  void setClashDependencies(Clash* pClash);
  void addEdgeDependency(Edge* pEdge, DependencySet* pDS);
  void addMergeDependency(ExprNode* pInd, ExprNode* pMergeTo, DependencySet* pDS);
  void addTypeDependency(ExprNode* pInd, ExprNode* pType, DependencySet* pDS);
  void addBranchAddDependency(Branch* pBranch);
  void addCloseBranchDependency(Branch* pBranch, DependencySet* pDS);

  void removeBranchDependencies(Branch* pBranch);

  DependencyEntry* getDependencies(ExprNode* pExprNode);
  void removeDependencies(ExprNode* pExprNode);

  SetOfDependency m_setClashIndex;
  Expr2DependencyEntry m_mapDependencies;
  KnowledgeBase* m_pKB;
  Branch2BranchDependencySetMap m_mapBranchIndex;
};

#endif
