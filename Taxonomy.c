#include "Taxonomy.h"
#include "TaxonomyNode.h"
#include "ReazienerUtils.h"
#include "TaxonomyPrinter.h"
#include <cassert>

extern ExprNode* EXPRNODE_TOP;
extern ExprNode* EXPRNODE_BOTTOM;
extern int g_iCommentIndent;

Taxonomy::Taxonomy()
{
  START_DECOMMENT2("Taxonomy::Taxonomy");
  m_bHideAnonTerms = FALSE;
  m_pTOP_NODE = addNode(EXPRNODE_TOP);
  m_pTOP_NODE->m_bHidden = FALSE;
  m_pBOTTOM_NODE = addNode(EXPRNODE_BOTTOM);
  m_pBOTTOM_NODE->m_bHidden = FALSE;
  m_pTOP_NODE->addSub(m_pBOTTOM_NODE);

  m_pPrinter = new TreeTaxonomyPrinter();
  END_DECOMMENT("Taxonomy::Taxonomy");
}

Taxonomy::Taxonomy(ExprNodes* paClasses, bool bHideTopBottom)
{
  START_DECOMMENT2("Taxonomy::Taxonomy");
  m_pTOP_NODE = addNode(EXPRNODE_TOP);
  m_pTOP_NODE->m_bHidden = bHideTopBottom;
  m_pBOTTOM_NODE = addNode(EXPRNODE_BOTTOM); 
  m_pBOTTOM_NODE->m_bHidden = bHideTopBottom;

  if( paClasses == NULL )
    m_pTOP_NODE->addSub(m_pBOTTOM_NODE);
  else
    {
      TaxonomyNodes aNodes;
      for(ExprNodes::iterator i = paClasses->begin(); i != paClasses->end(); i++ )
	{
	  ExprNode* pC = (ExprNode*)*i;
	  TaxonomyNode* pNode = addNode(pC);
	  pNode->m_aSupers.push_back(m_pTOP_NODE);
	  pNode->m_aSubs.push_back(m_pBOTTOM_NODE);
	  aNodes.push_back(pNode);
	}

      m_pTOP_NODE->setSubs(&aNodes);
      m_pBOTTOM_NODE->setSupers(&aNodes);
    }

  m_pPrinter = new TreeTaxonomyPrinter();
  END_DECOMMENT("Taxonomy::Taxonomy");
}

Taxonomy::Taxonomy(ExprNodeSet* paClasses, bool bHideTopBottom)
{
  START_DECOMMENT2("Taxonomy::Taxonomy");
  m_pTOP_NODE = addNode(EXPRNODE_TOP);
  m_pTOP_NODE->m_bHidden = bHideTopBottom;
  m_pBOTTOM_NODE = addNode(EXPRNODE_BOTTOM); 
  m_pBOTTOM_NODE->m_bHidden = bHideTopBottom;

  if( paClasses == NULL )
    m_pTOP_NODE->addSub(m_pBOTTOM_NODE);
  else
    {
      TaxonomyNodes aNodes;
      for(ExprNodeSet::iterator i = paClasses->begin(); i != paClasses->end(); i++ )
	{
	  ExprNode* pC = (ExprNode*)*i;
	  TaxonomyNode* pNode = addNode(pC);
	  pNode->m_aSupers.push_back(m_pTOP_NODE);
	  pNode->m_aSubs.push_back(m_pBOTTOM_NODE);
	  aNodes.push_back(pNode);
	}

      m_pTOP_NODE->setSubs(&aNodes);
      m_pBOTTOM_NODE->setSupers(&aNodes);
    }

  m_pPrinter = new TreeTaxonomyPrinter();
  END_DECOMMENT("Taxonomy::Taxonomy");
}

void Taxonomy::assertValid()
{
  
}

TaxonomyNode* Taxonomy::addNode(ExprNode* pC)
{
  START_DECOMMENT2("Taxonomy::addNode");

  bool bHide = m_bHideAnonTerms && !isPrimitive(pC);
  TaxonomyNode* pNode = new TaxonomyNode(pC, bHide);
  m_mNodes[pC] = pNode;

  printExprNodeWComment("pC=", pC);
  printExprNodeWComment("pNode=", pNode->m_pName);
  END_DECOMMENT("Taxonomy::addNode");
  return pNode;
}

