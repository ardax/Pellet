#include "TransitionGraph.h"
#include "Utils.h"
#include "Transition.h"
#include "ReazienerUtils.h"

TransitionGraph::TransitionGraph()
{
  m_pInitialState = NULL;
}

bool TransitionGraph::isConnected()
{
  StateSet visited;
  StateList stack;

  stack.push_back(m_pInitialState);
  visited.insert(m_pInitialState);

  while(stack.size()>0)
    {
      State* pState = stack.front();
      stack.pop_front();

      if( m_setAllStates.find(pState) == m_setAllStates.end() )
	return FALSE;

      for(TransitionSet::iterator i = pState->m_setTransitions.begin(); i != pState->m_setTransitions.end(); i++ )
	{
	  Transition* pTransition = (Transition*)*i;
	  if( visited.insert(pTransition->m_pTo).second )
	    stack.push_front(pTransition->m_pTo);
	}
    }

  return (visited.size()==m_setAllStates.size());
}

bool TransitionGraph::isDeterministic()
{
  if( m_setAllStates.find(m_pInitialState) == m_setAllStates.end() )
    assertFALSE("InternalReasonerException");
  
  for(StateSet::iterator i = m_setAllStates.begin(); i != m_setAllStates.end(); i++ )
    {
      State* pState = (State*)*i;
      PointerSet setSeenSymbols;
      for(TransitionSet::iterator j = pState->m_setTransitions.begin(); j != pState->m_setTransitions.end(); j++ )
	{
	  Transition* pT = (Transition*)*j;
	  void* pSymbol = pT->m_pName;

	  if( pSymbol == NULL || setSeenSymbols.find(pSymbol) != setSeenSymbols.end() )
	    return FALSE;
	  setSeenSymbols.insert(pSymbol);
	}
    }
  return TRUE;
}

// convert NFA into equivalent DFA
TransitionGraph* TransitionGraph::determinize()
{
  // Define a map for the new states in DFA. The key for the
  // elements in map is the set of NFA states and the value
  // is the new state in DFA
  map<StateSet*, State*> dStates;

  // start state of DFA is epsilon closure of start state in NFA
  State* pS = new State;
  StateSet* pSS = epsilonClosure(m_pInitialState, (new StateSet()));

  // unmarked states in dStates will be processed
  pS->m_bMarked = FALSE;
  dStates[pSS] = pS;
  m_pInitialState = pS;
  
  // if there are unprocessed states continue
  bool bHasUnmarked = TRUE;
  while(bHasUnmarked)
    {
      State* pU = NULL;
      StateSet* pSetU = NULL;

      bHasUnmarked = FALSE;

      for(map<StateSet*, State*>::iterator i = dStates.begin(); i != dStates.end(); i++ )
	{
	  pSS = i->first;
	  pS = i->second;

	  bHasUnmarked = !pS->m_bMarked;
	  if( bHasUnmarked )
	    break;
	}

      if( bHasUnmarked )
	{
	  // for each symbol in alphabet
	  for(PointerSet::iterator i = m_setAlphabet.begin(); i != m_setAlphabet.end(); i++ )
	    {
	      void* pA = (void*)*i;
	      
	      // find epsilon closure of move with a
	      pSetU = epsilonClosure( move(pSS, pA) );
	      // if result is empty continue
	      if( pSetU->size() == 0 )
		continue;

	      // check if this set of NFA states are
	      // already in dStates
	      if( dStates.find(pSetU) != dStates.end() )
		pU = (State*)(dStates.find(pSetU)->second);
	      else
		pU = NULL;
	      
	      // if the result is equal to NFA states
	      // associated with the processed state
	      // then add an edge from s to itself
	      // else create a new state and add edge
	      if( pU == NULL )
		{
		  pU = new State;
		  pU->m_bMarked = FALSE;
		  dStates[pSetU] = pU;
		}
	      else if( isEqualState(pU, pS) == 0 )
		pU = pS;
	      pS->addTransition(pA, pU);
	    }

	  // update s in dStates (since key is unchanged only
	  // the changed value i.e state s is updated in dStates)
	  pS->m_bMarked = TRUE;
	  dStates[pSS] = pS;
	}
    }

  // a set of final states for DFA
  StateSet setAcceptingStates;
  // clear all states
  m_setAllStates.clear();

  for(map<StateSet*, State*>::iterator i = dStates.begin(); i != dStates.end(); i++ )
    {
      // find DFA state and corresponding set of NFA states
      StateSet* pSS = i->first;
      State* pS = i->second;
      // add DFA state to state set
      m_setAllStates.insert(pS);
      // if any of NFA states are final update accepting states
      if( isFinal(pSS) )
	setAcceptingStates.insert(pS);
    }

  // accepting states becomes final states
  m_setFinalStates.clear();
  m_setFinalStates = setAcceptingStates;

  return this;
}

