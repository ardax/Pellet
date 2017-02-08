#ifndef _TRANSITION_
#define _TRANSITION_

#include <set>
using namespace std;

class State;

class Transition
{
 public:
  Transition(State* pTo);
  Transition(void* pName, State* pTo);

  bool hasName(const void* c) const;

  State* m_pTo;
  void* m_pName;
};

int isEqualTransition(const Transition* pTransition1, const Transition* pTransition2);
struct strCmpTransition
{
  bool operator()( const Transition* pTransition1, const Transition* pTransition2 ) const {
    return isEqualTransition( pTransition1, pTransition2 ) < 0;
  }
};
typedef set<Transition*> TransitionSet;

#endif