TaxonomyNode* Taxonomy::getNode(ExprNode* pC)
{
  ExprNode2TaxonomyNodeMap::iterator i = m_mNodes.find(pC);
  if( i != m_mNodes.end() )
    return (TaxonomyNode*)i->second;
  return NULL;
}

TaxonomyNode* Taxonomy::getBottom()
{
  return m_pBOTTOM_NODE;
}

TaxonomyNode* Taxonomy::getTop()
{
  return m_pTOP_NODE;
}

void Taxonomy::addEquivalentNode(ExprNode* pC, TaxonomyNode* pNode)
{
  START_DECOMMENT2("Taxonomy::addEquivalentNode");
  printExprNodeWComment("pC=", pC);
  printExprNodeWComment("pNode=", pNode->m_pName);

  bool bHide = (!isPrimitive(pC));
  if( !bHide )
    pNode->addEquivalent(pC);
  m_mNodes[pC] = pNode;
  END_DECOMMENT("Taxonomy::addEquivalentNode");
}

void Taxonomy::getAllEquivalents(ExprNode* pC, ExprNodeSet* pEquivalents)
{
  TaxonomyNode* pNode = getNode(pC);
  if( pNode == NULL )
    assertFALSE("'pC' is an unknown class!");
  if( pNode->m_bHidden )
    return;

  for(ExprNodeSet::iterator i = pNode->m_setEquivalents.begin(); i != pNode->m_setEquivalents.end(); i++ )
    pEquivalents->insert((ExprNode*)*i);

  if( m_bHideAnonTerms )
    removeAnonTerms(pEquivalents);
}

void Taxonomy::getEquivalents(ExprNode*pC, ExprNodeSet* pEquivalents)
{
  TaxonomyNode* pNode = getNode(pC);
  if( pNode == NULL )
    assertFALSE("'pC' is an unknown class!");
  if( pNode->m_bHidden )
    return;

  ExprNodeSet setEq;
  for(ExprNodeSet::iterator i = pNode->m_setEquivalents.begin(); i != pNode->m_setEquivalents.end(); i++ )
    {
      ExprNode* pE = (ExprNode*)*i;
      if( isEqual(pC, pE) != 0 )
	setEq.insert(pE);
    }
  
  if( m_bHideAnonTerms )
    removeAnonTerms(&setEq);

  for(ExprNodeSet::iterator i = setEq.begin(); i != setEq.end(); i++ )
    pEquivalents->insert((ExprNode*)*i);
}

bool Taxonomy::contains(ExprNode* pC)
{
  return (m_mNodes.find(pC)!=m_mNodes.end());
}

bool Taxonomy::isType(ExprNode* pInd, ExprNode* pC)
{
  TaxonomyNode* pNode = getNode(pC);
  if( pNode == NULL )
    assertFALSE( "pC is an unknown class!");
  return isType(pNode, pInd);
}

bool Taxonomy::isType(TaxonomyNode* pNode, ExprNode* pInd)
{
  if( pNode->m_setInstances.find(pInd) != pNode->m_setInstances.end() )
    return TRUE;
  for(TaxonomyNodes::iterator i = pNode->m_aSubs.begin(); i != pNode->m_aSubs.end(); i++ )
    {
      if( isType(((ExprNode*)*i), pInd) )
	return TRUE;
    }
  return FALSE;
}

