#include "DependencyIndex.h"
#include "Clash.h"
#include "DependencyEntry.h"
#include "KnowledgeBase.h"
#include "DependencySet.h"
#include "Branch.h"

DependencyIndex::DependencyIndex(KnowledgeBase* pKB)
{
  m_pKB = pKB;
}

void DependencyIndex::setClashDependencies(Clash* pClash)
{
  for(SetOfDependency::iterator i = m_setClashIndex.begin(); i != m_setClashIndex.end(); i++ )
    {
      ClashDependency* pClashDependency = (ClashDependency*)*i;

      //remove the dependency
      Expr2DependencyEntry::iterator ii = m_mapDependencies.find(pClashDependency->m_pAssertion);
      if( ii != m_mapDependencies.end() )
	{
	  DependencyEntry* pDE = (DependencyEntry*)ii->second;
	  if( pDE ) pDE->m_pClashDependency = NULL;
    }

  //clear the old index
  m_setClashIndex.clear();
  
  if( pClash == NULL )
    return;
  
  for( ExprNodeSet::iterator i = pClash->m_pDepends->m_setExplain.begin(); i != pClash->m_pDepends->m_setExplain.end(); i++ )
    {
      ExprNode* pAtom = (ExprNode*)*i;
    
      //check if this assertion exists
      if( m_pKB->m_setSyntacticAssertions.find(pAtom) != m_pKB->m_setSyntacticAssertions.end() )
	{
	  //if this entry does not exist then create it
	  if( m_mapDependencies.find(pAtom) == m_mapDependencies.end() )
	    m_mapDependencies[pAtom] = new DependencyEntry();
	  
	  ClashDependency* pNewDep = new ClashDependency(pAtom, pClash);
      
	  //set the dependency
	  DependencyEntry* pDEntry = (DependencyEntry*)(m_mapDependencies.find(pAtom)->second);
	  pDEntry->m_pClashDependency = pNewDep;
      
	  //update index
	  m_setClashIndex.insert(pNewDep);
	}
    }		
  }
}

void DependencyIndex::addEdgeDependency(Edge* pEdge, DependencySet* pDS)
{
  //loop over ds
  for(ExprNodeSet::iterator i = pDS->m_setExplain.begin(); i != pDS->m_setExplain.end(); i++ )
    {
      ExprNode* pAtom = (ExprNode*)*i;
      //check if this assertion exists
      if( m_pKB->m_setSyntacticAssertions.find(pAtom) != m_pKB->m_setSyntacticAssertions.end() )
	{
	  //if this entry does not exist then create it
	  DependencyEntry* pDE = NULL;
	  if( m_mapDependencies.find(pAtom) == m_mapDependencies.end() )
	    {
	      pDE = new DependencyEntry();
	      m_mapDependencies[pAtom] = pDE;
	    }
	  else
	    pDE = (DependencyEntry*)m_mapDependencies.find(pAtom)->second;
	      
	  //add the dependency
	  pDE->addEdgeDependency(pEdge);
	}
    }
}

// Add a new merge dependency
void DependencyIndex::addMergeDependency(ExprNode* pInd, ExprNode* pMergeTo, DependencySet* pDS)
{
  //loop over ds
  for(ExprNodeSet::iterator i = pDS->m_setExplain.begin(); i != pDS->m_setExplain.end(); i++ )
    {
      //get explain atom
      ExprNode* pAtom = (ExprNode*)*i;
      
      //check if this assertion exists
      ExprNodeSet::iterator iFind = m_pKB->m_setSyntacticAssertions.find(pAtom);
      if( iFind != m_pKB->m_setSyntacticAssertions.end() )
	{
	  //if this entry does not exist then create it
	  DependencyEntry* pDE = NULL;
	  Expr2DependencyEntry::iterator iFind2 = m_mapDependencies.find(pAtom);
	  if( iFind2 == m_mapDependencies.end() )
	    {
	      pDE = new DependencyEntry();
	      m_mapDependencies[pAtom] = pDE;
	    }
	  else
	    pDE = (DependencyEntry*)iFind2->second;

	  pDE->addMergeDependency(pInd, pMergeTo);
	}
    }
}

void DependencyIndex::addTypeDependency(ExprNode* pInd, ExprNode* pType, DependencySet* pDS)
{
  for(ExprNodeSet::iterator i = pDS->m_setExplain.begin(); i != pDS->m_setExplain.end(); i++ )
    {
      //get explain atom
      ExprNode* pAtom = (ExprNode*)*i;
      
      //check if this assertion exists
      ExprNodeSet::iterator iFind = m_pKB->m_setSyntacticAssertions.find(pAtom);
      if( iFind != m_pKB->m_setSyntacticAssertions.end() )
	{
	  //if this entry does not exist then create it
	  DependencyEntry* pDE = NULL;
	  Expr2DependencyEntry::iterator iFind2 = m_mapDependencies.find(pAtom);
	  if( iFind2 == m_mapDependencies.end() )
	    {
	      pDE = new DependencyEntry();
	      m_mapDependencies[pAtom] = pDE;
	    }
	  else
	    pDE = (DependencyEntry*)iFind2->second;

	  pDE->addTypeDependency(pInd, pType);
	} 
    }
}

