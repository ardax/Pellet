#include "Individual.h"
#include "Edge.h"
#include "DependencySet.h"
#include "KnowledgeBase.h"
#include "ABox.h"
#include "Role.h"
#include "State.h"
#include "Params.h"
#include "ReazienerUtils.h"
#include "CompletionQueue.h"
#include "QueueElement.h"
#include "Clash.h"

extern KnowledgeBase* g_pKB;
extern ExprNode* EXPRNODE_TOP;
extern ExprNode* EXPRNODE_BOTTOM;
extern DependencySet DEPENDENCYSET_EMPTY;
extern DependencySet DEPENDENCYSET_INDEPENDENT;

extern int g_iCommentIndent;

Individual::Individual(ExprNode* pName) : Node(pName, NULL)
{
  m_bLiteralNode = FALSE;
  m_iNominalLevel = NODE_BLOCKABLE;
  for(int i = 0; i < TYPECOUNT; i++ )
    m_iApplyNext[i] = 0;
}

Individual::Individual(ExprNode* pName, ABox* pABox, int iNominalLevel) : Node(pName, pABox)
{
  m_bLiteralNode = FALSE;
  m_iNominalLevel = iNominalLevel;
  for(int i = 0; i < TYPECOUNT; i++ )
    m_iApplyNext[i] = 0;
}

Individual::Individual(Individual* pIndividual, ABox* pABox) : Node(pIndividual, pABox)
{
  m_iNominalLevel = pIndividual->m_iNominalLevel;
  
  for(int i = 0; i < TYPECOUNT; i++ )
    {
      for(ExprNodes::iterator j = pIndividual->m_aTypes[i].begin(); j != pIndividual->m_aTypes[i].end(); j++ )
	m_aTypes[i].push_back((ExprNode*)*j);
      m_iApplyNext[i] = pIndividual->m_iApplyNext[i];
    }

  if( m_pABox == NULL )
    {
      m_pMergeTo = NULL;
      for(EdgeVector::iterator i = pIndividual->m_listOutEdges.m_listEdge.begin(); i != pIndividual->m_listOutEdges.m_listEdge.end(); i++ )
	{
	  Edge* pEdge = (Edge*)*i;
	  Individual* pFrom = this;
	  Individual* pTo = new Individual(pEdge->m_pTo->m_pName);
	  pTo->m_iNominalLevel = ((Individual*)pEdge->m_pTo)->m_iNominalLevel;
	  Edge* pNewEdge = new Edge(pEdge->m_pRole, pFrom, pTo, pEdge->m_pDepends);
	  m_listOutEdges.addEdge(pNewEdge);
	}
    }
  else
    {
      if( isPruned() )
	m_listOutEdges = pIndividual->m_listOutEdges;
    }
}

Edge* Individual::addEdge(Role* pRole, Node* pX, DependencySet* pDS)
{
  if( m_pABox->m_iCurrentBranchIndex > 0 && PARAMS_USE_COMPLETION_QUEUE() )
    {
      m_pABox->addEffected(m_pABox->m_iCurrentBranchIndex, m_pName);
      m_pABox->addEffected(m_pABox->m_iCurrentBranchIndex, pX->m_pName);
    }

  if( hasRSuccessor(pRole, pX) )
    {
      // TODO we might miss some of explanation axioms
      return NULL;
    }

  if( isPruned() )
    assertFALSE("Adding edge to a pruned node ");
  else if( isMerged() )
    return NULL;

  m_pABox->m_bChanged = TRUE;
  setChanged(Node::ALL, TRUE);
  setChanged(Node::MAX, TRUE);

  pDS = new DependencySet(pDS);
  pDS->m_iBranch = m_pABox->m_iCurrentBranchIndex;
  
  Edge* pEdge = new Edge(pRole, this, pX, pDS);
  
  //printf("New Edge(%x) = ", pEdge);
  //printExprNode(m_pName, TRUE);
  //printf(" TO=");
  //printExprNode(pX->m_pName);
  
  m_listOutEdges.addEdge(pEdge);
  pX->addInEdge(pEdge);

  return pEdge;
}

bool Individual::removeEdge(Edge* pEdge)
{
  if( !m_listOutEdges.removeEdge(pEdge) )
    assertFALSE("Trying to remove a non-existing edge ");
  return TRUE;
}

void Individual::addInEdge(Edge* pEdge)
{
  setChanged(Node::ALL, TRUE);
  setChanged(Node::MAX, TRUE);
  
  m_listInEdges.addEdge(pEdge); 
}

void Individual::addOutEdge(Edge* pEdge)
{
  setChanged(Node::ALL, TRUE);
  setChanged(Node::MAX, TRUE);
  
  m_listOutEdges.addEdge(pEdge); 
}

