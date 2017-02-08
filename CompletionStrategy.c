#include "CompletionStrategy.h"
#include "ABox.h"
#include "Node.h"
#include "Individual.h"
#include "Literal.h"
#include "Edge.h"
#include "DependencySet.h"
#include "KnowledgeBase.h"
#include "DependencyIndex.h"
#include "Params.h"
#include "ReazienerUtils.h"
#include "Role.h"
#include "ExpressionNode.h"
#include "QueueElement.h"
#include "CompletionQueue.h"
#include "RBox.h"
#include "TBox.h"
#include "Node.h"
#include "Clash.h"
#include "Datatype.h"
#include <limits.h>

extern KnowledgeBase* g_pKB;
extern ExprNode* EXPRNODE_TOP;
extern DependencySet DEPENDENCYSET_EMPTY;
extern DependencySet DEPENDENCYSET_INDEPENDENT;
extern int g_iCommentIndent;

CompletionStrategy::CompletionStrategy(ABox* pABox, Blocking* pBlocking)
{
  m_pABox = pABox;
  m_pBlocking = pBlocking;
  m_bMerging = FALSE;
}

void CompletionStrategy::initialize()
{
  START_DECOMMENT2("CompletionStrategy::initialize");
  DECOMMENT1("Branch size = %d\n", m_pABox->m_aBranches.size());

  for(BranchList::iterator i = m_pABox->m_aBranches.begin(); i != m_pABox->m_aBranches.end(); i++ )
    {
      Branch* pBranch = (Branch*)*i;
      pBranch->m_pCompletionStrategy = this;
    }

  if( m_pABox->isInitialized() )
    {
      bool bFirst = TRUE;
      
      for(ExprNodes::iterator i = m_pABox->m_aNodeList.begin(); i != m_pABox->m_aNodeList.end(); i++ )
	{
	  ExprNode* pExprNode = (ExprNode*)*i;

	  Expr2NodeMap::iterator iFind = m_pABox->m_mNodes.find(pExprNode);
	  if( iFind == m_pABox->m_mNodes.end() )
	    continue;
	  Node* pNode = (Node*)iFind->second;
	  
	  if( !pNode->isIndividual() )
	    continue;
	  Individual* pN = (Individual*)pNode;
	  
	  if( pN->isMerged() )
	    continue;

	  if( bFirst )
	    {
	      applyUniversalRestrictions(pN);
	      bFirst = FALSE;
	    }

	  applyAllValues(pN);
	  if( pN->isMerged() )
	    continue;
	  applyNominalRule(pN);
	  if( pN->isMerged() )
	    continue;
	  applySelfRule(pN);

	  //CHW-added for inc. queue must see if this is bad
	  for(EdgeVector::iterator j = pN->m_listOutEdges.m_listEdge.begin(); j != pN->m_listOutEdges.m_listEdge.end(); j++ )
	    {
	      Edge* pEdge = (Edge*)*j;
	      if( pEdge->m_pTo->isPruned() )
		continue;
	      applyPropertyRestrictions(pEdge);
	      if( pN->isMerged() )
		break;
	    }
	}
      END_DECOMMENT("CompletionStrategy::initialize");
      return;
    }

  m_pABox->m_iCurrentBranchIndex = 0;
  DECOMMENT("ABox.Branch set to 0");

  for(NodeMerges::iterator i = m_pABox->m_listToBeMerged.begin(); i != m_pABox->m_listToBeMerged.end(); i++ )
    m_aMergeList.push_back((NodeMerge*)*i);

  if( m_aMergeList.size() > 0 )
    mergeFirst();


  for(ExprNodes::iterator i = m_pABox->m_aNodeList.begin(); i != m_pABox->m_aNodeList.end(); i++ )
    {
      ExprNode* pExprNode = (ExprNode*)*i;
      printExprNodeWComment("Node=", pExprNode);

      Expr2NodeMap::iterator iFind = m_pABox->m_mNodes.find(pExprNode);
      if( iFind == m_pABox->m_mNodes.end() )
	continue;
      Node* pNode = (Node*)iFind->second;
      if( !pNode->isIndividual() )
	continue;
      Individual* pN = (Individual*)pNode;
      
      if( pN->isMerged() )
	continue;
      
      pN->setChanged(TRUE);
      applyUniversalRestrictions(pN);
      applyUnfoldingRule(pN);
      applySelfRule(pN);
      
      for(EdgeVector::iterator j = pN->m_listOutEdges.m_listEdge.begin(); j != pN->m_listOutEdges.m_listEdge.end(); j++ )
	{
	  Edge* pEdge = (Edge*)*j;
	  if( pEdge->m_pTo->isPruned() )
	    continue;
	  applyPropertyRestrictions(pEdge);
	  if( pN->isMerged() )
	    break;
	}
    }
  
  if( m_aMergeList.size() > 0 )
    mergeFirst();

  m_pABox->m_iCurrentBranchIndex = m_pABox->m_aBranches.size()+1;
  m_pABox->m_iTreeDepth = 1;
  m_pABox->m_bChanged = TRUE;
  m_pABox->m_bIsComplete = FALSE;
  m_pABox->m_bInitialized = TRUE;

  END_DECOMMENT("CompletionStrategy::initialize");
}

void CompletionStrategy::restore(Branch* pBranch)
{
  START_DECOMMENT2("CompletionStrategy::restore");
  m_pABox->setBranch(pBranch->m_iBranch);
  m_pABox->setClash(NULL);
  m_pABox->m_iAnonCount = pBranch->m_iAnonCount;
  m_pABox->m_bRulesNotApplied = TRUE;
  m_aMergeList.clear();

  if( PARAMS_USE_COMPLETION_QUEUE() )
    {
      ExprNodeSet setEffected;
      m_pABox->m_pCompletionQueue->removeEffect(pBranch->m_iBranch, &setEffected);
      
      //First remove all nodes from the node map that were added after this branch
      for(ExprNodes::iterator i = m_pABox->m_aNodeList.begin(); i != m_pABox->m_aNodeList.end(); i++ )
	{
	  ExprNode* pNext = (ExprNode*)*i;
	  m_pABox->m_mNodes.erase(pNext);
	  setEffected.erase(pNext);
	}

      //Next restore remaining nodes that were effected during the branch after the one we are restoring too.
      //Currenlty, I'm tracking the effected nodes through the application of the completion rules.
      for(ExprNodeSet::iterator i = setEffected.begin(); i != setEffected.end(); i++ )
	{
	  ExprNode* pX = (ExprNode*)*i;
	  Node* pNode = m_pABox->getNode(pX);
	  pNode->restore(pBranch->m_iBranch);
	}
    }
  else
    {
      //Below is the old code
      int iIndex = 0;
      int iSize = m_pABox->m_aNodeList.size();
      for(ExprNodes::iterator i = m_pABox->m_aNodeList.begin(); i != m_pABox->m_aNodeList.end(); i++, iIndex++ )
	{
	  ExprNode* pX = (ExprNode*)*i;
	  Node* pNode = m_pABox->getNode(pX);
	  if( iIndex >= pBranch->m_iNodeCount )
	    m_pABox->m_mNodes.erase(pX);
	  else
	    pNode->restore(pBranch->m_iBranch);
	}
    }

  //clear the nodelist as well
  int iSize = m_pABox->m_aNodeList.size();
  for(int i = pBranch->m_iNodeCount; i < iSize; i++ )
    m_pABox->m_aNodeList.pop_back();

  if( PARAMS_USE_COMPLETION_QUEUE() )
    {
      //reset the queues
      m_pABox->m_pCompletionQueue->restore(pBranch->m_iBranch);
      
      //This is new code that only fires all values for inds that are on the queue
      m_pABox->m_pCompletionQueue->init(CompletionQueue::ALLLIST);
      while( m_pABox->m_pCompletionQueue->hasNext(CompletionQueue::ALLLIST) )
	{
	  QueueElement* pNext = (QueueElement*)m_pABox->m_pCompletionQueue->getNext(CompletionQueue::ALLLIST);
	  applyAllValues(pNext);
	  if( m_pABox->isClosed() )
	    break;
	}
    }
  else
    {
      //Below is the old code for firing the all values rule
      for(Expr2NodeMap::iterator i = m_pABox->m_mNodes.begin(); i != m_pABox->m_mNodes.end(); i++ )
	{
	  Node* pN = (Node*)i->second;
	  if( !pN->isIndividual() ) continue;
	  Individual* pNode = (Individual*)pN;
	  applyAllValues(pNode);
	}
    }

  if( !m_pABox->isClosed() )
    m_pABox->validate();
  END_DECOMMENT("CompletionStrategy::restore");
}

void CompletionStrategy::applyUniversalRestrictions(Individual* pNode)
{
  START_DECOMMENT2("CompletionStrategy::applyUniversalRestrictions");
  printExprNodeWComment("pNode=", pNode->m_pName);

  addType(pNode, EXPRNODE_TOP, &DEPENDENCYSET_INDEPENDENT);

  Expr2ExprSetPairList* pUC = g_pKB->m_pTBox->getUC();
  if( pUC )
    {
      for(Expr2ExprSetPairList::iterator i = pUC->begin(); i != pUC->end(); i++ )
	{
	  Expr2ExprSetPair* pPair = (Expr2ExprSetPair*)*i;
	  ExprNode* pC = (ExprNode*)pPair->first;
	  ExprNodeSet* pExplain = (ExprNodeSet*)pPair->second;

	  DependencySet* pDS = new DependencySet(pExplain);
	  addType(pNode, pC, pDS);
	}
    }

  for(RoleSet::iterator r = g_pKB->m_pRBox->m_setReflexiveRoles.begin(); r != g_pKB->m_pRBox->m_setReflexiveRoles.end(); r++)
    {
      Role* pRole = (Role*)*r;
      addEdge(pNode, pRole, pNode, pRole->m_pExplainReflexive);
    }

  END_DECOMMENT("CompletionStrategy::applyUniversalRestrictions");
}

void CompletionStrategy::applyUnfoldingRuleOnIndividuals()
{
  START_DECOMMENT2("CompletionStrategy::applyUnfoldingRuleOnIndividuals");
  for(Expr2NodeMap::iterator i = m_pABox->m_mNodes.begin(); i != m_pABox->m_mNodes.end(); i++ )
    {
      Node* pN = (Node*)i->second;
      if( !pN->isIndividual() ) continue;
      Individual* pNode = (Individual*)pN;
      applyUnfoldingRule(pNode);
      if( m_pABox->isClosed() )
	break;
    }
  END_DECOMMENT("CompletionStrategy::applyUnfoldingRuleOnIndividuals");
}

void CompletionStrategy::applyUnfoldingRule(Individual* pNode)
{
  START_DECOMMENT2("CompletionStrategy::applyUnfoldingRule(2)");
  printExprNodeWComment("pNode=", pNode->m_pName);

  if( !pNode->canApply(Node::ATOM) || m_pBlocking->isBlocked(pNode) )
    {
      END_DECOMMENT("CompletionStrategy::applyUnfoldingRule");
      return;
    }

  int iSize =  pNode->m_aTypes[Node::ATOM].size();
  int iStartAt = pNode->m_iApplyNext[Node::ATOM], iIndex = 0;
  for(ExprNodes::iterator i = pNode->m_aTypes[Node::ATOM].begin(); i != pNode->m_aTypes[Node::ATOM].end() && iIndex < iSize ; i++, iIndex++ )
    {
      ExprNode* pC = (ExprNode*)*i;
      if( iIndex < iStartAt ) continue;
      
      if( !PARAMS_MAINTAIN_COMPLETION_QUEUE() && pNode->getDepends(pC) == NULL )
	continue;

      applyUnfoldingRule(pNode, pC);
      if( m_pABox->isClosed() )
	{
	  END_DECOMMENT("CompletionStrategy::applyUnfoldingRule");
	  return;
	}

      // it is possible that unfolding added new atomic 
      // concepts that we need to further unfold
       iSize = pNode->m_aTypes[Node::ATOM].size();  
    }

  pNode->m_iApplyNext[Node::ATOM] = iSize;
  END_DECOMMENT("CompletionStrategy::applyUnfoldingRule");
}

void CompletionStrategy::applyUnfoldingRule(Individual* pNode, ExprNode* pC)
{
  START_DECOMMENT2("CompletionStrategy::applyUnfoldingRule");
  printExprNodeWComment("pNode=", pNode->m_pName);
  printExprNodeWComment("pC=", pC);

  Expr2ExprSetPairList* pUnfoldingList = g_pKB->m_pTBox->unfold(pC);
  if( pUnfoldingList )
    {
      DependencySet* pDS = pNode->getDepends(pC);

      if( !PARAMS_MAINTAIN_COMPLETION_QUEUE() && pDS == NULL )
	{
	  END_DECOMMENT("CompletionStrategy::applyUnfoldingRule");
	  return;
	}

      for(Expr2ExprSetPairList::iterator i = pUnfoldingList->begin(); i != pUnfoldingList->end(); i++ )
	{
	  Expr2ExprSetPair* pPair = (Expr2ExprSetPair*)*i;
	  ExprNode* pUnfoldedConcept = (ExprNode*)pPair->first;
	  ExprNodeSet* pUnfoldingDS = (ExprNodeSet*)pPair->second;
	  DependencySet* pFinalDS = pDS->addExplain(pUnfoldingDS, m_pABox->m_bDoExplanation);

	  printExprNodeWComment("Unfolded Concept=", pUnfoldedConcept);
	  addType(pNode, pUnfoldedConcept, pFinalDS);
	}
    }
  END_DECOMMENT("CompletionStrategy::applyUnfoldingRule");
}