void Taxonomy::getSubSupers(ExprNode* pC, bool bDirect, bool bSupers, SetOfExprNodeSet* pSet)
{
  TaxonomyNode* pNode = getNode(pC);
  if( pNode == NULL )
    return;

  TaxonomyNodes aVisit;
  if( bSupers )
    {
      for(TaxonomyNodes::iterator i = pNode->m_aSupers.begin(); i != pNode->m_aSupers.end(); i++ )
	aVisit.push_back((TaxonomyNode*)*i);
    }
  else
    {
      for(TaxonomyNodes::iterator i = pNode->m_aSubs.begin(); i != pNode->m_aSubs.end(); i++ )
	aVisit.push_back((TaxonomyNode*)*i);
    }

  for(int j = 0; j < aVisit.size(); j++ )
    {
      pNode = (TaxonomyNode*)aVisit.at(j);

      if( pNode->m_bHidden )
	continue;

      ExprNodeSet* pSetAdd = new ExprNodeSet;
      copySet(pSetAdd, &(pNode->m_setEquivalents));

      if( m_bHideAnonTerms )
	removeAnonTerms(pSetAdd);
      if( pNode->m_setEquivalents.size() > 0 )
	pSet->insert(pSetAdd);

      if( !bDirect )
	{
	  if( bSupers )
	    {
	      for(TaxonomyNodes::iterator i = pNode->m_aSupers.begin(); i != pNode->m_aSupers.end(); i++ )
		aVisit.push_back((TaxonomyNode*)*i);
	    }
	  else
	    {
	      for(TaxonomyNodes::iterator i = pNode->m_aSubs.begin(); i != pNode->m_aSubs.end(); i++ )
		aVisit.push_back((TaxonomyNode*)*i);
	    }
	}
    }
}

void Taxonomy::getFlattenedSubSupers(ExprNode* pC, bool bDirect, bool bSupers, ExprNodeSet* pSet)
{
  START_DECOMMENT2("Taxonomy::getFlattenedSubSupers");
  printExprNodeWComment("pC=", pC);

  TaxonomyNode* pNode = getNode(pC);
  if( pNode == NULL )
    {
      END_DECOMMENT("Taxonomy::getFlattenedSubSupers(1)");
      return;
    }

  TaxonomyNodes aVisit;
  if( bSupers )
    {
      for(TaxonomyNodes::iterator i = pNode->m_aSupers.begin(); i != pNode->m_aSupers.end(); i++ )
	aVisit.push_back((TaxonomyNode*)*i);
    }
  else
    {
      for(TaxonomyNodes::iterator i = pNode->m_aSubs.begin(); i != pNode->m_aSubs.end(); i++ )
	aVisit.push_back((TaxonomyNode*)*i);
    }

  for(int j = 0; j < aVisit.size(); j++ )
    {
      pNode = (TaxonomyNode*)aVisit.at(j);
      if( pNode->m_bHidden )
	continue;
      
      for(ExprNodeSet::iterator i = pNode->m_setEquivalents.begin(); i != pNode->m_setEquivalents.end(); i++ )
	{
	  if( bSupers ) 
	    printExprNodeWComment("Super=", (ExprNode*)*i);
	  else
	    printExprNodeWComment("Sub=", (ExprNode*)*i);
	  printExprNodeWComment("via", pNode->m_pName);
	  
	  pSet->insert((ExprNode*)*i);
	}

      if( !bDirect )
	{
	  if( bSupers )
	    {
	      for(TaxonomyNodes::iterator i = pNode->m_aSupers.begin(); i != pNode->m_aSupers.end(); i++ )
		aVisit.push_back((TaxonomyNode*)*i);
	    }
	  else
	    {
	      for(TaxonomyNodes::iterator i = pNode->m_aSubs.begin(); i != pNode->m_aSubs.end(); i++ )
		aVisit.push_back((TaxonomyNode*)*i);
	    }
	}
    }

  if( m_bHideAnonTerms )
    removeAnonTerms(pSet);
  END_DECOMMENT("Taxonomy::getFlattenedSubSupers(2)");
}

void Taxonomy::removeAnonTerms(ExprNodeSet* pTerms)
{
  int iSize = pTerms->size(), iIndex = 0;
  for(ExprNodeSet::iterator i = pTerms->begin(); i != pTerms->end() && iIndex < iSize; i++, iIndex++ )
    {
      ExprNode* pTerm = (ExprNode*)*i;
      if( !isPrimitive(pTerm) && isEqual(pTerm, EXPRNODE_BOTTOM) != 0 )
	pTerms->erase(i);
    }
}

/**
 * Given a list of concepts, find all the Least Common Ancestors (LCA). Note
 * that a taxonomy is DAG not a tree so we do not have a unique LCA but a
 * set of LCA. The function might return a singleton list that contains TOP
 * if there are no lower level nodes that satisfy the LCA condition.
 */
