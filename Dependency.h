#ifndef _DEPENDENCY_
#define _DEPENDENCY_

#include "ExpressionNode.h"
#include <set>
class Clash;

enum DependencyTypes
  {
    DEPENDENCY_CLASH = 0,
    DEPENDENCY_MERGE,
    DEPENDENCY_TYPE,
    DEPENDENCY_BRANCHADD,
    DEPENDENCY_CLOSEBRANCH,
  };

class Dependency
{
 public:
  Dependency();

  //virtual int hashCode() = 0;
  virtual bool isEqual(const Dependency* pDependency) const = 0;

  short m_iDependencyType;
};

/*************************************************************************/
class ClashDependency : public Dependency
{
 public:
  ClashDependency();
  ClashDependency(ExprNode* pAssertion, Clash* pClash);

  bool isEqual(const Dependency* pDependency) const;

  ExprNode* m_pAssertion;
  Clash* m_pClash;
  
};

/*************************************************************************/
class TypeDependency : public Dependency
{
 public:
  TypeDependency(ExprNode* pInd, ExprNode* pType);

  bool isEqual(const Dependency* pDependency) const;

  ExprNode* m_pType;
  ExprNode* m_pInd;
};

/*************************************************************************/
class MergeDependency : public Dependency
{
 public:
  MergeDependency(ExprNode* pInd, ExprNode* pMergeIntoInd);

  bool isEqual(const Dependency* pDependency) const;

  ExprNode* m_pInd;
  ExprNode* m_pMergeIntoInd;
};

/*************************************************************************/
class Branch;

class BranchDependency : public Dependency
{
 public:
  BranchDependency(ExprNode* pAssertion);

  bool isEqual(const Dependency* pDependency) const;

  ExprNode* m_pAssertion;
};

typedef set<BranchDependency*> BranchDependencySet;

/*************************************************************************/

class BranchAddDependency : public BranchDependency
{
 public:
  BranchAddDependency(ExprNode* pAssertion, int iIndex, Branch* pBranch);

  bool isEqual(const Dependency* pDependency) const;

  Branch* m_pBranch;
};

/*************************************************************************/
class CloseBranchDependency : public BranchDependency
{
 public:
  CloseBranchDependency(ExprNode* pAssertion, int iTryNext, Branch* pBranch);

  bool isEqual(const Dependency* pDependency) const;
  ExprNode* getInd() const;

  Branch* m_pBranch;
  int m_iTryNext;
};

/*************************************************************************/
struct strCmpDependency
{
  bool operator()( const Dependency* pClashDep1, const Dependency* pClashDep2 ) const {
    return pClashDep1->isEqual(pClashDep2);
  }
};

typedef set<Dependency*, strCmpDependency> SetOfDependency;

#endif