void CompletionStrategy::applyUnfoldingRule(QueueElement* pElement)
{
  Individual* pNode = m_pABox->getIndividual(pElement->m_pNode);
  pNode = (Individual*)pNode->getSame();
  if( pNode->isPruned() )
    return;

  if( m_pBlocking->isBlocked(pNode) )
    {
      //readd this to the queue in case the node is unblocked at a later point
      //TODO: this will impose a memory overhead, so alternative techniques should be investigated
      m_pABox->m_pCompletionQueue->add(pElement, CompletionQueue::ATOMLIST);
      return;
    }

  ExprNode* pC = pElement->m_pLabel;
  applyUnfoldingRule(pNode, pC);
}

void CompletionStrategy::applyLiteralRule(QueueElement* pElement)
{
  Node* pNode = m_pABox->getNode(pElement->m_pNode);
  if( pNode->isIndividual() )
    return;

  //get the actual literal
  Literal* pLiteral = (Literal*)pNode;
  pLiteral = (Literal*)pLiteral->getSame();
  if( pLiteral->isPruned() )
    return;

  if( pLiteral->m_pValue )
    return;

  LiteralValueBranch* pNewBranch = new LiteralValueBranch(m_pABox,this, pLiteral, pLiteral->m_pDatatype);
  addBranch(pNewBranch);
  pNewBranch->tryNext();
  
}

void CompletionStrategy::applyLiteralRuleOnIndividuals()
{
  START_DECOMMENT2("CompletionStrategy::applyLiteralRuleOnIndividuals");
  for(Expr2NodeMap::iterator i = m_pABox->m_mNodes.begin(); i != m_pABox->m_mNodes.end(); i++ )
    {
      Node* pN = (Node*)i->second;
      if( pN->isIndividual() ) continue;
      Literal* pLit = (Literal*)pN;
      
      if( pLit->m_pValue )
	continue;

      LiteralValueBranch* pNewBranch = new LiteralValueBranch(m_pABox,this, pLit, pLit->m_pDatatype);
      addBranch(pNewBranch);
      pNewBranch->tryNext();

      if( m_pABox->isClosed() )
	break;
    }
  END_DECOMMENT("CompletionStrategy::applyLiteralRuleOnIndividuals");
}

void CompletionStrategy::checkDatatypeCount(QueueElement* pElement)
{
  Individual* pX = (Individual*)m_pABox->getNode(pElement->m_pNode);
  pX = (Individual*)pX->getSame();

  if( pX->isPruned() || (!pX->isChanged(Node::ALL) && !pX->isChanged(Node::MIN)) )
    return;
  checkDatatypeCount(pX);
}

void CompletionStrategy::checkDatatypeCountOnIndividuals()
{
  START_DECOMMENT2("CompletionStrategy::checkDatatypeCountOnIndividuals");
  for(Expr2NodeMap::iterator i = m_pABox->m_mNodes.begin(); i != m_pABox->m_mNodes.end(); i++ )
    {
      Node* pN = (Node*)i->second;
      if( !pN->isIndividual() ) continue;
      Individual* pNode = (Individual*)pN;
      
      checkDatatypeCount(pNode);

      if( m_pABox->isClosed() )
	break;
    }
  END_DECOMMENT("CompletionStrategy::checkDatatypeCountOnIndividuals");
}

void CompletionStrategy::checkDatatypeCount(Individual* pX)
{
  if( !pX->isChanged(Node::ALL) && !pX->isChanged(Node::MIN) )
    return;

  // for DatatypeProperties we have to compute the maximum number of
  // successors it can have on the fly. This is because as we go on
  // with completion there can be more concepts in the form all(dp, X)
  // added to node label. so first for each datatype property collect
  // all the different allValuesFrom axioms together

  
  ExprNode2ExprNodesMap mapAllValues;
  Expr2DependencySetMap mapDepends;

  for(ExprNodes::iterator i = pX->m_aTypes[Node::ALL].begin(); i != pX->m_aTypes[Node::ALL].end(); i++ )
    {
      ExprNode* pAV = (ExprNode*)*i;
      ExprNode* pR = (ExprNode*)pAV->m_pArgs[0];
      ExprNode* pC = (ExprNode*)pAV->m_pArgs[1];

      Role* pRole = g_pKB->getRole(pR);
      if( !pRole->isDatatypeRole() )
	continue;

      DependencySet* pDS = NULL;
      Expr2DependencySetMap::iterator iFind = mapDepends.find(pR);
      if( iFind != mapDepends.end() )
	pDS = (DependencySet*)iFind->second;

      if( !PARAMS_MAINTAIN_COMPLETION_QUEUE() && pX->getDepends(pAV) == NULL )
	continue;

      ExprNodes* pRanges = NULL;
      ExprNode2ExprNodesMap::iterator iFind2 = mapAllValues.find(pR);
      if( iFind2 != mapAllValues.end() )
	pRanges = (ExprNodes*)iFind2->second;
      else
	{
	  pRanges = new ExprNodes;
	  mapAllValues[pR] = pRanges;
	  pDS = &DEPENDENCYSET_EMPTY;
	}

      if( pC->m_iExpression == EXPR_AND )
	{
	  for(int j = 0; j < ((ExprNodeList*)pC->m_pArgList)->m_iUsedSize; j++ )
	    pRanges->push_back(((ExprNodeList*)pC->m_pArgList)->m_pExprNodes[j]);
	}
      else
	pRanges->push_back(pC);

      pDS = pDS->unionNew(pX->getDepends(pAV), m_pABox->m_bDoExplanation);
      mapDepends[pR] = pDS;
    }

  for(ExprNodes::iterator i = pX->m_aTypes[Node::MIN].begin(); i != pX->m_aTypes[Node::MIN].end(); i++ )
    {
      // mc stores the current type (the current minCard restriction)
      ExprNode* pMC = (ExprNode*)*i;
      ExprNode* pR = (ExprNode*)pMC->m_pArgs[0];
      Role* pRole = g_pKB->getRole(pR);
      ExprNode* pRange = pRole->m_pRange;

      if( !pRole->isDatatypeRole() || pRange == NULL )
	continue;

      if( !PARAMS_MAINTAIN_COMPLETION_QUEUE() && pX->getDepends(pMC) == NULL )
	continue;
      
      ExprNodes* pRanges = NULL;
      ExprNode2ExprNodesMap::iterator iFind2 = mapAllValues.find(pR);
      if( iFind2 != mapAllValues.end() )
	pRanges = (ExprNodes*)iFind2->second;
      else
	{
	  pRanges = new ExprNodes;
	  mapAllValues[pR] = pRanges;
	  mapDepends[pR] = &DEPENDENCYSET_INDEPENDENT;
	}

      pRanges->push_back(pRange);
    }

  for(ExprNode2ExprNodesMap::iterator i = mapAllValues.begin(); i != mapAllValues.end(); i++ )
    {
      ExprNode* pR = (ExprNode*)i->first;
      Role* pRole = g_pKB->getRole(pR);
      ExprNodes* pRanges = (ExprNodes*)i->second;
      
#if 0
      int n = abox.getDatatypeReasoner().intersection( dt ).size();
      if( n == ValueSpace.INFINITE || n == Integer.MAX_VALUE )
	continue;

      boolean clash = x.checkMaxClash( ATermUtils.makeNormalizedMax( r, n, ATermUtils.TOP_LIT ), DependencySet.INDEPENDENT );
      if( clash )
	return;
      
      DependencySet dsEdges = x.hasDistinctRNeighborsForMax( role, n + 1, ATermUtils.TOP_LIT );
      
      if( dsEdges != null ) {
	DependencySet ds = (DependencySet) depends.get( r );
	ds = ds.union( dsEdges, abox.doExplanation() );
	abox.setClash( Clash.unexplained( x, ds ) );
	return;
      }

#endif 
    }
}

void CompletionStrategy::applySelfRule(Individual* pNode)
{
  START_DECOMMENT2("CompletionStrategy::applySelfRule");
  printExprNodeWComment("Node=", pNode->m_pName);

  for(ExprNodes::iterator i = pNode->m_aTypes[Node::ATOM].begin(); i != pNode->m_aTypes[Node::ATOM].end(); i++ )
    {
      ExprNode* pC = (ExprNode*)*i;
      if( !PARAMS_MAINTAIN_COMPLETION_QUEUE() && pNode->getDepends(pC) == NULL )
	continue;

      if( pC->m_iExpression == EXPR_SELF )
	{
	  ExprNode* pPred = (ExprNode*)pC->m_pArgs[0];
	  Role* pRole = g_pKB->getRole(pPred);

	  addEdge(pNode, pRole, pNode, pNode->getDepends(pC));

	  if( m_pABox->isClosed() )
	    {
	      END_DECOMMENT("CompletionStrategy::applySelfRule");
	      return;
	    }
	}
    }
  END_DECOMMENT("CompletionStrategy::applySelfRule");
}

/**
 * Iterate through all the allValuesFrom restrictions on this individual and apply the
 * restriction
 */
void CompletionStrategy::applyAllValues(Individual* pNode)
{
  START_DECOMMENT2("CompletionStrategy::applyAllValues");
  printExprNodeWComment("Node=", pNode->m_pName);

  for(ExprNodes::iterator i = pNode->m_aTypes[Node::ALL].begin(); i != pNode->m_aTypes[Node::ALL].end(); i++ )
    {
      ExprNode* pAV = (ExprNode*)*i;
      DependencySet* pAVDepends = pNode->getDepends(pAV);
      
      if( !PARAMS_MAINTAIN_COMPLETION_QUEUE() && pNode->getDepends(pAV) == NULL )
	continue;

      applyAllValues(pNode, pAV, pAVDepends);
      
      if( pNode->isMerged() || m_pABox->isClosed() )
	{
	  END_DECOMMENT("CompletionStrategy::applyAllValues");
	  return;
	}

      // if there are self links through transitive properties restart
      if( pNode->isChanged(Node::ALL) )
	{
	  i = pNode->m_aTypes[Node::ALL].begin();
	  pNode->setChanged(Node::ALL, FALSE);
	}
    }
  END_DECOMMENT("CompletionStrategy::applyAllValues");
}

void CompletionStrategy::applyAllValues(QueueElement* pElement)
{
  Individual* pX = m_pABox->getIndividual(pElement->m_pNode);
  
  if( pX->isPruned() || pX->isMerged() )
    pX = (Individual*)pX->getSame();
  
  if( pX->isPruned() )
    return;

  applyAllValues(pX);
}

/**
 * Apply the allValues rule for the given type with the given dependency. The concept is in the
 * form all(r,C) and this function adds C to all r-neighbors of x
 */
