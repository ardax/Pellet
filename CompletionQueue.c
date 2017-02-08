#include "CompletionQueue.h"
#include "QueueElement.h"
#include "KnowledgeBase.h"
#include "ABox.h"

extern KnowledgeBase* g_pKB;

CompletionQueue::CompletionQueue()
{
  m_pABox = NULL;
}

CompletionQueue::~CompletionQueue()
{
}

CompletionQueue* CompletionQueue::copy()
{
  CompletionQueue* pCopy = new CompletionQueue();
  pCopy->m_pABox = m_pABox;

  //copy all the queues
  for(int i = 0; i < 11; i++ )
    {
      pCopy->m_Queue[i] = m_Queue[i];
      pCopy->m_gQueue[i] = m_gQueue[i];
      pCopy->m_gCurrent[i] = m_gCurrent[i];
      pCopy->m_Current[i] = m_Current[i];
      pCopy->m_CutOff[i] = m_CutOff[i];
    }

  //copy branch information
  for( vector<ICollections*>::iterator i = m_aBranches.begin(); i != m_aBranches.end(); i++ )
    {
      ICollections* pBranchArray = (ICollections*)*i;
      ICollections* pNewBranchArray = new ICollections;
      for(ICollections::iterator j = pBranchArray->begin(); j != pBranchArray->end(); j++ )
	{
	  ICollection* pBranch = (ICollection*)*j;
	  ICollection* pNewBranch = getNewICollection(pBranch);
	  pNewBranchArray->push_back(pNewBranch);
	}
      pCopy->m_aBranches.push_back(pNewBranchArray);
    }
  
  //copy branch effects
  for( vector<ExprNodeSet*>::iterator i = m_listBranchEffects.begin(); i != m_listBranchEffects.end(); i++ )
    {
      ExprNodeSet* pSet = (ExprNodeSet*)*i;
      ExprNodeSet* pNewSet = new ExprNodeSet;
      pNewSet->insert(pSet->begin(), pSet->end());
      pCopy->m_listBranchEffects.push_back(pNewSet);
    }

  return pCopy;
}

// Used to track individuals affect during each branch - needed for backjumping. 
void CompletionQueue::addEffected(int iBranchIndex, ExprNode* pExprNode)
{
  if( iBranchIndex <= 0 ) return;

  if( iBranchIndex < m_listBranchEffects.size() )
    {
      ExprNodeSet* pSet = m_listBranchEffects[iBranchIndex];
      if( pSet == NULL )
	{
	  pSet = new ExprNodeSet;
	  m_listBranchEffects[iBranchIndex] = pSet;
	}
      pSet->insert(pExprNode);
    }
  else
    {
      // expand the list
      for(int i = 0; i < (iBranchIndex-m_listBranchEffects.size()); i++ )
	m_listBranchEffects.push_back(NULL);
      m_listBranchEffects.push_back(new ExprNodeSet);
      m_listBranchEffects[iBranchIndex]->insert(pExprNode);
    }
}

void CompletionQueue::removeEffect(int iBranchIndex, ExprNodeSet* pEffected)
{
  int iIndex = 0;
  for(vector<ExprNodeSet*>::iterator i = m_listBranchEffects.begin(); i !=  m_listBranchEffects.end(); i++ )
    {
      if( iIndex >= iBranchIndex )
	{
	  ExprNodeSet* pSet = (ExprNodeSet*)*i;
	  pEffected->insert(pSet->begin(), pSet->end());
	  m_listBranchEffects[iIndex] = new ExprNodeSet;
	}
      iIndex++;
    }
}

void CompletionQueue::add(QueueElement* pElement, int iType)
{
  if( g_pKB->m_pABox->m_bSyntacticUpdate )
    m_gQueue[iType].push_back(pElement);
  else
    m_Queue[iType].push_back(pElement);
}

void CompletionQueue::init(int iType)
{
  m_CutOff[iType] = m_gQueue[iType].size()+m_Queue[iType].size();
}

bool CompletionQueue::hasNext(int iType)
{
  findNext(iType);
  
  if( ((m_Current[iType] < m_Queue[iType].size()) || (m_gCurrent[iType] < m_gQueue[iType].size())) &&
      ((m_gCurrent[iType]+m_Current[iType]) < m_CutOff[iType]) )
    return TRUE;
  return FALSE;
}

void* CompletionQueue::getNext(int iType)
{
  findNext(iType);

  QueueElement* pElement = NULL;
  if( m_gCurrent[iType] < m_gQueue[iType].size() )
    pElement = (QueueElement*)m_gQueue[iType].at( m_gCurrent[iType]++ );
  else
    pElement = (QueueElement*)m_Queue[iType].at( m_Current[iType]++ );
  return pElement;
}

