#ifndef _INDIVIDUAL_NODE_
#define _INDIVIDUAL_NODE_

#include "Node.h"
#include "EdgeList.h"
#include "ExpressionNode.h"
#include <map>

class Role;
class DependencySet;
class Edge;
class ABox;

#define TYPECOUNT 7

class Individual : public Node
{
 public:
  Individual(ExprNode* pName);
  Individual(ExprNode* pName, ABox* pABox, int iNominalLevel);
  Individual(Individual* pIndividual, ABox* pABox);

  virtual void addType(ExprNode* pC, DependencySet* pDS);
  virtual void removeType(ExprNode* pC);
  virtual bool restore(int iBranch);
  virtual void unprune(int iBranch);
  
  Edge* addEdge(Role* pRole, Node* pX, DependencySet* pDS);
  bool removeEdge(Edge* pEdge);
  void addOutEdge(Edge* pEdge);
  virtual void addInEdge(Edge* pEdge);
  virtual Node* copyTo(ABox* pABox);

  bool checkMinClash(ExprNode* pMinCard, DependencySet* pMinDepends);
  bool checkMaxClash(ExprNode* pMaxCard, DependencySet* pMaxDepends);
  bool isRedundantMin(ExprNode* pMinCard);
  bool isRedundantMax(ExprNode* pMaxCard);
  

  void getRNeighborEdges(Role* pRole, EdgeList* pEdgeList);
  void getRNeighborNames(Role* pRole, ExprNodeSet* pNames);
  void getRPredecessorEdges(Role* pRole, EdgeList* pEdgeList);
  void getEdgesTo(Node* pNode, Role* pRole, EdgeList* pEdgeList);
  void getObviousTypes(ExprNodes* pTypes, ExprNodes* pNonTypes);
  void getAncestors(Nodes* pAncestors);
  void getRSuccessors(Role* pRole, ExprNode* pC, NodeSet* pRSuccessors);
  void getSortedSuccessors(NodeSet* pSortedSuccessors);

  DependencySet* getDifferenceDependency(Node* pNode);

  int getMaxCard(Role* pRole);
  int getMinCard(Role* pRole);

  bool hasRSuccessor(Role* pR);
  bool hasRSuccessor(Role* pR, Node* pX);
  bool hasRNeighbor(Role* pR);
  bool hasDistinctRNeighborsForMin(Role* pRole, int iN, ExprNode* pC, bool bOnlyNominals = FALSE);
  DependencySet* hasDistinctRNeighborsForMax(Role* pRole, int iN, ExprNode* pC);
  DependencySet* hasMax1(Role* pRole);

  bool isBlockable();
  bool canApply(int iType);
  virtual bool isNominal();
  virtual bool isIndividual() { return TRUE; }
  virtual bool hasSuccessor(Node* pX);
  virtual int getNominalLevel() { return m_iNominalLevel; }

  void prune(DependencySet* pDS);

  int m_iNominalLevel;
  EdgeList m_listOutEdges;
  ExprNodes m_aTypes[TYPECOUNT];
  int m_iApplyNext[TYPECOUNT];
  Nodes m_aAncestors;
};

class State;
typedef pair<Individual*, State*> IndividualStatePair;

int isEqualIndividualStatePair(const IndividualStatePair* pIndividualStatePair1, const IndividualStatePair* pIndividualStatePair2);
struct strCmpIndividualStatePair
{
  bool operator()( const IndividualStatePair* pIndividualStatePair1, const IndividualStatePair* pIndividualStatePair2 ) const {
    return isEqualIndividualStatePair(pIndividualStatePair1, pIndividualStatePair2) < 0;
  }
};
typedef set<IndividualStatePair*, strCmpIndividualStatePair> IndividualStatePairSet;

#endif