void CompletionStrategy::applyAllValues(Individual* pX, ExprNode* pAV, DependencySet* pDS)
{
  START_DECOMMENT2("CompletionStrategy::applyAllValues(3)");
  printExprNodeWComment("Node.X=", pX->m_pName);
  printExprNodeWComment("AV=", pAV);

  if( pAV->m_iArity == 0 )
    assertFALSE("InternalReasonerException");

  ExprNode* pP = (ExprNode*)pAV->m_pArgs[0];
  ExprNode* pC = (ExprNode*)pAV->m_pArgs[1];
  ExprNodeList* pRoleChain = NULL;
  Role* pS = NULL;

  if( pP == NULL && pAV->m_pArgList )
    {
      pRoleChain = createExprNodeList((ExprNodeList*)pAV->m_pArgList);
      pS = g_pKB->getRole(pRoleChain->m_pExprNodes[0]);
      pop_front(pRoleChain);
    }
  else
    pS = g_pKB->getRole(pP);

  EdgeList edges;
  pX->getRNeighborEdges(pS, &edges);
  DECOMMENT1("RNeighbor edge count=%d", edges.size());
  for(EdgeVector::iterator i = edges.m_listEdge.begin();  i != edges.m_listEdge.end(); i++ )
    {
      Edge* pEdgeToY = (Edge*)*i;
      Node* pY = pEdgeToY->getNeighbor(pX);
      printExprNodeWComment("Edge2Y=", pY->m_pName);
      DependencySet* pFinalDS = pDS->unionNew(pEdgeToY->m_pDepends, m_pABox->m_bDoExplanation);
      
      if( pRoleChain == NULL || pRoleChain->m_iUsedSize == 0 )
	{
	  applyAllValues(pX, pS, pY, pC, pFinalDS);
	}
      else
	{
	  ExprNode* pAllRC = createExprNode(EXPR_ALL, pRoleChain, pC);
	  applyAllValues((Individual*)pY, pAllRC, pFinalDS);
	}

      if( pX->isMerged() || m_pABox->isClosed() )
	{
	  END_DECOMMENT("CompletionStrategy::applyAllValues(3)");
	  return;
	}
    }

  if( !pS->isSimple() )
    {
      for(ExprNodeListSet::iterator i = pS->m_setSubRoleChains.begin(); i != pS->m_setSubRoleChains.end(); i++ )
	{
	  ExprNodeList* pChain = createExprNodeList((ExprNodeList*)*i);
	  ExprNode* pListExpr = createExprNode(EXPR_LIST, pChain);
	  DependencySet* pSubChainDS = pS->getExplainSub(pListExpr);
	  Role* pRole = g_pKB->getRole( pChain->m_pExprNodes[0] );

	  EdgeList edges;
	  pX->getRNeighborEdges(pRole, &edges);
	  if( edges.m_listEdge.size() > 0 )
	    {
	      pop_front(pChain);
	      ExprNode* pAllRC = createExprNode(EXPR_ALL, createExprNodeList(pChain), pC);
	      
	      for(EdgeVector::iterator j = edges.m_listEdge.begin();  j != edges.m_listEdge.end(); j++ )
		{
		  Edge* pEdgeToY = (Edge*)*j;
		  Node* pY = pEdgeToY->getNeighbor(pX);
		  DependencySet* pFinalDS = pDS->unionNew(pEdgeToY->m_pDepends, m_pABox->m_bDoExplanation)->unionDS(pSubChainDS, m_pABox->m_bDoExplanation);
		  
		  applyAllValues(pX, pRole, pY, pAllRC, pFinalDS);

		  if( pX->isMerged() || m_pABox->isClosed() )
		    {
		      END_DECOMMENT("CompletionStrategy::applyAllValues(3)");
		      return;
		    }
		}
	    }
	}
    }
  END_DECOMMENT("CompletionStrategy::applyAllValues(3)");
}

void CompletionStrategy::applyAllValues(Individual* pSubj, Role* pPred, Node* pObj, ExprNode* pC, DependencySet* pDS)
{
  START_DECOMMENT2("CompletionStrategy::applyAllValues(2)");
  printExprNodeWComment("Subj=", pSubj->m_pName);
  printExprNodeWComment("Obj=", pObj->m_pName);
  printExprNodeWComment("C=", pC);

  if( !pObj->hasType(pC) )
    addType(pObj, pC, pDS);

  END_DECOMMENT("CompletionStrategy::applyAllValues(2)");
}

void CompletionStrategy::applyAllValues(Individual* pSubj, Role* pPred, Node* pObj, DependencySet* pDS)
{
  int iAllValuesSize = pSubj->m_aTypes[Node::ALL].size();
  for(ExprNodes::iterator i = pSubj->m_aTypes[Node::ALL].begin(); i != pSubj->m_aTypes[Node::ALL].end(); i++ )
    {
      ExprNode* pAV = (ExprNode*)*i;
      ExprNode* pP = (ExprNode*)pAV->m_pArgs[0];
      ExprNode* pC = (ExprNode*)pAV->m_pArgs[1];
      ExprNodeList* pRoleChain = NULL;
      Role* pS = NULL;
      
      if( pP == NULL && pAV->m_pArgList )
	{
	  pRoleChain = createExprNodeList((ExprNodeList*)pAV->m_pArgList);
	  pS = g_pKB->getRole(pRoleChain->m_pExprNodes[0]);
	  pop_front(pRoleChain);
	}
      else
	pS = g_pKB->getRole(pP);
      
      if( pPred->isSubRoleOf(pS) )
	{
	  DependencySet* pFinalDS = pSubj->getDepends(pAV)->unionNew(pDS, m_pABox->m_bDoExplanation)->unionDS(pS->getExplainSub(pPred->m_pName), m_pABox->m_bDoExplanation);
	  if( pRoleChain == NULL || pRoleChain->m_iUsedSize == 0 )
	    applyAllValues(pSubj, pS, pObj, pC, pFinalDS);
	  else
	    {
	      ExprNode* pAllRC = createExprNode(EXPR_ALL, pRoleChain, pC);
	      applyAllValues((Individual*)pObj, pAllRC, pFinalDS);
	    }

	  if( m_pABox->isClosed() )
	    return;
	}

      if( !pS->isSimple() )
	{
	  DependencySet* pFinalDS = pSubj->getDepends(pAV)->unionNew(pDS, m_pABox->m_bDoExplanation);
	  
	  for(ExprNodeListSet::iterator j = pS->m_setSubRoleChains.begin(); j != pS->m_setSubRoleChains.end(); j++ )
	    {
	      ExprNodeList* pChain = (ExprNodeList*)*j;
	      ExprNode* pListExpr = createExprNode(EXPR_LIST, pChain);
	      Role* pFirstRole = g_pKB->getRole(pChain->m_pExprNodes[0]);
	      if( !pPred->isSubRoleOf(pFirstRole) )
		continue;

	      ExprNodeList* pNewChain = createExprNodeList(pChain);
	      pop_front(pNewChain);
	      ExprNode* pAllRC = createExprNode(EXPR_ALL, pNewChain, pC);
	      applyAllValues(pSubj, pPred, pObj, pAllRC, pFinalDS->unionDS(pFirstRole->getExplainSub(pPred->m_pName), m_pABox->m_bDoExplanation)->unionDS(pS->getExplainSub(pListExpr), m_pABox->m_bDoExplanation));
	      
	      if( pSubj->isMerged() || m_pABox->isClosed() )
		return;
	    }
	}

      if( pSubj->isMerged() )
	return;

      pObj = pObj->getSame();
      // if there are self links then restart
      if( iAllValuesSize != pSubj->m_aTypes[Node::ALL].size() )
	{
	  i = pSubj->m_aTypes[Node::ALL].begin();
	  iAllValuesSize = pSubj->m_aTypes[Node::ALL].size();
	}
    }
}

void CompletionStrategy::applyNominalRule(Individual* pNode)
{
  if( PARAMS_USE_PSEUDO_NOMINALS() )
    return;

  for(ExprNodes::iterator i = pNode->m_aTypes[Node::NOM].begin(); i != pNode->m_aTypes[Node::NOM].end(); i++ )
    {
      ExprNode* pNC = (ExprNode*)*i;
      DependencySet* pDS = pNode->getDepends(pNC);
      
      if( !PARAMS_MAINTAIN_COMPLETION_QUEUE() && pDS == NULL )
	continue;
      
      applyNominalRule(pNode, pNC, pDS);

      if( m_pABox->isClosed() || pNode->isMerged() )
	return;
    }
}

void CompletionStrategy::applyNominalRuleOnIndividuals()
{
  START_DECOMMENT2("CompletionStrategy::applyNominalRuleOnIndividuals");
  for(Expr2NodeMap::iterator i = m_pABox->m_mNodes.begin(); i != m_pABox->m_mNodes.end(); i++ )
    {
      Node* pN = (Node*)i->second;
      if( !pN->isIndividual() ) continue;
      Individual* pNode = (Individual*)pN;

      if( !pNode->canApply(Node::NOM) || m_pBlocking->isBlocked(pNode) )
	continue;
      
      applyNominalRule(pNode);
      pNode->setChanged(Node::NOM, FALSE);
      
      if( m_pABox->isClosed() )
	break;
      
      if( pNode->isMerged() )
	applyNominalRule((Individual*)pNode->getSame());
    }
  END_DECOMMENT("CompletionStrategy::applyNominalRuleOnIndividuals");
}

void CompletionStrategy::applyNominalRule(QueueElement* pElement)
{
  if( PARAMS_USE_PSEUDO_NOMINALS() )
    return;

  //get individual from the queue element - need to chase it down on the fly, 
  //and return if its pruned
  Individual* pY = m_pABox->getIndividual(pElement->m_pNode);
  pY = (Individual*)pY->getSame();

  if( m_pBlocking->isBlocked(pY) )
    {
      //readd this to the queue in case the node is unblocked at a later point
      //TODO: this will impose a memory overhead, so alternative techniques should be investigated
      m_pABox->m_pCompletionQueue->add(pElement, CompletionQueue::NOMLIST);
      return;
    }

  if( pY->isPruned() )
    return;

  ExprNode* pNC = pElement->m_pLabel;
  DependencySet* pDS = pY->getDepends(pNC);

  if( !PARAMS_MAINTAIN_COMPLETION_QUEUE() && pDS == NULL )
    return;

  applyNominalRule(pY, pNC, pDS);
}

void CompletionStrategy::applyNominalRule(Individual* pY, ExprNode* pNC, DependencySet* pDS)
{
  START_DECOMMENT2("CompletionStrategy::applyNominalRule");
  printExprNodeWComment("NC=", pNC);

  m_pABox->copyOnWrite();

  ExprNode* pNominal = (ExprNode*)pNC->m_pArgs[0];
  // first find the individual for the given nominal
  Individual* pZ = m_pABox->getIndividual(pNominal);
  if( pZ == NULL )
    {
      if( isAnonNominal(pNominal) )
	pZ = m_pABox->addIndividual(pNominal);
      else
	assertFALSE("Nominal 'pNominal' not found in KB!");
    }
 
  // Get the value of mergedTo because of the following possibility:
  // Suppose there are three individuals like this
  // [x,{}],[y,{value(x)}],[z,{value(y)}]
  // After we merge x to y, the individual x is now represented by
  // the node y. It is too hard to update all the references of
  // value(x) so here we find the actual representative node
  // by calling getSame()
  if( pZ->isMerged() )
    {
      pDS = pDS->unionNew(pZ->getMergeDependency(TRUE), m_pABox->m_bDoExplanation);
      pZ = (Individual*)pZ->getSame();

      if( m_pABox->m_iCurrentBranchIndex > 0 && PARAMS_USE_COMPLETION_QUEUE() )
	m_pABox->addEffected(m_pABox->m_iCurrentBranchIndex, pZ->m_pName);
    }

  if( pY->isSame(pZ) )
    {
      END_DECOMMENT("CompletionStrategy::applyNominalRule(1)");
      return;
    }

  if( pY->isDifferent(pZ) )
    {
      pDS = pDS->unionNew(pY->getDifferenceDependency(pZ), m_pABox->m_bDoExplanation);
      if( m_pABox->m_bDoExplanation )
	m_pABox->setClash(Clash::nominal(pY, pDS, pZ->m_pName));
      else
	m_pABox->setClash(Clash::nominal(pY, pDS));
      END_DECOMMENT("CompletionStrategy::applyNominalRule(2)");
      return;
    }

  mergeTo(pY, pZ, pDS);
  END_DECOMMENT("CompletionStrategy::applyNominalRule(3)");
}

void CompletionStrategy::applyGuessingRuleOnIndividuals()
{
  START_DECOMMENT2("CompletionStrategy::applyGuessingRuleOnIndividuals");
  // CHECK HERE: CompletionStrategy.c
  for(Expr2NodeMap::iterator i = m_pABox->m_mNodes.begin(); i != m_pABox->m_mNodes.end(); i++ )
    {
      Node* pN = (Node*)i->second;
      if( !pN->isIndividual() ) continue;
      Individual* pNode = (Individual*)pN;

      if( pNode->isBlockable() )
	continue;
      
      for(ExprNodes::iterator j = pNode->m_aTypes[Node::MAX].begin(); j != pNode->m_aTypes[Node::MAX].end(); j++ )
	{
	  ExprNode* pMC = (ExprNode*)*j;
	  applyGuessingRule(pNode, pMC);
	  if( m_pABox->isClosed() )
	    {
	      END_DECOMMENT("CompletionStrategy::applyGuessingRuleOnIndividuals");
	      return;
	    }
	  if( pNode->isPruned() )
	    continue;
	}
    }
  END_DECOMMENT("CompletionStrategy::applyGuessingRuleOnIndividuals");
}

void CompletionStrategy::applyGuessingRule(QueueElement* pElement)
{
  Individual* pX = m_pABox->getIndividual(pElement->m_pNode);
  pX = (Individual*)pX->getSame();

  if( pX->isBlockable() )
    {
      //readd this to the queue in case the node is unblocked at a later point
      //TODO: this will impose a memory overhead, so alternative techniques should be investigated
      m_pABox->m_pCompletionQueue->add(pElement, CompletionQueue::GUESSLIST);
      return;
    }

  if( pX->isPruned() )
    return;
  applyGuessingRule(pX, pElement->m_pLabel);
}