Node* Individual::copyTo(ABox* pABox)
{
  return (new Individual(this, pABox));
}

bool Individual::hasRSuccessor(Role* pR)
{
  return (m_listOutEdges.hasEdge(pR));
}

bool Individual::hasRSuccessor(Role* pR, Node* pX)
{
  return (m_listOutEdges.hasEdge(this, pR, pX));
}

bool Individual::hasRNeighbor(Role* pRole)
{
  if( m_listOutEdges.hasEdge(pRole) )
    return TRUE;
  Role* pInvRole = pRole->m_pInverse;
  if( pInvRole && m_listInEdges.hasEdge(pInvRole) )
    return TRUE;
  return FALSE;
}

/**
 * Returns true if this individual has at least n distinct r-neighbors. If
 * only nominal neighbors are wanted then blockable ones will simply be 
 * ignored (note that this should only happen if r is an object property)
 */
bool Individual::hasDistinctRNeighborsForMin(Role* pRole, int iN, ExprNode* pC, bool bOnlyNominals)
{
  EdgeList listEdges;
  getRNeighborEdges(pRole, &listEdges);

  if( iN == 1 && !bOnlyNominals && isEqual(pC, EXPRNODE_TOP) == 0 )
    return (listEdges.size()>0);

  if( listEdges.size() < iN )
    return FALSE;

  list<Nodes*> allDisjointSets;
  for(EdgeVector::iterator i = listEdges.m_listEdge.begin(); i != listEdges.m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      Node* pY = pEdge->getNeighbor(this);
      
      if( !pY->hasType(pC) )
	continue;

      if( bOnlyNominals )
	{
	  if( pY->isBlockable() )
	    continue;
	  else if( iN == 1 )
	    return TRUE;
	}

      bool bAdded = FALSE;
      for(list<Nodes*>::iterator j = allDisjointSets.begin(); j != allDisjointSets.end(); j++ )
	{
	  bool bAddToThis = TRUE;
	  Nodes* pDisjointSet = (Nodes*)*j;
	  for(Nodes::iterator d = pDisjointSet->begin(); d != pDisjointSet->end(); d++ )
	    {
	      Node* pZ = (Node*)*d;
	      if( !pY->isDifferent(pZ) )
		{
		  bAddToThis = FALSE;
		  break;
		}
	    }

	  if( bAddToThis )
	    {
	      bAdded = TRUE;
	      pDisjointSet->push_back(pY);
	      if( pDisjointSet->size() >= iN )
		return TRUE;
	    }
	}

      if( !bAdded )
	{
	  Nodes* pNodes = new Nodes;
	  pNodes->push_back(pY);
	  allDisjointSets.push_back(pNodes);
	}

      if( iN == 1 && allDisjointSets.size() >= 1 )
	return TRUE;
    }
  return FALSE;
}

/**
 * Checks if this individual has at least n distinct r-neighbors that has 
 * a specific type. 
 */
DependencySet* Individual::hasDistinctRNeighborsForMax(Role* pRole, int iN, ExprNode* pC)
{
  bool bHasNeighbors = FALSE;

  // get all the edges to x with a role (or subrole of) r
  EdgeList edges;
  getRNeighborEdges(pRole, &edges);

  if( edges.size() >= iN )
    {
      vector<Nodes*> allDisjointSets;

    outerloop:
      for(EdgeVector::iterator i = edges.m_listEdge.begin(); i != edges.m_listEdge.end(); i++ )
	{
	  Edge* pEdge = (Edge*)*i;
	  Node* pY = pEdge->getNeighbor(this);
	  
	  if( !pY->hasType(pC) )
	    continue;

	  bool bAdded = FALSE;
	  int iSize = allDisjointSets.size();
	  for(int j = 0; j < iSize; j++ )
	    {
	      Nodes* pDisjointSet = (Nodes*)allDisjointSets.at(j);
	      bool bBreaked = FALSE;
	      for(Nodes::iterator k = pDisjointSet->begin(); k != pDisjointSet->end(); k++ )
		{
		  Node* pZ = (Node*)*k;
		  if( !pY->isDifferent(pZ) )
		    {
		      bBreaked = TRUE;
		      break;
		    }
		}

	      if( !bBreaked )
		{
		  bAdded = TRUE;
		  pDisjointSet->push_back(pY);
		  if( pDisjointSet->size() >= iN )
		    {
		      bHasNeighbors = TRUE;
		      goto outerloop;
		    }
		}
	    }

	  if( !bAdded )
	    {
	      Nodes* pNodes = new Nodes;
	      pNodes->push_back(pY);
	      allDisjointSets.push_back(pNodes);
	      if( iN == 1 )
		{
		  bHasNeighbors = TRUE;
		  goto outerloop;
		}
	    }
	}
    }

  if( !bHasNeighbors )
    return NULL;

  // we are being overly cautious here by getting the union of all
  // the edges to all r-neighbors 
  DependencySet* pDS = &DEPENDENCYSET_EMPTY;
  for(EdgeVector::iterator i = edges.m_listEdge.begin(); i != edges.m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      pDS = pDS->unionNew(pEdge->m_pDepends, m_pABox->m_bDoExplanation);
      Node* pNode = pEdge->getNeighbor(this);
      DependencySet* pTypeDS = pNode->getDepends(pC);
      if( pTypeDS )
	pDS = pDS->unionNew(pTypeDS, m_pABox->m_bDoExplanation);
    }

  return pDS;
}

