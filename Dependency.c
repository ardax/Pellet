#include "Dependency.h"
#include "Clash.h"
#include "Branch.h"

Dependency::Dependency()
{
  m_iDependencyType = -1;
}

/*************************************************************************/

ClashDependency::ClashDependency()
{
  m_pAssertion = NULL;
  m_pClash = NULL;
  m_iDependencyType = DEPENDENCY_CLASH;
}

ClashDependency::ClashDependency(ExprNode* pAssertion, Clash* pClash)
{
  m_pAssertion = pAssertion;
  m_pClash = pClash;
  m_iDependencyType = DEPENDENCY_CLASH;
}

bool ClashDependency::isEqual(const Dependency* pDependency) const
{
  if( pDependency->m_iDependencyType != m_iDependencyType ||
      ::isEqual(m_pAssertion, ((ClashDependency*)pDependency)->m_pAssertion) != 0 ||
      ::isEqual(m_pClash->m_pNode, ((ClashDependency*)pDependency)->m_pClash->m_pNode) != 0 )
    return FALSE;
  return TRUE;
}

/*************************************************************************/

TypeDependency::TypeDependency(ExprNode* pInd, ExprNode* pType)
{
  m_pType = pType;
  m_pInd = pInd;
  m_iDependencyType = DEPENDENCY_TYPE;
}

bool TypeDependency::isEqual(const Dependency* pDependency) const
{
  if( pDependency->m_iDependencyType != m_iDependencyType )
    return FALSE;

  if( ::isEqual(m_pType, ((TypeDependency*)pDependency)->m_pType) != 0 || ::isEqual(m_pInd, ((TypeDependency*)pDependency)->m_pInd) != 0 )
    return FALSE;
  return TRUE;
}

/*************************************************************************/

MergeDependency::MergeDependency(ExprNode* pInd, ExprNode* pMergeIntoInd)
{
  m_pInd = pInd;
  m_pMergeIntoInd = pMergeIntoInd;
  m_iDependencyType = DEPENDENCY_MERGE;
}

bool MergeDependency::isEqual(const Dependency* pDependency) const
{
  if( pDependency->m_iDependencyType != m_iDependencyType )
    return FALSE;

  if( ::isEqual(m_pInd, ((MergeDependency*)pDependency)->m_pInd) != 0 || ::isEqual(m_pMergeIntoInd, ((MergeDependency*)pDependency)->m_pMergeIntoInd) != 0 )
    return FALSE;
  return TRUE;
}

/*************************************************************************/

BranchDependency::BranchDependency(ExprNode* pAssertion)
{
  m_pAssertion = pAssertion;
}

bool BranchDependency::isEqual(const Dependency* pDependency) const
{
  if( pDependency->m_iDependencyType != m_iDependencyType )
    return FALSE;

  if( ::isEqual(m_pAssertion, ((BranchDependency*)pDependency)->m_pAssertion) != 0 )
    return FALSE;
  return TRUE;
}

/*************************************************************************/

BranchAddDependency::BranchAddDependency(ExprNode* pAssertion, int iIndex, Branch* pBranch) : BranchDependency(pAssertion)
{
  m_pBranch = pBranch;
  m_iDependencyType = DEPENDENCY_BRANCHADD;
}

bool BranchAddDependency::isEqual(const Dependency* pDependency) const
{
  if( !BranchDependency::isEqual(pDependency) )
    return FALSE;

  if( !m_pBranch->isEqual(((BranchAddDependency*)pDependency)->m_pBranch) )
    return FALSE;
  return TRUE;
}

/*************************************************************************/

CloseBranchDependency::CloseBranchDependency(ExprNode* pAssertion, int iTryNext, Branch* pBranch) : BranchDependency(pAssertion)
{
  m_pBranch = pBranch;
  m_iTryNext = iTryNext;
  m_iDependencyType = DEPENDENCY_CLOSEBRANCH;
}

bool CloseBranchDependency::isEqual(const Dependency* pDependency) const
{
  if( !BranchDependency::isEqual(pDependency) )
    return FALSE;

  if( !m_pBranch->isEqual(((CloseBranchDependency*)pDependency)->m_pBranch) || 
      m_iTryNext != ((CloseBranchDependency*)pDependency)->m_iTryNext ||
      ::isEqual(getInd(), ((CloseBranchDependency*)pDependency)->getInd()) != 0 )
    return FALSE;
  return TRUE;
}

ExprNode* CloseBranchDependency::getInd() const
{
  if( m_pBranch && m_pBranch->m_pNode )
    return m_pBranch->m_pNode->m_pName;
  return NULL;
}
