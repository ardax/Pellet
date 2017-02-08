#ifndef _SHOIQ_STRATEGY_
#define _SHOIQ_STRATEGY_

#include "SROIQStrategy.h"

class ABox;

class SHOIQStrategy : public SROIQStrategy
{
 public:
  SHOIQStrategy(ABox* pABox);

  virtual void printStrategyType() { printf("SHOIQ Strategy\n"); }  
};

#endif