bool Individual::isNominal()
{
  return (m_iNominalLevel!=NODE_BLOCKABLE);
}

bool Individual::canApply(int iType)
{
  return (m_iApplyNext[iType] < m_aTypes[iType].size());
}

void Individual::getRNeighborEdges(Role* pRole, EdgeList* pNeighbors)
{
  m_listOutEdges.getEdges(pRole, pNeighbors);
  Role* pInverseRole = pRole->m_pInverse;
  if( pInverseRole )
    m_listInEdges.getEdges(pInverseRole, pNeighbors);
}

void Individual::getRPredecessorEdges(Role* pRole, EdgeList* pEdgeList)
{
  m_listInEdges.getEdges(pRole, pEdgeList);
}

void Individual::getRNeighborNames(Role* pRole, ExprNodeSet* pNames)
{
  EdgeList aNeighborEdgeList;
  getRNeighborEdges(pRole, &aNeighborEdgeList);
  for(EdgeVector::iterator i = aNeighborEdgeList.m_listEdge.begin(); i != aNeighborEdgeList.m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      Node* pNode = pEdge->getNeighbor(this);
      pNames->insert(pNode->m_pName);
    }
}

void Individual::getEdgesTo(Node* pNode, Role* pRole, EdgeList* pEdgesTo)
{
  m_listOutEdges.getEdgesTo(pNode, pEdgesTo);
}

/**
 * Collects atomic concepts such that either that concept or its negation 
 * exist in the types list without depending on any non-deterministic branch. 
 * First list is filled with types and second list is filled with non-types,
 * i.e. this individual can never be an instance of any element in the 
 * second list. 
 */
void Individual::getObviousTypes(ExprNodes* pTypes, ExprNodes* pNonTypes)
{
  for(ExprNodes::iterator i = m_aTypes[Node::ATOM].begin(); i != m_aTypes[Node::ATOM].end(); i++ )
    {
      ExprNode* pC = (ExprNode*)*i;
      ExprNode2DependencySetMap::iterator iFind = m_mDepends.find(pC);
      if( iFind != m_mDepends.end() )
	{
	  DependencySet* pDS = (DependencySet*)iFind->second;
	  if( pDS->isIndependent() )
	    {
	      if( isPrimitive(pC) )
		pTypes->push_back(pC);
	      else if( isNegatedPrimitive(pC) )
		pNonTypes->push_back((ExprNode*)pC->m_pArgs[0]);
	    }
	}
    }
}

int Individual::getMaxCard(Role* pRole)
{
  int iMin = INT_MAX;
  for(ExprNodes::iterator i = m_aTypes[Node::MAX].begin(); i != m_aTypes[Node::MAX].end(); i++ )
    {
      ExprNode* pMC = (ExprNode*)*i;
      
      // max(r, n) is in normalized form not(min(p, n + 1))
      ExprNode* pMaxCard = (ExprNode*)pMC->m_pArgs[0];
      Role* pMaxRole = g_pKB->getRole( (ExprNode*)pMaxCard->m_pArgs[0] );
      int iMax = ((ExprNode*)pMaxCard->m_pArgs[1])->m_iTerm-1;
      
      if( pRole->isSubRoleOf(pMaxRole) && iMax < iMin )
	iMin = iMax;
    }

  if( pRole->isFunctional() && iMin > 1 )
    iMin = 1;
  return iMin;
}

int Individual::getMinCard(Role* pRole)
{
  int iMax = 0;
  for(ExprNodes::iterator i = m_aTypes[Node::MIN].begin(); i != m_aTypes[Node::MIN].end(); i++ )
    {
      ExprNode* pMinCard = (ExprNode*)*i;
      Role* pMinRole = g_pKB->getRole((ExprNode*)pMinCard->m_pArgs[0]);
      int iMin = ((ExprNode*)pMinCard->m_pArgs[1])->m_iTerm;

      if( pMinRole->isSubRoleOf(pRole) && iMax < iMin )
	iMax = iMin;
    }
  return iMax;
}

