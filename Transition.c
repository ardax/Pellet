#include "Transition.h"
#include "State.h"

Transition::Transition(State* pTo)
{
  m_pTo = pTo;
  m_pName = NULL;
}

Transition::Transition(void* pName, State* pTo)
{
  m_pName = pName;
  m_pTo = pTo;
}

bool Transition::hasName(const void* c) const
{
  if( m_pName == NULL )
    return (c==NULL);
  if( c == NULL )
    return FALSE;

  // CHECK HERE: Transition.c
  // we need to determine equality between c and name
  return (c==m_pName);
}

int isEqualTransition(const Transition* pTransition1, const Transition* pTransition2)
{
  if( pTransition1->hasName(pTransition2->m_pName) )
    return isEqualState(pTransition1->m_pTo, pTransition2->m_pTo);
  return 1;
}
