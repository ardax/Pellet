#include "Node.h"
#include "Params.h"
#include "ReazienerUtils.h"
#include "KnowledgeBase.h"
#include "CompletionQueue.h"
#include "ABox.h"
#include "Clash.h"
#include "QueueElement.h"
#include "Individual.h"

extern DependencySet DEPENDENCYSET_EMPTY;
extern DependencySet DEPENDENCYSET_INDEPENDENT;
extern KnowledgeBase* g_pKB;

Node::Node(ExprNode* pName, ABox* pABox)
{
  m_pABox = pABox;
  m_pName = pName;
  m_bIsRoot = !isAnon(pName);
  m_bIsConceptRoot = FALSE;

  m_pMergeDepends = &DEPENDENCYSET_INDEPENDENT;
  m_pPruned = NULL;
  m_iBranch = 0;
  m_iStatus = NODE_CHANGED;
  m_pMergeTo = this;

  m_iDepth = 1;
}

Node::Node(Node* pNode, ABox* pABox)
{
  m_pABox = pABox;
  m_pName = pNode->m_pName;

  m_bIsRoot = pNode->m_bIsRoot;
  m_bIsConceptRoot = pNode->m_bIsConceptRoot;
  
  m_pMergeDepends = pNode->m_pMergeDepends;
  m_pMergeTo = pNode->m_pMergeTo;
  m_setMerged = pNode->m_setMerged;
  m_pPruned = pNode->m_pPruned;

  // do not copy differents right now because we need to
  // update node references later anyway
  m_mDifferents.insert(pNode->m_mDifferents.begin(), pNode->m_mDifferents.end());
  m_mDepends.insert(pNode->m_mDepends.begin(), pNode->m_mDepends.end());

  if( pABox == NULL )
    {
      m_pMergeTo = NULL;
      for(EdgeVector::iterator i = pNode->m_listInEdges.m_listEdge.begin(); i != pNode->m_listInEdges.m_listEdge.end(); i++ )
	{
	  Edge* pEdge = (Edge*)*i;
	  Node* pTo = this;
	  Individual* pFrom = new Individual(pEdge->m_pFrom->m_pName);
	  Edge* pNewEdge = new Edge(pEdge->m_pRole, pFrom, pTo, pEdge->m_pDepends);
	  m_listInEdges.addEdge(pNewEdge);
	}
    }
  else
    {
      for(EdgeVector::iterator i = pNode->m_listInEdges.m_listEdge.begin(); i != pNode->m_listInEdges.m_listEdge.end(); i++ )
	m_listInEdges.addEdge((Edge*)*i);
    }

  m_iBranch = pNode->m_iBranch;
  m_iStatus = NODE_CHANGED;
}

bool Node::isPruned()
{
  return (m_pPruned!=NULL);
}

bool Node::isMerged()
{
  return (m_pMergeTo!=this);
}

bool Node::isNominal()
{
  return FALSE;
}

bool Node::isRootNominal()
{
  return (m_bIsRoot&&isNominal());
}

bool Node::isRoot()
{
  return (m_bIsRoot||isNominal());
}

Node* Node::getSame()
{
  if( m_pMergeTo == NULL )
    return NULL;
  if( m_pMergeTo == this )
    return this;
  return m_pMergeTo->getSame();
}

void Node::addType(ExprNode* pC, DependencySet* pDS)
{
  if( isPruned() )
    assertFALSE("Adding type to a pruned node ");
  else if( isMerged() )
    return;

  if( m_pABox->m_iCurrentBranchIndex >= 0 && PARAMS_USE_COMPLETION_QUEUE() )
    m_pABox->m_pCompletionQueue->addEffected(m_pABox->m_iCurrentBranchIndex, m_pName);

  DependencySet* pDSCopy = new DependencySet(pDS);
  pDSCopy->m_iBranch = m_pABox->m_iCurrentBranchIndex;
  int iMax = pDSCopy->getMax();
  if( pDSCopy->m_iBranch == -1 && iMax != 0 )
    pDSCopy->m_iBranch = iMax+1;
  
  m_mDepends[pC] = pDSCopy;
  m_pABox->m_bChanged = TRUE;
}

void Node::removeType(ExprNode* pC)
{
  m_mDepends.erase(pC);
  m_iStatus = NODE_CHANGED;
}
  
bool Node::hasType(ExprNode* pC)
{
  return (m_mDepends.find(pC)!=m_mDepends.end());
}

void Node::addInEdge(Edge* pEdge)
{ 
  m_listInEdges.addEdge(pEdge); 
}

bool Node::removeInEdge(Edge* pEdge)
{
  return m_listInEdges.removeEdge(pEdge);
}