// USER DEFINED FUNCTION
// compute from a set of states, the states that are
// reachable by any number of edges labeled epsilon
// from only one state
StateSet* TransitionGraph::epsilonClosure(State* pS, StateSet* pResult)
{
  pResult->insert(pS);
  for(TransitionSet::iterator i = pS->m_setTransitions.begin(); i != pS->m_setTransitions.end(); i++ )
    {
      Transition* pE = (Transition*)*i;
      
      // if this is an epsilon transition and the result
      // does not contain 'to' state then add the epsilon
      // closure of 'to' state to the result set
      if( pE->hasName(NULL) && pResult->find(pE->m_pTo) == pResult->end() )
	pResult = epsilonClosure(pE->m_pTo, pResult);
    }
  return pResult;
}

// compute from a set of states, the states that are
// reachable by any number of edges labeled epsilon
StateSet* TransitionGraph::epsilonClosure(StateSet* pSS)
{
  StateSet* pResult = new StateSet;
  for(StateSet::iterator i = pSS->begin(); i != pSS->end(); i++ )
    {
      State* pState = (State*)*i;
      pResult = epsilonClosure(pState, pResult);
    }
  return pResult;
}

// compute a NFA move from a set of states
// to states that are reachable by one edge labeled c
StateSet* TransitionGraph::move(StateSet* pSS, void* pC)
{
  StateSet* pResult = new StateSet;
  
  // for all the states in the set SS
  for(StateSet::iterator i = pSS->begin(); i != pSS->end(); i++ )
    {
      State* pState = (State*)*i;
      // for all the edges from state st
      for(TransitionSet::iterator j = pState->m_setTransitions.begin(); j != pState->m_setTransitions.end(); j++ )
	{
	  Transition* pE = (Transition*)*j;
	  // add the 'to' state if transition matches
	  if( pE->hasName(pC) )
	    pResult->insert( pE->m_pTo );
	}
    }
  
  return pResult;
}

// given a DFA, produce an equivalent minimized DFA
TransitionGraph* TransitionGraph::minimize()
{
  // partitions are set of states, where max # of sets = # of states
  StateSet* aPartitionSets = new StateSet[m_setAllStates.size()];
  int iNumPartitions = 1;

  // first partition is the set of final states
  aPartitionSets[0].insert(m_setFinalStates.begin(), m_setFinalStates.end());
  setPartition(&aPartitionSets[0], 0);
  
  // check if there are any states that are not final
  if( aPartitionSets[0].size() < m_setAllStates.size() )
    {
      // second partition is set of non-accepting states
      aPartitionSets[1].insert(m_setAllStates.begin(), m_setAllStates.end());
      for(StateSet::iterator i = m_setFinalStates.begin();  i != m_setFinalStates.end(); i++ )
	aPartitionSets[1].erase((State*)*i);
      setPartition(&aPartitionSets[1], 1);
      iNumPartitions++;
    }

  for(int i = 0; i < iNumPartitions; i++ )
    {
      // for all the states in a partition
      StateSet::iterator j = aPartitionSets[i].begin();
      State* pS = (State*)*j;
      j++;
      bool bPartitionCreated = FALSE;

      for(; j != aPartitionSets[i].end(); j++ )
	{
	  State* pT = (State*)*j;

	  // for all the symbols in an alphabet
	  for(PointerSet::iterator k = m_setAlphabet.begin(); k != m_setAlphabet.end(); k++ )
	    {
	      void* pA = (void*)*k;
	      
	      // find move(a) for the first and current state
	      int iSN = (pS->dMove(pA)==NULL)?-1:pS->dMove(pA)->m_iPartitionNum;
	      int iTN = (pT->dMove(pA)==NULL)?-1:pT->dMove(pA)->m_iPartitionNum;

	      // if they go to different partitions
	      if( iSN != iTN )
		{
		  // if a new partition was not created in this iteration
		  // create a new partition
		  if( !bPartitionCreated )
		    iNumPartitions++;
		  bPartitionCreated = TRUE;
		  // remove current state from this partition
		  aPartitionSets[i].erase(pT);
		  // add it to the new partition
		  aPartitionSets[iNumPartitions-1].insert(pT);
		  break;
		}
	    }
	}
      if( bPartitionCreated )
	{
	  // set the partition_num for all the states put into new partition
	  setPartition(&aPartitionSets[iNumPartitions-1], (iNumPartitions-1));
	  // start checking from the first partition
	  i = 0;
	}
    }

  // store the partition_num of the start state
  int iStartPartition = m_pInitialState->m_iPartitionNum;

  // for each partition the first state is marked as the representative 
  // of that partition and rest is removed from states
  for(int i = 0; i < iNumPartitions; i++ )
    {
      StateSet::iterator j = aPartitionSets[i].begin();
      State* pS = (State*)*j;
      j++;
      pS->m_pRep = pS;

      if( i == iStartPartition )
	m_pInitialState = pS;

      for(; j != aPartitionSets[i].end(); j++ )
	{
	  State* pT = (State*)*j;
	  m_setAllStates.erase(pT);
	  m_setFinalStates.erase(pT);
	  // set rep so that we can later update 
	  // edges to this state
	  pT->m_pRep = pS;
	}
    }

  // correct any edges that are going to states that are removed, 
  // by updating the target state to be the rep of partition which
  // dead state belonged to
  for(StateSet::iterator i = m_setAllStates.begin(); i != m_setAllStates.end(); i++ )
    {
      State* pT = (State*)*i;
      for(TransitionSet::iterator t = pT->m_setTransitions.begin(); t != pT->m_setTransitions.end(); t++ )
	{
	  Transition* pEdge = (Transition*)*t;
	  pEdge->m_pTo = pEdge->m_pTo->m_pRep;
	}
    }

  return this;
}

