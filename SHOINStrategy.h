#ifndef _SHOIN_STRATEGY_
#define _SHOIN_STRATEGY_

#include "SHOIQStrategy.h"

class ABox;

class SHOINStrategy : public SHOIQStrategy
{
 public:
  SHOINStrategy(ABox* pABox);

  virtual void applyChooseRuleOnIndividuals();
  virtual void printStrategyType() { printf("SHOIN Strategy\n"); }  
};

class SHONStrategy : public SHOINStrategy
{
 public:
  SHONStrategy(ABox* pABox);
  
  virtual void applyGuessingRuleOnIndividuals();
  virtual void printStrategyType() { printf("SHON Strategy\n"); }  
};

class SHINStrategy : public SHOINStrategy
{
 public:
  SHINStrategy(ABox* pABox);

  virtual void applyGuessingRuleOnIndividuals();
  virtual void applyNominalRuleOnIndividuals();
  virtual void printStrategyType() { printf("SHIN Strategy\n"); }  
};

class SHNStrategy : public SHOINStrategy
{
 public:
  SHNStrategy(ABox* pABox);

  virtual void applyNominalRuleOnIndividuals();
  virtual void printStrategyType() { printf("SHN Strategy\n"); }  
};

#endif