void CompletionStrategy::applyGuessingRule(Individual* pX, ExprNode* pMC)
{
  START_DECOMMENT2("CompletionStrategy::applyGuessingRule");
  printExprNodeWComment("X=", pX->m_pName);
  printExprNodeWComment("MC=", pMC);

  // max(r, n) is in normalized form not(min(p, n + 1))
  ExprNode* pMax = (ExprNode*)pMC->m_pArgs[0];
  Role* pRole = g_pKB->getRole((ExprNode*)pMax->m_pArgs[0]);
  int iN = ((ExprNode*)pMax->m_pArgs[1])->m_iTerm-1;

  // obviously if r is a datatype role then there can be no r-predecessor
  // and we cannot apply the rule
  if( pRole->isDatatypeRole() )
    {
      END_DECOMMENT("CompletionStrategy::applyGuessingRule");
      return;
    }

  // FIXME instead of doing the following check set a flag when the edge is added
  // check that x has to have at least one r neighbor y
  // which is blockable and has successor x
  // (so y is an inv(r) predecessor of x)
  bool bApply = FALSE;
  
  EdgeList edges;
  pX->getRPredecessorEdges(pRole->m_pInverse, &edges);
  for(EdgeVector::iterator i = edges.m_listEdge.begin(); i != edges.m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      Individual* pPred = pEdge->m_pFrom;
      
      if( pPred->isBlockable() )
	{
	  bApply = TRUE;
	  break;
	}
    }

  if( !bApply )
    {
      END_DECOMMENT("CompletionStrategy::applyGuessingRule");
      return;
    }

  if( pX->getMaxCard(pRole) < iN )
    {
      END_DECOMMENT("CompletionStrategy::applyGuessingRule");
      return;
    }

  if( pX->hasDistinctRNeighborsForMin(pRole, iN, EXPRNODE_TOP, TRUE) )
    {
      END_DECOMMENT("CompletionStrategy::applyGuessingRule");
      return;
    }

  // if( n == 1 ) {
  // throw new InternalReasonerException(
  // "Functional rule should have been applied " +
  // x + " " + x.isNominal() + " " + edges);
  // }
  
  int iGuessMin = pX->getMinCard(pRole);
  if( iGuessMin == 0 )
    iGuessMin = 1;

  // TODO not clear what the correct ds is so be pessimistic and include everything
  DependencySet* pDS = pX->getDepends(pMC);
  EdgeList edgesRNeighbors;
  pX->getRNeighborEdges(pRole->m_pInverse, &edgesRNeighbors);
  for(EdgeVector::iterator i = edgesRNeighbors.m_listEdge.begin(); i != edgesRNeighbors.m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      pDS = pDS->unionNew(pEdge->m_pDepends, m_pABox->m_bDoExplanation);
    }

  GuessBranch* pNewBranch = new GuessBranch(m_pABox, this, pX, pRole, iGuessMin, iN, pDS);
  addBranch(pNewBranch);
  pNewBranch->tryNext();
  END_DECOMMENT("CompletionStrategy::applyGuessingRule");
}

void CompletionStrategy::applyChooseRuleOnIndividuals()
{
  START_DECOMMENT2("CompletionStrategy::applyChooseRuleOnIndividuals");
  for(Expr2NodeMap::iterator i = m_pABox->m_mNodes.begin(); i != m_pABox->m_mNodes.end(); i++ )
    {
      Node* pN = (Node*)i->second;
      if( !pN->isIndividual() ) continue;
      Individual* pNode = (Individual*)pN;
      applyChooseRule(pNode);
      if( m_pABox->isClosed() )
	break;
    }
  END_DECOMMENT("CompletionStrategy::applyChooseRuleOnIndividuals");
}

void CompletionStrategy::applyChooseRule(QueueElement* pElement)
{
  Individual* pNode = m_pABox->getIndividual(pElement->m_pNode);
  pNode = (Individual*)pNode->getSame();
  if( pNode->isPruned() )
    return;
  applyChooseRule(pNode, pElement->m_pLabel);
}

void CompletionStrategy::applyChooseRule(Individual* pX)
{
  if( !pX->canApply(Node::MAX) || m_pBlocking->isIndirectlyBlocked(pX) )
    return;

  for(ExprNodes::iterator i = pX->m_aTypes[Node::MAX].begin(); i != pX->m_aTypes[Node::MAX].end(); i++ )
    {
      ExprNode* pMaxCard = (ExprNode*)*i;
      applyChooseRule(pX, pMaxCard);
    }
}

void CompletionStrategy::applyChooseRule(Individual* pX, ExprNode* pMaxCard)
{
  START_DECOMMENT2("CompletionStrategy::applyChooseRule");
  printExprNodeWComment("Node=", pX->m_pName);
  printExprNodeWComment("pMaxCard=", pMaxCard);

  // max(r, n, c) is in normalized form not(min(p, n + 1, c))
  ExprNode* pMax = (ExprNode*)pMaxCard->m_pArgs[0];
  Role* pRole = g_pKB->getRole((ExprNode*)pMax->m_pArgs[0]);
  ExprNode* pC = (ExprNode*)pMax->m_pArgs[2];
  ExprNode* pNotC = negate2(pC);

  if( isTop(pC) )
    {
      END_DECOMMENT("CompletionStrategy::applyChooseRule");
      return;
    }

  if( !PARAMS_MAINTAIN_COMPLETION_QUEUE() && pX->getDepends(pMaxCard) == NULL )
    {
      END_DECOMMENT("CompletionStrategy::applyChooseRule");
      return;
    }

  EdgeList edges;
  pX->getRNeighborEdges(pRole, &edges);
  for(EdgeVector::iterator i = edges.m_listEdge.begin(); i != edges.m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      Node* pNeighbor = (Node*)pEdge->getNeighbor(pX);
      
      if( !pNeighbor->hasType(pC) && !pNeighbor->hasType(pNotC) )
	{
	  ChooseBranch* pNewBranch = new ChooseBranch(m_pABox, this, pNeighbor, pC, pX->getDepends(pMaxCard));
	  addBranch(pNewBranch);
	  pNewBranch->tryNext();
	  
	  if( m_pABox->isClosed() )
	    break;
	}
    }
  END_DECOMMENT("CompletionStrategy::applyChooseRule");
}

void CompletionStrategy::applySomeValuesRuleOnIndividuals()
{
  START_DECOMMENT2("CompletionStrategy::applySomeValuesRuleOnIndividuals");
  for(Expr2NodeMap::iterator i = m_pABox->m_mNodes.begin(); i != m_pABox->m_mNodes.end(); i++ )
    {
      Node* pN = (Node*)i->second;
      if( !pN->isIndividual() ) continue;
      Individual* pNode = (Individual*)pN;
      applySomeValuesRule(pNode);
      if( m_pABox->isClosed() || pNode->isMerged() )
	break;
    }
  END_DECOMMENT("CompletionStrategy::applySomeValuesRuleOnIndividuals");
}

void CompletionStrategy::applySomeValuesRule(QueueElement* pElement)
{
  Individual* pNode = m_pABox->getIndividual(pElement->m_pNode);
  pNode = (Individual*)pNode->getSame();
  if( m_pBlocking->isBlocked(pNode) )
    {
      m_pABox->m_pCompletionQueue->add(pElement, CompletionQueue::SOMELIST);
      return;
    }

  if( pNode->isPruned() )
    return;
  applySomeValuesRule(pNode, pElement->m_pLabel);
}

void CompletionStrategy::applySomeValuesRule(Individual* pX)
{
  if( !pX->canApply(Node::SOME) || m_pBlocking->isBlocked(pX) )
    return;

  int iSize = pX->m_aTypes[Node::SOME].size();
  int iStartAt = pX->m_iApplyNext[Node::SOME], iIndex = 0;
  for(ExprNodes::iterator i = pX->m_aTypes[Node::SOME].begin(); i != pX->m_aTypes[Node::SOME].end() && iIndex < iSize; i++, iIndex++ )
    {
      ExprNode* pSV = (ExprNode*)*i;
      if( iIndex < iStartAt ) continue;
      applySomeValuesRule(pX, pSV);

      if( m_pABox->isClosed() || pX->isPruned() )
	return;
    }

  pX->m_iApplyNext[Node::SOME] = iSize;
}

void CompletionStrategy::applySomeValuesRule(Individual* pX, ExprNode* pSV)
{
  START_DECOMMENT2("CompletionStrategy::applySomeValuesRule");
  printExprNodeWComment("pX=", pX->m_pName);
  printExprNodeWComment("pSV=", pSV);

  // someValuesFrom is now in the form not(all(p. not(c)))
  ExprNode* pA = (ExprNode*)pSV->m_pArgs[0];
  ExprNode* pS = (ExprNode*)pA->m_pArgs[0];
  ExprNode* pC = (ExprNode*)pA->m_pArgs[1];

  if( !PARAMS_MAINTAIN_COMPLETION_QUEUE() && pX->getDepends(pSV) == NULL )
    {
      END_DECOMMENT("CompletionStrategy::applySomeValuesRule");
      return;
    }

  Role* pRole = g_pKB->getRole(pS);
  pC = negate2(pC);

  // Is there a r-neighbor that satisfies the someValuesFrom restriction
  bool bNeighborFound = FALSE;
  // Safety condition as defined in the SHOIQ algorithm.
  // An R-neighbor y of a node x is safe if
  // (i) x is blockable or if
  // (ii) x is a nominal node and y is not blocked.
  bool bNeighborSafe = pX->isBlockable();
  // y is going to be the node we create, and edge its connection to the
  // current node
  Node* pY = NULL;
  Edge* pEdge = NULL;

  // edges contains all the edges going into of coming out from the node
  // And labeled with the role R
  EdgeList edges;
  pX->getRNeighborEdges(pRole, &edges);
  for(EdgeVector::iterator i = edges.m_listEdge.begin(); i != edges.m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      pY = pEdge->getNeighbor(pX);
      
      if( pY->hasType(pC) )
	{
	  // CHECK HERE: CompletionStrategy.c
	  // OR in if statement must be AND based on given algorithm
	  bNeighborSafe |= (!pY->isIndividual() || !m_pBlocking->isBlocked((Individual*)pY));
	  if( bNeighborSafe )
	    {
	      bNeighborFound = TRUE;
	      break;
	    }
	}
    }

  // If we have found a R-neighbor with type C, continue, do nothing
  if( bNeighborFound )
    {
      END_DECOMMENT("CompletionStrategy::applySomeValuesRule");
      return;
    }

  // If not, we have to create it
  DependencySet* pDS = new DependencySet(pX->getDepends(pSV));
  
  // If the role is a datatype property...
  if( pRole->isDatatypeRole() )
    {
      Literal* pLiteral = (Literal*)pY;
      if( isNominal(pC) && !PARAMS_USE_PSEUDO_NOMINALS() )
	{
	  m_pABox->copyOnWrite();
	  pLiteral = m_pABox->addLiteral((ExprNode*)pC->m_pArgs[0]);
	}
      else
	{
	  if( !pRole->isFunctional() || pLiteral == NULL )
	    pLiteral = m_pABox->addLiteral();
	  pLiteral->addType(pC, pDS);
	}
    }
  // If it is an object property
  else
    {
      if( isNominal(pC) && !PARAMS_USE_PSEUDO_NOMINALS() )
	{
	  m_pABox->copyOnWrite();

	  ExprNode* pValue = (ExprNode*)pC->m_pArgs[0];
	  pY = m_pABox->getIndividual(pValue);

	  if( pY == NULL )
	    {
	      if( isAnonNominal(pValue) )
		pY = m_pABox->addIndividual(pValue);
	      else if( isLiteral(pValue) )
		assertFALSE( "Object Property pRole is used with a hasValue restriction where the value is a literal:");
	      else
		assertFALSE("Nominal 'c' is not found in the KB!");
	    }

	  if( pY->isMerged() )
	    {
	      pDS = pDS->unionNew(pY->getMergeDependency(TRUE), m_pABox->m_bDoExplanation);
	      pY = (Individual*)pY->getSame();
	    }

	  addEdge(pX, pRole, pY, pDS);
	}
      else
	{
	  bool bUseExistingNode = FALSE;
	  bool bUseExistingRole = FALSE;

	  DependencySet* pMaxCardDS = pRole->isFunctional()?(&DEPENDENCYSET_INDEPENDENT):pX->hasMax1(pRole);
	  if( pMaxCardDS )
	    {
	      pDS = pDS->unionNew(pMaxCardDS, m_pABox->m_bDoExplanation);
	      // if there is an r-neighbor and we can have at most one r then
	      // we should reuse that node and edge. there is no way that neighbor
	      // is not safe (a node is unsafe only if it is blockable and has
	      // a nominal successor which is not possible if there is a cardinality
	      // restriction on the property)
	      if( pEdge )
		bUseExistingRole = bUseExistingNode = TRUE;
	      else
		{
		  // this is the tricky part. we need some merges to happen
		  // under following conditions:
		  // 1) if r is functional and there is a p-neighbor where
		  // p is superproperty of r then we need to reuse that
		  // p neighbor for the some values restriction (no
		  // need to check subproperties because functionality of r
		  // precents having two or more successors for subproperties)
		  // 2) if r is not functional, i.e. max(r, 1) is in the types,
		  // then having a p neighbor (where p is subproperty of r)
		  // means we need to reuse that p-neighbor
		  // In either case if there are more than one such value we also
		  // need to merge them together
		  
		  RoleSet::iterator iStart, iEnd;
		  if( pRole->isFunctional() )
		    {
		      iStart = pRole->m_setFunctionalSupers.begin();
		      iEnd = pRole->m_setFunctionalSupers.end();
		    }
		  else
		    {
		      iStart = pRole->m_setSubRoles.begin();
		      iEnd = pRole->m_setSubRoles.end();
		    }

		  for(RoleSet::iterator i = iStart; i != iEnd; i++)
		    {
		      Role* pF = (Role*)*i;
		      EdgeList edges;
		      pX->getRNeighborEdges(pF, &edges);
		      if( edges.size() > 0 )
			{
			  if( bUseExistingNode )
			    {
			      DependencySet* pFDS = &DEPENDENCYSET_INDEPENDENT;
			      if( PARAMS_USE_TRACING() )
				{
				  if( pRole->isFunctional() )
				    pFDS = pRole->getExplainSuper(pF->m_pName);
				  else
				    pFDS = pRole->getExplainSub(pF->m_pName);
				}
			      
			      Edge* pOtherEdge = (Edge*)(*edges.m_listEdge.begin());
			      Node* pOtherNode = pOtherEdge->getNeighbor(pX);

			      DependencySet* pD = pDS->unionNew(pEdge->m_pDepends, m_pABox->m_bDoExplanation);
			      pD->unionDS(pOtherEdge->m_pDepends, m_pABox->m_bDoExplanation);
			      pD->unionDS(pFDS, m_pABox->m_bDoExplanation);
			      
			      mergeTo(pY, pOtherNode, pD);
			    }
			  else
			    {
			      bUseExistingNode = TRUE;
			      pEdge = (Edge*)(*edges.m_listEdge.begin());
			      pY = pEdge->getNeighbor(pX);
			    }
			}
		    }

		  if( pY )
		    pY = pY->getSame();
		}
	    }

	  if( bUseExistingNode )
	    pDS = pDS->unionNew(pEdge->m_pDepends, m_pABox->m_bDoExplanation);
	  else
	    {
	      pY = createFreshIndividual(FALSE);
	      pY->m_iDepth = pX->m_iDepth+1;

	      if( pX->m_iDepth >= m_pABox->m_iTreeDepth )
		m_pABox->m_iTreeDepth = pX->m_iDepth+1;
	    }

	  addType(pY, pC, pDS);

	  if( !bUseExistingRole )
	    addEdge(pX, pRole, pY, pDS);
	}
    }
  END_DECOMMENT("CompletionStrategy::applySomeValuesRule");
}