DependencySet* Node::getMergeDependency(bool bAll)
{
  if( !isMerged() || !bAll )
    return m_pMergeDepends;

  DependencySet* pDS = new DependencySet(m_pMergeDepends);
  Node* pN = m_pMergeTo;
  while( pN && pN->isMerged() )
    {
      pDS->unionDS(pN->m_pMergeDepends, m_pABox->doExplanation());
      pN = (Node*)pN->m_pMergeTo;
    }
  return pDS;
}

DependencySet* Node::getDepends(ExprNode* pC)
{
  ExprNode2DependencySetMap::iterator i = m_mDepends.find(pC);
  if( i != m_mDepends.end() )
    return (DependencySet*)i->second;
  return NULL;
}

bool Node::isDifferent(Node* pNode)
{
  return (m_mDifferents.find(pNode) != m_mDifferents.end());
}

bool Node::isSame(Node* pNode)
{
  return (isEqual(getSame(), pNode->getSame()) == 0);
}

void Node::setDifferent(Node* pNode, DependencySet* pDS)
{
  if( m_pABox->getBranch() >= 0 && PARAMS_USE_COMPLETION_QUEUE() )
    m_pABox->m_pCompletionQueue->addEffected(m_pABox->getBranch(), pNode->m_pName);
  
  if( isDifferent(pNode) )
    return;

  if( isSame(pNode) )
    {
      // CHW - added for incremental reasoning support - 
      // this is needed as we will need to backjump if possible
      if( PARAMS_USE_INCREMENTAL_CONSISTENCE() )
	{
	  DependencySet* pNewDS = pDS->unionNew(m_pMergeDepends, m_pABox->doExplanation());
	  pNewDS->unionDS(pNode->m_pMergeDepends, m_pABox->doExplanation());
	  m_pABox->setClash(new Clash(this, pNewDS, pNode->m_pName));
	}
      else
	m_pABox->setClash(new Clash(this, pDS, pNode->m_pName));
      return;
    }

  DependencySet* pDSCopy = new DependencySet(pDS);
  pDSCopy->m_iBranch = m_pABox->getBranch();
  m_mDifferents[pNode] = pDSCopy;
  pNode->setDifferent(this, pDSCopy);
}

bool Node::isChanged(int iType)
{
  return (m_iStatus & (1 << iType)) != 0;
}

void Node::setChanged(bool bChanged)
{
  m_iStatus = bChanged ? NODE_CHANGED : NODE_UNCHANGED;
}

void Node::setChanged(int iType, bool bChanged)
{
  if( bChanged )
    {
      m_iStatus = (m_iStatus | (1 << iType));
      
      //Check if we need to updated the completion queue 
      //Currently we only updated the changed lists for checkDatatypeCount()
      QueueElement* pNewElement = new QueueElement(m_pName, NULL);

      //update the datatype queue
      if( (iType == Node::ALL || iType == Node::MIN) && PARAMS_USE_COMPLETION_QUEUE() )
	m_pABox->m_pCompletionQueue->add(pNewElement, CompletionQueue::DATATYPELIST);

      //add node to effected list in queue
      if( m_pABox->m_iCurrentBranchIndex >= 0 && PARAMS_USE_COMPLETION_QUEUE() )
	m_pABox->addEffected(m_pABox->m_iCurrentBranchIndex, m_pName);
    }
  else
    {
      m_iStatus = (m_iStatus & ~(1 << iType));
      
      //add node to effected list in queue
      if( m_pABox->m_iCurrentBranchIndex >= 0 && PARAMS_USE_COMPLETION_QUEUE() )
	m_pABox->addEffected(m_pABox->m_iCurrentBranchIndex, m_pName);
    }
}

int isEqual(const Node* pNode1, const Node* pNode2)
{
  return isEqual(pNode1->m_pName, pNode2->m_pName);
}

Node* Node::copyTo(ABox* pABox)
{
  return NULL;
}

Node* Node::copy()
{
  return copyTo(NULL);
}

void Node::updateNodeReferences()
{
  m_pMergeTo = m_pABox->getNode(m_pMergeTo->m_pName);
  
  Node2DependencySetMap diffs;
  for(Node2DependencySetMap::iterator i = m_mDifferents.begin(); i != m_mDifferents.end(); i++ )
    {
      Node* pNode = (Node*)i->first;
      diffs[m_pABox->getNode(pNode->m_pName)] = (DependencySet*)i->second;
    }

  m_mDifferents = diffs;

  if( m_setMerged.size() > 0 )
    {
      NodeSet setSames;
      for(NodeSet::iterator i = m_setMerged.begin(); i != m_setMerged.end(); i++ )
	{
	  Node* pNode = (Node*)*i;
	  setSames.insert(m_pABox->getNode(pNode->m_pName));
	}
      m_setMerged = setSames;
    }

  EdgeVector aNewEdges;
  for(EdgeVector::iterator i = m_listInEdges.m_listEdge.begin(); i != m_listInEdges.m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      Individual* pFrom = m_pABox->getIndividual(pEdge->m_pFrom->m_pName);
      Edge* pNewEdge = new Edge(pEdge->m_pRole, pFrom, this, pEdge->m_pDepends);
      aNewEdges.push_back(pNewEdge);
      if( !isPruned() )
	pFrom->m_listOutEdges.addEdge(pNewEdge);
    }
  
  m_listInEdges.m_listEdge.clear();
  for(EdgeVector::iterator i = aNewEdges.begin(); i != aNewEdges.end(); i++ )
    m_listInEdges.addEdge((Edge*)*i);
  
}

