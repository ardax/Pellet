#include "TaxonomyNode.h"
#include "ReazienerUtils.h"
#include <cassert>
extern int g_iCommentIndent;

TaxonomyNode::TaxonomyNode(ExprNode* pName, bool bHidden)
{
  m_pName = pName;
  m_bHidden = bHidden;
  m_bMark = NOT_MARKED;
  m_setEquivalents.insert(pName);
} 

void TaxonomyNode::addSupers(TaxonomyNodes* paSupers)
{
  START_DECOMMENT2("TaxonomyNode::addSupers");
  printExprNodeWComment("this.Node=", m_pName);
  for(TaxonomyNodes::iterator i = paSupers->begin(); i != paSupers->end(); i++ )
    {
      TaxonomyNode* pOther = (TaxonomyNode*)*i;
      printExprNodeWComment("Super=", pOther->m_pName);

      m_aSupers.push_back(pOther);
      if( !m_bHidden )
	{
	  printExprNodeWComment("Sub(rev)=", m_pName);
	  pOther->m_aSubs.push_back(this);
	}
    }
  END_DECOMMENT("TaxonomyNode::addSupers");
}

void TaxonomyNode::addSub(TaxonomyNode* pNode)
{
  START_DECOMMENT2("TaxonomyNode::addSub");
  printExprNodeWComment("this.Node=", m_pName);

  if( isEqualTaxonomyNode(this, pNode) == 0 || contains(&m_aSubs, pNode) )
    {
      END_DECOMMENT("TaxonomyNode::addSub(1)");
      return;
    }

  printExprNodeWComment("Sub=", pNode->m_pName);
  m_aSubs.push_back(pNode);

  if( !m_bHidden )
    {
      printExprNodeWComment("Super(rev)=", m_pName);
      pNode->m_aSupers.push_back(this);

    }
  END_DECOMMENT("TaxonomyNode::addSub(2)");
}

void TaxonomyNode::removeSub(TaxonomyNode* pNode)
{
  START_DECOMMENT2("TaxonomyNode::removeSub");
  printExprNodeWComment("this.Node=", m_pName);
  printExprNodeWComment("Removed.Node=", pNode->m_pName);

  remove(&m_aSubs, pNode);
  remove(&(pNode->m_aSupers), this);
  pNode->m_mSuperExplanations.erase(this);
  END_DECOMMENT("TaxonomyNode::removeSub");
}

void TaxonomyNode::addSubs(TaxonomyNodes* paSubs)
{
  START_DECOMMENT2("TaxonomyNode::addSubs");
  printExprNodeWComment("this.Node=", m_pName);
  for(TaxonomyNodes::iterator i = paSubs->begin(); i != paSubs->end(); i++ )
    {
      TaxonomyNode* pOther = (TaxonomyNode*)*i;
      printExprNodeWComment("pSub=", pOther->m_pName);
      m_aSubs.push_back(pOther);
      if( !m_bHidden )
	pOther->m_aSupers.push_back(this);
    }
  END_DECOMMENT("TaxonomyNode::addSubs");
}

void TaxonomyNode::addEquivalent(ExprNode* pC)
{
  START_DECOMMENT2("TaxonomyNode::addEquivalent");
  printExprNodeWComment("pC=", pC);
  m_setEquivalents.insert(pC);
  END_DECOMMENT("TaxonomyNode::addEquivalent");
}

void TaxonomyNode::addSuperExplanation(TaxonomyNode* pSup, ExprNodeSet* pExplanation)
{
  SetOfExprNodeSet* pSet = NULL;
  TaxonomyNode2SetOfExprNodeSet::iterator i = m_mSuperExplanations.find(pSup);
  if( i == m_mSuperExplanations.end() )
    {
      pSet = new SetOfExprNodeSet;
      m_mSuperExplanations[pSup] = pSet;
    }
  else
    pSet = (SetOfExprNodeSet*)i->second;
  
  pSet->insert(pExplanation);
}