void DependencyIndex::addBranchAddDependency(Branch* pBranch)
{
  for(ExprNodeSet::iterator i = pBranch->m_pTermDepends->m_setExplain.begin(); i != pBranch->m_pTermDepends->m_setExplain.end(); i++ )
    {
      ExprNode* pAtom = (ExprNode*)*i;

      //check if this assertion exists
      ExprNodeSet::iterator iFind = m_pKB->m_setSyntacticAssertions.find(pAtom);
      if( iFind != m_pKB->m_setSyntacticAssertions.end() )
	{
	  //if this entry does not exist then create it
	  DependencyEntry* pDE = NULL;
	  Expr2DependencyEntry::iterator iFind2 = m_mapDependencies.find(pAtom);
	  if( iFind2 == m_mapDependencies.end() )
	    {
	      pDE = new DependencyEntry();
	      m_mapDependencies[pAtom] = pDE;
	    }
	  else
	    pDE = (DependencyEntry*)iFind2->second;
	  
	  //add the dependency
	  pDE->addBranchAddDependency(pAtom, pBranch->m_iBranch, pBranch);

	  // add depedency to index so that backjumping can be supported 
	  // (ie, we need a fast way to remove the branch dependencies
#if 0
	  if(!branchIndex.containsKey(branch))
	    {
	      Set<BranchDependency> newS = new HashSet<BranchDependency>();
	      newS.add(newDep);
	      branchIndex.put(branch, newS);
	    }
	  else
	    {
	      branchIndex.get(branch).add(newDep);
	    }
#endif
	}
    }
}

void DependencyIndex::addCloseBranchDependency(Branch* pBranch, DependencySet* pDS)
{
  for(ExprNodeSet::iterator i = pDS->m_setExplain.begin(); i != pDS->m_setExplain.end(); i++ )
    {
      //get explain atom
      ExprNode* pAtom = (ExprNode*)*i;
      
      //check if this assertion exists
      ExprNodeSet::iterator iFind = m_pKB->m_setSyntacticAssertions.find(pAtom);
      if( iFind != m_pKB->m_setSyntacticAssertions.end() )
	{
	  //if this entry does not exist then create it
	  DependencyEntry* pDE = NULL;
	  Expr2DependencyEntry::iterator iFind2 = m_mapDependencies.find(pAtom);
	  if( iFind2 == m_mapDependencies.end() )
	    {
	      pDE = new DependencyEntry();
	      m_mapDependencies[pAtom] = pDE;
	    }
	  else
	    pDE = (DependencyEntry*)iFind2->second;

	  BranchDependency* pNewDep = pDE->addCloseBranchDependency(pAtom, pBranch);
	  
	  // add depedency to index so that backjumping can be supported 
	  // (ie, we need a fast way to remove the branch dependencies
#if 0
	  if(!branchIndex.containsKey(branch))
	    {
	      Set<BranchDependency> newS = new HashSet<BranchDependency>();
	      newS.add(newDep);
	      branchIndex.put(branch, newS);
	    }
	  else
	    {
	      branchIndex.get(branch).add(newDep);
	    }
#endif
	}
    }
}

DependencyEntry* DependencyIndex::getDependencies(ExprNode* pExprNode)
{
  Expr2DependencyEntry::iterator i = m_mapDependencies.find(pExprNode);
  if( i != m_mapDependencies.end() )
    return (DependencyEntry*)i->second;
  return NULL;
}

void DependencyIndex::removeDependencies(ExprNode* pExprNode)
{
  m_mapDependencies.erase(pExprNode);
}

// Remove branch dependencies - this is needed due to backjumping!
void DependencyIndex::removeBranchDependencies(Branch* pBranch)
{
  Branch2BranchDependencySetMap::iterator i = m_mapBranchIndex.find(pBranch);
  if( i != m_mapBranchIndex.end() )
    {
      BranchDependencySet* pDeps = (BranchDependencySet*)i->second;
      for(BranchDependencySet::iterator j = pDeps->begin(); j != pDeps->end(); j++)
	{
	  BranchDependency* pBD = (BranchDependency*)*j;
	  if( pBD->m_iDependencyType == DEPENDENCY_BRANCHADD )
	    {
	      //remove the dependency
	      DependencyEntry* pDE = getDependencies(pBD->m_pAssertion);
	      pDE->m_setBranchAdds.erase(pBD);
	    }
	}
    }
  else
    {
      //TODO: why is this null? is this because of duplicate entries in the index set?
      //This seems to creep up in WebOntTest-I5.8-Manifest004 and 5 among others...
    }
}