void Node::unprune(int iBranch)
{
  m_pPruned = NULL;
  
  for(EdgeVector::iterator i = m_listInEdges.m_listEdge.begin(); i != m_listInEdges.m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      DependencySet* pDS = pEdge->m_pDepends;
      if( pDS->m_iBranch <= iBranch )
	{
	  Individual* pPred = pEdge->m_pFrom;
	  Role* pRole = pEdge->m_pRole;

	  // if both pred and *this* were merged to other nodes (in that order)
	  // there is a chance we might duplicate the edge so first check for
	  // the existence of the edge
	  if( !pPred->hasRSuccessor(pRole, this) )
	    pPred->addOutEdge(pEdge);
	}
    }
}

void Node::undoSetSame()
{
  m_pMergeTo->removeMerged(this);
  m_pMergeDepends = &DEPENDENCYSET_INDEPENDENT;
  m_pMergeTo = this;
}

void Node::removeMerged(Node* pNode)
{
  m_setMerged.erase(pNode);
}

bool Node::hasObviousType(ExprNode* pC)
{
  DependencySet* pDS = getDepends(pC);
  if( pDS && pDS->isIndependent() )
    return TRUE;

  if( isIndividual() && pC->m_iExpression == EXPR_SOME )
    {
      Individual* pInd = (Individual*)this;
      ExprNode* pR = (ExprNode*)pC->m_pArgs[0];
      ExprNode* pD = (ExprNode*)pC->m_pArgs[1];

      Role* pRole = g_pKB->getRole(pR);
      EdgeList edgeList;
      pInd->getRNeighborEdges(pRole, &edgeList);
      for(EdgeVector::iterator i = edgeList.m_listEdge.begin(); i != edgeList.m_listEdge.end(); i++ )
	{
	  Edge* pEdge = (Edge*)*i;
	  if( !pEdge->m_pDepends->isIndependent() )
	    continue;

	  Node* pY = pEdge->getNeighbor(pInd);
	  if( pY->hasObviousType(pD) )
	    return TRUE;
	}
    }
  return FALSE;
}

bool Node::hasObviousType(ExprNodeSet* pSet)
{
  for(ExprNodeSet::iterator i = pSet->begin(); i != pSet->end(); i++ )
    {
      ExprNode* pC = (ExprNode*)*i;
      DependencySet* pDS = getDepends(pC);
      if( pDS && pDS->isIndependent() )
	return TRUE;
    }
  return FALSE;
}

void Node::setSame(Node* pNode, DependencySet* pDS)
{
  if( isSame(pNode) )
    return;

  if( isDifferent(pNode) )
    {
      // CHW - added for incremental reasoning support - 
      // this is needed as we will need to backjump if possible
      if( PARAMS_USE_INCREMENTAL_CONSISTENCE() )
	m_pABox->setClash(Clash::nominal(this, pDS->unionNew(m_pMergeDepends, m_pABox->m_bDoExplanation)->unionDS(pNode->m_pMergeDepends, m_pABox->m_bDoExplanation), pNode->m_pName));
      else
	m_pABox->setClash(Clash::nominal(this, pDS, pNode->m_pName));
      return;
    }

  m_pMergeTo = pNode;
  m_pMergeDepends = new DependencySet(pDS);
  m_pMergeDepends->m_iBranch = m_pABox->m_iCurrentBranchIndex;
  pNode->addMerged(this);
}

void Node::addMerged(Node* pNode)
{
  m_setMerged.insert(pNode);
}

void Node::inheritDifferents(Node* pY, DependencySet* pDS)
{
  for(Node2DependencySetMap::iterator i = pY->m_mDifferents.begin(); i != pY->m_mDifferents.end(); i++ )
    {
      Node* pYDiff = (Node*)i->first;
      DependencySet* pYDS = (DependencySet*)i->second;
      DependencySet* pFinalDS = pDS->unionNew(pYDS, m_pABox->m_bDoExplanation);
      setDifferent(pYDiff, pFinalDS);
    }
}