void CompletionStrategy::applyMinRuleOnIndividuals()
{
  START_DECOMMENT2("CompletionStrategy::applyMinRuleOnIndividuals");
  for(Expr2NodeMap::iterator i = m_pABox->m_mNodes.begin(); i != m_pABox->m_mNodes.end(); i++ )
    {
      Node* pN = (Node*)i->second;
      if( !pN->isIndividual() ) continue;
      Individual* pNode = (Individual*)pN;
      applyMinRule(pNode);
      if( m_pABox->isClosed() )
	break;
    }
  END_DECOMMENT("CompletionStrategy::applyMinRuleOnIndividuals");
}

void CompletionStrategy::applyMinRule(QueueElement* pElement)
{
  Individual* pNode = m_pABox->getIndividual(pElement->m_pNode);
  pNode = (Individual*)pNode->getSame();
  if( m_pBlocking->isBlocked(pNode) )
    {
      m_pABox->m_pCompletionQueue->add(pElement, CompletionQueue::MINLIST);
      return;
    }

  if( pNode->isPruned() )
    return;

  applyMinRule(pNode, pElement->m_pLabel);
}

void CompletionStrategy::applyMinRule(Individual* pX)
{
  if( !pX->canApply(Node::MIN) || m_pBlocking->isBlocked(pX) )
    return;

  // We get all the minCard restrictions in the node and store
  // them in the list ''types''
  int iSize = pX->m_aTypes[Node::MIN].size();
  int iStartAt = pX->m_iApplyNext[Node::MIN], iIndex = 0;
  for(ExprNodes::iterator i = pX->m_aTypes[Node::MIN].begin(); i != pX->m_aTypes[Node::MIN].end() && iIndex < iSize; i++, iIndex++ )
    {
      ExprNode* pMC = (ExprNode*)*i;
      if( iIndex < iStartAt ) continue;
      applyMinRule(pX, pMC);
      if( m_pABox->isClosed() )
	return;
    }

  pX->m_iApplyNext[Node::MIN] = iSize;  
}

void CompletionStrategy::applyMinRule(Individual* pX, ExprNode* pMC)
{
  START_DECOMMENT2("CompletionStrategy::applyMinRule");
  printExprNodeWComment("pX=", pX->m_pName);
  printExprNodeWComment("pMC=", pMC);

  // We retrieve the role associated to the current
  // min restriction
  Role* pRole = g_pKB->getRole((ExprNode*)pMC->m_pArgs[0]);
  int iN = ((ExprNode*)pMC->m_pArgs[1])->m_iTerm;
  ExprNode* pC = (ExprNode*)pMC->m_pArgs[2];
  
  // FIXME make sure all neighbors are safe
  if( pX->hasDistinctRNeighborsForMin(pRole, iN, pC) )
    {
      END_DECOMMENT("CompletionStrategy::applyMinRule");
      return;
    }

  DependencySet* pDS = pX->getDepends(pMC);
  
  if( !PARAMS_MAINTAIN_COMPLETION_QUEUE() && pDS == NULL )
    {
      END_DECOMMENT("CompletionStrategy::applyMinRule");
      return;
    }

  Nodes aNodes;
  for(int i = 0; i < iN; i++ )
    {
      Node* pNode = NULL;
      if( pRole->isDatatypeRole() )
	pNode = m_pABox->addLiteral();
      else
	{
	  pNode = createFreshIndividual(FALSE);
	  pNode->m_iDepth = pX->m_iDepth+1;

	  if( pX->m_iDepth >= m_pABox->m_iTreeDepth )
	    m_pABox->m_iTreeDepth = pX->m_iDepth+1;
	}

      Node* pSucc = pNode;
      DependencySet* pFinalDS = pDS;
      
      addEdge(pX, pRole, pSucc, pDS);
      if( pSucc->isPruned() )
	{
	  pSucc = pSucc->m_pMergeTo;
	  pFinalDS = pFinalDS->unionNew(pSucc->getMergeDependency(TRUE), m_pABox->m_bDoExplanation);
	}
      
      addType(pSucc, pC, pFinalDS);
      for(Nodes::iterator k = aNodes.begin(); k != aNodes.end(); k++ )
	pSucc->setDifferent(((Node*)*k), pDS);

      aNodes.push_back(pNode);
    }
  END_DECOMMENT("CompletionStrategy::applyMinRule");
}

void CompletionStrategy::applyMaxRuleOnIndividuals()
{
  START_DECOMMENT2("CompletionStrategy::applyMaxRuleOnIndividuals");
  for(Expr2NodeMap::iterator i = m_pABox->m_mNodes.begin(); i != m_pABox->m_mNodes.end(); i++ )
    {
      Node* pN = (Node*)i->second;
      if( !pN->isIndividual() ) continue;
      Individual* pNode = (Individual*)pN;
      applyMaxRule(pNode);
      if( m_pABox->isClosed() )
	break;
    }
  END_DECOMMENT("CompletionStrategy::applyMaxRuleOnIndividuals");
}

void CompletionStrategy::applyMaxRule(QueueElement* pElement)
{
  Individual* pNode = m_pABox->getIndividual(pElement->m_pNode);
  pNode = (Individual*)pNode->getSame();
  if( m_pBlocking->isIndirectlyBlocked(pNode) )
    {
      m_pABox->m_pCompletionQueue->add(pElement, CompletionQueue::MAXLIST);
      return;
    }

  if( pNode->isPruned() )
    return;

  applyMaxRule(pNode, pElement->m_pLabel);
}

void CompletionStrategy::applyMaxRule(Individual* pX)
{
  if( !pX->canApply(Node::MAX) || m_pBlocking->isIndirectlyBlocked(pX) )
    return;

  for(ExprNodes::iterator i = pX->m_aTypes[Node::MAX].begin(); i != pX->m_aTypes[Node::MAX].end(); i++ )
    {
      ExprNode* pMC = (ExprNode*)*i;
      applyMaxRule(pX, pMC);
      if( m_pABox->isClosed() || pX->isMerged() )
	return;
    }

  pX->setChanged(Node::MAX, FALSE);
}

void CompletionStrategy::applyMaxRule(Individual* pX, ExprNode* pMC)
{
  START_DECOMMENT2("CompletionStrategy::applyMaxRule");
  printExprNodeWComment("pX=", pX->m_pName);
  printExprNodeWComment("pMC=", pMC);

  // max(r, n) is in normalized form not(min(p, n + 1))
  ExprNode* pMax = (ExprNode*)pMC->m_pArgs[0];

  Role* pRole = g_pKB->getRole((ExprNode*)pMax->m_pArgs[0]);
  int iN = ((ExprNode*)pMax->m_pArgs[1])->m_iTerm-1;
  ExprNode* pC = (ExprNode*)pMax->m_pArgs[2];

  DependencySet* pDS = pX->getDepends(pMC);
  
  if( !PARAMS_MAINTAIN_COMPLETION_QUEUE() && pDS == NULL )
    {
      END_DECOMMENT("CompletionStrategy::applyMaxRule");
      return;
    }

  if( iN == 1 )
    {
      applyFunctionalMaxRule(pX, pRole, pC, pDS);
      if( m_pABox->isClosed() )
	{
	  END_DECOMMENT("CompletionStrategy::applyMaxRule");
	  return;
	}
    }
  else
    {
      bool bHasMore = TRUE;
      while(bHasMore)
	{
	  bHasMore = applyMaxRule(pX, pRole, pC, iN, pDS);
	  if( m_pABox->isClosed() || pX->isMerged() )
	    {
	      END_DECOMMENT("CompletionStrategy::applyMaxRule");
	      return;
	    }

	  if( bHasMore )
	    {
	      // subsequent merges depend on the previous merge
	      pDS = pDS->unionNew( new DependencySet(m_pABox->m_aBranches.size()), m_pABox->m_bDoExplanation);
	    }
	}
    }
  END_DECOMMENT("CompletionStrategy::applyMaxRule");
}

bool CompletionStrategy::applyMaxRule(Individual* pX, Role* pRole, ExprNode* pC, int iN, DependencySet* pDS)
{
  START_DECOMMENT2("CompletionStrategy::applyMaxRule(2)");
  printExprNodeWComment("pX=", pX->m_pName);
  printExprNodeWComment("pC=", pC);
  printExprNodeWComment("Role=", pRole->m_pName);
  DECOMMENT1("iN=%d", iN);
    m_pABox->validate();

  EdgeList edges;
  pX->getRNeighborEdges(pRole, &edges);
  NodeSet setNeighbors;
  edges.getFilteredNeighbors(pX, pC, &setNeighbors);

  int n = setNeighbors.size();

  // if restriction was maxCardinality 0 then having any R-neighbor
  // violates the restriction. no merge can fix this. compute the
  // dependency and return
  if( iN == 0 && n > 0 )
    {
      for(EdgeVector::iterator i = edges.m_listEdge.begin(); i != edges.m_listEdge.end(); i++)
	{
	  Edge* pEdge = (Edge*)*i;
	  Node* pNeighbor = pEdge->getNeighbor(pX);
	  DependencySet* pTypeDS = pNeighbor->getDepends(pC);
	  if( pTypeDS )
	    {
	      DependencySet* pSubDS = pRole->getExplainSub(pEdge->m_pRole->m_pName);
	      if( pSubDS )
		pDS = pDS->unionNew(pSubDS, m_pABox->m_bDoExplanation);
	      pDS = pDS->unionNew(pEdge->m_pDepends, m_pABox->m_bDoExplanation);
	      pDS = pDS->unionNew(pTypeDS, m_pABox->m_bDoExplanation);
	    }
	}

      m_pABox->setClash(Clash::maxCardinality(pX, pDS, pRole->m_pName, 0));
      END_DECOMMENT("CompletionStrategy::applyMaxRule");
      return FALSE;
    }

  // if there are less than n neighbors than max rule won't be triggered
  // return false because no more merge required for this role
  if( n <= iN )
    {
      END_DECOMMENT("CompletionStrategy::applyMaxRule");
      return FALSE;
    }

  // create the pairs to be merged
  NodeMerges mergePairs;
  DependencySet* pDifferenceDS = findMergeNodes(&setNeighbors, pX, &mergePairs);
  pDS = pDS->unionNew(pDifferenceDS, m_pABox->m_bDoExplanation);

  // if no pairs were found, i.e. all were defined to be different from
  // each other, then it means this max cardinality restriction is
  // violated. dependency of this clash is on all the neighbors plus the
  // dependency of the restriction type
  if( mergePairs.size() == 0 )
    {
      DependencySet* pDSEdges = pX->hasDistinctRNeighborsForMax(pRole, iN+1, pC);
      if( pDSEdges == NULL )
	{
	  DECOMMENT("No node pair found to merge");
	  m_pABox->setClash(Clash::maxCardinality(pX, pDS));
	  END_DECOMMENT("CompletionStrategy::applyMaxRule");
	  return FALSE;
	}
      else
	{
	  if( m_pABox->m_bDoExplanation )
	    m_pABox->setClash(Clash::maxCardinality(pX, pDS->unionNew(pDSEdges, m_pABox->m_bDoExplanation), pRole->m_pName, iN));
	  else
	    m_pABox->setClash(Clash::maxCardinality(pX, pDS->unionNew(pDSEdges, m_pABox->m_bDoExplanation)));
	  END_DECOMMENT("CompletionStrategy::applyMaxRule");
	  return FALSE;
	}
    }
  
  // add the list of possible pairs to be merged in the branch list
  MaxBranch* pNewBranch = new MaxBranch(m_pABox, this, pX, pRole, iN, pC, &mergePairs, pDS);
  addBranch(pNewBranch);
  
  if( pNewBranch->tryNext() == FALSE )
    {
      END_DECOMMENT("CompletionStrategy::applyMaxRule");
    return FALSE;
    }

  // if there were exactly k + 1 neighbors the previous step would
  // eliminate one node and only n neighbors would be left. This means
  // restriction is satisfied. If there were more than k + 1 neighbors
  // merging one pair would not be enough and more merges are required,
  // thus false is returned
  END_DECOMMENT("CompletionStrategy::applyMaxRule");
  return n > iN + 1;
}

