#include "SROIQIncStrategy.h"
#include "ABox.h"
#include "KnowledgeBase.h"
#include "Node.h"
#include "Individual.h"

extern KnowledgeBase* g_pKB;

SROIQIncStrategy::SROIQIncStrategy(ABox* pABox) : SROIQStrategy(pABox)
{

}

void SROIQIncStrategy::applyGuessingRuleOnIndividuals()
{
  // overwrite empty
}

void SROIQIncStrategy::initialize()
{
  //call super - note that the initialization for new individuals will 
  //not be reached in this call; therefore it is performed below
  SROIQStrategy::initialize();

  //we will also need to add stuff to the queue in the event of a deletion
  if( g_pKB->m_bABoxDeletion )
    {
      for(NodeSet::iterator i = m_pABox->m_setUpdatedIndividuals.begin(); i != m_pABox->m_setUpdatedIndividuals.end(); i++ )
	{
	  Individual* pNode = (Individual*)*i;

	  if( pNode->isMerged() )
	    pNode = (Individual*)pNode->getSame();

	  //apply unfolding rule
	  pNode->m_iApplyNext[Node::ATOM] = 0;
	  applyUnfoldingRule(pNode);

	  applyAllValues(pNode);
	  if( pNode->isMerged() )
	    continue;

	  applyNominalRule(pNode);
	  if( pNode->isMerged() )
	    continue;

	  applySelfRule(pNode);

	  //CHW-added for inc. queue must see if this is bad
	  for(EdgeVector::iterator j = pNode->m_listOutEdges.m_listEdge.begin(); j != pNode->m_listOutEdges.m_listEdge.end(); j++ )
	    {
	      Edge* pEdge = (Edge*)*j;
	      if( pEdge->m_pTo->isPruned() )
		continue;
	      applyPropertyRestrictions(pEdge);
	      if( pNode->isMerged() )
		break;
	    }

	  //There are cases under deletions when a label can be removed from a node which should actually exist; this is demonstated by IncConsistencyTests.testMerge3()
	  //In this case a merge fires before the disjunction is applied on one of the nodes being merged; when the merge is undone by a removal, the disjunction for the
	  //original node must be applied! This is resolved by the following check for all rules which introduce branches; note that this approach can clearly be optimized.
	  //Another possible approach would be to utilize sets of sets in dependency sets, however this is quite complicated.
          
	  pNode->m_iApplyNext[Node::OR] = 0;
	  applyDisjunctionRule(pNode);
	  
	  pNode->m_iApplyNext[Node::MAX] = 0;
	  applyMaxRule(pNode);

	  pNode->m_iApplyNext[Node::MAX] = 0;
	  applyChooseRule(pNode);

	  //may need to do the same for literals?
	}
    }
  
  //if this is an incremental addition we may need to merge nodes and handle
  //newly added individuals
  if( g_pKB->m_bABoxAddition )
    {
      //merge nodes - note branch must be temporarily set to 0 to ensure that asssertion
      //will not be restored during backtracking
      int iBranch = m_pABox->m_iCurrentBranchIndex;
      m_pABox->m_iCurrentBranchIndex = 0;

      for(NodeMerges::iterator j = m_pABox->m_listToBeMerged.begin(); j != m_pABox->m_listToBeMerged.end(); j++ )
	m_aMergeList.push_back((NodeMerge*)*j);

      if( m_aMergeList.size() > 0 )
	mergeFirst();

      //Apply necessary intialization to any new individual added 
      //Currently, this is a replication of the code from CompletionStrategy.initialize()
      
      for(NodeSet::iterator i = m_pABox->m_setNewIndividuals.begin(); i != m_pABox->m_setNewIndividuals.end(); i++ )
	{
	  Individual* pNode = (Individual*)*i;
	  
	  if( pNode->isMerged() )
	    continue;

	  pNode->setChanged(TRUE);
	  applyUniversalRestrictions(pNode);
	  applyUnfoldingRule(pNode);
	  applySelfRule(pNode);

	  for(EdgeVector::iterator j = pNode->m_listOutEdges.m_listEdge.begin(); j != pNode->m_listOutEdges.m_listEdge.end(); j++ )
	    {
	      Edge* pEdge = (Edge*)*j;
	      if( pEdge->m_pTo->isPruned() )
		continue;
	      applyPropertyRestrictions(pEdge);
	      if( pNode->isMerged() )
		break;
	    }
	}
      
      if( m_aMergeList.size() > 0 )
	mergeFirst();

      m_pABox->m_iCurrentBranchIndex = iBranch;
    }

}