void Taxonomy::computeLCA(ExprNodeList* pList, ExprNodes* pLCA)
{
  // FIXME does not work when one of the elements is an ancestor of the rest
  // TODO what to do with equivalent classes?
  // TODO improve efficiency
  
  // argument to retrieve all supers (not just direct ones)
  bool bAllSupers = FALSE;
  // argument to retrieve supers in a flat set
  bool bFlat = TRUE;

  // get the first concept
  ExprNode* pC = pList->m_pExprNodes[0];
  
  // add all its ancestor as possible LCA candidates
  ExprNodeSet setAncestors;
  getFlattenedSubSupers(pC, bAllSupers, TRUE, &setAncestors);
  for(ExprNodeSet::iterator i = setAncestors.begin(); i != setAncestors.end(); i++ )
    pLCA->push_back((ExprNode*)*i);
  
  for(int i = 0; i < pList->m_iUsedSize; i++ )
    {
      pC = pList->m_pExprNodes[i];
      
      // take the intersection of possible candidates to get rid of
      // uncommon ancestors
      ExprNodeSet setAncestors2;
      getFlattenedSubSupers(pC, bAllSupers, TRUE, &setAncestors2);
      retainAll(pLCA, &setAncestors2);

      if( pLCA->size() == 1 )
	{
	  assert( isEqual(((ExprNode*)(*pLCA->begin())), EXPRNODE_TOP) == 0 );
	  return ;
	}
    }

  // we have all common ancestors now remove the ones that have
  // descendants in the list
  ExprNodeSet setToBeRemoved;
  for(ExprNodes::iterator i = pLCA->begin(); i != pLCA->end(); i++ )
    {
      pC = (ExprNode*)*i;
      if( setToBeRemoved.find(pC) != setToBeRemoved.end() )
	continue;

      ExprNodeSet setSupers;
      getFlattenedSubSupers(pC, bAllSupers, TRUE, &setSupers);
      setToBeRemoved.insert(setSupers.begin(), setSupers.end());
    }

  removeAll(pLCA, &setToBeRemoved);
}

void Taxonomy::merge(TaxonomyNode* pNode1, TaxonomyNode* pNode2)
{
  START_DECOMMENT2("Taxonomy::merge");
  printExprNodeWComment("Node1=", pNode1->m_pName);
  printExprNodeWComment("Node2=", pNode2->m_pName);

  TaxonomyNodes aTaxonomyNodes;
  aTaxonomyNodes.push_back(pNode1);
  aTaxonomyNodes.push_back(pNode2);
  TaxonomyNode* pNode = mergeNodes(&aTaxonomyNodes);
  removeCycles(pNode);
  END_DECOMMENT("Taxonomy::merge");
}

TaxonomyNode* Taxonomy::mergeNodes(TaxonomyNodes* pMergeList)
{
  START_DECOMMENT2("Taxonomy::mergeNodes");
  for(TaxonomyNodes::iterator i = pMergeList->begin(); i != pMergeList->end(); i++ )
    {
      TaxonomyNode* pOther = (TaxonomyNode*)*i;
      printExprNodeWComment("Node=", pOther->m_pName);
    }

  TaxonomyNode* pNode = NULL;
  if( ::contains(pMergeList, m_pTOP_NODE) )
    pNode = m_pTOP_NODE;
  else if( ::contains(pMergeList, m_pBOTTOM_NODE) )
    pNode = m_pBOTTOM_NODE;
  else
    pNode = (TaxonomyNode*)(*pMergeList->begin());

  TaxonomyNodeSet setMerged;
  setMerged.insert(pNode);

  for(TaxonomyNodes::iterator i = pMergeList->begin(); i != pMergeList->end(); i++ )
    {
      TaxonomyNode* pOther = (TaxonomyNode*)*i;
      if( setMerged.find(pOther) != setMerged.end() )
	continue;
      setMerged.insert(pOther);

      for(TaxonomyNodes::iterator j = pOther->m_aSubs.begin(); j != pOther->m_aSubs.end(); j++ )
	{
	  TaxonomyNode* pSub = (TaxonomyNode*)*j;
	  if( !::contains(pMergeList, pSub) )
	    pNode->addSub(pSub);
	}

      for(TaxonomyNodes::iterator j = pOther->m_aSupers.begin(); j != pOther->m_aSupers.end(); j++ )
	{
	  TaxonomyNode* pSuper = (TaxonomyNode*)*j;
	  if( !::contains(pMergeList, pSuper) )
	    {
	      pSuper->addSub(pNode);
	      SetOfExprNodeSet* pSet = pOther->getSuperExplanations(pSuper);
	      if( pSet )
		{
		  for(SetOfExprNodeSet::iterator s = pSet->begin(); s != pSet->end(); s++ )
		    {
		      ExprNodeSet* pExp = (ExprNodeSet*)*s;
		      pNode->addSuperExplanation(pSuper, pExp);
		    }
		}
	    }
	}

      removeNode(pOther);
      
      for(ExprNodeSet::iterator j = pOther->m_setEquivalents.begin(); j != pOther->m_setEquivalents.end(); j++ )
	addEquivalentNode(((ExprNode*)*j), pNode);
    }

  END_DECOMMENT("Taxonomy::mergeNodes");
  return pNode;
}