void CompletionQueue::findNext(int iType)
{
  bool bFound = FALSE;

  // first search the global queue - this is done due to incremental 
  // additions as well as additions to the abox in ABox.isConsistent()
  for(; m_gCurrent[iType] < m_gQueue[iType].size() && (m_gCurrent[iType] + m_Current[iType]) < m_CutOff[iType]; m_gCurrent[iType]++ )
    {
      //Get next node from queue
      QueueElement* pNext = NULL;

      //get element from global queue
      pNext = (QueueElement*)m_gQueue[iType].at(m_gCurrent[iType]);
      Node* pNode = NULL;
      Expr2NodeMap::iterator iFind = m_pABox->m_mNodes.find(pNext->m_pNode);
      if( iFind != m_pABox->m_mNodes.end() )
	pNode = (Node*)iFind->second;

      if( pNode == NULL )
	continue;
      
      //run down actual node on the fly
      pNode = pNode->getSame();

      // TODO: This could most likely be removed and only done once 
      // when an element is popped off the queue
      if( iType == LITERALLIST && !pNode->isIndividual() && !pNode->isPruned() )
	{
	  bFound = TRUE;
	  break;
	}
      else if( pNode->isIndividual() && !pNode->isPruned() )
	{
	  bFound = TRUE;
	  break;
	}
    }

  //return if found an element on the global queue
  if( bFound )
    return;

  //similarly we must check the regular queue for any elements
  for(; m_Current[iType] < m_Queue[iType].size() && (m_gCurrent[iType] + m_Current[iType]) < m_CutOff[iType]; m_Current[iType]++ )
    {
      //Get next node from queue
      QueueElement* pNext = NULL;
      
      //get element off of the normal queue
      pNext = (QueueElement*)m_Queue[iType].at(m_Current[iType]);
      Node* pNode = NULL;
      Expr2NodeMap::iterator iFind = m_pABox->m_mNodes.find(pNext->m_pNode);
      if( iFind != m_pABox->m_mNodes.end() )
	pNode = (Node*)iFind->second;

      if( pNode == NULL )
	continue;
      
      //run down actual node on the fly
      pNode = pNode->getSame();

      // TODO: This could most likely be removed and only done once 
      // when an element is popped off the queue
      if( iType == LITERALLIST && !pNode->isIndividual() && !pNode->isPruned() )
	{
	  break;
	}
      else if( pNode->isIndividual() && !pNode->isPruned() )
	{
	  break;
	}
    }
}

// Reset the queue to be the current nodes in the abox;
// Also reset the type index to 0
void CompletionQueue::restore(int iBranch)
{
  //clear the extra branches
  if( iBranch+1 < m_aBranches.size() )
    {
      for(int i = iBranch+1; i < m_aBranches.size(); i++ )
	m_aBranches.pop_back();
    }

  ICollections* pTheBranch = (ICollections*)m_aBranches.at(iBranch);
  int iBranchP = 0;

  //reset queues - currently do not reset guess list
  for(int i = 0; i < 11; i++ )
    {
      ICollection* pIndex = (ICollection*)pTheBranch->at(i);

      //set the current pointer
      m_Current[i] = getAtI(pIndex, 0);
      //get the branch pointer - use it to clear the queues
      iBranchP = getAtI(pIndex, 1);
      //get the end type pointer
      m_CutOff[i] = getAtI(pIndex, 2);
      //get the global queue pointer
      m_gCurrent[i] = getAtI(pIndex, 3);
      
      int iQueueSize = m_Queue[i].size();
      for(int i = min(iQueueSize, m_CutOff[i]); i < iQueueSize; i++ )
	m_Queue[i].pop_back();
    }
}

/**
 * Set branch pointers to current pointer. This 
 * is done whenever abox.incrementBranch is called 
 */
void CompletionQueue::incrementBranch(int iBranch)
{
  ICollections* pTheBranch;

  //if branch exists get it, else create new object
  if( iBranch < m_aBranches.size() )
    {
      pTheBranch = m_aBranches.at(iBranch);
      pTheBranch->clear();
    }
  else
    pTheBranch = new ICollections;

  //set all branch pointers to the be the current pointer of each queue
  for(int i = 0; i < 11; i++ )
    {
      ICollection* pBranch = getNewICollection();
      
      addI(pBranch, m_Current[i]);
      addI(pBranch, (m_gQueue[i].size() + m_Queue[i].size()));
      addI(pBranch, m_Queue[i].size()+1);
      addI(pBranch, m_gCurrent[i]);
      
      pTheBranch->push_back(pBranch);
    }

  if( iBranch < m_aBranches.size() )
    m_aBranches[iBranch] = pTheBranch;
  else
    {
      for(int i = 0; i <= (iBranch-m_aBranches.size()); i++ )
	m_aBranches.push_back(NULL);
      m_aBranches[iBranch] = pTheBranch;
    }
}
