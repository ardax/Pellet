#ifndef _BRANCH_
#define _BRANCH_

#include "ZList.h"
#include "ExpressionNode.h"
#include "DependencySet.h"
#include "NodeMerge.h"
#include <vector>

class ABox;
class Node;
class DependencySet;
class CompletionStrategy;
class Role;
class Individual;
class Datatype;
class Literal;

class Branch
{
 public:
  Branch(ABox* pABox, CompletionStrategy* pStrategy, Node* pX, DependencySet* pDS, int iN);

  enum BranchTypes
    {
      BRANCH_DISJUNCTION = 0,
      BRANCH_GUESS,
      BRANCH_LITERALVALUE,
      BRANCH_MAX,
      BRANCH_RULE,
      BRANCH_CHOOSE,
    };

  virtual bool isEqual(Branch* pBranch);
  virtual Branch* copyTo(ABox* pABox);
  virtual void setLastClash(DependencySet* pDS);

  virtual bool tryNext();
  virtual void tryBranch() = 0;
  virtual void shiftTryNext(int iIndex) {}

  virtual DependencySet* getCombinedClash() { return m_pPrevDS; }

  ABox* m_pABox;

  short m_iBranchType;
  int   m_iBranch;
  int   m_iTryCount;
  int   m_iTryNext;

  ExprNode* m_pNodeName;
  Node* m_pNode;
  Individual* m_pIndv;

  // store things that can be changed after this branch
  int m_iAnonCount;
  int m_iNodeCount;

  DependencySet* m_pTermDepends;
  DependencySet* m_pPrevDS;
  
  CompletionStrategy* m_pCompletionStrategy;
};

class GuessBranch : public Branch
{
 public:
  GuessBranch(ABox* pABox, CompletionStrategy* pStrategy, Individual* pX, Role* pRole, int iMinGuess, int iMaxGuess, DependencySet* pDS);

  virtual void tryBranch();
  virtual Branch* copyTo(ABox* pABox);

  Role* m_pRole;
  int m_iMinGuess;
};

class MaxBranch : public Branch
{
 public:
  MaxBranch(ABox* pABox, CompletionStrategy* pStrategy, Individual* pX, Role* pRole, int iN, ExprNode* pQualification, NodeMerges* pMergePairs, DependencySet* pDS);
  MaxBranch(ABox* pABox, CompletionStrategy* pStrategy, Individual* pX, Role* pRole, int iN, ExprNode* pQualification, ZCollection* pMergePairs, DependencySet* pDS);

  virtual void tryBranch();
  virtual Branch* copyTo(ABox* pABox);
  virtual void shiftTryNext(int iIndex);
  virtual void setLastClash(DependencySet* pDS);

  ZCollection* m_pMergePairs;
  ExprNode* m_pQualification;
  int m_iN;
  Role* m_pRole;
  ZCollection m_zcPrevDS;
};

typedef vector<int> ints;

class DisjunctionBranch : public Branch
{
 public:
  DisjunctionBranch(ABox* pABox, CompletionStrategy* pStrategy, Node* pNode, ExprNode* pDisjunction, DependencySet* pDS);
  DisjunctionBranch(ABox* pABox, CompletionStrategy* pStrategy, Node* pNode, ExprNode* pDisjunction, DependencySet* pDS, ExprNodes* aDisj);
  DisjunctionBranch(ABox* pABox, CompletionStrategy* pStrategy, Node* pNode, ExprNode* pDisjunction, DependencySet* pDS, ZCollection* aDisj);

  virtual void tryBranch();
  virtual Branch* copyTo(ABox* pABox);
  virtual int preferredDisjunct();
  virtual void shiftTryNext(int iIndex);
  virtual void setLastClash(DependencySet* pDS);

  ExprNode* m_pDisjunction;
  ZCollection m_zcDisj;
  ZCollection m_zcPrevDS;
  ICollection m_icOrder;
};

class ChooseBranch : public DisjunctionBranch
{
 public:
  ChooseBranch(ABox* pABox, CompletionStrategy* pStrategy, Node* pNode, ExprNode* pC, DependencySet* pDS);

  Datatype* m_pDatatype;
  int m_iShuffle;
};

class LiteralValueBranch : public Branch
{
 public:
  LiteralValueBranch(ABox* pABox, CompletionStrategy* pStrategy, Literal* pLiteral, Datatype* pDataType);

  virtual void tryBranch();
  virtual Branch* copyTo(ABox* pABox);
  virtual void shiftTryNext(int iIndex);

  Datatype* m_pDatatype;
  int m_iShuffle;
};

typedef vector<Branch*> BranchList;

#endif