// renumber states of TG in preorder, beginning at start state
TransitionGraph* TransitionGraph::renumber()
{
  for(StateSet::iterator i = m_setAllStates.begin(); i != m_setAllStates.end(); i++ )
    {
      State* pState = (State*)*i;
      pState->m_iPartitionNum = 0;
      pState->m_bMarked = FALSE;
    }

  StateList listWork;
  int iVal = 0;
  listWork.push_front(m_pInitialState);
  
  while(listWork.size()>0)
    {
      State* pState = listWork.front();
      listWork.pop_front();
      pState->m_iName = iVal++;
      pState->m_bMarked = TRUE;

      for(TransitionSet::iterator t = pState->m_setTransitions.begin(); t != pState->m_setTransitions.end(); t++ )
	{
	  Transition* pEdge = (Transition*)*t;
	  if( !pEdge->m_pTo->m_bMarked )
	    listWork.push_back(pEdge->m_pTo);
	}
    }

  return this;
}
  
StatePairList* TransitionGraph::findTransitions(void* pTransition, StatePairList* pStatePairList)
{
  StatePairList* pResult = new StatePairList;
  for(StateSet::iterator i = m_setAllStates.begin(); i != m_setAllStates.end(); i++ )
    {
      State* pState = (State*)*i;
      State* pState2 = pState->dMove(pTransition);
      if( pState2 )
	{
	  StatePair* pStatePair = new StatePair;
	  pStatePair->first = pState;
	  pStatePair->second = pState2;
	  pResult->push_back(pStatePair);
	}
    }
  return pResult;
}

TransitionGraph* TransitionGraph::insert(TransitionGraph* pT, State* pInitial, State* pFinal)
{
  TransitionGraph* pTCopy = pT->copy();

  // combine all states
  m_setAllStates.insert(pTCopy->m_setAllStates.begin(), pTCopy->m_setAllStates.end());

  // combine the alphabets
  m_setAlphabet.insert(pTCopy->m_setAlphabet.begin(), pTCopy->m_setAlphabet.end());
  
  // add an epsilon edge from the given initial state to
  // current TG's initial state
  pInitial->addTransition( pT->m_pInitialState);

  // from all final states add an epsilon edge to new final state
  for(StateSet::iterator i = pT->m_setFinalStates.begin(); i != pT->m_setFinalStates.end(); i++ )
    {
      State* pState = (State*)*i;
      pState->addTransition(pFinal);
    }

  return this;
}

void TransitionGraph::addTransition(State* pBegin, void* pTransition, State* pEnd)
{
  pBegin->addTransition(pTransition, pEnd);
  
  // transition != Transition.EPSILON
  if( pTransition != NULL )
    m_setAlphabet.insert(pTransition);
}

void TransitionGraph::addTransition(State* pBegin, State* pEnd)
{
  pBegin->addTransition(pEnd);
}

void TransitionGraph::addFinalState(State* pFinal)
{
  m_setFinalStates.insert(pFinal);
}

State* TransitionGraph::createNewState()
{
  State* pState = new State;
  m_setAllStates.insert(pState);
  return pState;
}

