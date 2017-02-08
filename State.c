#include "State.h"
#include "ReazienerUtils.h"

State::State()
{
  m_bMarked = FALSE;
  m_iPartitionNum = 0;
  m_pRep = NULL;
  m_iName = 0;
}

// add an edge to from current state to s on transition
void State::addTransition(void* pTransition, State* pS)
{
  if( pTransition == NULL || pS == NULL )
    assertFALSE("NullPointerException");

  Transition* pT = new Transition(pTransition, pS);
  m_setTransitions.insert(pT);
}

// add an edge to from current state to s on epsilon transition
void State::addTransition(State* pS)
{
  if( pS == NULL )
    assertFALSE("NullPointerException");

  Transition* pT = new Transition(pS);
  m_setTransitions.insert(pT);
}

State* State::dMove(void* pObj)
{
  for(TransitionSet::iterator t = m_setTransitions.begin(); t != m_setTransitions.end(); t++ )
    {
      Transition* pT = (Transition*)*t;
      if( pT->hasName(pObj) )
	return pT->m_pTo;
    }
  return NULL;
}

int isEqualState(const State* pState1, const State* pState2)
{
  return (pState1->m_iName-pState2->m_iName);
}