/**
 * Prune the given node by removing all links going to nominal nodes and recurse
 * through all successors. No need to remove incoming edges because either the node
 * is the first one being pruned so the merge function already handled it or
 * this is a successor node and its successor is also being pruned
 */
void Individual::prune(DependencySet* pDS)
{
  START_DECOMMENT2("Individual::prune");
  printExprNodeWComment("This.node=", m_pName);

  //add to effected list of queue
  if( m_pABox->m_iCurrentBranchIndex >= 0 && PARAMS_USE_COMPLETION_QUEUE() )
    m_pABox->addEffected(m_pABox->m_iCurrentBranchIndex, m_pName);
  
  m_pPruned = pDS;

  for(EdgeVector::iterator i = m_listOutEdges.m_listEdge.begin(); i != m_listOutEdges.m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      Node* pSucc = pEdge->m_pTo;
      printExprNodeWComment("Succ=", pSucc->m_pName);
      
      if( isEqual(pSucc, this) == 0 )
	continue;
      else if( pSucc->isNominal() )
	{
	  pSucc->removeInEdge(pEdge);
	  DECOMMENT("Removing incoming edge of successor");
	}
      else
	pSucc->prune(pDS);
    }
  END_DECOMMENT("Individual::Individual");
}

bool Individual::hasSuccessor(Node* pX)
{
  return m_listOutEdges.hasEdgeTo(pX);
}

bool Individual::isBlockable()
{
  return (m_iNominalLevel==NODE_BLOCKABLE);
}

DependencySet* Individual::getDifferenceDependency(Node* pNode)
{
  if( PARAMS_USE_UNIQUE_NAME_ASSUMPTION() )
    {
      if( isNamedIndividual() && pNode->isNamedIndividual() )
	return &(DEPENDENCYSET_INDEPENDENT);
    }
  return Node::getDifferenceDependency(pNode);
}

DependencySet* Individual::hasMax1(Role* pRole)
{
  for(ExprNodes::iterator i = m_aTypes[Node::MAX].begin(); i != m_aTypes[Node::MAX].end(); i++ )
    {
      // max(r, n, c) is in normalized form not(min(p, n + 1))
      ExprNode* pMaxCard = (ExprNode*)((ExprNode*)*i)->m_pArgs[0];
      Role* pMaxRole = g_pKB->getRole((ExprNode*)pMaxCard->m_pArgs[0]);
      int iMax = ((ExprNode*)pMaxCard->m_pArgs[1])->m_iTerm-1;
      ExprNode* pMaxQ = (ExprNode*)pMaxCard->m_pArgs[2];
      
      // FIXME returned dependency set might be wrong 
      // if there are two types max(r,1) and max(p,1) where r subproperty of p
      // then the dependency set what we return might be wrong
      if( iMax == 1 && pRole->isSubRoleOf(pMaxRole) && isTop(pMaxQ) )
	return getDepends(pMaxCard);
    }
  return NULL;
}

void Individual::getAncestors(Nodes* pAncestors)
{
  // empty list
  if( isNominal() )
    return;

  if( m_aAncestors.size() > 0 )
    {
      for(Nodes::iterator i = m_aAncestors.begin(); i != m_aAncestors.end(); i++ )
	pAncestors->push_back((Node*)*i);
      return;
    }
  
  Node* pNode = getParent();
  while(pNode)
    {
      m_aAncestors.push_back(pNode);
      pAncestors->push_back(pNode);
      pNode = pNode->getParent();
    }
}

void Individual::getRSuccessors(Role* pRole, ExprNode* pC, NodeSet* pRSuccessors)
{
  for(EdgeVector::iterator i = m_listOutEdges.m_listEdge.begin(); i != m_listOutEdges.m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      Node* pOther = pEdge->getNeighbor(this);
      if( pOther->hasType(pC) )
	pRSuccessors->insert(pOther);
    }
}

void Individual::getSortedSuccessors(NodeSet* pSortedSuccessors)
{
  m_listOutEdges.getSuccessors(pSortedSuccessors);
  pSortedSuccessors->erase(this);
}

