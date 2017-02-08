#ifndef _SROIQ_STRATEGY_
#define _SROIQ_STRATEGY_

#include "CompletionStrategy.h"

class ABox;
class Blocking;

class SROIQStrategy : public CompletionStrategy
{
 public:
  SROIQStrategy(ABox* pABox);
  SROIQStrategy(ABox* pABox, Blocking* pBlocking);
  
  virtual bool backtrack();
  virtual ABox* complete();
  virtual bool supportsPseudoModelCompletion();
  virtual void printStrategyType() { printf("SROIQ Strategy\n"); }
};

#endif