void CompletionStrategy::applyDisjunctionRuleOnIndividuals()
{
  START_DECOMMENT2("CompletionStrategy::applyDisjunctionRuleOnIndividuals");
  for(Expr2NodeMap::iterator i = m_pABox->m_mNodes.begin(); i != m_pABox->m_mNodes.end(); i++ )
    {
      Node* pN = (Node*)i->second;
      if( !pN->isIndividual() ) continue;
      Individual* pNode = (Individual*)pN;
      applyDisjunctionRule(pNode);
      if( m_pABox->isClosed() || pNode->isMerged() )
	break;
    }
  END_DECOMMENT("CompletionStrategy::applyDisjunctionRuleOnIndividuals");
}

void CompletionStrategy::applyDisjunctionRule(QueueElement* pElement)
{
  Individual* pNode = m_pABox->getIndividual(pElement->m_pNode);
  pNode = (Individual*)pNode->getSame();

  if( pNode->isPruned() )
    return;

  if( m_pBlocking->isIndirectlyBlocked(pNode) )
    {
      m_pABox->m_pCompletionQueue->add(pElement, CompletionQueue::ORLIST);
      return;
    }

  ExprNode* pDisjunction = pElement->m_pLabel;

  if( !PARAMS_MAINTAIN_COMPLETION_QUEUE() && pNode->getDepends(pDisjunction) == NULL )
    return;

  // disjunction is now in the form not(and([not(d1), not(d2), ...]))
  ExprNode* pA = (ExprNode*)pDisjunction->m_pArgs[0];
  ExprNodeList* pDisjuncts = (ExprNodeList*)pA->m_pArgList;
  
  ExprNodes aDisj;
  for(int i = 0; i < pDisjuncts->m_iUsedSize; i++ )
    {
      ExprNode* pDisj = negate2(pDisjuncts->m_pExprNodes[i]);
      aDisj.push_back(pDisj);
      if( pNode->hasType(pDisj) )
	return;
    }

  DisjunctionBranch* pNewBranch = new DisjunctionBranch(m_pABox, this, pNode, pDisjunction, pNode->getDepends(pDisjunction), &aDisj);
  addBranch(pNewBranch);
  pNewBranch->tryNext();
}

void CompletionStrategy::applyDisjunctionRule(Individual* pX)
{
  START_DECOMMENT2("CompletionStrategy::applyDisjunctionRule");
  printExprNodeWComment("pX=", pX->m_pName);

  pX->setChanged(Node::OR, FALSE);

  if( !pX->canApply(Node::OR) || m_pBlocking->isIndirectlyBlocked(pX) )
    {
      END_DECOMMENT("CompletionStrategy::applyMaxRule");
      return;
    }

  ExprNodes aDisjunctions;
  int iSize = pX->m_aTypes[Node::OR].size(), iIndex = 0;
  for(ExprNodes::iterator i = pX->m_aTypes[Node::OR].begin(); i != pX->m_aTypes[Node::OR].end(); i++ )
    {
      if( iIndex >= pX->m_iApplyNext[Node::OR] )
	aDisjunctions.push_back((ExprNode*)*i);
      iIndex++;
    }

  //if( PARAMS_USE_DISJUNCTION_SORTING() != PARAMS_NO_SORTING() )
  // DisjunctionSorting.sort( node, disjunctions );
  
  for(ExprNodes::iterator j = aDisjunctions.begin(); j != aDisjunctions.end(); j++)
    {
      ExprNode* pDisjunction = (ExprNode*)*j;
      
      if( !PARAMS_MAINTAIN_COMPLETION_QUEUE() && pX->getDepends(pDisjunction) == NULL )
	continue;

      printExprNodeWComment("Disj=", pDisjunction);

      // disjunction is now in the form not(and([not(d1), not(d2), ...]))
      ExprNode* pA = (ExprNode*)pDisjunction->m_pArgs[0];
      ExprNodeList* pDisjuncts = (ExprNodeList*)pA->m_pArgList;
      ExprNodes aDisj;
      
      bool bContinue = FALSE;
      for(int i = 0; i < pDisjuncts->m_iUsedSize; i++ )
	{
	  ExprNode* pNode = negate2(pDisjuncts->m_pExprNodes[i]);
	  aDisj.push_back(pNode);
	  if( pX->hasType(pNode) )
	    {
	      bContinue = TRUE;
	      break;
	    }
	}
      if( bContinue )
	continue;

      DisjunctionBranch* pNewBranch = new DisjunctionBranch(m_pABox, this, pX, pDisjunction, pX->getDepends(pDisjunction), &aDisj);
      addBranch(pNewBranch);
      pNewBranch->tryNext();

      if( m_pABox->isClosed() || pX->isMerged() )
	{
	  END_DECOMMENT("CompletionStrategy::applyMaxRule");
	  return;
	}
    }
  pX->m_iApplyNext[Node::OR] = iSize;
  END_DECOMMENT("CompletionStrategy::applyMaxRule");
}

void CompletionStrategy::applyPropertyRestrictions(Edge* pEdge)
{
  START_DECOMMENT2("CompletionStrategy::applyPropertyRestrictions");

  Individual* pSubj = pEdge->m_pFrom;
  Role* pPred = pEdge->m_pRole;
  Node* pObj = pEdge->m_pTo;
  DependencySet* pDS = pEdge->m_pDepends;

  applyDomainRange(pSubj, pPred, pObj, pDS);

  if( pSubj->isPruned() || pObj->isPruned() )
    {
      END_DECOMMENT("CompletionStrategy::applyPropertyRestrictions");
      return;
    }

  applyFunctionality(pSubj, pPred, pObj);

  if( pSubj->isPruned() || pObj->isPruned() )
    {
      END_DECOMMENT("CompletionStrategy::applyPropertyRestrictions");
      return;
    }

  applyDisjointness(pSubj, pPred, pObj, pDS);
  applyAllValues(pSubj, pPred, pObj, pDS);

  if( pSubj->isPruned() || pObj->isPruned() )
    {
      END_DECOMMENT("CompletionStrategy::applyPropertyRestrictions");
      return;
    }

  if( pPred->isObjectRole() )
    {
      Individual* pO = (Individual*)pObj;
      applyAllValues(pO,  pPred->m_pInverse, pSubj, pDS);
      checkReflexivitySymmetry(pSubj, pPred, pO, pDS);
      checkReflexivitySymmetry(pO, pPred->m_pInverse, pSubj, pDS);
    }
  END_DECOMMENT("CompletionStrategy::applyPropertyRestrictions");
}

void CompletionStrategy::applyDomainRange(Individual* pSubj, Role* pPred, Node* pObj, DependencySet* pDS)
{
  START_DECOMMENT2("CompletionStrategy::applyDomainRange");
  printExprNodeWComment("Subj=", pSubj->m_pName);
  printExprNodeWComment("Role=", pPred->m_pName);
  printExprNodeWComment("Obj=", pObj->m_pName);

  ExprNode* pDomain = pPred->m_pDomain;
  ExprNode* pRange = pPred->m_pRange;

  if( pDomain )
    addType(pSubj, pDomain, pDS->unionNew(pPred->m_pExplainDomain, m_pABox->m_bDoExplanation));
  if( pRange )
    addType(pObj, pRange, pDS->unionNew(pPred->m_pExplainRange, m_pABox->m_bDoExplanation));
  END_DECOMMENT("CompletionStrategy::applyDomainRange");
}

void CompletionStrategy::applyFunctionality(Individual* pSubj, Role* pPred, Node* pObj)
{
  DependencySet* pMaxCardDS = pPred->isFunctional()?pPred->m_pExplainFunctional:pSubj->hasMax1(pPred);

  if( pMaxCardDS )
    applyFunctionalMaxRule(pSubj, pPred, Role::getTop(pPred), pMaxCardDS);

  if( pPred->isDatatypeRole() && pPred->isInverseFunctional() )
    applyFunctionalMaxRule((Literal*)pObj, pPred, &DEPENDENCYSET_INDEPENDENT);
  else if( pPred->isObjectRole() )
    {
      Individual* pVal = (Individual*)pObj;
      Role* pInvRole = pPred->m_pInverse;
      
      pMaxCardDS = pInvRole->isFunctional()?pInvRole->m_pExplainFunctional:pVal->hasMax1(pInvRole);
      if( pMaxCardDS )
	applyFunctionalMaxRule(pVal, pInvRole, EXPRNODE_TOP, pMaxCardDS);
    }
}

void CompletionStrategy::applyFunctionalMaxRule(Individual* pX, Role* pS, ExprNode* pC, DependencySet* pDS)
{
  START_DECOMMENT2("CompletionStrategy::applyFunctionalMaxRule(Individual)");
  printExprNodeWComment("Indv=", pX->m_pName);
  printExprNodeWComment("Role=", pS->m_pName);
  printExprNodeWComment("pC=", pC);
  RoleSet setFunctionalSupers;

  if( pS->m_setFunctionalSupers.size() == 0 )
    setFunctionalSupers.insert(pS);
  else
    {
      for( RoleSet::iterator i = pS->m_setFunctionalSupers.begin(); i != pS->m_setFunctionalSupers.end(); i++ )
	{
	  Role* pRole = (Role*)*i;
	  setFunctionalSupers.insert(pRole);
	}
    }

  for( RoleSet::iterator i = setFunctionalSupers.begin(); i != setFunctionalSupers.end(); i++ )
    {
      Role* pRole = (Role*)*i;
      
      if( PARAMS_USE_TRACING() )
	pDS = pDS->unionNew(pS->getExplainSuper(pRole->m_pName), m_pABox->m_bDoExplanation)->unionDS(pRole->m_pExplainFunctional, m_pABox->m_bDoExplanation);
      
      EdgeList edges;
      pX->getRNeighborEdges(pRole, &edges);

      // if there is not more than one edge then func max rule won't be triggered
      if( edges.size() <= 1 )
	continue;

      // find all distinct R-neighbors of x
      NodeSet setNeighbors;
      edges.getFilteredNeighbors(pX, pC, &setNeighbors);
      
      // if there is not more than one neighbor then func max rule won't be triggered
      if( setNeighbors.size() <= 1 )
	continue;
      
      Node* pHead = NULL;
      bool bPhaseOne = TRUE;
      for(EdgeVector::iterator j = edges.m_listEdge.begin(); j != edges.m_listEdge.end(); j++ )
	{
	  Edge* pEdge = (Edge*)*j;
	  if( bPhaseOne )
	    {
	      // find the head and its corresponding dependency information. 
	      // since head is not necessarily the first element in the 
	      // neighbor list we need to first find the un-pruned node 
	      pHead = pEdge->getNeighbor(pX);
	      
	      if( pHead->isPruned() || setNeighbors.find(pHead) == setNeighbors.end() )
		continue;
	      
	      // this node is included in the merge list because the edge
	      // exists and the node has the qualification in its types
	      pDS = pDS->unionNew(pEdge->m_pDepends, m_pABox->m_bDoExplanation);
	      pDS = pDS->unionNew(pHead->getDepends(pC), m_pABox->m_bDoExplanation);

	      bPhaseOne = FALSE;
	    }
	  else
	    {
	      // now iterate through the rest of the elements in the neighbors
	      // and merge them to the head node. it is possible that we will
	      // switch the head at some point because of merging rules such
	      // that you alway merge to a nominal of higher level
	      Node* pNext = pEdge->getNeighbor(pX);
	      
	      if( pNext->isPruned() || setNeighbors.find(pNext) == setNeighbors.end() )
		continue;

	      // it is possible that there are multiple edges to the same
	      // node, e.g. property p and its super property, so check if
	      // we already merged this one
	      if( pHead->isSame(pNext) )
		continue;

	      // this node is included in the merge list because the edge
	      // exists and the node has the qualification in its types
	      pDS = pDS->unionNew(pEdge->m_pDepends, m_pABox->m_bDoExplanation);
	      pDS = pDS->unionNew(pNext->getDepends(pC), m_pABox->m_bDoExplanation);
	      
	      if( pNext->isDifferent(pHead) )
		{
		  pDS = pDS->unionNew(pHead->getDepends(pC), m_pABox->m_bDoExplanation);
		  pDS = pDS->unionNew(pNext->getDepends(pC), m_pABox->m_bDoExplanation);
		  pDS = pDS->unionNew(pNext->getDifferenceDependency(pHead), m_pABox->m_bDoExplanation);


		  if( pRole->isFunctional() )
		    m_pABox->setClash(Clash::functionalCardinality(pX, pDS, pRole->m_pName));
		  else
		    m_pABox->setClash(Clash::maxCardinality(pX, pDS, pRole->m_pName, 1));
		  break;
		}

	      if( pX->isNominal() && pHead->isBlockable() && pNext->isBlockable() && pHead->hasSuccessor(pX) && pNext->hasSuccessor(pX) )
		{
		  Individual* pNewNominal = createFreshIndividual(TRUE);
		  addEdge(pX, pRole, pNewNominal, pDS);
		  break;
		}
	      else if( pNext->getNominalLevel() < pHead->getNominalLevel() || (!pHead->isNominal() && pNext->hasSuccessor(pX)) )
		{
		  Node* pTmp = pHead;
		  pHead = pNext;
		  pNext = pTmp;
		}

	      mergeTo(pNext, pHead, pDS);
	      
	      if( m_pABox->isClosed() )
		{
		  END_DECOMMENT("CompletionStrategy::applyFunctionalMaxRule");
		  return;
		}

	      if( pHead->isPruned() )
		{
		  pDS = pDS->unionNew(pHead->getMergeDependency(TRUE), m_pABox->m_bDoExplanation);
		  pHead = pHead->getSame();
		}
	    }
	}
    }
  END_DECOMMENT("CompletionStrategy::applyFunctionalMaxRule");
}