void Individual::addType(ExprNode* pC, DependencySet* pDS)
{
  START_DECOMMENT2("Individual::addType");
  printExprNodeWComment("Node=", m_pName);
  printExprNodeWComment("Type=", pC);
  
  if( isPruned() )
    {
      return;
      assertFALSE("Adding type to a pruned node ");
    }
  else if( isMerged() )
    {
      END_DECOMMENT("Individual::addType(1)");
      return;
    }

  if( m_mDepends.find(pC) != m_mDepends.end() )
    {
      END_DECOMMENT("Individual::addType(2)");
      return;
    }

  pDS = new DependencySet(pDS);
  pDS->m_iBranch = m_pABox->m_iCurrentBranchIndex;

  // if we are checking entailment using a pseduo model, abox.branch 
  // is set to -1. however, since applyAllValues is done automatically
  // and the edge used in applyAllValues may depend on a branch we want
  // this type to be deleted when that edge goes away, i.e. we backtrack
  // to a position before the max dependency of this type
  int iMax = pDS->getMax();
  if( pDS->m_iBranch == -1 && iMax != 0 )
    pDS->m_iBranch = iMax+1;

  m_mDepends[pC] = pDS;
  m_pABox->m_bChanged = TRUE;

  QueueElement* pElement = new QueueElement(m_pName, pC);
  ExprNode* pNotC = negate2(pC);
  DependencySet* pClashDepends = NULL;
  ExprNode2DependencySetMap::iterator iFind = m_mDepends.find(pNotC);
  if( iFind != m_mDepends.end() )
    pClashDepends = (DependencySet*)iFind->second;

  if( pClashDepends )
    {
      printExprNodeWComment("pNotC exists in types=", pNotC);
      if( m_pABox->m_bDoExplanation )
	{
	  ExprNode* pPositive = (pC->m_iExpression==EXPR_NOT)?pC:pNotC;
	  m_pABox->setClash(Clash::atomic(this, pClashDepends->unionNew(pDS, m_pABox->m_bDoExplanation), pPositive));
	}
      else
	m_pABox->setClash(Clash::atomic(this, pClashDepends->unionNew(pDS, m_pABox->m_bDoExplanation)));
    }

  if( isPrimitive(pC) )
    {
      setChanged(Node::ATOM, TRUE);
      m_aTypes[Node::ATOM].push_back(pC);

      if( PARAMS_USE_COMPLETION_QUEUE() )
	m_pABox->m_pCompletionQueue->add(pElement, CompletionQueue::ATOMLIST);
    }
  else
    {
      if( pC->m_iExpression == EXPR_AND )
	{
	  for(int i = 0; i < ((ExprNodeList*)pC->m_pArgList)->m_iUsedSize; i++ )
	    {
	      ExprNode* pConj = ((ExprNodeList*)pC->m_pArgList)->m_pExprNodes[i];
	      addType(pConj, pDS);
	    }
	}
      else if( pC->m_iExpression == EXPR_ALL )
	{
	  setChanged(Node::ALL, TRUE);
	  m_aTypes[Node::ALL].push_back(pC);

	  if( PARAMS_USE_COMPLETION_QUEUE() )
	    m_pABox->m_pCompletionQueue->add(pElement, CompletionQueue::ALLLIST);
	}
      else if( pC->m_iExpression == EXPR_MIN )
	{
	  if( checkMinClash(pC, pDS) )
	    {
	      END_DECOMMENT("Individual::addType(3)");
	      return;
	    }
	  else if( !isRedundantMin(pC) )
	    {
	      m_aTypes[Node::MIN].push_back(pC);
	      setChanged(Node::MIN, TRUE);

	      if( PARAMS_USE_COMPLETION_QUEUE() )
		m_pABox->m_pCompletionQueue->add(pElement, CompletionQueue::MINLIST);
	    }
	}
      else if( pC->m_iExpression == EXPR_NOT )
	{
	  ExprNode* pX = (ExprNode*)pC->m_pArgs[0];
	  if( pX->m_iExpression == EXPR_AND )
	    {
	      m_aTypes[Node::OR].push_back(pC);
	      setChanged(Node::OR, TRUE);
	      
	      if( PARAMS_USE_COMPLETION_QUEUE() )
		m_pABox->m_pCompletionQueue->add(pElement, CompletionQueue::ORLIST);
	    }
	  else if( pX->m_iExpression == EXPR_ALL )
	    {
	      m_aTypes[Node::SOME].push_back(pC);
	      setChanged(Node::SOME, TRUE);
	      
	      if( PARAMS_USE_COMPLETION_QUEUE() )
		m_pABox->m_pCompletionQueue->add(pElement, CompletionQueue::SOMELIST);
	    }
	  else if( pX->m_iExpression == EXPR_MIN )
	    {
	      if( checkMaxClash(pC, pDS) )
		{
		  END_DECOMMENT("Individual::addType(3)");
		  return;
		}
	      else if( !isRedundantMax(pX) )
		{
		  m_aTypes[Node::MAX].push_back(pC);
		  setChanged(Node::MAX, TRUE);
		  
		  if( PARAMS_USE_COMPLETION_QUEUE() )
		    {
		      m_pABox->m_pCompletionQueue->add(pElement, CompletionQueue::MAXLIST);
		      m_pABox->m_pCompletionQueue->add(pElement, CompletionQueue::CHOOSELIST);
		      m_pABox->m_pCompletionQueue->add(pElement, CompletionQueue::GUESSLIST);
		    }
		}
	    }
	  else if( ::isNominal(pX) )
	    {
	      m_aTypes[Node::ATOM].push_back(pC);
	      setChanged(Node::ATOM, TRUE);
	      
	      if( PARAMS_USE_COMPLETION_QUEUE() )
		m_pABox->m_pCompletionQueue->add(pElement, CompletionQueue::ATOMLIST);
	    }
	  else if( pX->m_iExpression == EXPR_SELF )
	    {
	      ExprNode* pP = (ExprNode*)pC->m_pArgs[0];
	      Role* pRole = g_pKB->getRole(pP);
	      if( pRole )
		{
		  EdgeList selfEdges, edgesToSelfEdges;
		  m_listOutEdges.getEdges(pRole, &selfEdges);
		  selfEdges.getEdgesTo(this, &edgesToSelfEdges);

		  if( edgesToSelfEdges.size() > 0 )
		    m_pABox->setClash(Clash::unexplained(this, edgesToSelfEdges.getDepends(m_pABox->m_bDoExplanation)));
		}
	    }
	  else if( pX->m_iArity == 0 )
	    {
	      m_aTypes[Node::ATOM].push_back(pC);
	      setChanged(Node::ATOM, TRUE);
	      
	      if( PARAMS_USE_COMPLETION_QUEUE() )
		m_pABox->m_pCompletionQueue->add(pElement, CompletionQueue::ATOMLIST);
	    }
	  else
	    {
	      printExprNodeWComment("Invalid=", pX);
	      assertFALSE("Invalid type ");
	    }
	}
      else if( pC->m_iExpression == EXPR_VALUE )
	{
	  setChanged(Node::NOM, TRUE);
	  m_aTypes[Node::NOM].push_back(pC);

	  if( PARAMS_USE_COMPLETION_QUEUE() )
	    m_pABox->m_pCompletionQueue->add(pElement, CompletionQueue::NOMLIST);
	}
      else if( pC->m_iExpression == EXPR_SELF )
	{
	  m_aTypes[Node::ATOM].push_back(pC);
	  setChanged(Node::ATOM, TRUE);
	}
      else
	{
	  m_mDepends[EXPRNODE_BOTTOM] = pDS;
	}
    }
  END_DECOMMENT("Individual::addType(4)");
}

