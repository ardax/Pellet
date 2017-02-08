#include "Branch.h"
#include "Node.h"
#include "DependencySet.h"
#include "DependencyIndex.h"
#include "ABox.h"
#include "Clash.h"
#include "Role.h"
#include "Params.h"
#include "Individual.h"
#include "KnowledgeBase.h"
#include "Literal.h"
#include "Datatype.h"
#include "QueueElement.h"
#include "CompletionQueue.h"
#include "CompletionStrategy.h"
#include "ReazienerUtils.h"
#include "ZList.h"

#include <cassert>
#include <limits.h>

extern ExprNode* EXPRNODE_TOP;
extern DependencySet DEPENDENCYSET_EMPTY;
extern DependencySet DEPENDENCYSET_INDEPENDENT;
extern KnowledgeBase* g_pKB;
extern int g_iCommentIndent;

Branch::Branch(ABox* pABox, CompletionStrategy* pStrategy, Node* pX, DependencySet* pDS, int iN)
{
  m_pABox = pABox;
  m_pCompletionStrategy = pStrategy;
  m_pNode = pX;
  m_pNodeName = m_pNode->m_pName;
  
  if( pX->isIndividual() )
    m_pIndv = (Individual*)pX;
  else
    m_pIndv = NULL;

  m_iBranchType = 0;
  m_iTryCount = iN;
  m_iTryNext = 0;

  m_iBranch = m_pABox->m_iCurrentBranchIndex;
  m_iAnonCount = m_pABox->m_iAnonCount;
  m_iNodeCount = m_pABox->m_mNodes.size();

  m_pTermDepends = pDS;
  m_pPrevDS = &DEPENDENCYSET_EMPTY;

  assert(m_iTryCount>0);
}

bool Branch::isEqual(Branch* pBranch)
{
  assert(FALSE);
  return FALSE;
}

Branch* Branch::copyTo(ABox* pABox)
{
  return NULL;
}

bool Branch::tryNext()
{
  START_DECOMMENT2("Branch::tryNext");
  if( m_pABox->isClosed() )
    {
      END_DECOMMENT("Branch::tryNext");
      return FALSE;
    }

  tryBranch();
  if( m_pABox->isClosed() )
    {
      if( !PARAMS_USE_INCREMENTAL_DELETION() )
	m_pABox->getClash()->m_pDepends->remove(m_iBranch);
    }
  END_DECOMMENT("Branch::tryNext");
  return !m_pABox->isClosed();
}

