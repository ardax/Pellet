#ifndef _EMPTY_SHN_STRATEGY_
#define _EMPTY_SHN_STRATEGY_

#include "SHOIQStrategy.h"
#include "ExpressionNode.h"

class ABox;
class Branch;
class Individual;


class EmptySHNStrategy : public SHOIQStrategy
{
 public:
  EmptySHNStrategy(ABox* pABox);

  enum
    {
      NONE = 0x00,
      HIT = 0x01,
      MISS = 0x02,
      FAIL = 0x04,
      ADD = 0x08,
      ALL = 0x0F
    };

  void initialize();
  ABox* complete();
  void restore(Branch* pBranch);
  bool backtrack();

  Individual* getNextIndividual();
  void expand(Individual* pX);
  int cachedSat(Individual* pX);
  ExprNode* createConcept(Individual* pX);
  void addBranch(Branch* pNewBranch);

  bool supportsPseudoModelCompletion() { return FALSE; }
  virtual void printStrategyType() { printf("Empty SHN Strategy\n"); }  

  Individual* m_pRoot;
  Nodes m_aMayNeedExpanding;
  Nodes m_aMNX;
  Node2ExprNodeMap m_mCachedNodes;
};

#endif
