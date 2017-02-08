#ifndef _BLOCKING_
#define _BLOCKING_

#include "Node.h"
class Individual;

class Blocking
{
 public:
  Blocking();
  
  bool equals(Individual* pX, Individual* pY);
  bool subset(Individual* pX, Individual* pY);

  bool isBlocked(Individual* pX);
  virtual bool isDirectlyBlocked(Individual* pX, Nodes* pAncestors) = 0;
  virtual bool isIndirectlyBlocked(Individual* pX);
  virtual bool isIndirectlyBlocked(Nodes* pAncestors);
};

/********************************************************************/
/********************************************************************/
/********************************************************************/

class SubsetBlocking : public Blocking
{
 public:
  SubsetBlocking() {}
  
  virtual bool isDirectlyBlocked(Individual* pX, Nodes* pAncestors);
};

/********************************************************************/
/********************************************************************/
/********************************************************************/

class DoubleBlocking : public Blocking
{
 public:
  DoubleBlocking() {}
  
  bool isDirectlyBlocked(Individual* pX, Nodes* pAncestors);
};

/********************************************************************/
/********************************************************************/
/********************************************************************/

class OptimizedDoubleBlocking : public Blocking
{
 public:
  OptimizedDoubleBlocking(){}

  virtual bool isDirectlyBlocked(Individual* pX, Nodes* pAncestors);
  bool block1(Individual* pW, Individual* pW1);
  bool block2(Individual* pW, Individual* pV, Individual* pW1);
  bool block3(Individual* pW, Individual* pV, Individual* pW1);
  bool block4(Individual* pW, Individual* pV, Individual* pW1);
  bool block5(Individual* pW, Individual* pV, Individual* pW1);
  bool block6(Individual* pW, Individual* pW1);
  
};

/********************************************************************/
/********************************************************************/
/********************************************************************/

#endif
