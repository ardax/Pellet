#include "DependencyEntry.h"
#include "Clash.h"
#include "Branch.h"

DependencyEntry::DependencyEntry()
{
  m_pClashDependency = NULL;
}

void DependencyEntry::addEdgeDependency(Edge* pEdge)
{
  m_setEdges.insert(pEdge);
}

void DependencyEntry::addMergeDependency(ExprNode* pInd, ExprNode* pMergeTo)
{
  m_setMerges.insert(new MergeDependency(pInd, pMergeTo));
}

void DependencyEntry::addTypeDependency(ExprNode* pInd, ExprNode* pType)
{
  m_setTypes.insert(new TypeDependency(pInd, pType));
}

BranchDependency* DependencyEntry::addBranchAddDependency(ExprNode* pAssertion, int iBranchIndex, Branch* pBranch)
{
  BranchDependency* pB = new BranchAddDependency(pAssertion, iBranchIndex, pBranch);
  m_setBranchAdds.insert(pB);
  return pB;
}

BranchDependency* DependencyEntry::addCloseBranchDependency(ExprNode* pAssertion, Branch* pBranch)
{
  BranchDependency* pB = new CloseBranchDependency(pAssertion, pBranch->m_iTryNext, pBranch);
  if( m_setBranchCloses.find(pB) != m_setBranchCloses.end() )
    m_setBranchCloses.erase(pB);
  m_setBranchCloses.insert(pB);
  return pB;
}