PointerSet* TransitionGraph::getAlphabet()
{
  return &(m_setAlphabet);
}

bool TransitionGraph::isFinal(State* pState)
{
  return (m_setFinalStates.find(pState) != m_setFinalStates.end());
}

// test whether Set<State> is DFA final state (contains NFA final state)
bool TransitionGraph::isFinal(StateSet* pStateSet)
{
  for(StateSet::iterator i = pStateSet->begin(); i != pStateSet->end(); i++ )
    {
      State* pState = (State*)*i;
      if( m_setFinalStates.find(pState) != m_setFinalStates.end() )
	return TRUE;
    }
  return FALSE;
}

void TransitionGraph::setPartition(StateSet* pStateSet, int iNum)
{
  for(StateSet::iterator i = pStateSet->begin(); i != pStateSet->end(); i++ )
    ((State*)*i)->m_iPartitionNum = iNum;
}

State* TransitionGraph::getFinalState()
{
  if( m_setFinalStates.size() == 0 )
    assertFALSE("There are no final states!");
  else if( m_setFinalStates.size() == 1 )
    assertFALSE("There is more than one final state!");

  return (State*)(*m_setFinalStates.begin());
}

TransitionGraph* TransitionGraph::copy()
{
  TransitionGraph* pCopyTG = new TransitionGraph();
  pCopyTG->m_setAlphabet = m_setAlphabet;
  
  StateMap mapNewStates;
  for(StateSet::iterator i = m_setAllStates.begin(); i != m_setAllStates.end(); i++ )
    {
      State* pS1 = (State*)*i;
      State* pN1 = NULL;
      StateMap::iterator iFind = mapNewStates.find(pS1);
      if( iFind != mapNewStates.end() )
	pN1 = (State*)iFind->second;

      if( pN1 == NULL )
	{
	  pN1 = pCopyTG->createNewState();
	  mapNewStates[pS1] = pN1;
	}

      if( m_setFinalStates.find(pS1) != m_setFinalStates.end() )
	pCopyTG->m_setFinalStates.insert(pN1);

      for(TransitionSet::iterator t = pS1->m_setTransitions.begin(); t != pS1->m_setTransitions.end(); t++ )
	{
	  Transition* pT = (Transition*)*t;
	  State* pS2 = pT->m_pTo;
	  void* pSymbol = pT->m_pName;

	  State* pN2 = NULL;
	  StateMap::iterator iFind = mapNewStates.find(pS2);
	  if( iFind != mapNewStates.end() )
	    pN2 = (State*)iFind->second;

	  if( pN2 == NULL )
	    {
	      pN2 = pCopyTG->createNewState();
	      mapNewStates[pS2] = pN2;
	    }
	  
	  if( pSymbol == NULL )
	    pN1->addTransition(pN2);
	  else
	    pN1->addTransition(pSymbol, pN2);
	}
    }

  pCopyTG->m_pInitialState = NULL;
  StateMap::iterator iFind = mapNewStates.find(m_pInitialState);
  if( iFind != mapNewStates.end() )
    pCopyTG->m_pInitialState = (State*)iFind->second;

  return pCopyTG;
}

// -------------------------------------------------------------//
// -------------------------------------------------------------//
// --------------Make changes past this point-------------------//
// -------------------------------------------------------------//
// -------------------------------------------------------------//

// ---------------------------------------------------
// modify TG so that it accepts strings accepted by
// either current TG or new TG

TransitionGraph* TransitionGraph::choice(TransitionGraph* pTG)
{
  State* pStart = createNewState();
  State* pFinal = createNewState();

  // combine all states and final states
  m_setAllStates.insert(pTG->m_setAllStates.begin(), pTG->m_setAllStates.end());
  m_setFinalStates.insert(pTG->m_setFinalStates.begin(), pTG->m_setFinalStates.end());

  // add an epsilon edge from new start state to
  // current TG's and parameter TG's start state
  pStart->addTransition(m_pInitialState);
  pStart->addTransition(pTG->m_pInitialState);
  m_pInitialState = pStart;

  // from all final states add an epsilon edge to new final state
  for(StateSet::iterator i = m_setFinalStates.begin(); i != m_setFinalStates.end(); i++ )
    {
      State* pState = (State*)*i;
      pState->addTransition(pFinal);
    }
  
  // make f the only final state
  m_setFinalStates.clear();
  m_setFinalStates.insert(pFinal);

  // combine the alphabets
  m_setAlphabet.insert(pTG->m_setAlphabet.begin(), pTG->m_setAlphabet.end());

  return this;
}