void Branch::setLastClash(DependencySet* pDS)
{
  if( m_iTryNext >= 0 )
    {
      m_pPrevDS = m_pPrevDS->unionDS(pDS, m_pABox->m_bDoExplanation);
      if( PARAMS_USE_INCREMENTAL_DELETION() )
	{
	  //CHW - added for incremental deletions support THIS SHOULD BE MOVED TO SUPER
	  g_pKB->m_pDependencyIndex->addCloseBranchDependency(this, pDS);
	}
    }
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

GuessBranch::GuessBranch(ABox* pABox, 
			 CompletionStrategy* pStrategy, 
			 Individual* pX, 
			 Role* pRole, 
			 int iMinGuess, 
			 int iMaxGuess, 
			 DependencySet* pDS) :  Branch(pABox, pStrategy, pX, pDS, (iMaxGuess-iMinGuess+1))
{
  m_iBranchType = BRANCH_GUESS;
  m_pRole = pRole;
  m_iMinGuess = iMinGuess;
 }

void GuessBranch::tryBranch()
{
  START_DECOMMENT2("GuessBranch::tryBranch");
  m_pABox->incrementBranch();
  
  DependencySet* pDS = m_pTermDepends;
  for(; m_iTryNext < m_iTryCount; m_iTryNext++ )
    {
      // start with max possibility and decrement at each try  
      int n = m_iMinGuess+m_iTryCount-m_iTryNext-1;
      
      pDS = pDS->unionNew((new DependencySet(m_iBranch)), m_pABox->m_bDoExplanation);
      
      // add the max cardinality for guess
      m_pCompletionStrategy->addType(m_pIndv, createNormalizedMax(m_pRole->m_pName, n, EXPRNODE_TOP), pDS);

      Nodes aIndividuals;
      for(int c = 0; c  <n; c++ )
	{
	  Individual* pIndividual = m_pCompletionStrategy->createFreshIndividual(TRUE);
	  m_pCompletionStrategy->addEdge(m_pIndv, m_pRole, pIndividual, pDS);
	  for(Nodes::iterator j = aIndividuals.begin(); j != aIndividuals.end(); j++ )
	    {
	      Individual* pInd = (Individual*)*j;
	      pIndividual->setDifferent(pInd, pDS);
	    }
	  aIndividuals.push_back(pIndividual);
	}

      // add the min cardinality restriction just to make early clash detection easier
      m_pCompletionStrategy->addType(m_pIndv, createExprNode(EXPR_MIN, m_pRole->m_pName, n, EXPRNODE_TOP), pDS);

      if( m_pABox->isClosed() )
	{
	  DependencySet* pClashDepends = m_pABox->getClash()->m_pDepends;
	  if( pClashDepends->contains(m_iBranch) )
	    {
	      // we need a global restore here because the merge operation modified three
	      // different nodes and possibly other global variables
	      m_pCompletionStrategy->restore(this);

	      // global restore sets the branch number to previous value so we need to
	      // increment it again
	      m_pABox->incrementBranch();

	      setLastClash(pClashDepends);
	    }
	  else
	    {
	      END_DECOMMENT("GuessBranch::tryBranch");
	      return;
	    }
	}
      else
	{
	  END_DECOMMENT("GuessBranch::tryBranch");
	  return;
	}
    }

  pDS = getCombinedClash();

  if( !PARAMS_USE_INCREMENTAL_DELETION() )
    pDS->remove(m_iBranch);

  m_pABox->setClash(Clash::unexplained(m_pIndv, pDS));
  END_DECOMMENT("GuessBranch::tryBranch");
}

Branch* GuessBranch::copyTo(ABox* pABox)
{
  Individual* pX = m_pABox->getIndividual(m_pIndv->m_pName);
  Branch* pGuessBranch = new GuessBranch(m_pABox, NULL, pX, m_pRole, m_iMinGuess, (m_iMinGuess+m_iTryCount-1), m_pTermDepends);

  pGuessBranch->m_iAnonCount = m_iAnonCount;
  pGuessBranch->m_iNodeCount = m_iNodeCount;
  pGuessBranch->m_iBranch = m_iBranch;
  pGuessBranch->m_pNodeName = m_pIndv->m_pName;
  pGuessBranch->m_pCompletionStrategy = m_pCompletionStrategy;
  pGuessBranch->m_iTryNext = m_iTryNext;

  return pGuessBranch;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

MaxBranch::MaxBranch(ABox* pABox, 
		     CompletionStrategy* pStrategy, 
		     Individual* pX,
		     Role* pRole, 
		     int iN, 
		     ExprNode* pQualification, 
		     NodeMerges* pMergePairs, 
		     DependencySet* pDS) : Branch(pABox, pStrategy, pX, pDS, pMergePairs->size())
{
  m_iBranchType = BRANCH_MAX;
  m_pRole = pRole;
  m_iN = iN;
  m_pQualification = pQualification;

  m_pMergePairs = getNewZCollection();

  initZ(&m_zcPrevDS, FALSE,  pMergePairs->size());
  initZ(m_pMergePairs, FALSE,  pMergePairs->size());

  for(NodeMerges::iterator i = pMergePairs->begin(); i != pMergePairs->end(); i++ )
    addZ(m_pMergePairs, ((NodeMerge*)*i));
}

MaxBranch::MaxBranch(ABox* pABox, 
		     CompletionStrategy* pStrategy, 
		     Individual* pX,
		     Role* pRole, 
		     int iN, 
		     ExprNode* pQualification, 
		     ZCollection* pMergePairs, 
		     DependencySet* pDS) : Branch(pABox, pStrategy, pX, pDS, pMergePairs->m_iUsedSize)
{
  m_iBranchType = BRANCH_MAX;
  m_pRole = pRole;
  m_iN = iN;
  m_pMergePairs = pMergePairs;
  m_pQualification = pQualification;

  initZ(&m_zcPrevDS, FALSE, pMergePairs->m_iUsedSize);
}

void MaxBranch::tryBranch()
{
  START_DECOMMENT2("MaxBranch::tryBranch");
  DECOMMENT1("CurrBranch=%d", m_iBranch);
  m_pABox->incrementBranch();
  
  //we must re-add this individual to the max queue. 
  //This is because we may still need to keep 
  //applying the max rule for additional merges		
  //recreate the label for the individuals
  ExprNode* pMaxCon = createExprNode(EXPR_MAX, m_pRole->m_pName, m_iN, m_pQualification);
  //normalize the label
  pMaxCon = normalize(pMaxCon);
  //create the queue element
  QueueElement* pElement = new QueueElement(m_pIndv->m_pName, pMaxCon);
  //add to the queue
  m_pABox->m_pCompletionQueue->add(pElement, CompletionQueue::MAXLIST);
  m_pABox->m_pCompletionQueue->add(pElement, CompletionQueue::CHOOSELIST);

  DependencySet* pDS = m_pTermDepends;
  for(; m_iTryNext < m_iTryCount; m_iTryNext++)
    {
      DECOMMENT1("TryNext=%d", m_iTryNext);
      DECOMMENT1("TryCount=%d", m_iTryCount);

      if( PARAMS_USE_SEMANTIC_BRANCHING() )
	{
	  for(int m = 0; m < m_iTryNext; m++ )
	    {
	      NodeMerge* pNM = (NodeMerge*)getAtZ(m_pMergePairs, m);
	      Node* pY = m_pABox->getNode(pNM->m_pY);
	      Node* pZ = m_pABox->getNode(pNM->m_pZ);
	      pY->setDifferent(pZ, (DependencySet*)getAtZ(&m_zcPrevDS, m));
	      // already commented in original code
	      //strategy.addType( y, ATermUtils.makeNot( ATermUtils.makeValue( z.getName() ) ), prevDS[m] );
	    }
	}

      NodeMerge* pNM = (NodeMerge*)getAtZ(m_pMergePairs, m_iTryNext);
      Node* pY = m_pABox->getNode(pNM->m_pY);
      Node* pZ = m_pABox->getNode(pNM->m_pZ);
      
      pDS = pDS->unionNew(new DependencySet(m_iBranch), m_pABox->m_bDoExplanation);
      
      // max cardinality merge also depends on all the edges 
      // between the individual that has the cardinality and 
      // nodes that are going to be merged 
      EdgeList edgesRNeighbors;
      m_pIndv->getRNeighborEdges(m_pRole, &edgesRNeighbors);
      bool bYEdge = FALSE, bZEdge = FALSE;
      for(EdgeVector::iterator i = edgesRNeighbors.m_listEdge.begin(); i != edgesRNeighbors.m_listEdge.end(); i++ )
	{
	  Edge* pEdge = (Edge*)*i;
	  Node* pNeighbor = pEdge->getNeighbor(m_pIndv);
	  
	  if( ::isEqual(pNeighbor, pY) == 0 )
	    {
	      pDS = pDS->unionNew(pEdge->m_pDepends, m_pABox->m_bDoExplanation);
	      bYEdge = TRUE;
	    }
	  else if( ::isEqual(pNeighbor, pZ) == 0 )
	    {
	      pDS = pDS->unionNew(pEdge->m_pDepends, m_pABox->m_bDoExplanation);
	      bZEdge = TRUE;
	    }
	}

      // if there is no edge coming into the node that is
      // going to be merged then it is not possible that
      // they are affected by the cardinality restriction
      // just die instead of possibly unsound results
      if( !bZEdge || !bYEdge )
	assertFALSE("An error occurred related to the max cardinality restriction about ");
      
      // if the neighbor nodes did not have the qualification
      // in their type list they would have not been affected
      // by the cardinality restriction. so this merges depends
      // on their types
      pDS = pDS->unionNew(pY->getDepends(m_pQualification), m_pABox->m_bDoExplanation);
      pDS = pDS->unionNew(pZ->getDepends(m_pQualification), m_pABox->m_bDoExplanation);

      // if there were other merges based on the exact same cardinality
      // restriction then this merge depends on them, too (we wouldn't
      // have to merge these two nodes if the previous merge did not
      // eliminate some other possibilities)
      for(int i = m_pABox->m_aBranches.size()-2; i >= 0; i-- )
	{
	  Branch* pBranch = m_pABox->m_aBranches.at(i);
	  if( pBranch->m_iBranchType == BRANCH_MAX )
	    {
	      MaxBranch* pPrevBranch = (MaxBranch*)pBranch;
	      if( ::isEqual(pPrevBranch->m_pIndv, m_pIndv) == 0 && 
		  ::isEqual(pPrevBranch->m_pRole, m_pRole) == 0 &&
		  ::isEqual(pPrevBranch->m_pQualification, m_pQualification) == 0 )
		{
		  pDS->add(pPrevBranch->m_iBranch);
		}
	      else
		break;
	    }
	  else
	    break;
	}

      m_pCompletionStrategy->mergeTo(pY, pZ, pDS);

      // early crash
      if( m_pABox->isClosed() )
	{
	  DependencySet* pClashDepends = m_pABox->getClash()->m_pDepends;
	  if( pClashDepends->contains(m_iBranch) )
	    {
	      // we need a global restore here because the merge operation modified three
	      // different nodes and possibly other global variables
	      m_pCompletionStrategy->restore(this);

	      // global restore sets the branch number to previous value so we need to
	      // increment it again
	      m_pABox->incrementBranch();

	      setLastClash(pClashDepends);
	    }
	  else
	    {
	      END_DECOMMENT("MaxBranch::tryBranch");
	      return;
	    }
	}
      else
	{
	  END_DECOMMENT("MaxBranch::tryBranch");
	  return;
	}
    }

  pDS = getCombinedClash();
  if( !PARAMS_USE_INCREMENTAL_DELETION() )
    pDS->remove(m_iBranch);

  if( m_pABox->m_bDoExplanation )
    m_pABox->setClash(Clash::maxCardinality(m_pIndv, pDS, m_pRole->m_pName, m_iN));
  else
    m_pABox->setClash(Clash::maxCardinality(m_pIndv, pDS));
  END_DECOMMENT("MaxBranch::tryBranch");
}

Branch* MaxBranch::copyTo(ABox* pABox)
{
  Individual* pX = m_pABox->getIndividual(m_pIndv->m_pName);
  MaxBranch* pMB = new MaxBranch(m_pABox, NULL, pX, m_pRole, m_iN, m_pQualification, m_pMergePairs, m_pTermDepends);
  
  pMB->m_iAnonCount = m_iAnonCount;
  pMB->m_iNodeCount = m_iNodeCount;
  pMB->m_iBranch = m_iBranch;
  pMB->m_pNodeName = m_pIndv->m_pName;
  pMB->m_pCompletionStrategy = m_pCompletionStrategy;
  pMB->m_iTryNext = m_iTryNext;
  copyZs(&(pMB->m_zcPrevDS), &m_zcPrevDS);

  return pMB;
}

/*
 * Added for to re-open closed branches.
 * This is needed for incremental reasoning through deletions
 */
void MaxBranch::shiftTryNext(int iOpenIndex)
{
  //re-open the merge pair
  NodeMerge* pNM = (NodeMerge*)removeAtZ(m_pMergePairs, iOpenIndex);
  addZ(m_pMergePairs, pNM);
  
  //shift the previous ds
  removeAtZ(&m_zcPrevDS, iOpenIndex);
 
  m_iTryNext--;
}

void MaxBranch::setLastClash(DependencySet* pDS)
{
  Branch::setLastClash(pDS);
  if( m_iTryNext >= 0 )
    setAtZ(&(m_zcPrevDS), m_iTryNext, pDS);
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

DisjunctionBranch::DisjunctionBranch(ABox* pABox, 
				     CompletionStrategy* pStrategy, 
				     Node* pNode, 
				     ExprNode* pDisjunction, 
				     DependencySet* pDS) : Branch(pABox, pStrategy, pNode, pDS, 0)
{
  m_iBranchType = BRANCH_DISJUNCTION;
  m_pDisjunction = pDisjunction;

  initZ(&m_zcDisj);
  initZ(&m_zcPrevDS);
  initI(&m_icOrder);
}

DisjunctionBranch::DisjunctionBranch(ABox* pABox, 
				     CompletionStrategy* pStrategy, 
				     Node* pNode, 
				     ExprNode* pDisjunction, 
				     DependencySet* pDS, 
				     ExprNodes* paDisj) : Branch(pABox, pStrategy, pNode, pDS, paDisj->size())
{
  START_DECOMMENT2("DisjunctionBranch::DisjunctionBranch");
  m_iBranchType = BRANCH_DISJUNCTION;
  m_pDisjunction = pDisjunction;

  initZ(&m_zcDisj, FALSE, paDisj->size());
  initZ(&m_zcPrevDS, FALSE, paDisj->size());
  initI(&m_icOrder, paDisj->size());

  if( paDisj )
    {
      int iIndex = 0;
      for(ExprNodes::iterator i = paDisj->begin(); i != paDisj->end(); i++ )
	{
	  printExprNodeWComment("New Disj=", ((ExprNode*)*i));
	  addZ(&m_zcDisj, ((ExprNode*)*i));
	  addI(&m_icOrder, iIndex++);
	}
    }
  END_DECOMMENT("DisjunctionBranch::DisjunctionBranch");
}

DisjunctionBranch::DisjunctionBranch(ABox* pABox, 
				     CompletionStrategy* pStrategy, 
				     Node* pNode, 
				     ExprNode* pDisjunction, 
				     DependencySet* pDS, 
				     ZCollection* pZDisj) : Branch(pABox, pStrategy, pNode, pDS, pZDisj->m_iUsedSize)
{
  m_iBranchType = BRANCH_DISJUNCTION;
  m_pDisjunction = pDisjunction;

  initZ(&m_zcDisj, FALSE, pZDisj->m_iUsedSize);
  initZ(&m_zcPrevDS, FALSE, pZDisj->m_iUsedSize);
  copyZs(&m_zcDisj, pZDisj);

  for(int i = 0; i < pZDisj->m_iUsedSize; i++ )
    addI(&m_icOrder, i);
}

void DisjunctionBranch::tryBranch()
{
  START_DECOMMENT2("DisjunctionBranch::tryBranch");
  DECOMMENT1("CurrBranch=%d", m_iBranch);
  m_pABox->incrementBranch();

  ICollection* pStats = NULL;
  if( PARAMS_USE_DISJUNCTION_SORTING() )
    {
      map<ExprNode*, ICollection*>::iterator i = m_pABox->m_mDisjBranchStats.find(m_pDisjunction);
      if( i != m_pABox->m_mDisjBranchStats.end() )
	pStats = (ICollection*)i->second;

      if( pStats == NULL )
	{
	  int iPreference = preferredDisjunct();
	  pStats = getNewICollection();
	  for(int j = 0; j < 11; j++ )
	    addI(pStats, ((j!=iPreference)?0:INT_MIN));
	  m_pABox->m_mDisjBranchStats[m_pDisjunction] = pStats;
	}

      if( m_iTryNext > 0 )
	pStats->m_pList[getAtI(&m_icOrder, m_iTryNext-1)]++;

      if( pStats )
	{
	  int iMinIndex = m_iTryNext;
	  int iMinValue = getAtI(pStats, m_iTryNext);
	  for(int i = m_iTryNext+1; i < pStats->m_iUsedSize; i++ )
	    {
	      if( getAtI(pStats, i) < iMinValue )
		{
		  iMinIndex = i;
		  iMinValue = getAtI(pStats, i);
		}
	    }

	  if( iMinIndex != m_iTryNext )
	    {
	      ExprNode* pSelDisj = (ExprNode*)getAtZ(&m_zcDisj, iMinIndex);
	      setAtZ(&m_zcDisj, iMinIndex, (ExprNode*)getAtZ(&m_zcDisj, m_iTryNext));
	      setAtZ(&m_zcDisj, m_iTryNext, pSelDisj);

	      setAtI(&m_icOrder, iMinIndex, m_iTryNext);
	      setAtI(&m_icOrder, m_iTryNext, iMinIndex);
	    }
	}
    }
  
  for(; m_iTryNext < m_iTryCount; m_iTryNext++ )
    {
      DECOMMENT1("TryNext=%d", m_iTryNext);
      DECOMMENT1("TryCount=%d", m_iTryCount);
      ExprNode* pD = (ExprNode*)getAtZ(&m_zcDisj, m_iTryNext);
      printExprNodeWComment("Try=", pD);
      
      if( PARAMS_USE_SEMANTIC_BRANCHING() )
	{
	  for(int m = 0; m < m_iTryNext; m++ )
	    m_pCompletionStrategy->addType(m_pIndv, negate2((ExprNode*)getAtZ(&m_zcDisj, m)), (DependencySet*)getAtZ(&m_zcPrevDS, m));
	}

      DependencySet* pDS = NULL;
      if( m_iTryNext == m_iTryCount-1 && !PARAMS_SATURATE_TABLEAU() )
	{
	  pDS = m_pTermDepends;
	  for(int m = 0; m < m_iTryNext; m++ )
	    pDS = pDS->unionNew((DependencySet*)getAtZ(&m_zcPrevDS, m), m_pABox->m_bDoExplanation);

	  //CHW - added for incremental reasoning and rollback through deletions
	  if( PARAMS_USE_INCREMENTAL_DELETION() )
	    pDS->m_setExplain.insert(m_pTermDepends->m_setExplain.begin(), m_pTermDepends->m_setExplain.end());
	  else
	    pDS->remove(m_iBranch);
	}
      else
	{
	  //CHW - Changed for tracing purposes
	  if( PARAMS_USE_INCREMENTAL_DELETION() )
	    pDS = m_pTermDepends->unionNew((new DependencySet(m_iBranch)), m_pABox->m_bDoExplanation);
	  else
	    {
	      pDS = new DependencySet(m_iBranch);
	      //added for tracing
	      pDS->m_setExplain.insert(m_pTermDepends->m_setExplain.begin(), m_pTermDepends->m_setExplain.end());
	    }
	}

      ExprNode* pNotD = negate2(pD);
      DependencySet* pClashDepends = PARAMS_SATURATE_TABLEAU()?NULL:m_pIndv->getDepends(pNotD);
      if( pClashDepends == NULL )
	{
	  m_pCompletionStrategy->addType(m_pIndv, pD, pDS);
	  // we may still find a clash if concept is allValuesFrom
	  // and there are some conflicting edges
	  if( m_pABox->isClosed() )
	    pClashDepends = m_pABox->getClash()->m_pDepends;

	  if( pClashDepends )
	    {
	      DECOMMENT("ClashDepends exists via m_pABox->getClash()->m_pDepends");
	    }
	}
      else
	{
	  printExprNodeWComment("Negated pD exists=", pNotD);
	  pClashDepends = pClashDepends->unionNew(pDS, m_pABox->m_bDoExplanation);
	}

      if( pClashDepends )
	{
	  if( PARAMS_USE_DISJUNCTION_SORTING() )
	    {
	      if( pStats == NULL )
		{
		  pStats = getNewICollection();
		  for(int i = 0; i < m_zcDisj.m_iUsedSize; i++ )
		    addI(pStats, 0);
		  m_pABox->m_mDisjBranchStats[m_pDisjunction] = pStats;
		}
	      pStats->m_pList[getAtI(&m_icOrder, m_iTryNext)]++;
	    }

	  // do not restore if we do not have any more branches to try. after
	  // backtrack the correct branch will restore it anyway. more
	  // importantly restore clears the clash info causing exceptions
	  if( m_iTryNext < m_iTryCount-1 && pClashDepends->contains(m_iBranch) )
	    {
	      // do not restore if we find the problem without adding the concepts 
	      if( m_pABox->isClosed() )
		{
		  // we need a global restore here because one of the disjuncts could be an 
		  // all(r,C) that changed the r-neighbors
		  m_pCompletionStrategy->restore(this);
		  // global restore sets the branch number to previous value so we need to
		  // increment it again
		  m_pABox->incrementBranch();
		}
	      setLastClash(new DependencySet(pClashDepends));
	    }
	  else
	    {
	      // set the clash only if we are returning from the function
	      if( m_pABox->m_bDoExplanation )
		{
		  ExprNode *pPositive = (pNotD->m_iExpression==EXPR_NOT)?pD:pNotD;
		  m_pABox->setClash(Clash::atomic(m_pIndv, pClashDepends->unionNew(pDS, m_pABox->m_bDoExplanation), pPositive));
		}
	      else
		m_pABox->setClash(Clash::atomic(m_pIndv, pClashDepends->unionNew(pDS, m_pABox->m_bDoExplanation)));
	      
	      //CHW - added for inc reasoning
	      if( PARAMS_USE_INCREMENTAL_DELETION() )
		g_pKB->m_pDependencyIndex->addCloseBranchDependency(this, m_pABox->getClash()->m_pDepends);

	      END_DECOMMENT("DisjunctionBranch::tryBranch");
	      return;
	    }
	}
      else
	{
	  END_DECOMMENT("DisjunctionBranch::tryBranch");
	  return;
	}
    }

  // this code is not unreachable. if there are no branches left restore does not call this 
  // function, and the loop immediately returns when there are no branches left in this
  // disjunction. If this exception is thrown it shows a bug in the code.
  assertFALSE("This exception should not be thrown!");
  END_DECOMMENT("DisjunctionBranch::tryBranch");
}

Branch* DisjunctionBranch::copyTo(ABox* pABox)
{
  Individual* pX = m_pABox->getIndividual(m_pIndv->m_pName);
  DisjunctionBranch* pDB = new DisjunctionBranch(m_pABox, NULL, pX, m_pDisjunction, m_pTermDepends, &m_zcDisj);
  
  pDB->m_iAnonCount = m_iAnonCount;
  pDB->m_iNodeCount = m_iNodeCount;
  pDB->m_iBranch = m_iBranch;
  pDB->m_pNodeName = m_pIndv->m_pName;
  pDB->m_pCompletionStrategy = m_pCompletionStrategy;
  pDB->m_iTryNext = m_iTryNext;

  copyZs(&(pDB->m_zcPrevDS), &m_zcPrevDS);
  copyIs(&(pDB->m_icOrder), &m_icOrder);

  return pDB;
}

int DisjunctionBranch::preferredDisjunct()
{
  if( m_zcDisj.m_iUsedSize != 2 )
    return -1;

  ExprNode *pEN1 = (ExprNode*)getAtZ(&m_zcDisj, 0);
  ExprNode* pEN2 = (ExprNode*)getAtZ(&m_zcDisj, 1);
  
  if( isPrimitive(pEN1) && 
      pEN2->m_iExpression == EXPR_ALL && 
      ((ExprNode*)pEN2->m_pArgs[1])->m_iExpression == EXPR_NOT )
    return 1;

  if( isPrimitive(pEN2) && 
      pEN1->m_iExpression == EXPR_ALL && 
      ((ExprNode*)pEN1->m_pArgs[1])->m_iExpression == EXPR_NOT )
    return 0;
  return -1;
}

void DisjunctionBranch::shiftTryNext(int iIndex)
{
  int iOrder = getAtI(&m_icOrder, iIndex);
  ExprNode* pDis = (ExprNode*)getAtZ(&m_zcDisj, iIndex);

  //TODO: also need to handle semantic branching	
  if( PARAMS_USE_SEMANTIC_BRANCHING() )
    {
      //if(this.ind.getDepends(ATermUtils.makeNot(dis)) != null)
      //check if the depedency is the same as preDS - if so, then we know that we added it
    }

  //need to shift both prevDS and next and order disjfirst
  removeAtZ(&m_zcPrevDS, iIndex);
  removeAtZ(&m_zcDisj, iIndex);
  // CHECK HERE : Branch.h
  // orginal code does not change order
  // order[i] = order[i];

  //move open label to end
  addZ(&m_zcDisj, pDis);
  // but update order
  setAtI(&m_icOrder, m_zcDisj.m_iUsedSize-1, m_zcDisj.m_iUsedSize-1);

  //decrement trynext
  m_iTryNext--;
}

void DisjunctionBranch::setLastClash(DependencySet* pDS)
{
  Branch::setLastClash(pDS);
  if( m_iTryNext >= 0 )
    setAtZ(&(m_zcPrevDS), m_iTryNext, pDS);
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

ChooseBranch::ChooseBranch(ABox* pABox, 
			   CompletionStrategy* pStrategy, 
			   Node* pNode, 
			   ExprNode* pC, 
			   DependencySet* pDS) : DisjunctionBranch(pABox, pStrategy, pNode, pC, pDS)
{
  m_iBranchType = BRANCH_CHOOSE;

  initZ(&m_zcDisj, FALSE, 2);
  initI(&m_icOrder, 2);

  addZ(&m_zcDisj, pC);
  addZ(&m_zcDisj, negate2(pC));

  addI(&m_icOrder, 0);
  addI(&m_icOrder, 1);
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

LiteralValueBranch::LiteralValueBranch(ABox* pABox, 
				       CompletionStrategy* pStrategy, 
				       Literal* pLiteral, 
				       Datatype* pDatatype) : Branch(pABox, pStrategy, pLiteral, &DEPENDENCYSET_INDEPENDENT, 0) // pDatatype->size() == ValueSpace.INFINITE ? Integer.MAX_VALUE : datatype.size()
{
  m_iBranchType = BRANCH_LITERALVALUE;
  m_iShuffle = m_pABox->m_iCurrentBranchIndex;
  m_pDatatype = pDatatype;
}

void LiteralValueBranch::tryBranch()
{
  m_pABox->incrementBranch();

  DependencySet* pDS = m_pTermDepends;
  for(; m_iTryNext < m_iTryCount; m_iTryNext++)
    {
      int iTryIndex = (m_iTryNext+m_iShuffle)%m_iTryCount;
      ExprNode* pValue = m_pDatatype->getValue(iTryIndex);
      pDS = pDS->unionNew(new DependencySet(m_iBranch), m_pABox->m_bDoExplanation);
      
      Node* pY = m_pNode;
      Node* pZ = m_pABox->getNode(pValue);
      if( pZ == NULL )
	pZ = m_pABox->addLiteral(pValue);
      
      m_pCompletionStrategy->mergeTo(pY, pZ, pDS);
      
      // early clash
      if( m_pABox->isClosed() )
	{
	  DependencySet* pClashDepends = m_pABox->getClash()->m_pDepends;
	  if( pClashDepends->contains(m_iBranch) )
	    {
	      // we need a global restore here because the merge operation modified three
	      // different nodes and possibly other global variables
	      m_pCompletionStrategy->restore(this);

	      // global restore sets the branch number to previous value so we need to
	      // increment it again
	      m_pABox->incrementBranch();

	      setLastClash(pClashDepends);
	    }
	  else
	    return;
	}
      else
	return;
    }

  pDS = getCombinedClash();
  if( !PARAMS_USE_INCREMENTAL_DELETION() )
    pDS->remove(m_iBranch);

  m_pABox->setClash(Clash::unexplained(m_pNode, pDS));
}

Branch* LiteralValueBranch::copyTo(ABox* pABox)
{
  Literal* pX = m_pABox->getLiteral(m_pNode->m_pName);
  LiteralValueBranch *pLVB = new LiteralValueBranch(m_pABox, NULL, pX, m_pDatatype);
  
  pLVB->m_iShuffle = m_iShuffle;
  pLVB->m_iAnonCount = m_iAnonCount;
  pLVB->m_iNodeCount = m_iNodeCount;
  pLVB->m_iBranch = m_iBranch;
  pLVB->m_pNodeName = m_pNode->m_pName;
  pLVB->m_pCompletionStrategy = m_pCompletionStrategy;
  pLVB->m_iTryNext = m_iTryNext;

  return pLVB;
}

/**
 * Added for to re-open closed branches.
 * This is needed for incremental reasoning through deletions
 */
void LiteralValueBranch::shiftTryNext(int iIndex)
{
  m_iTryNext--;
}
