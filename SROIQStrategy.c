#include "SROIQStrategy.h"
#include "Blocking.h"
#include "DependencySet.h"
#include "Clash.h"
#include "ABox.h"
#include "ReazienerUtils.h"
#include "Params.h"
#include "Branch.h"
#include "KnowledgeBase.h"
#include "DependencyIndex.h"
#include "Expressivity.h"
#include "CompletionQueue.h"
#include "QueueElement.h"
#include "Blocking.h"

extern int g_iCommentIndent;
extern KnowledgeBase* g_pKB;

SROIQStrategy::SROIQStrategy(ABox* pABox) : CompletionStrategy(pABox, new DoubleBlocking())
{

}

SROIQStrategy::SROIQStrategy(ABox* pABox, Blocking* pBlocking) : CompletionStrategy(pABox, pBlocking)
{
}

bool SROIQStrategy::backtrack()
{
  START_DECOMMENT2("SROIQStrategy::backtrack");
  bool bBranchFound = FALSE;

  while( !bBranchFound )
    {
      int iLastBranch = m_pABox->getClash()->m_pDepends->getMax();

      if( iLastBranch <= 0 )
	{
	  DECOMMENT("RETURN bBranchFound=0 (1)");
	  END_DECOMMENT("SROIQStrategy::backtrack(1)");
	  return FALSE;
	}
      else if( iLastBranch > m_pABox->m_aBranches.size() )
	{
	  for(int i = 0; i < m_pABox->getClash()->m_pDepends->m_bitsetDepends.size(); i++ )
	    printf("i=%d\n", m_pABox->getClash()->m_pDepends->m_bitsetDepends.at(i)?1:0);
	  
	  char cErrorMsg[1024];
	  sprintf(cErrorMsg, "Backtrack: Trying to backtrack to branch %d but has only %d branches", iLastBranch, m_pABox->m_aBranches.size());
	  assertFALSE(cErrorMsg);
	}
      else if( PARAMS_USE_INCREMENTAL_DELETION() )
	{
	  //get the last branch
	  Branch* pBranch = m_pABox->m_aBranches.at(iLastBranch-1);
	  
	  // if this is the last disjunction, merge pair, etc. for the branch 
	  // (i.e, br.tryNext == br.tryCount-1)  and there are no other branches 
	  // to test (ie. abox.getClash().depends.size()==2), 
	  // then update depedency index and return false
	  if( (pBranch->m_iTryNext == pBranch->m_iTryCount-1) && m_pABox->getClash()->m_pDepends->size() == 2 )
	    {
	      g_pKB->m_pDependencyIndex->addCloseBranchDependency(pBranch, m_pABox->getClash()->m_pDepends);
	      DECOMMENT("RETURN bBranchFound=0 (2)");
	      END_DECOMMENT("SROIQStrategy::backtrack(2)");
	      return FALSE;
	    }
	  
	}

      //CHW - added for incremental deletion support
      if( PARAMS_USE_TRACING() && PARAMS_USE_INCREMENTAL_CONSISTENCE() )
	{
	  //we must clean up the KB dependecny index
	  int iSize = m_pABox->m_aBranches.size();
	  for(int i = iSize-1; i >= iLastBranch; i-- )
	    {
	      Branch* pBr = m_pABox->m_aBranches.at(i);
	      //remove from the dependency index
	      g_pKB->m_pDependencyIndex->removeBranchDependencies(pBr);
	      m_pABox->m_aBranches.pop_back();
	    }
	}
      else
	{
	  //old approach
	  int iSize = m_pABox->m_aBranches.size();
	  for(int i = iLastBranch; i < iSize; i++ )
	    m_pABox->m_aBranches.pop_back();
	}

      Branch* pNewBranch = m_pABox->m_aBranches.at(iLastBranch-1);
      if( iLastBranch != pNewBranch->m_iBranch )
	assertFALSE("Backtrack: Trying to backtrack to branch 'iLastBranch' but got 'pNewBranch->m_iBranch'");
      
      if( pNewBranch->m_iTryNext < pNewBranch->m_iTryCount )
	pNewBranch->setLastClash(m_pABox->m_pClash->m_pDepends);

      pNewBranch->m_iTryNext++;
      
      if( pNewBranch->m_iTryNext < pNewBranch->m_iTryCount )
	{
	  restore(pNewBranch);
	  bBranchFound = pNewBranch->tryNext();
	}
      else
	m_pABox->getClash()->m_pDepends->remove(iLastBranch);
    }

  DECOMMENT1("RETURN bBranchFound=%d (3)", bBranchFound);
  END_DECOMMENT("SROIQStrategy::backtrack(3)");
  return bBranchFound;
}

