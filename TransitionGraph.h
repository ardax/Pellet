#ifndef _TRANSITION_GRAPH_
#define _TRANSITION_GRAPH_

#include "State.h"
#include "Role.h"

typedef set<void*> PointerSet;

class TransitionGraph
{
 public:
  TransitionGraph();

  bool isConnected();
  bool isDeterministic();

  bool isFinal(State* pState);
  bool isFinal(StateSet* pStateSet);

  TransitionGraph* copy();

  TransitionGraph* insert(TransitionGraph* pT, State* pInitial, State* pFinal);
  
  StateSet* move(StateSet* pSS, void* pObj);
  StateSet* epsilonClosure(State* pS, StateSet* pResult);
  StateSet* epsilonClosure(StateSet* pSS);
  
  TransitionGraph* choice(TransitionGraph* pTG);

  StatePairList* findTransitions(void* pTransition, StatePairList* pStatePairList);
  void addTransition(State* pBegin, void* pTransition, State* pEnd);
  void addTransition(State* pBegin, State* pEnd);
  void setInitialState(State* pInitial) { m_pInitialState = pInitial; }
  void addFinalState(State* pFinal);

  void setPartition(StateSet* pStateSet, int iNum);

  PointerSet* getAlphabet();

  State* createNewState();
  State* getFinalState();

  TransitionGraph* determinize();
  TransitionGraph* minimize();
  TransitionGraph* renumber();
  
  State* m_pInitialState;
  StateSet m_setFinalStates;
  StateSet m_setAllStates;
  PointerSet m_setAlphabet;
};

#endif
