#ifndef _DEPENDENCY_ENTRY_
#define _DEPENDENCY_ENTRY_


#include "Dependency.h"
#include "DependencySet.h"
#include "Edge.h"

/**
 * Structure for containing all dependencies for a given assertion. 
 * This is the object stored in the dependency index 
 */

class DependencyEntry
{
 public:
  DependencyEntry();

  void addEdgeDependency(Edge* pEdge);
  void addMergeDependency(ExprNode* pInd, ExprNode* pMergeTo);
  void addTypeDependency(ExprNode* pInd, ExprNode* pType);
  BranchDependency* addBranchAddDependency(ExprNode* pAssertion, int iBranchIndex, Branch* pBranch);
  BranchDependency* addCloseBranchDependency(ExprNode* pAssertion, Branch* pBranch);

  SetOfDependency m_setTypes;
  SetOfDependency m_setMerges;
  SetOfDependency m_setBranchAdds;
  SetOfDependency m_setBranchCloses;
  ClashDependency* m_pClashDependency;
  EdgeSet m_setEdges;
};

#endif