Individual* CompletionStrategy::createFreshIndividual(bool bIsNominal)
{
  START_DECOMMENT2("CompletionStrategy::createFreshIndividual");
  Individual* pInd = m_pABox->addFreshIndividual(bIsNominal);
  applyUniversalRestrictions(pInd);
  END_DECOMMENT("CompletionStrategy::createFreshIndividual");
  return pInd;
}

void CompletionStrategy::applyFunctionalMaxRule(Literal *pX, Role* pR, DependencySet* pDS)
{
  START_DECOMMENT2("CompletionStrategy::applyFunctionalMaxRule(literal)\n");
  // Set functionalSupers = s.getFunctionalSupers();
  // if( functionalSupers.isEmpty() )
  // functionalSupers = SetUtils.singleton( s );
  // for(Iterator it = functionalSupers.iterator(); it.hasNext(); ) {
  // Role r = (Role) it.next();
  
  EdgeList edges;
  pX->m_listInEdges.getEdges(pR, &edges);

  // if there is not more than one edge then func max rule won't be triggered
  if( edges.size() <= 1 )
    {
      END_DECOMMENT("CompletionStrategy::applyFunctionalMaxRule");
      return; // continue
    }

  // find all distinct R-neighbors of x
  NodeSet setNeighbors;
  edges.getNeighbors(pX, &setNeighbors);

  // if there is not more than one neighbor then func max rule won't be triggered
  if( setNeighbors.size() <= 1 )
    {
      END_DECOMMENT("CompletionStrategy::applyFunctionalMaxRule");
      return; // continue
    }

  Individual* pHead = NULL;
  DependencySet* pHeadDS = NULL;
  for(EdgeVector::iterator i = edges.m_listEdge.begin(); i != edges.m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      Individual* pInd = pEdge->m_pFrom;
      
      if( pInd->isNominal() && (pHead == NULL || pInd->getNominalLevel() < pHead->getNominalLevel()) )
	{
	  pHead = pInd;
	  pHeadDS = pEdge->m_pDepends;
	}
    }

  // if there is no nominal in the merge list we need to create one
  if( pHead == NULL )
    pHead = m_pABox->addFreshIndividual(TRUE);
  else
    pDS = pDS->unionNew(pHeadDS, m_pABox->m_bDoExplanation);
  
  for(EdgeVector::iterator i = edges.m_listEdge.begin(); i != edges.m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      Individual* pNext = pEdge->m_pFrom;

      if( pNext->isPruned() )
	continue;

      // it is possible that there are multiple edges to the same
      // node, e.g. property p and its super property, so check if
      // we already merged this one
      if( pHead->isSame(pNext) )
	continue;

      pDS = pDS->unionNew(pEdge->m_pDepends, m_pABox->m_bDoExplanation);

      if( pNext->isDifferent(pHead) )
	{
	  pDS = pDS->unionNew(pNext->getDifferenceDependency(pHead), m_pABox->m_bDoExplanation);
	  if( pR->isFunctional() )
	    m_pABox->setClash(Clash::functionalCardinality(pX, pDS, pR->m_pName));
	  else
	    m_pABox->setClash(Clash::maxCardinality(pX, pDS, pR->m_pName, 1));
	  break;
	}
      
      mergeTo(pNext, pHead, pDS);

      if( m_pABox->isClosed() )
	{
	  END_DECOMMENT("CompletionStrategy::applyFunctionalMaxRule");
	  return;
	}

      if( pHead->isPruned() )
	{
	  pDS = pDS->unionNew(pHead->getMergeDependency(TRUE), m_pABox->m_bDoExplanation);
	  pHead = (Individual*)pHead->getSame();
	}
    }
  END_DECOMMENT("CompletionStrategy::applyFunctionalMaxRule");
}

void CompletionStrategy::applyDisjointness(Individual* pSubj, Role* pPred, Node* pObj, DependencySet* pDS)
{
  START_DECOMMENT2("CompletionStrategy::applyDisjointness");

  // TODO what about inv edges?
  // TODO improve this check
  if( pPred->m_setDisjointRoles.size() == 0 )
    {
      END_DECOMMENT("CompletionStrategy::applyDisjointness");
      return;
    }

  EdgeList edges;
  pSubj->m_listOutEdges.getEdgesTo(pObj, &edges);

  for(EdgeVector::iterator i = edges.m_listEdge.begin(); i != edges.m_listEdge.end(); i++ )
    {
      Edge* pOtherEdge = (Edge*)*i;
      if( pPred->m_setDisjointRoles.find(pOtherEdge->m_pRole) !=  pPred->m_setDisjointRoles.end() )
	{
	  m_pABox->setClash(Clash::unexplained(pSubj, pDS->unionNew(pOtherEdge->m_pDepends, m_pABox->m_bDoExplanation), "Disjoint properties"));
	  END_DECOMMENT("CompletionStrategy::applyDisjointness");
	  return;
	}
    }
  END_DECOMMENT("CompletionStrategy::applyDisjointness");
}

void CompletionStrategy::checkReflexivitySymmetry(Individual* pSubj, Role* pPred, Individual* pObj, DependencySet* pDS)
{
  if( pPred->isAntisymmetric() && pObj->hasRSuccessor(pPred, pSubj) )
    {
      EdgeList edges;
      pObj->m_listOutEdges.getEdgesTo(pObj, &edges);
      
      if( PARAMS_USE_TRACING() )
	pDS = pDS->unionNew(pPred->m_pExplainAntisymmetric, m_pABox->m_bDoExplanation);
      m_pABox->setClash(Clash::unexplained(pSubj, pDS, "Antisymmetric property "));
    }
  else if( isEqual(pSubj, pObj) == 0 )
    {
      if( pPred->isIrreflexive() )
	{
	  m_pABox->setClash(Clash::unexplained(pSubj, pDS->unionNew(pPred->m_pExplainIrreflexive, m_pABox->m_bDoExplanation), "Irreflexive property "));
	}
      else
	{
	  ExprNode* pNotSelfP = createExprNode(EXPR_NOT, createExprNode(EXPR_SELF, pPred->m_pName));
	  if( pSubj->hasType(pNotSelfP) )
	    m_pABox->setClash(Clash::unexplained(pSubj, pDS->unionNew(pSubj->getDepends(pNotSelfP), m_pABox->m_bDoExplanation)));
	}
    }
}    

void CompletionStrategy::mergeFirst()
{
  NodeMerge* pMerge = (NodeMerge*)m_aMergeList.front();
  m_aMergeList.pop_front();

  Node* pY = m_pABox->getNode(pMerge->m_pY);
  Node* pZ = m_pABox->getNode(pMerge->m_pZ);
  DependencySet* pDS = pMerge->m_pDS;

  if( pY->isMerged() )
    {
      pDS = pDS->unionNew(pY->getMergeDependency(TRUE), m_pABox->m_bDoExplanation);
      pY = pY->getSame();
    }
  if( pZ->isMerged() )
    {
      pDS = pDS->unionNew(pZ->getMergeDependency(TRUE), m_pABox->m_bDoExplanation);
      pZ = pZ->getSame();
    }

  if( pY->isPruned() || pZ->isPruned() )
    return;

  mergeTo(pY, pZ, pDS);
}

// Merge node y into z. Node y and all its descendants 
// will be pruned from the completion graph.
void CompletionStrategy::mergeTo(Node* pY, Node* pZ, DependencySet* pDS)
{
  START_DECOMMENT2("CompletionStrategy::mergeTo");
  printExprNodeWComment("Y", pY->m_pName);
  printExprNodeWComment("Z", pZ->m_pName);

  //add to effected list of queue
  if( m_pABox->m_iCurrentBranchIndex >= 0 && PARAMS_USE_COMPLETION_QUEUE() )
    {
      m_pABox->addEffected(m_pABox->m_iCurrentBranchIndex, pY->m_pName);
      m_pABox->addEffected(m_pABox->m_iCurrentBranchIndex, pZ->m_pName);
    }

  //add to merge dependency to dependency index
  if( PARAMS_USE_INCREMENTAL_DELETION() )
    g_pKB->m_pDependencyIndex->addMergeDependency(pY->m_pName, pZ->m_pName, pDS);

  if( pY->isSame(pZ) )
    {
      DECOMMENT("Y is same Z");
      END_DECOMMENT("CompletionStrategy::mergeTo");
      return;
    }
  else if( pY->isDifferent(pZ) )
    {
      DECOMMENT("Y is different Z");
      m_pABox->setClash(Clash::nominal(pY, pY->getDifferenceDependency(pZ)->unionNew(pDS, m_pABox->m_bDoExplanation)));
      END_DECOMMENT("CompletionStrategy::mergeTo");
      return;
    }

  if( !pY->isSame(pZ) )
    {
      m_pABox->m_bChanged = TRUE;
      
      if( m_bMerging )
	{
	  mergeLater(pY, pZ, pDS);
	  DECOMMENT("mergeLater");
	  END_DECOMMENT("CompletionStrategy::mergeTo");
	  return;
	}
      else
	m_bMerging = TRUE;

      pDS = new DependencySet(pDS);
      pDS->m_iBranch = m_pABox->m_iCurrentBranchIndex;

      if( !pY->isIndividual() && !pZ->isIndividual() )
	mergeLiterals((Literal*)pY, (Literal*)pZ, pDS);
      else if( pY->isIndividual() && pZ->isIndividual() )
	mergeIndividuals((Individual*)pY, (Individual*)pZ, pDS);
      else
	assertFALSE("Invalid merge operation!");
    }

  m_bMerging = FALSE;
  if( m_aMergeList.size() > 0 )
    {
      if( m_pABox->isClosed() )
	{
	  END_DECOMMENT("CompletionStrategy::mergeTo");
	  return;
	}
      mergeFirst();
    }
  END_DECOMMENT("CompletionStrategy::mergeTo");
}

/**
 * Merge literal y into x. Literal y will be pruned from* the completion graph.
 */
void CompletionStrategy::mergeLiterals(Literal* pY, Literal* pX, DependencySet* pDS)
{
  pY->setSame(pX, pDS);
  pX->addAllTypes(&(pY->m_mDepends), pDS);

  // for all edges (z, r, y) add an edge (z, r, x)
  for(EdgeVector::iterator i = pY->m_listInEdges.m_listEdge.begin(); i != pY->m_listInEdges.m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      Individual* pZ = pEdge->m_pFrom;
      Role* pRole = pEdge->m_pRole;
      DependencySet* pFinalDS = pDS->unionNew(pEdge->m_pDepends, m_pABox->m_bDoExplanation);
      
      addEdge(pZ, pRole, pX, pFinalDS);

      // only remove the edge from z and keep a copy in y for a
      // possible restore operation in the future
      pZ->removeEdge(pEdge);
      
      //add to effected list of queue
      if( m_pABox->m_iCurrentBranchIndex >= 0 && PARAMS_USE_COMPLETION_QUEUE() )
	m_pABox->addEffected(m_pABox->m_iCurrentBranchIndex, pZ->m_pName);
    }

  pX->inheritDifferents(pY, pDS);
  pY->prune(pDS);
}