bool Individual::checkMinClash(ExprNode* pMinCard, DependencySet* pMinDepends)
{
  Role* pMinRole = g_pKB->getRole((ExprNode*)pMinCard->m_pArgs[0]);
  if( pMinRole == NULL )
    return FALSE;
  int iMin = ((ExprNode*)pMinCard->m_pArgs[1])->m_iTerm;
  ExprNode* pMinC = (ExprNode*)pMinCard->m_pArgs[2];

  if( pMinRole->isFunctional() && iMin > 1 )
    {
      m_pABox->setClash(new Clash(this, Clash::MIN_MAX, pMinDepends));
      return TRUE;
    }

  for(ExprNodes::iterator i = m_aTypes[Node::MAX].begin(); i != m_aTypes[Node::MAX].end(); i++ )
    {
      ExprNode* pMC = (ExprNode*)*i;
      
      // max(r, n) is in normalized form not(min(p, n + 1))
      ExprNode* pMaxCard = (ExprNode*)pMC->m_pArgs[0];
      Role* pMaxR = g_pKB->getRole((ExprNode*)pMaxCard->m_pArgs[0]);
      if( pMaxR == NULL )
	return FALSE;
      int iMax = ((ExprNode*)pMaxCard->m_pArgs[1])->m_iTerm-1;
      ExprNode* pMaxC = (ExprNode*)pMaxCard->m_pArgs[2];
      
      if( iMax < iMin && ::isEqual(pMinC, pMaxC) == 0 && pMinRole->isSubRoleOf(pMaxR) )
	{
	  DependencySet* pMaxDepends = getDepends(pMC);
	  DependencySet* pSubDepends = pMaxR->getExplainSub(pMinRole->m_pName);
	  DependencySet* pDS = pMinDepends->unionNew(pMaxDepends, m_pABox->m_bDoExplanation)->unionDS(pSubDepends, m_pABox->m_bDoExplanation);

	  m_pABox->setClash(new Clash(this, Clash::MIN_MAX, pDS));
	  return TRUE;
	}
    }

  return FALSE;
}