void TaxonomyNode::removeMultiplePaths()
{
  if( !m_bHidden )
    {
      for(TaxonomyNodes::iterator i = m_aSupers.begin(); i != m_aSupers.end(); i++ )
	{
	  TaxonomyNode* pSup = (TaxonomyNode*)*i;
	  for(TaxonomyNodes::iterator j = m_aSubs.begin(); j != m_aSubs.end(); j++ )
	    {
	      TaxonomyNode* pSub = (TaxonomyNode*)*j;
	      pSup->removeSub(pSub);
	    }
	}
    }
}

void TaxonomyNode::setSubs(TaxonomyNodes* pSubs)
{
  START_DECOMMENT2("TaxonomyNode::setSubs");
  printExprNodeWComment("this.Node=", m_pName);
  m_aSubs.clear();
  for(TaxonomyNodes::iterator i = pSubs->begin(); i != pSubs->end(); i++ )
    {
      printExprNodeWComment("Sub=", ((TaxonomyNode*)*i)->m_pName);
      m_aSubs.push_back((TaxonomyNode*)*i);
    }
  END_DECOMMENT("TaxonomyNode::setSubs");
}

void TaxonomyNode::setSupers(TaxonomyNodes* pSupers)
{
  START_DECOMMENT2("TaxonomyNode::setSupers");
  printExprNodeWComment("this.Node=", m_pName);
  m_aSupers.clear();
  for(TaxonomyNodes::iterator i = pSupers->begin(); i != pSupers->end(); i++ )
    {
      printExprNodeWComment("Super=", ((TaxonomyNode*)*i)->m_pName);
      m_aSupers.push_back((TaxonomyNode*)*i);
    }
  END_DECOMMENT("TaxonomyNode::setSupers");
}

bool TaxonomyNode::isLeaf()
{
  return (m_aSubs.size()==1 && ((TaxonomyNode*)(*m_aSubs.begin()))->isBottom());
}

bool TaxonomyNode::isBottom()
{
  return (m_aSubs.size()==0);
}

SetOfExprNodeSet* TaxonomyNode::getSuperExplanations(TaxonomyNode* pNode)
{
  TaxonomyNode2SetOfExprNodeSet::iterator i = m_mSuperExplanations.find(pNode);
  if( i != m_mSuperExplanations.end() )
    return (SetOfExprNodeSet*)i->second;
  return NULL;
}

void TaxonomyNode::disconnect()
{
  for(TaxonomyNodes::iterator i = m_aSubs.begin(); i != m_aSubs.end(); i++ )
    {
      TaxonomyNode* pSub = (TaxonomyNode*)*i;
      remove(&(pSub->m_aSupers), this);
    }
  m_aSubs.clear();
  for(TaxonomyNodes::iterator i = m_aSupers.begin(); i != m_aSupers.end(); i++ )
    {
      TaxonomyNode* pSup = (TaxonomyNode*)*i;
      remove(&(pSup->m_aSubs), this);
    }
  m_aSupers.clear();
}

void TaxonomyNode::addInstance(ExprNode* pInd)
{
  m_setInstances.insert(pInd);
}

void TaxonomyNode::setInstances(ExprNodeSet* pSet)
{
  m_setInstances.clear();
  for(ExprNodeSet::iterator i = pSet->begin(); i != pSet->end(); i++ )
    m_setInstances.insert((ExprNode*)*i);
}

int isEqualTaxonomyNode(const TaxonomyNode* pTaxonomyNode1, const TaxonomyNode* pTaxonomyNode2)
{
  return isEqual(pTaxonomyNode1->m_pName, pTaxonomyNode2->m_pName);
}

bool contains(TaxonomyNodes* pNodeList, TaxonomyNode* pNode)
{
  for(TaxonomyNodes::iterator i = pNodeList->begin(); i != pNodeList->end(); i++ )
    {
      TaxonomyNode* pTN = (TaxonomyNode*)*i;
      if( pTN == pNode )
	return TRUE;
      else if( isEqualTaxonomyNode(pTN, pNode) == 0 )
	assert(FALSE);
    }
  return FALSE;
}

bool remove(TaxonomyNodes* pList, TaxonomyNode* pNode)
{
  for(TaxonomyNodes::iterator i = pList->begin(); i != pList->end(); i++ )
    {
      if( isEqualTaxonomyNode(((TaxonomyNode*)*i), pNode) == 0 )
	{
	  pList->erase(i);
	  return TRUE;
	}
    }
  return FALSE;
}