void Taxonomy::removeNode(TaxonomyNode* pNode)
{
  pNode->disconnect();
  m_mNodes.erase(pNode->m_pName);
}

/**
 * Walk through the super nodes of the given node and when a cycle is
 * detected merge all the nodes in that path
 */
void Taxonomy::removeCycles(TaxonomyNode* pNode)
{
  ExprNode2TaxonomyNodeMap::iterator iFind = m_mNodes.find(pNode->m_pName);
  if( iFind != m_mNodes.end() )
    {
      TaxonomyNode* pTaxonomyNode = (TaxonomyNode*)iFind->second;
      if( isEqualTaxonomyNode(pTaxonomyNode, pNode) != 0 )
	assertFALSE("This node does not exist in the taxonomy: ");

      TaxonomyNodes aPath;
      removeCycles(pNode, &aPath);
    }
}

/**
 * Given a node and (a possibly empty) path of sub nodes, remove cycles by
 * merging all the nodes in the path.
 */
bool Taxonomy::removeCycles(TaxonomyNode* pNode, TaxonomyNodes* pPath)
{
  START_DECOMMENT2("Taxonomy::removeCycles");
  printExprNodeWComment("Node=", pNode->m_pName);
  for(int i = 0; i < pNode->m_aSupers.size(); i++)
    {
      TaxonomyNode* pSup = (TaxonomyNode*)pNode->m_aSupers.at(i);
      printExprNodeWComment("Super=", pSup->m_pName);
    }

  // cycle detected
  if( ::contains(pPath, pNode) )
    {
      mergeNodes(pPath);
      END_DECOMMENT("Taxonomy::removeCycles");
      return TRUE;
    }
  
  // no cycle yet, add this node to the path and continue
  pPath->push_back(pNode);
  for(int i = 0; i < pNode->m_aSupers.size(); )
    {
      TaxonomyNode* pSup = (TaxonomyNode*)pNode->m_aSupers.at(i);
      // remove cycles involving super node
      removeCycles(pSup, pPath);
      // remove the super from the path
      ::remove(pPath, pSup);
      // if the super has been removed then no need
      // to increment the index
      if( i < pNode->m_aSupers.size() && isEqualTaxonomyNode(pNode->m_aSupers.at(i), pSup) == 0 )
	i++;
    }
  END_DECOMMENT("Taxonomy::removeCycles");
  return FALSE;
}

/**
 * Sort the nodes in the taxonomy using topological ordering starting from
 * top to bottom.
 */
