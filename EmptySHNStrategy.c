#include "EmptySHNStrategy.h"
#include "ABox.h"
#include "Branch.h"
#include "Blocking.h"
#include "Individual.h"
#include "ConceptCache.h"
#include "ReazienerUtils.h"
#include "Params.h"
#include "DependencySet.h"
#include "Clash.h"

extern DependencySet DEPENDENCYSET_EMPTY;
extern ExprNode* EXPRNODE_TOP;
extern ExprNode* EXPRNODE_BOTTOM;

EmptySHNStrategy::EmptySHNStrategy(ABox* pABox) : SHOIQStrategy(pABox)
{
  m_pBlocking = new SubsetBlocking();
  m_pRoot = NULL;
}

void EmptySHNStrategy::initialize()
{
  m_aMergeList.clear();
  m_mCachedNodes.clear();

  if( m_pABox->m_mNodes.begin() != m_pABox->m_mNodes.end() )
    m_pRoot = (Individual*)m_pABox->m_mNodes.begin()->second;
  
  if( m_pRoot )
    {
      m_pRoot->setChanged(TRUE);
      applyUniversalRestrictions(m_pRoot);
    }
  m_pABox->m_iCurrentBranchIndex = 1;
  m_pABox->m_iTreeDepth = 1;
  m_pABox->m_bChanged = TRUE;
  m_pABox->m_bIsComplete = FALSE;
  m_pABox->setInitialized(TRUE);
}

ABox* EmptySHNStrategy::complete()
{
  if( m_pABox->m_mNodes.size() == 0 )
    {
      m_pABox->m_bIsComplete = TRUE;
      return m_pABox;
    }
  else if( m_pABox->m_mNodes.size() > 1 )
    assertFALSE("EmptySHNStrategy can only be used with an ABox that has a single individual.");

  initialize();
  
  m_aMayNeedExpanding.clear();
  m_aMayNeedExpanding.push_back(m_pRoot);

  while( !m_pABox->m_bIsComplete && !m_pABox->isClosed() )
    {
      Individual* pX = getNextIndividual();
      if( pX == NULL )
	{
	  m_pABox->m_bIsComplete = TRUE;
	  break;
	}

      expand(pX);
      
      if( m_pABox->isClosed() )
	{
	  if( backtrack() )
	    m_pABox->setClash(NULL);
	  else
	    m_pABox->m_bIsComplete = TRUE;
	}
    }

  if( PARAMS_USE_ADVANCED_CACHING() )
    {
      // if completion tree is clash free cache all sat concepts
      if( !m_pABox->isClosed() )
	{
	  for(Expr2NodeMap::iterator i = m_pABox->m_mNodes.begin(); i != m_pABox->m_mNodes.end(); i++ )
	    {
	      Node* pN = (Node*)i->second;
	      if( !pN->isIndividual() ) continue;
	      Individual* pNode = (Individual*)pN;
	      ExprNode* pC = NULL;
	      
	      Node2ExprNodeMap::iterator iFind = m_mCachedNodes.find(pNode);
	      if( iFind != m_mCachedNodes.end() )
		pC = (ExprNode*)iFind->second;

	      if( pC )
		{
		  if( m_pABox->m_pCache->putSat(pC, TRUE) )
		    {
		      if( pC->m_iExpression == EXPR_AND )
			{
			  for(int j = 0; j < ((ExprNodeList*)pC->m_pArgList)->m_iUsedSize; j++ )
			    {
			      ExprNode* pD = ((ExprNodeList*)pC->m_pArgList)->m_pExprNodes[j];
			      m_pABox->m_pCache->putSat(pD, TRUE);
			    }
			}
		    }
		}
	    }
	}
    }

  return m_pABox;
}

Individual* EmptySHNStrategy::getNextIndividual()
{
  Node* pNext = NULL;
  while( m_aMayNeedExpanding.size() > 0 )
    {
      pNext = m_aMayNeedExpanding.front();
      if( !pNext->isIndividual() )
	{
	  pNext = NULL;
	  m_aMayNeedExpanding.pop_front();
	}
      else
	break;
    }
  return (Individual*)pNext;
}