bool Individual::checkMaxClash(ExprNode* pNormalizedMax, DependencySet* pMaxDepends)
{
  ExprNode* pMaxCard = (ExprNode*)pNormalizedMax->m_pArgs[0];
  Role* pMaxRole = g_pKB->getRole((ExprNode*)pMaxCard->m_pArgs[0]);
  if( pMaxRole == NULL )
    return FALSE;
  int iMax = ((ExprNode*)pMaxCard->m_pArgs[1])->m_iTerm-1;
  ExprNode* pMaxC = (ExprNode*)pMaxCard->m_pArgs[2];

  for(ExprNodes::iterator i = m_aTypes[Node::MIN].begin(); i != m_aTypes[Node::MIN].end(); i++ )
    {
      ExprNode* pMinCard = (ExprNode*)*i;

      Role* pMinRole = g_pKB->getRole((ExprNode*)pMinCard->m_pArgs[0]);
      if( pMinRole == NULL )
	return FALSE;
      int iMin = ((ExprNode*)pMinCard->m_pArgs[1])->m_iTerm;
      ExprNode* pMinC = (ExprNode*)pMinCard->m_pArgs[2];
      
      if( iMax < iMin && ::isEqual(pMinC, pMaxC) == 0 && pMinRole->isSubRoleOf(pMaxRole) )
	{
	  DependencySet* pMinDepends = getDepends(pMinCard);
	  DependencySet* pSubDepends = pMaxRole->getExplainSub(pMinRole->m_pName);
	  DependencySet* pDS = pMinDepends->unionNew(pMaxDepends, m_pABox->m_bDoExplanation)->unionDS(pSubDepends, m_pABox->m_bDoExplanation);

	  DECOMMENT1("MinRole=%x", pMinRole);
	  DECOMMENT1("MaxRole=%x", pMaxRole);
	  printExprNodeWComment("Max=", pNormalizedMax);
	  printExprNodeWComment("Min=", pMinCard);

	  m_pABox->setClash(new Clash(this, Clash::MIN_MAX, pDS));
	  return TRUE;
	}
    }

  return FALSE;
}

bool Individual::isRedundantMin(ExprNode* pMinCard)
{
  Role* pMinRole = g_pKB->getRole((ExprNode*)pMinCard->m_pArgs[0]);
  if( pMinRole == NULL )
    return FALSE;
  
  int iMin = ((ExprNode*)pMinCard->m_pArgs[1])->m_iTerm;
  ExprNode* pMinQ = (ExprNode*)pMinCard->m_pArgs[2];

  for(ExprNodes::iterator i = m_aTypes[Node::MIN].begin(); i != m_aTypes[Node::MIN].end(); i++ )
    {
      ExprNode* pPrevMinCard = (ExprNode*)*i;
      Role* pPrevMinRole = g_pKB->getRole((ExprNode*)pPrevMinCard->m_pArgs[0]);
      if( pPrevMinRole == NULL )
	continue;

      int iPrevMin = ((ExprNode*)pPrevMinCard->m_pArgs[1])->m_iTerm-1;
      ExprNode* pPrevMinQ = (ExprNode*)pPrevMinCard->m_pArgs[2];

      if( iMin <= iPrevMin && pPrevMinRole->isSubRoleOf(pMinRole) && (::isEqual(pMinQ, pPrevMinQ) == 0 || isTop(pMinQ)) )
	return TRUE;
    }

  return FALSE;
}

bool Individual::isRedundantMax(ExprNode* pMaxCard)
{
  Role* pMaxRole = g_pKB->getRole((ExprNode*)pMaxCard->m_pArgs[0]);
  if( pMaxRole == NULL )
    return FALSE;
  
  int iMax = ((ExprNode*)pMaxCard->m_pArgs[1])->m_iTerm-1;
  if( iMax == 1 && pMaxRole->isFunctional() )
    return TRUE;

  ExprNode* pMaxQ = (ExprNode*)pMaxCard->m_pArgs[2];

  for(ExprNodes::iterator i = m_aTypes[Node::MAX].begin(); i != m_aTypes[Node::MAX].end(); i++ )
    {
      ExprNode* pMC = (ExprNode*)*i;
      
      // max(r, n) is in normalized form not(min(p, n + 1))
      ExprNode* pPrevMaxCard = (ExprNode*)pMC->m_pArgs[0];
      Role* pPrevMaxRole = g_pKB->getRole((ExprNode*)pPrevMaxCard->m_pArgs[0]);
      if( pPrevMaxRole == NULL )
	continue;

      int iPrevMax = ((ExprNode*)pPrevMaxCard->m_pArgs[1])->m_iTerm-1;
      ExprNode* pPrevMaxQ = (ExprNode*)pPrevMaxCard->m_pArgs[2];

      if( iMax >= iPrevMax && pMaxRole->isSubRoleOf(pPrevMaxRole) && (::isEqual(pMaxQ, pPrevMaxQ) == 0 || isTop(pMaxQ)) )
	return TRUE;
    }

  return FALSE;
}

