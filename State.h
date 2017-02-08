#ifndef _STATE_
#define _STATE_

#include "Utils.h"
#include "Transition.h"
#include <list>
#include <set>
#include <map>
using namespace std;


class State
{
 public:
  State();

  void addTransition(void* pTransition, State* pS);
  void addTransition(State* pS);
  State* dMove(void* pObj);

  // set of outgoing edges from state
  TransitionSet m_setTransitions;
  
  // for epsilonClosure(), renumber()
  // flag whether state already processed
  bool m_bMarked;

  // for minimize()
  // number of partition
  int m_iPartitionNum;

  // representative state for partition
  State* m_pRep;

  // number of state
  int m_iName;
};

typedef pair<State*, State*> StatePair;
typedef list<StatePair*> StatePairList;

int isEqualState(const State* pState1, const State* pState2);
struct strCmpState
{
  bool operator()( const State* pState1, const State* pState2 ) const {
    return isEqualState( pState1, pState2 ) < 0;
  }
};
typedef set<State*, strCmpState> StateSet;
typedef map<State*, State*, strCmpState> StateMap;
typedef list<State*> StateList;



#endif