ABox* SROIQStrategy::complete()
{
  START_DECOMMENT2("SROIQStrategy::complete");
  // FIXME the expressivity of the completion graph might be different
  // than th original ABox
  Expressivity* pExpressivity = g_pKB->getExpressivity();
  bool bFullDatatypeReasoning = (PARAMS_USE_FULL_DATATYPE_REASONING() && (pExpressivity->hasCardinalityD() || pExpressivity->hasKeys()));
  
  initialize();
  
  while(!m_pABox->m_bIsComplete )
    {
      while( m_pABox->m_bChanged && !m_pABox->isClosed() )
	{
	  m_pABox->m_bChanged = FALSE;


	  if( !PARAMS_USE_PSEUDO_NOMINALS() )
	    {
	      if( PARAMS_USE_COMPLETION_QUEUE() )
		{
		  //init the end pointer for the queue
		  m_pABox->m_pCompletionQueue->init(CompletionQueue::NOMLIST);
		  while( m_pABox->m_pCompletionQueue->hasNext(CompletionQueue::NOMLIST) )
		    {
		      applyNominalRule((QueueElement*)m_pABox->m_pCompletionQueue->getNext(CompletionQueue::NOMLIST));
		      if( m_pABox->isClosed() ) break;
		    }
		}
	      else
		applyNominalRuleOnIndividuals();
	      if( m_pABox->isClosed() ) break;
	    }

	  if( PARAMS_USE_COMPLETION_QUEUE() )
	    {
	      //init the end pointer for the queue
	      m_pABox->m_pCompletionQueue->init(CompletionQueue::GUESSLIST);
	      while( m_pABox->m_pCompletionQueue->hasNext(CompletionQueue::GUESSLIST) )
		{
		  applyGuessingRule((QueueElement*)m_pABox->m_pCompletionQueue->getNext(CompletionQueue::GUESSLIST));
		  if( m_pABox->isClosed() ) break;
		}
	    }
	  else
	    applyGuessingRuleOnIndividuals();
	  if( m_pABox->isClosed() ) break;

	  if( PARAMS_USE_COMPLETION_QUEUE() )
	    {
	      m_pABox->m_pCompletionQueue->init(CompletionQueue::CHOOSELIST);
	      while( m_pABox->m_pCompletionQueue->hasNext(CompletionQueue::CHOOSELIST) )
		{
		  applyChooseRule((QueueElement*)m_pABox->m_pCompletionQueue->getNext(CompletionQueue::CHOOSELIST));
		  if( m_pABox->isClosed() ) break;
		}
	    }
	  else
	    applyChooseRuleOnIndividuals();
	  if( m_pABox->isClosed() ) break;

	  if( PARAMS_USE_COMPLETION_QUEUE() )
	    {
	      m_pABox->m_pCompletionQueue->init(CompletionQueue::MAXLIST);
	      while( m_pABox->m_pCompletionQueue->hasNext(CompletionQueue::MAXLIST) )
		{
		  applyMaxRule((QueueElement*)m_pABox->m_pCompletionQueue->getNext(CompletionQueue::MAXLIST));
		  if( m_pABox->isClosed() ) break;
		}
	    }
	  else
	    applyMaxRuleOnIndividuals();
	  if( m_pABox->isClosed() ) break;

	  if( bFullDatatypeReasoning )
	    {
	      if( PARAMS_USE_COMPLETION_QUEUE() )
		{
		  m_pABox->m_pCompletionQueue->init(CompletionQueue::DATATYPELIST);
		  while( m_pABox->m_pCompletionQueue->hasNext(CompletionQueue::DATATYPELIST) )
		    {
		      checkDatatypeCount((QueueElement*)m_pABox->m_pCompletionQueue->getNext(CompletionQueue::DATATYPELIST));
		      m_pABox->m_pCompletionQueue->init(CompletionQueue::DATATYPELIST);
		      if( m_pABox->isClosed() ) break;
		    }
		}
	      else
		checkDatatypeCountOnIndividuals();
	      if( m_pABox->isClosed() ) break;

	      if( PARAMS_USE_COMPLETION_QUEUE() )
		{
		  m_pABox->m_pCompletionQueue->init(CompletionQueue::LITERALLIST);
		  while( m_pABox->m_pCompletionQueue->hasNext(CompletionQueue::LITERALLIST) )
		    {
		      applyLiteralRule((QueueElement*)m_pABox->m_pCompletionQueue->getNext(CompletionQueue::LITERALLIST));
		      if( m_pABox->isClosed() ) break;
		    }
		}
	      else
		applyLiteralRuleOnIndividuals();
	      if( m_pABox->isClosed() ) break;
	    }
	  
	  if( PARAMS_USE_COMPLETION_QUEUE() )
	    {
	      m_pABox->m_pCompletionQueue->init(CompletionQueue::ATOMLIST);
	      while( m_pABox->m_pCompletionQueue->hasNext(CompletionQueue::ATOMLIST) )
		{
		  applyUnfoldingRule((QueueElement*)m_pABox->m_pCompletionQueue->getNext(CompletionQueue::ATOMLIST));
		  if( m_pABox->isClosed() ) break;
		}
	    }
	  else
	    applyUnfoldingRuleOnIndividuals();
	  if( m_pABox->isClosed() ) break;
	  
	  if( PARAMS_USE_COMPLETION_QUEUE() )
	    {
	      m_pABox->m_pCompletionQueue->init(CompletionQueue::ORLIST);
	      while( m_pABox->m_pCompletionQueue->hasNext(CompletionQueue::ORLIST) )
		{
		  applyDisjunctionRule((QueueElement*)m_pABox->m_pCompletionQueue->getNext(CompletionQueue::ORLIST));
		  if( m_pABox->isClosed() ) break;
		}
	    }
	  else
	    applyDisjunctionRuleOnIndividuals();
	  if( m_pABox->isClosed() ) break;
	  
	  if( PARAMS_USE_COMPLETION_QUEUE() )
	    {
	      m_pABox->m_pCompletionQueue->init(CompletionQueue::SOMELIST);
	      while( m_pABox->m_pCompletionQueue->hasNext(CompletionQueue::SOMELIST) )
		{
		  applySomeValuesRule((QueueElement*)m_pABox->m_pCompletionQueue->getNext(CompletionQueue::SOMELIST));
		  if( m_pABox->isClosed() ) break;
		}
	    }
	  else
	    applySomeValuesRuleOnIndividuals();
	  if( m_pABox->isClosed() ) break;
	  
	  if( PARAMS_USE_COMPLETION_QUEUE() )
	    {
	      m_pABox->m_pCompletionQueue->init(CompletionQueue::MINLIST);
	      while( m_pABox->m_pCompletionQueue->hasNext(CompletionQueue::MINLIST) )
		{
		  applyMinRule((QueueElement*)m_pABox->m_pCompletionQueue->getNext(CompletionQueue::MINLIST));
		  if( m_pABox->isClosed() ) break;
		}
	    }
	  else
	    applyMinRuleOnIndividuals();
	  if( m_pABox->isClosed() ) break;
	}

      if( m_pABox->isClosed() )
	{
	  if( backtrack() )
	    m_pABox->setClash(NULL);
	  else
	    m_pABox->m_bIsComplete = TRUE;
	}
      else
	{
	  if( PARAMS_SATURATE_TABLEAU() )
	    {
	      Branch* pUnexploredBranch = NULL;
	      int iSize = m_pABox->m_aBranches.size(), iIndex = 0;
	      for(BranchList::iterator i = m_pABox->m_aBranches.begin(); i != m_pABox->m_aBranches.end() && iIndex < iSize; i++, iIndex++ )
		{
		  pUnexploredBranch = (Branch*)*i;
		  pUnexploredBranch->m_iTryNext++;
		  
		  if( pUnexploredBranch->m_iTryNext < pUnexploredBranch->m_iTryCount )
		    {
		      restore(pUnexploredBranch);
		      pUnexploredBranch->tryNext();
		      break;
		    }
		  else
		    {
		      m_pABox->m_aBranches.erase(i);
		      pUnexploredBranch = NULL;
		    }
		}

	      if( pUnexploredBranch == NULL )
		m_pABox->m_bIsComplete = TRUE;
	    }
	  else
	    m_pABox->m_bIsComplete = TRUE;
	}
    }

  END_DECOMMENT("SROIQStrategy::complete");
  return m_pABox;
}

bool SROIQStrategy::supportsPseudoModelCompletion()
{
  return TRUE;
}