DependencySet* Node::getDifferenceDependency(Node* pNode)
{
  Node2DependencySetMap::iterator iFind = m_mDifferents.find(pNode);
  if( iFind != m_mDifferents.end() )
    return (DependencySet*)iFind->second;
  return NULL;
}

bool Node::restore(int iBranch)
{
  if( m_pPruned )
    {
      if( m_pPruned->m_iBranch > iBranch )
	{
	  if( m_pMergeDepends->m_iBranch > iBranch )
	    undoSetSame();
	  unprune(iBranch);
	}
      else
	return FALSE;
    }

  ExprNodes aConjunctions;
  m_iStatus = NODE_CHANGED;

  for(ExprNode2DependencySetMap::iterator i = m_mDepends.begin(); i != m_mDepends.end(); )
    {
      ExprNode* pC = (ExprNode*)i->first;
      DependencySet* pD = (DependencySet*)i->second;
      
      bool bRemoveType = PARAMS_USE_SMART_RESTORE() ? (pD->getMax()>=iBranch):(pD->m_iBranch>iBranch);

      if( bRemoveType )
	{
	  m_mDepends.erase(i++);
	  removeType(pC);
	}
      else
	{
	  i++;
	  if( PARAMS_USE_SMART_RESTORE() && pC->m_iExpression == EXPR_AND )
	    {
	      aConjunctions.push_back(pC);
	    }
	}
    }

  // with smart restore there is a possibility that we remove a conjunct 
  // but not the conjunction. this is the case if conjunct was added before 
  // the conjunction but depended on an earlier branch. so we need to make
  // sure all conjunctions are actually applied
  if( PARAMS_USE_SMART_RESTORE() )
    {
      for(ExprNodes::iterator i = aConjunctions.begin(); i != aConjunctions.end(); i++ )
	{
	  ExprNode* pC = (ExprNode*)*i;
	  DependencySet* pD = getDepends(pC);
	  
	  for(int j = 0; j < ((ExprNodeList*)pC->m_pArgList)->m_iUsedSize; j++ )
	    addType(((ExprNodeList*)pC->m_pArgList)->m_pExprNodes[j], pD);
	}
    }

  ExprNodeMap::iterator iFind = m_pABox->m_mTypeAssertions.find(m_pName);
  if( iFind != m_pABox->m_mTypeAssertions.end() )
    {
      ExprNode* pC = (ExprNode*)iFind->second;
      addType(pC, &DEPENDENCYSET_INDEPENDENT);
    }

  for(Node2DependencySetMap::iterator i = m_mDifferents.begin(); i != m_mDifferents.end(); i++ )
    {
      Node* pNode = (Node*)i->first;
      DependencySet* pDS = (DependencySet*)i->second;

      if( pDS->m_iBranch > iBranch )
	m_mDifferents.erase(i);
    }

  for(EdgeVector::iterator i = m_listInEdges.m_listEdge.begin(); i != m_listInEdges.m_listEdge.end(); )
    {
      Edge* pEdge = (Edge*)*i;
      DependencySet* pDS = pEdge->m_pDepends;

      if( pDS->m_iBranch > iBranch )
	i = m_listInEdges.m_listEdge.erase(i);
      else
	i++;
    }

  return TRUE;
}

Individual* Node::getParent()
{
  if( isBlockable() )
    {
      if( m_listInEdges.size() == 0 )
	return NULL;
      else
	{
	  // reflexive properties!
	  for(EdgeVector::iterator i = m_listInEdges.m_listEdge.begin(); i != m_listInEdges.m_listEdge.end(); i++ )
	    {
	      Edge* pEdge = (Edge*)*i;
	      if( isEqual(pEdge->m_pFrom, this) != 0 )
		return pEdge->m_pFrom;
	    }
	}
    }
  return NULL;
}

void Node::getPredecessors(NodeSet* pAncestors)
{
  m_listInEdges.getPredecessors(pAncestors);
}

void Node::getPath(ExprNodes* pPath)
{
  if( isNamedIndividual() )
    {
      pPath->push_back(m_pName);
    }
  else
    {
      NodeSet setCycleCache;
      Node* pNode = this;
      while( pNode->m_listInEdges.size() > 0 )
	{
	  Edge* pInEdge = pNode->m_listInEdges.m_listEdge.front();
	  pNode = pInEdge->m_pFrom;

	  if( setCycleCache.find(pNode) != setCycleCache.end() )
	    break;

	  setCycleCache.insert(pNode);
	  pPath->push_front( pInEdge->m_pRole->m_pName );
	  if( pNode->isNamedIndividual() )
	    {
	      pPath->push_front( pNode->m_pName );
	      break;
	    }
	}
    }
}