void Taxonomy::topologicalSort(bool bIncludeEquivalents, ExprNodes* pSorted)
{
  START_DECOMMENT2("Taxonomy::topologicalSort");
  TaxonomyNodeSet setLeftNodes;
  TaxonomyNodes aPendingNodes;
  TaxonomyNode2Int mDegrees;

  for(ExprNode2TaxonomyNodeMap::iterator i = m_mNodes.begin(); i != m_mNodes.end(); i++)
    {
      TaxonomyNode* pNode = (TaxonomyNode*)i->second;
      setLeftNodes.insert(pNode);
      int iDegree = pNode->m_aSupers.size();
      if( iDegree == 0 )
	aPendingNodes.push_back(pNode);
      mDegrees[pNode] = iDegree;

      //printf("Node(Degree=%d)=", iDegree);
      //printExprNode(pNode->m_pName);
    }

  if( aPendingNodes.size() != 1 )
    assertFALSE("More than one node with no incoming edges ");

  int iSize = setLeftNodes.size();
  for(int i = 0; i < iSize; i++ )
    {
      if( aPendingNodes.size() == 0 )
	{
	  //for(TaxonomyNodeSet::iterator k = setLeftNodes.begin(); k != setLeftNodes.end(); k++ )
	  //{
	  //  printf("not expanded=");
	  //  printExprNode(((TaxonomyNode*)*k)->m_pName);
	  //}
	  assertFALSE("Cycle detected in the taxonomy!");
	}

      TaxonomyNode* pNode = (TaxonomyNode*)(*aPendingNodes.begin());

      //printf("Size=%d Node(%d/%d)=", aPendingNodes.size(), i, iSize);
      //printExprNode(pNode->m_pName);

      int iDegree = 0;
      TaxonomyNode2Int::iterator iFind = mDegrees.find(pNode);
      if( iFind != mDegrees.end() )
	iDegree = (int)iFind->second;
      if( iDegree != 0 )
	assertFALSE("Cycle detected in the taxonomy pNode");

      ::remove(&aPendingNodes, pNode);
      setLeftNodes.erase(pNode);

      if( bIncludeEquivalents )
	{
	  for(ExprNodeSet::iterator j = pNode->m_setEquivalents.begin(); j != pNode->m_setEquivalents.end(); j++ )
	    pSorted->push_back((ExprNode*)*j);
	}
      else
	pSorted->push_back(pNode->m_pName);

      for(TaxonomyNodes::iterator j = pNode->m_aSubs.begin(); j != pNode->m_aSubs.end(); j++ )
	{
	  TaxonomyNode* pSub = (TaxonomyNode*)*j;
	  int iDegree = 0;
	  TaxonomyNode2Int::iterator iFind = mDegrees.find(pSub);
	  if( iFind != mDegrees.end() )
	    iDegree = (int)iFind->second;
	  if( iDegree == 1 )
	    {
	      aPendingNodes.push_back(pSub);
	      //printf("\tadded = ");
	      //printExprNode(pSub->m_pName);
	      mDegrees[pSub] = 0;
	    }
	  else
	    {
	      //printf("waiting(%d)=", iDegree);
	      // printExprNode(pSub->m_pName);
	      mDegrees[pSub] = iDegree-1;
	    }
	}
    }

  if( setLeftNodes.size() != 0 )
    assertFALSE("Failed to sort elements: ");

  END_DECOMMENT("Taxonomy::topologicalSort");
}

void Taxonomy::getInstances(ExprNode* pC, ExprNodeSet* pInstances, bool bDirect)
{
  TaxonomyNode* pNode = getNode(pC);
  if( pNode == NULL )
    assertFALSE("pC is an unknown class!");
  
  if( bDirect )
    {
      for(ExprNodeSet::iterator i = pNode->m_setInstances.begin(); i != pNode->m_setInstances.end(); i++ )
	pInstances->insert((ExprNode*)*i);
    }
  else
    getInstancesHelper(pNode, pInstances);
}

void Taxonomy::getInstancesHelper(TaxonomyNode* pNode, ExprNodeSet* pInstances)
{
  for(ExprNodeSet::iterator i = pNode->m_setInstances.begin(); i != pNode->m_setInstances.end(); i++ )
    pInstances->insert((ExprNode*)*i);
  for(TaxonomyNodes::iterator i = pNode->m_aSubs.begin(); i != pNode->m_aSubs.end(); i++ )
    getInstancesHelper(((TaxonomyNode*)*i), pInstances);
}

void Taxonomy::print()
{
  m_pPrinter->print(this);
}

void Taxonomy::printInFile(char* pFileName)
{
  m_pPrinter->printInFile(this, pFileName);
}
