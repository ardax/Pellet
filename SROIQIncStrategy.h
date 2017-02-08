#ifndef _SROIQ_INC_STRATEGY_
#define _SROIQ_INC_STRATEGY_

#include "SROIQStrategy.h"

class ABox;

class SROIQIncStrategy : public SROIQStrategy
{
 public:
  SROIQIncStrategy(ABox* pABox);

  virtual void applyGuessingRuleOnIndividuals();
  virtual void initialize();
  virtual void printStrategyType() { printf("SROIQ Incremental Strategy\n"); }
};

#endif