void EmptySHNStrategy::expand(Individual* pX)
{
  if( m_pBlocking->isBlocked(pX) )
    {
      m_aMayNeedExpanding.pop_front();
      return;
    }

  if( PARAMS_USE_ADVANCED_CACHING() )
    {
      int iCachedSat = cachedSat(pX);
      if( iCachedSat != -1 )
	{
	  if( iCachedSat == 1 )
	    m_aMayNeedExpanding.pop_front();
	  else
	    {
	      // set the clash information to be the union of all types
	      DependencySet* pDS = &DEPENDENCYSET_EMPTY;
	      
	      for(ExprNode2DependencySetMap::iterator i = pX->m_mDepends.begin(); i != pX->m_mDepends.end(); i++ )
		{
		  ExprNode* pC = (ExprNode*)i->first;
		  pDS = pDS->unionNew(pX->getDepends(pC), m_pABox->m_bDoExplanation);
		}
	      m_pABox->setClash(Clash::atomic(pX, pDS));
	    }
	  return;
	}
    }

  do
    {
      applyUnfoldingRule(pX);
      if( m_pABox->isClosed() )
	return;
      
      applyDisjunctionRule(pX);
      if( m_pABox->isClosed() )
	return;

      if( pX->canApply(Node::ATOM) || pX->canApply(Node::OR) )
	continue;

      applySomeValuesRule(pX);
      if( m_pABox->isClosed() )
	return;
      
      applyMinRule(pX);
      if( m_pABox->isClosed() )
	return;
             
      // we don't have any inverse properties but we could have 
      // domain restrictions which means we might have to re-apply
      // unfolding and disjunction rules
      if( pX->canApply(Node::ATOM) || pX->canApply(Node::OR) )
	continue;
      
      applyChooseRule(pX);
      if( m_pABox->isClosed() )
	return;

      applyMaxRule(pX);
      if( m_pABox->isClosed() )
	return;
    }
  while( pX->canApply(Node::ATOM) || pX->canApply(Node::OR) || pX->canApply(Node::SOME) || pX->canApply(Node::MIN) );

  m_aMayNeedExpanding.pop_front();
  
  NodeSet setSortedSuccessors;
  pX->getSortedSuccessors(&setSortedSuccessors);

  for(NodeSet::iterator i = setSortedSuccessors.begin(); i != setSortedSuccessors.end(); i++ )
    {
      if( PARAMS_USE_DEPTHFIRST_SEARCH() )
	m_aMayNeedExpanding.push_front((Node*)*i);
      else
	m_aMayNeedExpanding.push_back((Node*)*i);
    }
}

int EmptySHNStrategy::cachedSat(Individual* pX)
{
  if( ::isEqual(pX, m_pRoot) )
    return -1;

  ExprNode* pC = createConcept(pX);

  for(Node2ExprNodeMap::iterator i = m_mCachedNodes.begin(); i != m_mCachedNodes.end(); i++ )
    {
      ExprNode* pE = (ExprNode*)i->second;
      if( isEqual(pE, pC) == 0 )
	return 1;
    }
  
  int iSat = m_pABox->getCachedSat(pC);
  if( iSat == -1 && pC->m_iExpression == EXPR_AND )
    {
      ExprNodeList* pConcepts = (ExprNodeList*)pC->m_pArgList;
      if( pConcepts->m_iUsedSize == 2 )
	{
	  ExprNode* pC1 = pConcepts->m_pExprNodes[0];
	  ExprNode* pC2 = pConcepts->m_pExprNodes[1];
	  CachedNode* pCached1 = m_pABox->getCached(pC1);
	  CachedNode* pCached2 = m_pABox->getCached(pC2);

	  if( pCached1 && pCached1->isComplete() && pCached2 && pCached2->isComplete() )
	    {
	      iSat = m_pABox->mergable(pCached1->m_pNode, pCached2->m_pNode, (pCached1->m_pDepends->isIndependent()&&pCached2->m_pDepends->isIndependent()));
	      if( iSat != -1 )
		m_pABox->m_pCache->putSat(pC, (iSat==1));
	    }
	}
    }

  if( iSat == -1 )
    m_mCachedNodes[pX] = pC;

  return iSat;
}

ExprNode* EmptySHNStrategy::createConcept(Individual* pX)
{
  ExprNodeSet set;
  for(ExprNode2DependencySetMap::iterator i = pX->m_mDepends.begin(); i != pX->m_mDepends.end(); i++ )
    set.insert((ExprNode*)i->first);

  set.erase(EXPRNODE_TOP);
  set.erase(createExprNode(EXPR_VALUE, pX->m_pName));

  if( set.size() == 0 )
    return EXPRNODE_TOP;

  ExprNodeList* pList = createExprNodeList();
  int iCount = 0;
  for(ExprNodeSet::iterator i = set.begin(); i != set.end(); i++ )
    {
      ExprNode* pC = (ExprNode*)*i;
      if( pC->m_iExpression != EXPR_AND )
	addExprToList(pList, pC);
    }
  
  return createExprNode(EXPR_AND, pList);
}