void Individual::removeType(ExprNode* pC)
{
  m_mDepends.erase(pC);
  setChanged(TRUE);

  if( isPrimitive(pC) || pC->m_iExpression == EXPR_SELF )
    m_aTypes[Node::ATOM].remove(pC);
  else
    {
      if( pC->m_iExpression == EXPR_AND )
	{
	  // commented out
	}
      else if( pC->m_iExpression == EXPR_ALL )
	m_aTypes[Node::ALL].remove(pC);
      else if( pC->m_iExpression == EXPR_MIN )
	m_aTypes[Node::MIN].remove(pC);
      else if( pC->m_iExpression == EXPR_NOT )
	{
	  ExprNode* pX = (ExprNode*)pC->m_pArgs[0];
	  if( pX->m_iExpression == EXPR_AND )
	    m_aTypes[Node::OR].remove(pC);
	  else if( pX->m_iExpression == EXPR_ALL )
	    m_aTypes[Node::SOME].remove(pC);
	  else if( pX->m_iExpression == EXPR_MIN )
	    m_aTypes[Node::MAX].remove(pC);
	  else if( ::isNominal(pX) )
	    m_aTypes[Node::ATOM].remove(pC);
	  else if( pX->m_iArity == 0 )
	    m_aTypes[Node::ATOM].remove(pC);
	  else
	    assertFALSE("Invalid type ");
	}
      else if( pC->m_iExpression == EXPR_VALUE )
	m_aTypes[Node::NOM].remove(pC);
      else
	assertFALSE("Invalid type ");
    }
}

bool Individual::restore(int iBranch)
{
  bool bRestore = Node::restore(iBranch);
  if( !bRestore )
    return FALSE;

  for(int i = 0; i < TYPECOUNT; i++ )
    m_iApplyNext[i] = 0;

  int iSize = m_listOutEdges.m_listEdge.size(), iIndex = 0;
  for(EdgeVector::iterator i = m_listOutEdges.m_listEdge.begin(); i != m_listOutEdges.m_listEdge.end() && iIndex < iSize; iIndex++ )
    {
      Edge* pEdge = (Edge*)*i;
      Node* pNode = pEdge->m_pTo;
      DependencySet* pDS = pEdge->m_pDepends;

      if( pDS->m_iBranch > iBranch )
	{
	  i = m_listOutEdges.m_listEdge.erase(i);
	  //printf("Removing edge to(%d > %d) ", pDS->m_iBranch, iBranch);
	  //printExprNode(pNode->m_pName, TRUE);
	  //printf("(this=");
	  //printExprNode(m_pName);
	}
      else
	{
	  i++;
	  //printf("Not removing edge to(%d > %d) ", pDS->m_iBranch, iBranch);
	  //printExprNode(pNode->m_pName, TRUE);
	  //printf("(this=");
	  //printExprNode(m_pName);
	}
    }

  return TRUE;
}

void Individual::unprune(int iBranch)
{
  Node::unprune(iBranch);

  for(EdgeVector::iterator i = m_listOutEdges.m_listEdge.begin(); i != m_listOutEdges.m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      Node* pNode = pEdge->m_pTo;
      DependencySet* pDS = pEdge->m_pDepends;
      
      if( pDS->m_iBranch <= iBranch )
	{
	  Node* pSucc = pEdge->m_pTo;
	  Role* pRole = pEdge->m_pRole;

	  if( !pSucc->m_listInEdges.hasEdge(this, pRole, pSucc) )
	    pSucc->addInEdge(pEdge);
	}
    }
}

int isEqualIndividual(const Individual* pIndividual1, const Individual* pIndividual2)
{
  return isEqual(pIndividual1, pIndividual2);
}

int isEqualIndividualStatePair(const IndividualStatePair* pIndividualStatePair1, const IndividualStatePair* pIndividualStatePair2)
{
  if( isEqualIndividual((Individual*)pIndividualStatePair1->first, (Individual*)pIndividualStatePair2->first) == 0 &&
      isEqualState((State*)pIndividualStatePair1->second, (State*)pIndividualStatePair2->second) == 0 )
    return 0;
  else if( isEqualIndividual((Individual*)pIndividualStatePair1->first, (Individual*)pIndividualStatePair2->first) == 0 )
    return isEqualState((State*)pIndividualStatePair1->second, (State*)pIndividualStatePair2->second);
  else if( isEqualState((State*)pIndividualStatePair1->second, (State*)pIndividualStatePair2->second) == 0 )
    return isEqualIndividual((Individual*)pIndividualStatePair1->first, (Individual*)pIndividualStatePair2->first);
  
  return isEqualIndividual((Individual*)pIndividualStatePair1->first, (Individual*)pIndividualStatePair2->first)+isEqualState((State*)pIndividualStatePair1->second, (State*)pIndividualStatePair2->second);
}