/**
 * Merge individual y into x. Individual y and all its descendants will be pruned from the
 * completion graph.
 * pY = Individual being pruned
 * pX = Individual that is being merged into
 * pDS = Dependency of this merge operation
 */
void CompletionStrategy::mergeIndividuals(Individual* pY, Individual* pX, DependencySet* pDS)
{
  pY->setSame(pX, pDS);

  // if both x and y are blockable x still remains blockable (nominal level
  // is still set to BLOCKABLE), if one or both are nominals then x becomes
  // a nominal with the minimum level
  pX->m_iNominalLevel = min(pX->getNominalLevel(), pY->getNominalLevel());

  // copy the types
  for(ExprNode2DependencySetMap::iterator i = pY->m_mDepends.begin(); i != pY->m_mDepends.end(); i++ )
    {
      ExprNode* pYType = (ExprNode*)i->first;
      DependencySet* pYDS = (DependencySet*)i->second;
      DependencySet* pFinalDS = pDS->unionNew(pYDS, m_pABox->m_bDoExplanation);
      addType(pX, pYType, pFinalDS);
    }
 
  // for all edges (z, r, y) add an edge (z, r, x)
  for(EdgeVector::iterator i = pY->m_listInEdges.m_listEdge.begin(); i != pY->m_listInEdges.m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      Individual* pZ = pEdge->m_pFrom;
      Role* pRole = pEdge->m_pRole;
      DependencySet* pFinalDS = pDS->unionNew(pEdge->m_pDepends, m_pABox->m_bDoExplanation);
      
      // if y has a self edge then x should have the same self edge
      if( isEqual(pY, pZ) == 0 )
	addEdge(pX, pRole, pX, pFinalDS);
      // if z is already a successor of x add the reverse edge
      else if( pX->hasSuccessor(pZ) )
	{
	  // FIXME what if there were no inverses in this expressitivity
	  addEdge(pX, pRole->m_pInverse, pZ, pFinalDS);
	}
      else
	addEdge(pZ, pRole, pX, pFinalDS);
      
      // only remove the edge from z and keep a copy in y for a
      // possible restore operation in the future
      pZ->removeEdge(pEdge);

      //add to effected list of queue
      if( m_pABox->m_iCurrentBranchIndex >= 0 && PARAMS_USE_COMPLETION_QUEUE() )
	m_pABox->addEffected(m_pABox->m_iCurrentBranchIndex, pZ->m_pName);
    }

  // for all z such that y != z set x != z
  pX->inheritDifferents(pY, pDS);
  
  // we want to prune y early due to an implementation issue about literals
  // if y has an outgoing edge to a literal with concrete value
  pY->prune(pDS);

  // for all edges (y, r, z) where z is a nominal add an edge (x, r, z)
  for(EdgeVector::iterator i = pY->m_listOutEdges.m_listEdge.begin(); i != pY->m_listOutEdges.m_listEdge.end(); i++ )
    {
      Edge* pEdge = (Edge*)*i;
      Node* pZ = pEdge->m_pTo;

      if( pZ->isNominal() && isEqual(pY, pZ) != 0 )
	{
	  Role* pRole = pEdge->m_pRole;
	  DependencySet* pFinalDS = pDS->unionNew(pEdge->m_pDepends, m_pABox->m_bDoExplanation);
	  addEdge(pX, pRole, pZ, pFinalDS);
	  
	  //add to effected list of queue
	  if( m_pABox->m_iCurrentBranchIndex >= 0 && PARAMS_USE_COMPLETION_QUEUE() )
	    m_pABox->addEffected(m_pABox->m_iCurrentBranchIndex, pZ->m_pName);

	  // do not remove edge here because prune will take care of that
	}
    }
}

void CompletionStrategy::mergeLater(Node* pY, Node* pZ, DependencySet* pDS)
{
  m_aMergeList.push_back(new NodeMerge(pY, pZ, pDS));
}

void CompletionStrategy::addEdge(Individual* pSubj, Role* pPred, Node* pObj, DependencySet* pDS)
{
  START_DECOMMENT2("CompletionStrategy::addEdge");
  printExprNodeWComment("Subj=", pSubj->m_pName);
  printExprNodeWComment("Role=", pPred->m_pName);
  printExprNodeWComment("Obj=", pObj->m_pName);


  Edge* pEdge = pSubj->addEdge(pPred, pObj, pDS);

  //add to the kb dependencies
  if( PARAMS_USE_INCREMENTAL_DELETION() )
    g_pKB->m_pDependencyIndex->addEdgeDependency(pEdge, pDS);

  if( PARAMS_USE_COMPLETION_QUEUE() )
    {
      // update the queue as we are adding an edge - we must add
      // elements to the MAXLIST
      updateQueueAddEdge(pSubj, pPred, pObj);
    }
  
  if( pEdge )
    {
      // note that we do not need to enforce the guess rule for 
      // datatype properties because we may only have inverse 
      // functional datatype properties which will be handled
      // inside applyPropertyRestrictions
      if( pSubj->isBlockable() && pObj->isNominal() && pObj->isIndividual() )
	{
	  Individual* pO = (Individual*)pObj;
	  int iMax = pO->getMaxCard(pPred->m_pInverse);
	  if( iMax != INT_MAX && !pO->hasDistinctRNeighborsForMin(pPred->m_pInverse, iMax, EXPRNODE_TOP, TRUE) )
	    {
	      int iGuessMin = pO->getMinCard(pPred->m_pInverse);
	      if( iGuessMin == 0 )
		iGuessMin = 1;

	      if( iGuessMin > iMax )
		{
		  END_DECOMMENT("CompletionStrategy::addEdge");
		  return;
		}

	      Branch* pGuessBranch = new GuessBranch(m_pABox, this, pO, pPred->m_pInverse, iGuessMin, iMax, pDS);
	      addBranch(pGuessBranch);

	      // try a merge that does not trivially fail
	      if( pGuessBranch->tryNext() == FALSE )
		{
		  END_DECOMMENT("CompletionStrategy::addEdge");
		  return;
		}
	      if( m_pABox->isClosed() )
		{
		  END_DECOMMENT("CompletionStrategy::addEdge");
		  return;
		}
	      if( pSubj->isMerged() )
		{
		  END_DECOMMENT("CompletionStrategy::addEdge");
		  return;
		}
	    }
	}
      applyPropertyRestrictions(pEdge);
    }
  END_DECOMMENT("CompletionStrategy::addEdge");
}

void CompletionStrategy::addType(Node* pNode, ExprNode* pC, DependencySet* pDS)
{
  START_DECOMMENT2("CompletionStrategy::addType");
  printExprNodeWComment("pC=", pC);
  printExprNodeWComment("pNode=", pNode->m_pName);

  if( m_pABox->isClosed() )
    {
      END_DECOMMENT("CompletionStrategy::addType");
      return;
    }

  pNode->addType(pC, pDS);

  //update dependency index for this node
  if( PARAMS_USE_INCREMENTAL_DELETION() )
    g_pKB->m_pDependencyIndex->addTypeDependency(pNode->m_pName, pC,  pDS);

  if( pC->m_iExpression == EXPR_AND )
    {
      for(int i = 0; i < ((ExprNodeList*)pC->m_pArgList)->m_iUsedSize; i++ )
	{
	  ExprNode* pConj = ((ExprNodeList*)pC->m_pArgList)->m_pExprNodes[i];
	  addType(pNode, pConj, pDS);
	  pNode = pNode->getSame();
	}
    }
  else if( pC->m_iExpression == EXPR_ALL )
    {
      applyAllValues((Individual*)pNode, pC, pDS);
    }
  else if( pC->m_iExpression == EXPR_SELF )
    {
      ExprNode* pPred = (ExprNode*)pC->m_pArgs[0];
      Role* pRole = g_pKB->getRole(pPred);
      addEdge((Individual*)pNode, pRole, pNode, pDS);
    }
  // else if( c.getAFun().equals( ATermUtils.VALUE ) ) {
  // applyNominalRule( (Individual) node, c, ds);
  // }
  END_DECOMMENT("CompletionStrategy::addType");
}

void CompletionStrategy::addBranch(Branch* pNewBranch)
{
  START_DECOMMENT2("CompletionStrategy::addBranch");
  m_pABox->m_aBranches.push_back(pNewBranch);

  if( pNewBranch->m_iBranch != m_pABox->m_aBranches.size() )
    assertFALSE("Invalid branch created!");

  //CHW - added for incremental deletion support
  if( supportsPseudoModelCompletion() && PARAMS_USE_INCREMENTAL_DELETION() )
    g_pKB->m_pDependencyIndex->addBranchAddDependency(pNewBranch);
  END_DECOMMENT("CompletionStrategy::addBranch");
}

/**
 * This method updates the queue in the event that there is an edge added 
 * between two nodes. The individual must be added back onto the MAXLIST 
 */
void CompletionStrategy::updateQueueAddEdge(Individual* pSubj, Role* pPred, Node *pObj)
{
  // for each min and max card restrictions for the subject, a new
  // queueElement must be generated and added
  for(ExprNodes::iterator i = pSubj->m_aTypes[Node::MAX].begin(); i != pSubj->m_aTypes[Node::MAX].end(); i++ )
    {
      ExprNode* pC = (ExprNode*)*i;
      ExprNode* pMax = (ExprNode*)pC->m_pArgs[0];
      Role* pRole = g_pKB->getRole( (ExprNode*)pMax->m_pArgs[0] );
      
      if( pPred->isSubRoleOf(pRole) || isEqual(pPred, pRole) )
	{
	  QueueElement* pNewElement = new QueueElement(pSubj->m_pName, pC);
	  m_pABox->m_pCompletionQueue->add(pNewElement, CompletionQueue::MAXLIST);
	  m_pABox->m_pCompletionQueue->add(pNewElement, CompletionQueue::CHOOSELIST);
	}
    }

  // if the predicate has an inverse or is inversefunctional and the obj
  // is an individual, then add the object to the list.
  if( (pPred->hasNamedInverse() || pPred->isInverseFunctional()) && pObj->isIndividual() )
    {
      for(ExprNodes::iterator i = ((Individual*)pObj)->m_aTypes[Node::MAX].begin(); i != ((Individual*)pObj)->m_aTypes[Node::MAX].end(); i++ )
	{
	  ExprNode* pC = (ExprNode*)*i;
	  ExprNode* pMax = (ExprNode*)pC->m_pArgs[0];
	  Role* pRole = g_pKB->getRole( (ExprNode*)pMax->m_pArgs[0] );
	  
	  if( pPred->isSubRoleOf(pRole->m_pInverse) || isEqual(pPred, pRole->m_pInverse) )
	    {
	      QueueElement* pNewElement = new QueueElement(pObj->m_pName, pC);
	      m_pABox->m_pCompletionQueue->add(pNewElement, CompletionQueue::MAXLIST);
	      m_pABox->m_pCompletionQueue->add(pNewElement, CompletionQueue::CHOOSELIST);
	    }
	}
    }
}

DependencySet* CompletionStrategy::findMergeNodes(NodeSet* pSetNeighbors, Individual* pNode, NodeMerges* pPairs)
{
  DependencySet* pDS = &DEPENDENCYSET_INDEPENDENT;

  vector<Node*> aNeighbors;
  for(NodeSet::iterator i = pSetNeighbors->begin(); i != pSetNeighbors->end(); i++ )
    aNeighbors.push_back((Node*)*i);

  int iSize = aNeighbors.size();
  for(int i = 0; i < iSize; i++ )
    {
      Node* pY = aNeighbors.at(i);
      for(int j = i+1; j < iSize; j++ )
	{
	  Node* pX = aNeighbors.at(j);
	  
	  if( pY->isDifferent(pX) )
	    {
	      pDS = pDS->unionNew(pY->getDifferenceDependency(pX), m_pABox->m_bDoExplanation);
	      continue;
	    }

	  // 1. if x is a nominal node (of lower level), then Merge(y, x)
	  if( pX->getNominalLevel() < pY->getNominalLevel() )
	    pPairs->push_back((new NodeMerge(pY, pX)));
	  // 2. if y is a nominal node or an ancestor of x, then Merge(x, y)
	  else if( pY->isNominal() )
	    pPairs->push_back((new NodeMerge(pX, pY)));
	  // 3. if y is an ancestor of x, then Merge(x, y)
	  // Note: y is an ancestor of x iff the max cardinality
	  // on node merges the "node"'s parent y with "node"'s
	  // child x
	  else if( pY->hasSuccessor(pNode) )
	    pPairs->push_back((new NodeMerge(pX, pY)));
	  // 4. else Merge(y, x)
	  else
	    pPairs->push_back((new NodeMerge(pY, pX)));
	}
    }

  return pDS;
}