void EmptySHNStrategy::restore(Branch* pBranch)
{
  Node* pClashNode = m_pABox->getClash()->m_pNode;

  ExprNodes aPath;
  pClashNode->getPath(&aPath);
  aPath.push_back( pClashNode->m_pName );
  
  m_pABox->m_iCurrentBranchIndex = pBranch->m_iBranch;
  m_pABox->setClash(NULL);
  m_pABox->m_iAnonCount = pBranch->m_iAnonCount;
  
  m_aMergeList.clear();

  int iIndex = 0;
  for(ExprNodes::iterator i = m_pABox->m_aNodeList.begin(); i != m_pABox->m_aNodeList.end(); i++ )
    {
      ExprNode* pX = (ExprNode*)*i;
      Node* pNode = m_pABox->getNode(pX);
      if( iIndex >= pBranch->m_iNodeCount )
	{
	  m_pABox->m_mNodes.erase(pX);

	  ExprNode* pC = NULL;
	  Node2ExprNodeMap::iterator iFind = m_mCachedNodes.find(pNode);
	  if( iFind != m_mCachedNodes.end() )
	    pC = (ExprNode*)iFind->second;
	  m_mCachedNodes.erase(pNode);

	  if( pC && PARAMS_USE_ADVANCED_CACHING() )
	    m_pABox->m_pCache->putSat(pC, FALSE);
	}
      else
	{
	  pNode->restore(pBranch->m_iBranch);
	  
	  // FIXME should we look at the clash path or clash node
	  if( ::isEqual(pNode, pClashNode) )
	    m_mCachedNodes.erase(pNode);
	}
      iIndex++;
    }

  int iSize = m_pABox->m_aNodeList.size();
  for(int i = pBranch->m_iNodeCount; i < iSize; i++ )
    m_pABox->m_aNodeList.pop_back();
  
  for(Expr2NodeMap::iterator i = m_pABox->m_mNodes.begin(); i != m_pABox->m_mNodes.end(); i++ )
    {
      Node* pN = (Node*)i->second;
      if( !pN->isIndividual() ) continue;
      Individual* pNode = (Individual*)pN;
      applyAllValues(pNode);
    }
}

bool EmptySHNStrategy::backtrack()
{
  bool bBranchFound = FALSE;
  while( !bBranchFound )
    {
      int iLastBranch = m_pABox->getClash()->m_pDepends->getMax();
      if( iLastBranch <= 0 )
	return FALSE;
      
      Branch* pNewBranch = NULL;
      if( iLastBranch <= m_pABox->m_aBranches.size() )
	{
	  for(int i = iLastBranch; i < m_pABox->m_aBranches.size(); i++ )
	    m_pABox->m_aBranches.pop_back();
	  pNewBranch = m_pABox->m_aBranches.at(iLastBranch-1);
	  
	  if( pNewBranch == NULL || iLastBranch != pNewBranch->m_iBranch )
	    assertFALSE("Internal error in reasoner: Trying to backtrack branch ");

	  if( pNewBranch->m_iTryNext < pNewBranch->m_iTryCount )
	    pNewBranch->setLastClash(m_pABox->getClash()->m_pDepends);

	  pNewBranch->m_iTryNext++;

	  if( pNewBranch->m_iTryNext < pNewBranch->m_iTryCount )
	    {
	      restore(pNewBranch);
	      bBranchFound = pNewBranch->tryNext();
	    }
	}

      if( !bBranchFound )
	m_pABox->getClash()->m_pDepends->remove(iLastBranch);
      else
	{
	  m_aMayNeedExpanding.clear();
	  for(Nodes::iterator i = m_aMNX.begin(); i != m_aMNX.end(); i++ )
	    m_aMayNeedExpanding.push_back((Node*)*i);
	}
    }
  m_pABox->validate();
  return bBranchFound;
}

void EmptySHNStrategy::addBranch(Branch* pNewBranch)
{
  CompletionStrategy::addBranch(pNewBranch);

  //newBranch.put("mnx", new ArrayList(mayNeedExpanding));
  m_aMNX.clear();
  for(Nodes::iterator i = m_aMayNeedExpanding.begin(); i != m_aMayNeedExpanding.end(); i++ )
    m_aMNX.push_back((Node*)*i);
}
